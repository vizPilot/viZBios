
#include "LvglLibCommon.h"

#include <Protocol/GraphicsOutput.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *EfiGop;
    lv_color32_t *Buffer[2];
    UINTN BufferSize;
    UINTN Width;
    UINTN Height;
} UEFI_DISP_DATA;

STATIC
VOID
uefi_disp_delete_evt_cb (
    lv_event_t *Event
    )
{
    lv_display_t *Display;
    UEFI_DISP_DATA *Data;

    Display = lv_event_get_target(Event);
    Data = (UEFI_DISP_DATA *)lv_display_get_driver_data(Display);

    if (Data == NULL) {
        return;
    }

    if (Data->Buffer[0] != NULL) {
        FreePool(Data->Buffer[0]);
    }

    if (Data->Buffer[1] != NULL) {
        FreePool(Data->Buffer[1]);
    }

    lv_display_set_driver_data(Display, NULL);
    FreePool(Data);
}

STATIC
VOID
uefi_disp_flush (
    lv_display_t *Display,
    const lv_area_t *Area,
    uint8_t *PxMap
    )
{
    EFI_STATUS Status;
    UEFI_DISP_DATA *Data;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Source;
    UINTN X1, Y1, X2, Y2;
    UINTN Width, Height;
    UINTN SourceStride;
    UINTN SourceOffset;

    Data = (UEFI_DISP_DATA *)lv_display_get_driver_data(Display);

    if (Data == NULL || Data->EfiGop == NULL ||
        Data->EfiGop->Mode == NULL ||
        Data->EfiGop->Mode->Info == NULL ||
        PxMap == NULL || Area == NULL) {
        lv_display_flush_ready(Display);
        return;
    }

    Info = Data->EfiGop->Mode->Info;

    if (Info->PixelFormat !=
        PixelBlueGreenRedReserved8BitPerColor) {
        DEBUG((DEBUG_ERROR,
            "viZBIOS: Unsupported GOP pixel format\n"));
        lv_display_flush_ready(Display);
        return;
    }

    /* Reject invalid areas before converting signed coordinates. */
    if (Area->x2 < Area->x1 || Area->y2 < Area->y1 ||
        Area->x2 < 0 || Area->y2 < 0 ||
        Area->x1 >= (INT32)Data->Width ||
        Area->y1 >= (INT32)Data->Height) {
        lv_display_flush_ready(Display);
        return;
    }

    X1 = (Area->x1 < 0) ? 0 : (UINTN)Area->x1;
    Y1 = (Area->y1 < 0) ? 0 : (UINTN)Area->y1;
    X2 = (Area->x2 >= (INT32)Data->Width)
       ? Data->Width - 1 : (UINTN)Area->x2;
    Y2 = (Area->y2 >= (INT32)Data->Height)
       ? Data->Height - 1 : (UINTN)Area->y2;

    if (X2 < X1 || Y2 < Y1) {
        lv_display_flush_ready(Display);
        return;
    }

    Width = X2 - X1 + 1;
    Height = Y2 - Y1 + 1;

    /*
     * DIRECT mode: PxMap is assumed to address a full-screen,
     * tightly packed 32-bit LVGL buffer.
     */
    SourceStride = Data->Width * sizeof(lv_color32_t);

    if (Y1 > MAX_UINTN / SourceStride) {
        lv_display_flush_ready(Display);
        return;
    }

    SourceOffset = Y1 * SourceStride;

    if (X1 > (MAX_UINTN - SourceOffset) / sizeof(lv_color32_t)) {
        lv_display_flush_ready(Display);
        return;
    }

    SourceOffset += X1 * sizeof(lv_color32_t);
    Source = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(PxMap + SourceOffset);

    Status = Data->EfiGop->Blt(
        Data->EfiGop,
        Source,
        EfiBltBufferToVideo,
        0,
        0,
        X1,
        Y1,
        Width,
        Height,
        SourceStride
    );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,
            "viZBIOS: GOP Blt failed: %r\n", Status));
    }

    lv_display_flush_ready(Display);
}

lv_display_t *
lv_uefi_disp_create (
    int32_t HorRes,
    int32_t VerRes
    )
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UEFI_DISP_DATA *Data;
    lv_display_t *Display;
    UINTN PixelCount;
    UINTN BufferSize;

    if (HorRes <= 0 || VerRes <= 0) {
        return NULL;
    }

    Status = gBS->LocateProtocol(
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (VOID **)&Gop
        );

    if (EFI_ERROR(Status) || Gop == NULL ||
        Gop->Mode == NULL || Gop->Mode->Info == NULL) {
        return NULL;
    }

    Info = Gop->Mode->Info;

    if (Info->PixelFormat !=
        PixelBlueGreenRedReserved8BitPerColor) {
        DEBUG((DEBUG_ERROR,
            "viZBIOS: GOP format is not BGRX32\n"));
        return NULL;
    }

    if ((UINTN)HorRes > MAX_UINTN / (UINTN)VerRes) {
        return NULL;
    }

    PixelCount = (UINTN)HorRes * (UINTN)VerRes;

    if (PixelCount > MAX_UINTN / sizeof(lv_color32_t)) {
        return NULL;
    }

    BufferSize = PixelCount * sizeof(lv_color32_t);

    Data = AllocateZeroPool(sizeof(*Data));
    if (Data == NULL) {
        return NULL;
    }

    Data->EfiGop = Gop;
    Data->Width = (UINTN)HorRes;
    Data->Height = (UINTN)VerRes;
    Data->BufferSize = BufferSize;

    Data->Buffer[0] = AllocatePool(BufferSize);
    Data->Buffer[1] = AllocatePool(BufferSize);

    if (Data->Buffer[0] == NULL || Data->Buffer[1] == NULL) {
        if (Data->Buffer[0] != NULL) {
            FreePool(Data->Buffer[0]);
        }

        if (Data->Buffer[1] != NULL) {
            FreePool(Data->Buffer[1]);
        }

        FreePool(Data);
        return NULL;
    }

    Display = lv_display_create(HorRes, VerRes);
    if (Display == NULL) {
        FreePool(Data->Buffer[0]);
        FreePool(Data->Buffer[1]);
        FreePool(Data);
        return NULL;
    }

    lv_display_set_driver_data(Display, Data);
    lv_display_set_flush_cb(Display, uefi_disp_flush);

    lv_display_set_buffers(
        Display,
        Data->Buffer[0],
        Data->Buffer[1],
        BufferSize,
        LV_DISPLAY_RENDER_MODE_DIRECT
        );

    lv_display_add_event_cb(
        Display,
        uefi_disp_delete_evt_cb,
        LV_EVENT_DELETE,
        NULL
        );

    return Display;
}

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <Protocol/viZBIOSProtocol.h>

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_STATUS Status;
    VIZBIOS_PROTOCOL *ViZBIOS;

    ViZBIOS = NULL;

    Print(L"\r\n");
    Print(L"========================\r\n");
    Print(L"       viZBIOS App\r\n");
    Print(L"========================\r\n");

    Status = gBS->LocateProtocol(
        &gViZBIOSProtocolGuid,
        NULL,
        (VOID **)&ViZBIOS
        );

    if (EFI_ERROR(Status)) {
        Print(L"[ERROR] No se encontro el protocolo viZBIOS.\r\n");
        Print(L"Estado: %r\r\n", Status);
        Print(L"Presiona una tecla para salir.\r\n");
        gST->ConIn->ReadKeyStroke(gST->ConIn, &(EFI_INPUT_KEY){0});
        return Status;
    }

    if (ViZBIOS == NULL || ViZBIOS->Launch == NULL) {
        Print(L"[ERROR] Protocolo o funcion Launch no disponible.\r\n");
        return EFI_UNSUPPORTED;
    }

    Print(L"[OK] Protocolo viZBIOS encontrado.\r\n");
    Print(L"Invocando viZBIOS...\r\n");

    Status = ViZBIOS->Launch();

    if (EFI_ERROR(Status)) {
        Print(L"[ERROR] viZBIOS devolvio: %r\r\n", Status);
    } else {
        Print(L"[OK] viZBIOS regreso correctamente.\r\n");
    }

    return Status;
}

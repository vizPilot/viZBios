#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/GraphicsOutput.h>

#include <Protocol/viZBIOSProtocol.h>
#include <viZBIOS.h>

#include "Color.h"
#include "viZBIOSDxe.h"

typedef struct {
  UINT32 Background;
  UINT32 HeaderBackground;
  UINT32 TextColor;
  UINT32 HighlightBackground;
  UINT32 HighlightText;
} VIZBIOS_THEME;

VIZBIOS_THEME DefaultTheme = {
  0x00202080,  // fondo azul oscuro
  0x00404040,  // header gris
  0x00FFFFFF,  // texto blanco
  0x00808080,  // selección gris claro
  0x00000000   // texto seleccionado negro
};

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  UINT32 Width;
  UINT32 Height;
  UINT32 PixelsPerScanLine;
  VIZBIOS_THEME Theme;
  UINTN SelectedIndex;
} VIZBIOS_CONTEXT;

typedef struct _VIZBIOS_MENU VIZBIOS_MENU;

typedef struct {
  CHAR16 *Text;
  VOID (*Action)(VIZBIOS_CONTEXT*);
  VIZBIOS_MENU *SubMenu;
} VIZBIOS_MENU_ITEM;

struct _VIZBIOS_MENU {
  CHAR16 *Title;
  VIZBIOS_MENU_ITEM *Items;
  UINTN ItemCount;
};

VIZBIOS_MENU_ITEM MainMenu[] = {
  { L"Boot Options", NULL },
  { L"Device Info", NULL },
  { L"Advanced", NULL },
  { L"Exit", NULL }
};

VOID ClearScreen(EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, UINT8 R, UINT8 G, UINT8 B)
{
  UINT32 x, y;
  UINT32 *FrameBuffer = (UINT32*)Gop->Mode->FrameBufferBase;

  UINT32 Color = (B) | (G << 8) | (R << 16);

  for (y = 0; y < Gop->Mode->Info->VerticalResolution; y++) {
      for (x = 0; x < Gop->Mode->Info->HorizontalResolution; x++) {
          FrameBuffer[y * Gop->Mode->Info->PixelsPerScanLine + x] = Color;
      }
  }
}

EFI_GUID gViZBIOSProtocolGuid = VIZBIOS_PROTOCOL_GUID;

STATIC
EFI_STATUS
EFIAPI
ViZBIOSLaunch (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "viZBIOS Launching...\n"));

  // Aquí irá tu UI
  // Aquí usarás GOP
  // Aquí irá LVGL

  return EFI_SUCCESS;
}

STATIC VIZBIOS_PROTOCOL mViZBIOSProtocol = {
  ViZBIOSLaunch
};

EFI_STATUS
EFIAPI
viZBIOSDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HANDLE ViZBIOSHandle;

  DEBUG ((EFI_D_INFO, "Entering viZBIOS Setup\n"));

  Status = gBS->LocateProtocol (
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  (VOID **)&mViZBIOSGop
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "GOP unavailable: %r\n", Status));
    return Status;
  }

  ViZBIOSHandle = NULL;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ViZBIOSHandle,
                  &gViZBIOSProtocolGuid,
                  &mViZBIOSProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Cannot install viZBIOS protocol: %r\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "viZBIOS protocol installed successfully\n"));

  return EFI_SUCCESS;
}
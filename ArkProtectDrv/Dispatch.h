#ifndef CXX_Dispatch_H
#define CXX_Dispatch_H
#include <ntifs.h>

#include "ProcessCore.h"



//  π”√Method_Neither
/*
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
*/

#define IOCTL_ARKPROTECT_PROCESSNUM  (UINT32)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ARKPROTECT_ENUMPROCESS (UINT32)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


NTSTATUS
APIoControlPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#endif // !CXX_Dispatch_H
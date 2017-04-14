#ifndef CXX_main_H
#define CXX_main_H
#include <ntifs.h>

#include "Private.h"


#define DEVICE_NAME  L"\\Device\\ArkProtectDeviceName"
#define LINK_NAME    L"\\??\\ArkProtectLinkName"
#define FILE_DEVICE_ARKPROTECT           0x8005


NTSTATUS
APInitializeDynamicData(IN OUT PDYNAMIC_DATA DynamicData);

NTSTATUS
APDefaultPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID
APUnloadDriver(IN PDRIVER_OBJECT DriverObject);



#endif // !CXX_main_H

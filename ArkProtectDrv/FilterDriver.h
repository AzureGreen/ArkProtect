#ifndef CXX_FilterDriver_H
#define CXX_FilterDriver_H

#include <ntifs.h>
#include "Private.h"
#include "DriverCore.h"

typedef enum _eFilterType
{
	ft_Unkonw,
	ft_File,				// 文件------------
	ft_Disk,                // 磁盘    文件相关
	ft_Volume,		        // 卷  ------------- 
	ft_Keyboard,            // 键盘
	ft_Mouse,				// 鼠标            硬件接口
	ft_I8042prt,			// 键盘驱动
	ft_Tcpip,				// tcpip-------------------
	ft_Ndis,				// 网络驱动接口
	ft_PnpManager,          // 即插即用管理器       网络相关
	ft_Tdx,				    // 网络相关
	ft_Raw
} eFilterType;


typedef struct _FILTER_DRIVER_ENTRY_INFORMATION
{
	eFilterType FilterType;
	UINT_PTR    FilterDeviceObject;
	WCHAR       wzFilterDriverName[MAX_PATH];
	WCHAR       wzFilterDeviceName[MAX_PATH];
	WCHAR       wzAttachedDriverName[MAX_PATH];
	WCHAR       wzFilePath[MAX_PATH];
} FILTER_DRIVER_ENTRY_INFORMATION, *PFILTER_DRIVER_ENTRY_INFORMATION;

typedef struct _FILTER_DRIVER_INFORMATION
{
	UINT32                          NumberOfFilterDrivers;
	FILTER_DRIVER_ENTRY_INFORMATION FilterDriverEntry[1];
} FILTER_DRIVER_INFORMATION, *PFILTER_DRIVER_INFORMATION;



NTSTATUS 
APInsertFilterDriverToList(IN PDEVICE_OBJECT AttachDeviceObject, IN PDRIVER_OBJECT AttachedDriverObject, OUT PFILTER_DRIVER_INFORMATION fdi, IN UINT32 FilterDriverCount, IN eFilterType FilterType);

NTSTATUS
APGetFilterDriverByDriverName(IN WCHAR * wzDriverName, OUT PFILTER_DRIVER_INFORMATION fdi, IN UINT32 FilterDriverCount, IN eFilterType FilterType);

NTSTATUS
APEnumFilterDriver(OUT PVOID OutputBuffer, IN UINT32 OutputLength);


#endif // !CXX_FilterDriver_H


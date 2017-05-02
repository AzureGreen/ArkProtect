#include "FilterDriver.h"

extern DYNAMIC_DATA	g_DynamicData;

UINT32 g_VolumeStartCount = 0;
UINT32 g_FileSystemStartCount = 0;


/************************************************************************
*  Name : APGetFilterDriverInfo
*  Param: HighDeviceObject              上层设备对象
*  Param: LowDriverObject               下层驱动对象
*  Param: fdi                           Ring3Buffer
*  Param: FilterDriverCount             Count
*  Param: FilterType                    类型
*  Ret  : BOOL
*  与驱动层通信，枚举进程模块信息
************************************************************************/
NTSTATUS 
APGetFilterDriverInfo(IN PDEVICE_OBJECT HighDeviceObject, IN PDRIVER_OBJECT LowDriverObject, OUT PFILTER_DRIVER_INFORMATION fdi,
	IN UINT32 FilterDriverCount, IN eFilterType FilterType)
{
	if (HighDeviceObject && MmIsAddressValid((PVOID)HighDeviceObject)
		&& LowDriverObject && MmIsAddressValid((PVOID)LowDriverObject)
		&& fdi && MmIsAddressValid((PVOID)fdi))
	{
		if (FilterDriverCount > fdi->NumberOfFilterDrivers)
		{
			PDRIVER_OBJECT        HighDriverObject = HighDeviceObject->DriverObject;		// 去挂钩设备的驱动对象（上层）
			PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = NULL;

			if (FilterType == ft_File || FilterType == ft_Raw)
			{
				// 判断是否已经在List里了
				/*if (g_FileSystemStartCount == 0)
				{
					g_FileSystemStartCount = fdi->NumberOfFilterDrivers;
				}*/
				for (UINT32 i = 0; i < fdi->NumberOfFilterDrivers; i++)
				{
					if (_wcsnicmp(fdi->FilterDriverEntry[i].wzFilterDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzFilterDriverName)) == 0 &&
						_wcsnicmp(fdi->FilterDriverEntry[i].wzAttachedDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzAttachedDriverName)) == 0)
					{
						return STATUS_SUCCESS;
					}
				}
			}
			if (FilterType == ft_Volume)
			{
				// 判断是否已经在List里了
				/*if (g_VolumeStartCount == 0)
				{
					g_VolumeStartCount = fdi->NumberOfFilterDrivers;
				}*/
				for (UINT32 i = 0; i < fdi->NumberOfFilterDrivers; i++)
				{
					if (_wcsnicmp(fdi->FilterDriverEntry[i].wzFilterDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzFilterDriverName)) == 0 &&
						_wcsnicmp(fdi->FilterDriverEntry[i].wzAttachedDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzAttachedDriverName)) == 0)
					{
						return STATUS_SUCCESS;
					}
				}
			}

			fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].FilterType = FilterType;
			fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].FilterDeviceObject = (UINT_PTR)HighDeviceObject;

			// 挂钩驱动（上层）（过滤驱动对象）
			if (APIsUnicodeStringValid(&(HighDriverObject->DriverName)))
			{
				//RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDriverName, MAX_PATH);
				RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDriverName, HighDriverObject->DriverName.Buffer, HighDriverObject->DriverName.Length);
			}

			// 挂钩驱动（上层）（过滤驱动的设备对象名称）
			APGetDeviceObjectNameInfo(HighDeviceObject, fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDeviceName);

			// 被挂钩驱动（下层）(宿主驱动对象)
			if (APIsUnicodeStringValid(&(LowDriverObject->DriverName)))
			{
				//RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzAttachedDriverName, MAX_PATH);
				RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzAttachedDriverName, LowDriverObject->DriverName.Buffer, LowDriverObject->DriverName.Length);
			}

			// 过滤驱动对象路径
			LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)HighDriverObject->DriverSection;

			if ((UINT_PTR)LdrDataTableEntry > g_DynamicData.MinKernelSpaceAddress)
			{
				if (APIsUnicodeStringValid(&(LdrDataTableEntry->FullDllName)))
				{
				//	RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, MAX_PATH);
					RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, LdrDataTableEntry->FullDllName.Buffer, LdrDataTableEntry->FullDllName.Length);
				}
				else if (APIsUnicodeStringValid(&(LdrDataTableEntry->BaseDllName)))
				{
				//	RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, MAX_PATH);
					RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, LdrDataTableEntry->BaseDllName.Buffer, LdrDataTableEntry->BaseDllName.Length);
				}
			}
		}
		else
		{
			return STATUS_BUFFER_TOO_SMALL;
		}

		fdi->NumberOfFilterDrivers++;

		return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}


NTSTATUS
APGetFilterDriverByDriverName(IN WCHAR *wzDriverName, OUT  PFILTER_DRIVER_INFORMATION fdi, IN UINT32 FilterDriverCount, IN eFilterType FilterType)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uniDriverName;
	PDRIVER_OBJECT DriverObject = NULL;

	RtlInitUnicodeString(&uniDriverName, wzDriverName);

	Status = ObReferenceObjectByName(
		&uniDriverName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&DriverObject);

	if (NT_SUCCESS(Status) && DriverObject && MmIsAddressValid((PVOID)DriverObject))
	{
		PDEVICE_OBJECT DeviceObject = NULL;

		// 遍历水平层次结构 NextDevice 设备链
		for (DeviceObject = DriverObject->DeviceObject;
			DeviceObject && MmIsAddressValid((PVOID)DeviceObject);
			DeviceObject = DeviceObject->NextDevice)
		{
			PDRIVER_OBJECT LowDriverObject = DeviceObject->DriverObject;
			PDEVICE_OBJECT HighDeviceObject = NULL;

			// 遍历垂直层次结构 AttachedDevice  设备栈
			for (HighDeviceObject = DeviceObject->AttachedDevice;
				HighDeviceObject;
				HighDeviceObject = HighDeviceObject->AttachedDevice)
			{
				// HighDeviceObject --> 去挂载的驱动（上层）
				// LowDriverObject --> 被挂载的驱动（下层）
				Status = APGetFilterDriverInfo(HighDeviceObject, LowDriverObject, fdi, FilterDriverCount, FilterType);
				LowDriverObject = HighDeviceObject->DriverObject;
			}

		}

		ObDereferenceObject(DriverObject);
	}

	return Status;
}




NTSTATUS
APEnumFilterDriver(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	PFILTER_DRIVER_INFORMATION fdi = (PFILTER_DRIVER_INFORMATION)OutputBuffer;

	UINT32 FilterDriverCount = (OutputLength - sizeof(FILTER_DRIVER_INFORMATION)) / sizeof(FILTER_DRIVER_ENTRY_INFORMATION);

	// 写上所有的驱动名。

	g_VolumeStartCount = 0;
	g_FileSystemStartCount = 0;

	APGetFilterDriverByDriverName(L"\\Driver\\Disk", fdi, FilterDriverCount, ft_Disk);
	APGetFilterDriverByDriverName(L"\\Driver\\volmgr", fdi, FilterDriverCount, ft_Volume);
	APGetFilterDriverByDriverName(L"\\FileSystem\\ntfs", fdi, FilterDriverCount, ft_File);
	APGetFilterDriverByDriverName(L"\\FileSystem\\fastfat", fdi, FilterDriverCount, ft_File);
	APGetFilterDriverByDriverName(L"\\Driver\\kbdclass", fdi, FilterDriverCount, ft_Keyboard);
	APGetFilterDriverByDriverName(L"\\Driver\\mouclass", fdi, FilterDriverCount, ft_Mouse);
	APGetFilterDriverByDriverName(L"\\Driver\\i8042prt", fdi, FilterDriverCount, ft_I8042prt);
	APGetFilterDriverByDriverName(L"\\Driver\\tdx", fdi, FilterDriverCount, ft_Tdx);
	APGetFilterDriverByDriverName(L"\\Driver\\NDIS", fdi, FilterDriverCount, ft_Ndis);
	APGetFilterDriverByDriverName(L"\\Driver\\PnpManager", fdi, FilterDriverCount, ft_PnpManager);
	APGetFilterDriverByDriverName(L"\\FileSystem\\Raw", fdi, FilterDriverCount, ft_Raw);


	if (FilterDriverCount >= fdi->NumberOfFilterDrivers)
	{
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}

	return Status;
}
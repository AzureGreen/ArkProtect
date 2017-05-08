#include "main.h"

extern PVOID            g_ReloadNtImage;
extern PVOID            g_ReloadWin32kImage;

DYNAMIC_DATA	        g_DynamicData = { 0 };
PDRIVER_OBJECT          g_DriverObject = NULL;      // 保存全局驱动对象
PEPROCESS               g_SystemEProcess = NULL;    // 保存全局系统进程
PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList = NULL;// 加载模块List

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegisterPath)
{
	NTSTATUS       Status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = NULL;

	UNICODE_STRING uniDeviceName = { 0 };
	UNICODE_STRING uniLinkName = { 0 };

	RtlInitUnicodeString(&uniDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uniLinkName, LINK_NAME);

	// 创建设备对象
	Status = IoCreateDevice(DriverObject, 0, &uniDeviceName, FILE_DEVICE_ARKPROTECT, 0, FALSE, &DeviceObject);
	if (NT_SUCCESS(Status))
	{
		// 创建设备链接名
		Status = IoCreateSymbolicLink(&uniLinkName, &uniDeviceName);
		if (NT_SUCCESS(Status))
		{
			for (INT32 i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
			{
				DriverObject->MajorFunction[i] = APDefaultPassThrough;
			}

			DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = APIoControlPassThrough;

			DriverObject->DriverUnload = APUnloadDriver;

			g_DriverObject = DriverObject;

			g_SystemEProcess = PsGetCurrentProcess();   // 保存系统进程体结构

			g_PsLoadedModuleList = (PLDR_DATA_TABLE_ENTRY)((PLDR_DATA_TABLE_ENTRY)g_DriverObject->DriverSection)->InLoadOrderLinks.Flink;  // 拿到Ldr链表首单元（空头节点）

			Status = APInitializeDynamicData(&g_DynamicData);			// 初始化信息

			DbgPrint("ArkProtect is Starting!!!");
		}
		else
		{
			DbgPrint("Create SymbolicLink Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Create Device Object Failed\r\n");
	}

	return Status;
}


/************************************************************************
*  Name : APInitDynamicData
*  Param: DynamicData			信息
*  Ret  : NTSTATUS
*  初始化信息
************************************************************************/
NTSTATUS
APInitializeDynamicData(IN OUT PDYNAMIC_DATA DynamicData)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	RTL_OSVERSIONINFOEXW	VersionInfo = { 0 };
	VersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

	if (DynamicData == NULL)
	{
		return STATUS_INVALID_ADDRESS;
	}

	RtlZeroMemory(DynamicData, sizeof(DYNAMIC_DATA));

	// 获得计算机版本信息
	Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInfo);
	if (NT_SUCCESS(Status))
	{
		UINT32 Version = (VersionInfo.dwMajorVersion << 8) | (VersionInfo.dwMinorVersion << 4) | VersionInfo.wServicePackMajor;
		DynamicData->WinVersion = (eWinVersion)Version;

		DbgPrint("%x\r\n", DynamicData->WinVersion);

		switch (Version)
		{
		case WINVER_7:
		case WINVER_7_SP1:
		{
#ifdef _WIN64
			//////////////////////////////////////////////////////////////////////////
			// EProcess
			DynamicData->ThreadListHead_KPROCESS = 0x030;
			DynamicData->ObjectTable = 0x200;
			DynamicData->SectionObject = 0x268;
			DynamicData->InheritedFromUniqueProcessId = 0x290;
			DynamicData->ThreadListHead_EPROCESS = 0x308;

			//////////////////////////////////////////////////////////////////////////
			// EThread
			DynamicData->Priority = 0x07b;
			DynamicData->Teb = 0x0b8;
			DynamicData->ContextSwitches = 0x134;
			DynamicData->State = 0x164;
			DynamicData->PreviousMode = 0x1f6;
			DynamicData->Process = 0x210;
			DynamicData->ThreadListEntry_KTHREAD = 0x2f8;
			DynamicData->StartAddress = 0x390;    ////
			DynamicData->Cid = 0x3b8;		////
			DynamicData->Win32StartAddress = 0x418;    ////
			DynamicData->ThreadListEntry_ETHREAD = 0x428;   ////
			DynamicData->SameThreadApcFlags = 0x458;    ////

			//////////////////////////////////////////////////////////////////////////
			// Object
			DynamicData->SizeOfObjectHeader = 0x030;

			DynamicData->HandleTableEntryOffset = 0x010;

			//////////////////////////////////////////////////////////////////////////

			DynamicData->MaxUserSpaceAddress = 0x000007FFFFFFFFFF;

			DynamicData->MinKernelSpaceAddress = 0xFFFFF00000000000;
#else

			//////////////////////////////////////////////////////////////////////////
			// EProcess
			DynamicData->ThreadListHead_KPROCESS = 0x02c;
			DynamicData->ObjectTable = 0x0f4;
			DynamicData->SectionObject = 0x128;
			DynamicData->InheritedFromUniqueProcessId = 0x140;
			DynamicData->ThreadListHead_EPROCESS = 0x188;

			//////////////////////////////////////////////////////////////////////////
			// EThread
			DynamicData->Priority = 0x057;
			DynamicData->Teb = 0x088;
			DynamicData->ContextSwitches = 0x064;
			DynamicData->State = 0x068;
			DynamicData->PreviousMode = 0x13a;
			DynamicData->Process = 0x150;
			DynamicData->ThreadListEntry_KTHREAD = 0x1e0;
			DynamicData->StartAddress = 0x218;
			DynamicData->Cid = 0x22c;
			DynamicData->Win32StartAddress = 0x260;
			DynamicData->ThreadListEntry_ETHREAD = 0x268;
			DynamicData->SameThreadApcFlags = 0x288;

			//////////////////////////////////////////////////////////////////////////
			// Object
			DynamicData->SizeOfObjectHeader = 0x018;

			DynamicData->HandleTableEntryOffset = 0x008;

			//////////////////////////////////////////////////////////////////////////

			DynamicData->MaxUserSpaceAddress = 0x7FFFFFFF;

			DynamicData->MinKernelSpaceAddress = 0x80000000;


#endif
			break;
		}
		default:
			break;
		}

	}

	return Status;
}


NTSTATUS
APDefaultPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


VOID
APUnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING  uniLinkName;
	PDEVICE_OBJECT	NextDeviceObject = NULL;
	PDEVICE_OBJECT  CurrentDeviceObject = NULL;
	RtlInitUnicodeString(&uniLinkName, LINK_NAME);

	IoDeleteSymbolicLink(&uniLinkName);		// 删除链接名
	CurrentDeviceObject = DriverObject->DeviceObject;
	while (CurrentDeviceObject != NULL)		// 循环遍历删除设备链
	{
		NextDeviceObject = CurrentDeviceObject->NextDevice;
		IoDeleteDevice(CurrentDeviceObject);
		CurrentDeviceObject = NextDeviceObject;
	}

	if (g_ReloadNtImage)
	{
		ExFreePool(g_ReloadNtImage);
		g_ReloadNtImage = NULL;
	}

	if (g_ReloadWin32kImage)
	{
		ExFreePool(g_ReloadWin32kImage);
		g_ReloadWin32kImage = NULL;
	}

	DbgPrint("ArkProtect is stopped!!!");
}
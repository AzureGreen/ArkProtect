#include "ProcessHandle.h"


extern DYNAMIC_DATA	g_DynamicData;


/************************************************************************
*  Name : APGetHandleType
*  Param: Handle				句柄	 （IN）
*  Param: wzHandleType			句柄类型 （OUT）
*  Ret  : BOOLEAN
*  ZwQueryObject+ObjectTypeInformation查询句柄类型
************************************************************************/
VOID
APGetHandleType(IN HANDLE Handle, OUT PWCHAR wzHandleType)
{
	PVOID Buffer = NULL;

	Buffer = ExAllocatePool(PagedPool, PAGE_SIZE);
	if (Buffer)
	{
		UINT32   ReturnLength = 0;

		// 保存之前的模式，转成KernelMode
		PETHREAD EThread = PsGetCurrentThread();
		UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

		RtlZeroMemory(Buffer, PAGE_SIZE);

		__try
		{
			NTSTATUS Status = ZwQueryObject(Handle, ObjectTypeInformation, Buffer, PAGE_SIZE, &ReturnLength);

			if (NT_SUCCESS(Status))
			{
				PPUBLIC_OBJECT_TYPE_INFORMATION ObjectTypeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)Buffer;
				if (ObjectTypeInfo->TypeName.Buffer != NULL &&
					ObjectTypeInfo->TypeName.Length > 0 &&
					MmIsAddressValid(ObjectTypeInfo->TypeName.Buffer))
				{
					if (ObjectTypeInfo->TypeName.Length / sizeof(WCHAR) >= MAX_PATH - 1)
					{
						StringCchCopyW(wzHandleType, MAX_PATH, ObjectTypeInfo->TypeName.Buffer);
						//wcsncpy(wzHandleType, ObjectTypeInfo->TypeName.Buffer, (MAX_PATH - 1));
					}
					else
					{
						StringCchCopyW(wzHandleType, ObjectTypeInfo->TypeName.Length / sizeof(WCHAR) + 1, ObjectTypeInfo->TypeName.Buffer);
						//wcsncpy(wzHandleType, ObjectTypeInfo->TypeName.Buffer, ObjectTypeInfo->TypeName.Length);
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
			wzHandleType = NULL;
		}

		APChangeThreadMode(EThread, PreviousMode);

		ExFreePool(Buffer);
	}
}

/************************************************************************
*  Name : APGetHandleName
*  Param: Handle				句柄	 （IN）
*  Param: wzHandleName			句柄名称 （OUT）
*  Ret  : BOOLEAN
*  ZwQueryObject+ObjectNameInformation查询句柄名称
************************************************************************/
VOID
APGetHandleName(IN HANDLE Handle, OUT PWCHAR wzHandleName)
{
	PVOID Buffer = NULL;

	Buffer = ExAllocatePool(PagedPool, PAGE_SIZE);
	if (Buffer)
	{
		UINT32   ReturnLength = 0;

		// 保存之前的模式，转成KernelMode
		PETHREAD EThread = PsGetCurrentThread();
		UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

		RtlZeroMemory(Buffer, PAGE_SIZE);

		__try
		{
			NTSTATUS Status = ZwQueryObject(Handle, ObjectNameInformation, Buffer, PAGE_SIZE, &ReturnLength);

			if (NT_SUCCESS(Status))
			{
				POBJECT_NAME_INFORMATION ObjectNameInfo = (POBJECT_NAME_INFORMATION)Buffer;
				if (ObjectNameInfo->Name.Buffer != NULL &&
					ObjectNameInfo->Name.Length > 0 &&
					MmIsAddressValid(ObjectNameInfo->Name.Buffer))
				{
					if (ObjectNameInfo->Name.Length / sizeof(WCHAR) >= MAX_PATH - 1)
					{
						StringCchCopyW(wzHandleName, MAX_PATH, ObjectNameInfo->Name.Buffer);
						//wcsncpy(wzHandleName, ObjectNameInfo->Name.Buffer, (MAX_PATH - 1));
					}
					else
					{
						StringCchCopyW(wzHandleName, ObjectNameInfo->Name.Length / sizeof(WCHAR) + 1, ObjectNameInfo->Name.Buffer);
						//wcsncpy(wzHandleName, ObjectNameInfo->Name.Buffer, ObjectNameInfo->Name.Length);
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
			wzHandleName = NULL;
		}

		APChangeThreadMode(EThread, PreviousMode);

		ExFreePool(Buffer);
	}
}


/************************************************************************
*  Name : CopyHandleInformation
*  Param: EProcess			进程结构体				 （IN）
*  Param: Handle			进程句柄				 （IN）
*  Param: Object			进程对象				 （IN）
*  Param: phi				Ring3层进程句柄信息结构体（OUT）
*  Ret  : NTSTATUS
*  枚举目标进程的句柄信息，存入Ring3提供结构体
************************************************************************/
VOID
APGetProcessHandleInfo(IN PEPROCESS EProcess, IN HANDLE Handle, IN PVOID Object, OUT PPROCESS_HANDLE_INFORMATION phi)
{
	if (Object && MmIsAddressValid(Object))
	{
		KAPC_STATE	ApcState = { 0 };

		phi->HandleEntry[phi->NumberOfHandles].Handle = Handle;
		phi->HandleEntry[phi->NumberOfHandles].Object = Object;

		if (MmIsAddressValid((PUINT8)Object - g_DynamicData.SizeOfObjectHeader))
		{
			//phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = (UINT32)*(PUINT_PTR)((PUINT8)Object - g_DynamicData.SizeOfObjectHeader);
			phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = (UINT32)((POBJECT_HEADER)((PUINT8)Object - g_DynamicData.SizeOfObjectHeader))->PointerCount;
		}
		else
		{
			phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = 0;
		}

		// 转到目标进程空间上下背景文里
		KeStackAttachProcess(EProcess, &ApcState);

		APGetHandleName(Handle, phi->HandleEntry[phi->NumberOfHandles].wzHandleName);
		APGetHandleType(Handle, phi->HandleEntry[phi->NumberOfHandles].wzHandleType);

		KeUnstackDetachProcess(&ApcState);
	}
}


/************************************************************************
*  Name : APEnumProcessHandleByZwQuerySystemInformation
*  Param: ProcessId			      进程id
*  Param: EProcess			      进程结构体
*  Param: phi
*  Param: HandleCount
*  Ret  : NTSTATUS
*  
************************************************************************/
NTSTATUS
APEnumProcessHandleByZwQuerySystemInformation(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_HANDLE_INFORMATION phi, IN UINT32 HandleCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UINT32   ReturnLength = PAGE_SIZE;

	// 保存之前的模式，转成KernelMode
	PETHREAD EThread = PsGetCurrentThread();
	UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

	do
	{
		PVOID Buffer = ExAllocatePool(PagedPool, ReturnLength);
		if (Buffer != NULL)
		{
			RtlZeroMemory(Buffer, ReturnLength);

			// 扫描系统所有进程的句柄信息
			Status = ZwQuerySystemInformation(SystemHandleInformation, Buffer, ReturnLength, &ReturnLength);
			if (NT_SUCCESS(Status))
			{
				PSYSTEM_HANDLE_INFORMATION shi = (PSYSTEM_HANDLE_INFORMATION)Buffer;

				for (UINT32 i = 0; i < shi->NumberOfHandles; i++)
				{
					if (ProcessId == (UINT32)shi->Handles[i].UniqueProcessId)
					{
						if (HandleCount > phi->NumberOfHandles)
						{
							APGetProcessHandleInfo(EProcess, (HANDLE)shi->Handles[i].HandleValue, (PVOID)shi->Handles[i].Object, phi);
						}
						// 记录句柄个数
						phi->NumberOfHandles++;
					}
				}
			}
			ExFreePool(Buffer);
		}
	} while (Status == STATUS_INFO_LENGTH_MISMATCH);

	APChangeThreadMode(EThread, PreviousMode);

	// 枚举到了东西
	if (phi->NumberOfHandles)
	{
		Status = STATUS_SUCCESS;
	}

	return Status;
}



/************************************************************************
*  Name : APEnumProcessModule
*  Param: ProcessId			      进程Id
*  Param: OutputBuffer            ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举进程模块
************************************************************************/
NTSTATUS
APEnumProcessHandle(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    HandleCount = (OutputLength - sizeof(PROCESS_HANDLE_INFORMATION)) / sizeof(PROCESS_HANDLE_ENTRY_INFORMATION);
	PEPROCESS EProcess = NULL;

	if (ProcessId == 0)
	{
		return Status;
	}
	else
	{
		Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &EProcess);
	}

	if (NT_SUCCESS(Status) && APIsValidProcess(EProcess))
	{
		// 因为之后需要Attach到目标进程空间（私有内存），所以需要申请内核空间的内存（共用的内存）
		PPROCESS_HANDLE_INFORMATION phi = (PPROCESS_HANDLE_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (phi)
		{
			RtlZeroMemory(phi, OutputLength);

			Status = APEnumProcessHandleByZwQuerySystemInformation(ProcessId, EProcess, phi, HandleCount);
			if (NT_SUCCESS(Status))
			{
				if (HandleCount >= phi->NumberOfHandles)
				{
					RtlCopyMemory(OutputBuffer, phi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_HANDLE_INFORMATION)OutputBuffer)->NumberOfHandles = phi->NumberOfHandles;    // 让Ring3知道需要多少个
					Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
				}
			}

			ExFreePool(phi);
			phi = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}


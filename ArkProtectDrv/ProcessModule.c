#include "ProcessModule.h"

extern DYNAMIC_DATA	g_DynamicData;

/************************************************************************
*  Name : APIsModuleInList
*  Param: BaseAddress			模块基地址（OUT）
*  Param: ModuleSize			模块大小（IN）
*  Ret  : NTSTATUS
*  通过FileObject获得进程完整路径
************************************************************************/
BOOLEAN
APIsModuleInList(IN UINT_PTR BaseAddress, IN UINT32 ModuleSize, IN PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	BOOLEAN bOk = FALSE;
	UINT32  i = 0;
	ModuleCount = pmi->NumberOfModules > ModuleCount ? ModuleCount : pmi->NumberOfModules;

	for (i = 0; i < ModuleCount; i++)
	{
		if (BaseAddress == pmi->ModuleEntry[i].BaseAddress &&
			ModuleSize == pmi->ModuleEntry[i].SizeOfImage)
		{
			bOk = TRUE;
			break;
		}
	}
	return bOk;
}


/************************************************************************
*  Name : APEnumProcessModuleByPeb
*  Param: EProcess			      进程结构体
*  Param: pmi			          ring3内存
*  Param: ModuleCount
*  Ret  : NTSTATUS
*  通过遍历peb的Ldr三根链表中的一个表（处理Wow64）
************************************************************************/
NTSTATUS
APEnumProcessModuleByZwQueryVirtualMemory(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	HANDLE   ProcessHandle = NULL;

	Status = ObOpenObjectByPointer(EProcess, 
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		GENERIC_ALL,
		*PsProcessType,
		KernelMode,
		&ProcessHandle);
	if (NT_SUCCESS(Status))
	{
		MEMORY_BASIC_INFORMATION mbi = { 0 };
		PMEMORY_SECTION_NAME msn = (PMEMORY_SECTION_NAME)ExAllocatePool(PagedPool, MAX_PATH);
		if (msn)
		{
			__try
			{
				for (SIZE_T BaseAddress = 0; BaseAddress <= (SIZE_T)g_DynamicData.MaxUserSpaceAddress; BaseAddress += 0x10000)
				{
					SIZE_T ReturnLength = 0;

					Status = ZwQueryVirtualMemory(ProcessHandle,
						(PVOID)BaseAddress,
						MemoryBasicInformation,
						(PVOID)&mbi,
						sizeof(MEMORY_BASIC_INFORMATION),
						&ReturnLength);
					// 判断类型，若是Image则查询SectionName
					if (NT_SUCCESS(Status) && mbi.Type == MEM_IMAGE)
					{
						Status = ZwQueryVirtualMemory(ProcessHandle,
							(PVOID)BaseAddress,
							MemorySectionName,
							(PVOID)msn,
							sizeof(MEMORY_SECTION_NAME),
							&ReturnLength);
						// 判断错误是不是申请的内存不足
						if (!NT_SUCCESS(Status) && Status == STATUS_INFO_LENGTH_MISMATCH)
						{
							ExFreePool(msn);
							msn = (PMEMORY_SECTION_NAME)ExAllocatePool(PagedPool, ReturnLength);
							if (msn)
							{
								Status = ZwQueryVirtualMemory(ProcessHandle,
									(PVOID)BaseAddress,
									MemorySectionName,
									(PVOID)msn,
									sizeof(MEMORY_SECTION_NAME),
									&ReturnLength);
								if (!NT_SUCCESS(Status))
								{
									continue;
								}
							}
							else
							{
								continue;
							}
						}
					
						if (!APIsModuleInList((UINT_PTR)mbi.BaseAddress, (UINT32)mbi.RegionSize, pmi, ModuleCount))
						{
							if (ModuleCount > pmi->NumberOfModules)	// Ring3给的大 就继续插
							{	
								WCHAR  wzNtFullPath[MAX_PATH] = { 0 };
								SIZE_T TravelAddress = 0;

								// 模块名称
								APDosPathToNtPath(msn->NameBuffer, wzNtFullPath);
								StringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, wcslen(wzNtFullPath) + 1, wzNtFullPath);
								
								// 模块基地址
								pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)mbi.BaseAddress;
								
								// 获得模块大小
								for (SIZE_T TravelAddress = 0; TravelAddress <= (SIZE_T)g_DynamicData.MaxUserSpaceAddress; TravelAddress += mbi.RegionSize)
								{
									Status = ZwQueryVirtualMemory(ProcessHandle,
										(PVOID)TravelAddress,
										MemoryBasicInformation,
										(PVOID)&mbi,
										sizeof(MEMORY_BASIC_INFORMATION),
										&ReturnLength);
									if (NT_SUCCESS(Status) && mbi.Type != MEM_IMAGE)
									{
										break;
									}
								}

								pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = TravelAddress - BaseAddress;
								
							}
							pmi->NumberOfModules++;
						}
					}
				}

				// 枚举到了东西
				if (pmi->NumberOfModules)
				{
					Status = STATUS_SUCCESS;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("Catch Exception\r\n");
				Status = STATUS_UNSUCCESSFUL;
			}
		}
		ZwClose(ProcessHandle);
	}
	return Status;
}


/************************************************************************
*  Name : APFillProcessModuleInfoByTravelLdr
*  Param: LdrListEntry			模块基地址（OUT）
*  Param: ModuleSize			模块大小（IN）
*  Ret  : NTSTATUS
*  通过FileObject获得进程完整路径
************************************************************************/
VOID
APFillProcessModuleInfoByTravelLdr(IN PLIST_ENTRY LdrListEntry, IN eLdrType LdrType, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{

	for (PLIST_ENTRY TravelListEntry = LdrListEntry->Flink;
		TravelListEntry != LdrListEntry;
		TravelListEntry = (PLIST_ENTRY)TravelListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = NULL;
		switch (LdrType)
		{
		case lt_InLoadOrderModuleList:
		{
			LdrDataTableEntry = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
			break;
		}
		case lt_InMemoryOrderModuleList:
		{
			LdrDataTableEntry = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
			break;
		}
		case lt_InInitializationOrderModuleList:
		{
			LdrDataTableEntry = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
			break;
		}
		default:
			break;
		}

		if ((PUINT8)LdrDataTableEntry > 0 && MmIsAddressValid(LdrDataTableEntry))
		{
			// 插入
			if (!APIsModuleInList((UINT_PTR)LdrDataTableEntry->DllBase, LdrDataTableEntry->SizeOfImage, pmi, ModuleCount))
			{
				if (ModuleCount > pmi->NumberOfModules)	// Ring3给的大 就继续插
				{
					pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)LdrDataTableEntry->DllBase;
					pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = LdrDataTableEntry->SizeOfImage;
					//wcsncpy(pmi->ModuleEntry[pmi->NumberOfModules].wzFullPath, LdrDataTableEntry->FullDllName.Buffer, LdrDataTableEntry->FullDllName.Length);
					StringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, LdrDataTableEntry->FullDllName.Length, LdrDataTableEntry->FullDllName.Buffer);
				}
				pmi->NumberOfModules++;
			}
		}
	}
}


/************************************************************************
*  Name : APEnumProcessModuleByPeb
*  Param: EProcess			      进程结构体
*  Param: pmi			          ring3内存
*  Param: ModuleCount			  
*  Ret  : NTSTATUS
*  通过遍历peb的Ldr三根链表中的一个表（处理Wow64）
************************************************************************/
NTSTATUS
APEnumProcessModuleByPeb(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	BOOLEAN bAttach = FALSE;
	KAPC_STATE ApcState;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PPEB Peb = NULL;

	KeStackAttachProcess(EProcess, &ApcState);     // attach到目标进程内,因为要访问的是用户空间地址
	bAttach = TRUE;

	__try
	{
		LARGE_INTEGER	Interval = { 0 };
		Interval.QuadPart = -25011 * 10 * 1000;		// 250 毫秒

		if (PsGetProcessWow64Process(EProcess))		// 还要处理 Wow64的问题
		{
			PPEB32 Peb32 = (PPEB32)PsGetProcessWow64Process(EProcess);
			if (Peb32 == NULL)
			{
				return Status;
			}
			
			for (INT i = 0; !Peb32->Ldr && i < 10; i++)
			{
				// Sleep 等待加载
				KeDelayExecutionThread(KernelMode, TRUE, &Interval);
			}

			if (Peb32->Ldr)
			{
				ProbeForRead((PVOID)Peb32->Ldr, sizeof(UINT32), sizeof(UINT8));

				// Travel InLoadOrderModuleList
				for (PLIST_ENTRY32 TravelListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)Peb32->Ldr)->InLoadOrderModuleList.Flink;
					TravelListEntry != &((PPEB_LDR_DATA32)Peb32->Ldr)->InLoadOrderModuleList;
					TravelListEntry = (PLIST_ENTRY32)TravelListEntry->Flink)
				{
					PLDR_DATA_TABLE_ENTRY32 LdrDataTableEntry32 = NULL;
					LdrDataTableEntry32 = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

					if ((PUINT8)LdrDataTableEntry32 > 0 && MmIsAddressValid(LdrDataTableEntry32))
					{
						// 插入
						if (!APIsModuleInList((UINT_PTR)LdrDataTableEntry32->DllBase, LdrDataTableEntry32->SizeOfImage, pmi, ModuleCount))
						{
							if (ModuleCount > pmi->NumberOfModules)	// Ring3给的大 就继续插
							{
								pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)LdrDataTableEntry32->DllBase;
								pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = LdrDataTableEntry32->SizeOfImage;
								StringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, LdrDataTableEntry32->FullDllName.Length, (LPCWSTR)LdrDataTableEntry32->FullDllName.Buffer);
							}
							pmi->NumberOfModules++;
						}
					}
				}
				// 枚举到了东西
				if (pmi->NumberOfModules)
				{
					Status = STATUS_SUCCESS;
				}
			}
		}

		// Native process
		Peb = PsGetProcessPeb(EProcess);
		if (Peb == NULL)
		{
			return Status;
		}

		for (INT i = 0; Peb->Ldr == 0 && i < 10; i++)
		{
			// Sleep 等待加载
			KeDelayExecutionThread(KernelMode, TRUE, &Interval);
		}

		if (Peb->Ldr > 0)
		{
			// 因为peb是用户层数据，可能无法访问
			ProbeForRead((PVOID)Peb->Ldr, sizeof(PVOID), sizeof(UINT8));

			for (PLIST_ENTRY TravelListEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
				TravelListEntry != &Peb->Ldr->InLoadOrderModuleList;
				TravelListEntry = (PLIST_ENTRY)TravelListEntry->Flink)
			{
				PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = NULL;
				LdrDataTableEntry = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

				if ((PUINT8)LdrDataTableEntry > 0 && MmIsAddressValid(LdrDataTableEntry))
				{
					// 插入
					if (!APIsModuleInList((UINT_PTR)LdrDataTableEntry->DllBase, LdrDataTableEntry->SizeOfImage, pmi, ModuleCount))
					{
						if (ModuleCount > pmi->NumberOfModules)	// Ring3给的大 就继续插
						{
							pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)LdrDataTableEntry->DllBase;
							pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = LdrDataTableEntry->SizeOfImage;
							StringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, LdrDataTableEntry->FullDllName.Length, LdrDataTableEntry->FullDllName.Buffer);
						}
						pmi->NumberOfModules++;
					}
				}
			}

			// 枚举三根链表
//			APFillProcessModuleInfoByTravelLdr((PLIST_ENTRY)&(LdrData->InLoadOrderModuleList), lt_InLoadOrderModuleList, pmi, ModuleCount);
//			APFillProcessModuleInfoByTravelLdr((PLIST_ENTRY)&(LdrData->InMemoryOrderModuleList), lt_InMemoryOrderModuleList, pmi, ModuleCount);
//			APFillProcessModuleInfoByTravelLdr((PLIST_ENTRY)&(LdrData->InInitializationOrderModuleList), lt_InInitializationOrderModuleList, pmi, ModuleCount);
			// 枚举到了东西
			if (pmi->NumberOfModules)
			{
				Status = STATUS_SUCCESS;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Catch Exception\r\n");
		Status = STATUS_UNSUCCESSFUL;
	}

	if (bAttach)
	{
		KeUnstackDetachProcess(&ApcState);
		bAttach = FALSE;
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
APEnumProcessModule(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    ModuleCount = (OutputLength - sizeof(PROCESS_MODULE_INFORMATION)) / sizeof(PROCESS_MODULE_ENTRY_INFORMATION);
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
		PPROCESS_MODULE_INFORMATION pmi = (PPROCESS_MODULE_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (pmi)
		{
			RtlZeroMemory(pmi, OutputLength);

/*			Status = APEnumProcessModuleByZwQueryVirtualMemory(EProcess, pmi, ModuleCount);
			if (NT_SUCCESS(Status))
			{
				if (ModuleCount >= pmi->NumberOfModules)
				{
					RtlCopyMemory(OutputBuffer, pmi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_MODULE_INFORMATION)OutputBuffer)->NumberOfModules = pmi->NumberOfModules;    // 让Ring3知道需要多少个
					Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
				}
			}
			else
			{*/
				Status = APEnumProcessModuleByPeb(EProcess, pmi, ModuleCount);
				if (NT_SUCCESS(Status))
				{
					if (ModuleCount >= pmi->NumberOfModules)
					{
						RtlCopyMemory(OutputBuffer, pmi, OutputLength);
						Status = STATUS_SUCCESS;
					}
					else
					{
						((PPROCESS_MODULE_INFORMATION)OutputBuffer)->NumberOfModules = pmi->NumberOfModules;    // 让Ring3知道需要多少个
						Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
					}
				}
			//}

			ExFreePool(pmi);
			pmi = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}







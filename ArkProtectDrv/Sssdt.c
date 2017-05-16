#include "Sssdt.h"

extern DYNAMIC_DATA            g_DynamicData;
extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;
extern PWCHAR                  g_SssdtFunctionName[0x400];

PVOID    g_ReloadWin32kImage = NULL;       // 重载Win32k的基地址
PKSERVICE_TABLE_DESCRIPTOR g_CurrentWin32pServiceTableAddress = NULL;   // 当前系统运行着的Win32k的ServiceTable基地址
KSERVICE_TABLE_DESCRIPTOR  g_ReloadWin32pServiceTableAddress = { 0 };   // ShadowSsdt也在Ntkrnl里，ShawdowSsdt->base在Win32k里
UINT_PTR g_OriginalSssdtFunctionAddress[0x400] = { 0 };    // SssdtFunction原本的地址
//UINT32   g_SssdtItem[0x400] = { 0 };                       // Sssdt表里面原始存放的数据


/************************************************************************
*  Name : APGetCurrentSssdtAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得SSSDT地址 （x86 搜索导出表/x64 硬编码，算偏移）
************************************************************************/
UINT_PTR
APGetCurrentSssdtAddress()
{
	UINT_PTR CurrentSssdtAddress = 0;

#ifdef _WIN64
	/*
	kd> rdmsr c0000082
	msr[c0000082] = fffff800`03e81640
	*/
	PUINT8	StartSearchAddress = (PUINT8)__readmsr(0xC0000082);   // fffff800`03ecf640
	PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0, v3 = 0;
	INT32   iOffset = 0;    // 002320c7 偏移不会超过4字节

	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		/*
		kd> u fffff800`03e81640 l 500
		nt!KiSystemCall64:
		fffff800`03e81640 0f01f8          swapgs
		......

		nt!KiSystemServiceRepeat:
		fffff800`03e9c772 4c8d15c7202300  lea     r10,[nt!KeServiceDescriptorTable (fffff800`040ce840)]
		fffff800`03e9c779 4c8d1d00212300  lea     r11,[nt!KeServiceDescriptorTableShadow (fffff800`040ce880)]
		fffff800`03e9c780 f7830001000080000000 test dword ptr [rbx+100h],80h

		TargetAddress = CurrentAddress + Offset + 7
		fffff800`040ce840 = fffff800`03e9c772 + 0x002320c7 + 7
		*/

		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			v1 = *i;
			v2 = *(i + 1);
			v3 = *(i + 2);
			if (v1 == 0x4c && v2 == 0x8d && v3 == 0x1d)		// 硬编码  lea r11
			{
				RtlCopyMemory(&iOffset, i + 3, 4);
				CurrentSssdtAddress = (UINT_PTR)(iOffset + (UINT64)i + 7);
				break;
			}
		}
	}

#else
	UINT32 KeAddSystemServiceTableAddress = 0;

	APGetNtosExportVariableAddress(L"KeAddSystemServiceTable", (PVOID*)&KeAddSystemServiceTableAddress);
	if (KeAddSystemServiceTableAddress)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeAddSystemServiceTableAddress;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;

		for (i = StartSearchAddress; i<EndSearchAddress; i++)
		{
			/*
			3: kd> u KeAddSystemServiceTable l 10
			nt!KeAddSystemServiceTable:
			83fa30a0 8bff            mov     edi,edi
			83fa30a2 55              push    ebp
			83fa30a3 8bec            mov     ebp,esp
			83fa30a5 837d1801        cmp     dword ptr [ebp+18h],1
			83fa30a9 7760            ja      nt!KeAddSystemServiceTable+0x6b (83fa310b)
			83fa30ab 8b4518          mov     eax,dword ptr [ebp+18h]
			83fa30ae c1e004          shl     eax,4
			83fa30b1 83b8000bf88300  cmp     dword ptr nt!KeServiceDescriptorTable (83f80b00)[eax],0
			83fa30b8 7551            jne     nt!KeAddSystemServiceTable+0x6b (83fa310b)
			83fa30ba 8d88400bf883    lea     ecx,nt!KeServiceDescriptorTableShadow (83f80b40)[eax]
			83fa30c0 833900          cmp     dword ptr [ecx],0
			*/

			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 6);
				if (v1 == 0x8d && v2 == 0x88 && v3 == 0x83)		// 硬编码  lea ecx
				{
					RtlCopyMemory(&CurrentSssdtAddress, i + 2, 4);
					break;
				}
			}
		}

	}

#endif

	DbgPrint("SSDTAddress is %p\r\n", CurrentSssdtAddress);

	return CurrentSssdtAddress;
}


UINT_PTR
APGetCurrentWin32pServiceTable()
{
	/*
	kd> dq fffff800`040be980
	fffff800`040be980  fffff800`03e87800 00000000`00000000
	fffff800`040be990  00000000`00000191 fffff800`03e8848c
	fffff800`040be9a0  fffff960`000e1f00 00000000`00000000
	fffff800`040be9b0  00000000`0000033b fffff960`000e3c1c

	kd> dq win32k!W32pServiceTable
	fffff960`000e1f00  fff0b501`fff3a740 001021c0`000206c0
	fffff960`000e1f10  00022640`00096000 ffde0b03`fff9a900

	*/

	if (g_CurrentWin32pServiceTableAddress == NULL)
	{
		(UINT_PTR)g_CurrentWin32pServiceTableAddress = APGetCurrentSssdtAddress() + sizeof(KSERVICE_TABLE_DESCRIPTOR);    // 过Ssdt 
	}

	return (UINT_PTR)g_CurrentWin32pServiceTableAddress;
}


/************************************************************************
*  Name : APFixWin32pServiceTable
*  Param: ImageBase			    新模块加载基地址 （PVOID）
*  Param: OriginalBase		    原模块加载基地址 （PVOID）
*  Ret  : VOID
*  修正Win32pServiceTable 以及base里面的函数
************************************************************************/
VOID
APFixWin32pServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase)
{
	UINT_PTR KrnlOffset = (INT64)((UINT_PTR)ImageBase - (UINT_PTR)OriginalBase);

	DbgPrint("Krnl Offset :%x\r\n", KrnlOffset);

	// 给SSDT赋值

	g_ReloadWin32pServiceTableAddress.Base = (PUINT_PTR)((UINT_PTR)(g_CurrentWin32pServiceTableAddress->Base) + KrnlOffset);
	g_ReloadWin32pServiceTableAddress.Limit = g_CurrentWin32pServiceTableAddress->Limit;
	g_ReloadWin32pServiceTableAddress.Number = g_CurrentWin32pServiceTableAddress->Number;

	DbgPrint("New Win32pServiceTable:%p\r\n", g_ReloadWin32pServiceTableAddress);
	DbgPrint("New Win32pServiceTable Base:%p\r\n", g_ReloadWin32pServiceTableAddress.Base);

	// 给Base里的每个成员赋值（函数地址）
	if (MmIsAddressValid(g_ReloadWin32pServiceTableAddress.Base))
	{

#ifdef _WIN64

		// 刚开始保存的是函数的真实地址，我们保存在自己的全局数组中
		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_OriginalSssdtFunctionAddress[i] = *(UINT64*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + i * 8);
		}

		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			UINT32 Temp = 0;
			Temp = (UINT32)(g_OriginalSssdtFunctionAddress[i] - (UINT64)g_CurrentWin32pServiceTableAddress->Base);
			Temp += ((UINT64)g_CurrentWin32pServiceTableAddress->Base & 0xffffffff);
			// 更新Ssdt->base中的成员为相对于Base的偏移
			*(UINT32*)((UINT64)g_ReloadWin32pServiceTableAddress.Base + i * 4) = (Temp - ((UINT64)g_CurrentWin32pServiceTableAddress->Base & 0xffffffff)) << 4;
		}

		DbgPrint("Current%p\n", g_CurrentWin32pServiceTableAddress->Base);
		DbgPrint("Reload%p\n", g_ReloadWin32pServiceTableAddress.Base);

	/*	for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_SssdtItem[i] = *(UINT32*)((UINT64)g_ReloadWin32pServiceTableAddress.Base + i * 4);
		}*/
#else
		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_OriginalSssdtFunctionAddress[i] = *(UINT32*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + i * 4);
			//g_SssdtItem[i] = g_OriginalSssdtFunctionAddress[i];
			*(UINT32*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + i * 4) += KrnlOffset;      // 将所有Ssdt函数地址转到我们新加载到内存中的地址
		}
#endif // _WIN64

	}
	else
	{
		DbgPrint("New Win32pServiceTable Base is not valid\r\n");
	}
}


/************************************************************************
*  Name : APReloadWin32k
*  Param: VOID
*  Ret  : NTSTATUS
*  重载内核第一模块
************************************************************************/
NTSTATUS
APReloadWin32k()
{
	NTSTATUS    Status = STATUS_SUCCESS;

	if (g_ReloadWin32kImage == NULL)
	{
		PVOID          FileBuffer = NULL;
		PLDR_DATA_TABLE_ENTRY Win32kLdr = NULL;

		Win32kLdr = APGetDriverModuleLdr(L"win32k.sys", g_PsLoadedModuleList);

		Status = STATUS_UNSUCCESSFUL;

		// 2.读取第一模块文件到内存，按内存对齐格式完成PE的IAT，BaseReloc修复
		FileBuffer = APGetFileBuffer(&Win32kLdr->FullDllName);
		if (FileBuffer)
		{
			PIMAGE_DOS_HEADER DosHeader = NULL;
			PIMAGE_NT_HEADERS NtHeader = NULL;
			PIMAGE_SECTION_HEADER SectionHeader = NULL;

			DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
			if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
			{
				NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)FileBuffer + DosHeader->e_lfanew);
				if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
				{
					g_ReloadWin32kImage = ExAllocatePool(NonPagedPool, NtHeader->OptionalHeader.SizeOfImage);
					if (g_ReloadWin32kImage)
					{
						DbgPrint("New Base::%p\r\n", g_ReloadWin32kImage);

						// 2.1.开始拷贝数据
						RtlZeroMemory(g_ReloadWin32kImage, NtHeader->OptionalHeader.SizeOfImage);
						// 2.1.1.拷贝头
						RtlCopyMemory(g_ReloadWin32kImage, FileBuffer, NtHeader->OptionalHeader.SizeOfHeaders);
						// 2.1.2.拷贝节区
						SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
						for (UINT16 i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
						{
							RtlCopyMemory((PUINT8)g_ReloadWin32kImage + SectionHeader[i].VirtualAddress,
								(PUINT8)FileBuffer + SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData);
						}

						// 2.2.修复导入地址表
						APFixImportAddressTable(g_ReloadWin32kImage);

						// 2.3.修复重定向表
						APFixRelocBaseTable(g_ReloadWin32kImage, Win32kLdr->DllBase);

						// 2.4.修复SSDT
						APFixWin32pServiceTable(g_ReloadWin32kImage, Win32kLdr->DllBase);

						Status = STATUS_SUCCESS;
					}
					else
					{
						DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
					}
				}
				else
				{
					DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
				}
			}
			else
			{
				DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
			}
			ExFreePool(FileBuffer);
			FileBuffer = NULL;
		}

	}

	return Status;
}


/************************************************************************
*  Name : APEnumSssdtHook
*  Param: shi              
*  Param: SssdtFunctionCount
*  Ret  : NTSTATUS
*  重载Win32k 检查Sssdt Hook
************************************************************************/
NTSTATUS
APEnumSssdtHookByReloadWin32k(OUT PSSSDT_HOOK_INFORMATION shi, IN UINT32 SssdtFunctionCount)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;
	// 1.获得当前的SSSDT
	g_CurrentWin32pServiceTableAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentWin32pServiceTable();
	if (g_CurrentWin32pServiceTableAddress && MmIsAddressValid(g_CurrentWin32pServiceTableAddress))
	{
		// 2.Attach到gui进程
		PEPROCESS GuiEProcess = APGetGuiProcess();
		if (GuiEProcess &&MmIsAddressValid(GuiEProcess))
		{
			KAPC_STATE	ApcState = { 0 };

			// 转到目标进程空间上下背景文里
			KeStackAttachProcess(GuiEProcess, &ApcState);

			// 3.重载内核SSDT(得到原先的SSDT函数地址数组)
			Status = APReloadWin32k();
			if (NT_SUCCESS(Status))
			{
				// 3.对比Original&Current
				for (UINT32 i = 0; i < g_CurrentWin32pServiceTableAddress->Limit; i++)
				{
					if (SssdtFunctionCount >= shi->NumberOfSssdtFunctions)
					{
#ifdef _WIN64
						// 64位存储的是 偏移（高28位）
						//INT32 OriginalOffset = g_SssdtItem[i] >> 4;
						INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentWin32pServiceTableAddress->Base + i * 4)) >> 4;    // 带符号位的移位

						UINT64 CurrentSssdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + CurrentOffset);
						UINT64 OriginalSssdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#else
						// 32位存储的是 绝对地址
						UINT32 CurrentSssdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentWin32pServiceTableAddress->Base + i * 4);
						UINT32 OriginalSssdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#endif // _WIN64

						if (OriginalSssdtFunctionAddress != CurrentSssdtFunctionAddress)   // 表明被Hook了
						{
							shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].bHooked = FALSE;
						}
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].Ordinal = i;
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].CurrentAddress = CurrentSssdtFunctionAddress;
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].OriginalAddress = OriginalSssdtFunctionAddress;

						RtlStringCchCopyW(shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].wzFunctionName, wcslen(g_SssdtFunctionName[i]) + 1, g_SssdtFunctionName[i]);

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					shi->NumberOfSssdtFunctions++;
				}
			}
			else
			{
				DbgPrint("Reload Win32k & Sssdt Failed\r\n");
			}

			KeUnstackDetachProcess(&ApcState);
		}
	}
	else
	{
		DbgPrint("Get Current Sssdt Failed\r\n");
	}

	return Status;
}


/************************************************************************
*  Name : APEnumSssdtHook
*  Param: OutputBuffer            ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举进程模块
************************************************************************/
NTSTATUS
APEnumSssdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    SssdtFunctionCount = (OutputLength - sizeof(SSSDT_HOOK_INFORMATION)) / sizeof(SSSDT_HOOK_ENTRY_INFORMATION);

	PSSSDT_HOOK_INFORMATION shi = (PSSSDT_HOOK_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
	if (shi)
	{
		RtlZeroMemory(shi, OutputLength);

		Status = APEnumSssdtHookByReloadWin32k(shi, SssdtFunctionCount);
		if (NT_SUCCESS(Status))
		{
			if (SssdtFunctionCount >= shi->NumberOfSssdtFunctions)
			{
				RtlCopyMemory(OutputBuffer, shi, OutputLength);
				Status = STATUS_SUCCESS;
			}
			else
			{
				((PSSSDT_HOOK_INFORMATION)OutputBuffer)->NumberOfSssdtFunctions = shi->NumberOfSssdtFunctions;    // 让Ring3知道需要多少个
				Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
			}
		}

		ExFreePool(shi);
		shi = NULL;
	}

	return Status;
}


/************************************************************************
*  Name : APResumeSssdtHook
*  Param: Ordinal           函数序号
*  Ret  : NTSTATUS
*  恢复指定的SsdtHook进程模块
************************************************************************/
NTSTATUS
APResumeSssdtHook(IN UINT32 Ordinal)
{
	NTSTATUS       Status = STATUS_UNSUCCESSFUL;

	if (Ordinal == RESUME_ALL_HOOKS)
	{
		// 恢复所有SsdtHook

		// 对比Original&Current
		for (UINT32 i = 0; i < g_CurrentWin32pServiceTableAddress->Limit; i++)
		{

#ifdef _WIN64
			// 64位存储的是 偏移（高28位）
			INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentWin32pServiceTableAddress->Base + i * 4)) >> 4;    // 带符号位的移位

			UINT64 CurrentSsdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + CurrentOffset);
			UINT64 OriginalSsdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#else
			// 32位存储的是 绝对地址
			UINT32 CurrentSsdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentWin32pServiceTableAddress->Base + i * 4);
			UINT32 OriginalSsdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#endif // _WIN64

			if (OriginalSsdtFunctionAddress != CurrentSsdtFunctionAddress)   // 表明被Hook了
			{
				APPageProtectOff();

				*(UINT32*)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + i * 4) = *(UINT32*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + i * 4);

				APPageProtectOn();
			}
		}

		Status = STATUS_SUCCESS;
	}
	else if (Ordinal > g_CurrentWin32pServiceTableAddress->Limit)
	{
		Status = STATUS_INVALID_PARAMETER;
	}
	else
	{
		// 恢复指定项的SssdtHook
		// 需要做的是将当前Sssdt中保存的值改为g_ReloadWin32pServiceTableAddress.Base中的保存的值

		APPageProtectOff();

		*(UINT32*)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + Ordinal * 4) = *(UINT32*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + Ordinal * 4);

		APPageProtectOn();

		Status = STATUS_SUCCESS;
	}

	return Status;
}


UINT_PTR
APGetSssdtFunctionAddress(IN PCWCHAR wzFunctionName)
{
	UINT32   Ordinal = 0;
	BOOL     bOk = FALSE;
	UINT_PTR FunctionAddress = 0;

	g_CurrentWin32pServiceTableAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentWin32pServiceTable();
	if (g_CurrentWin32pServiceTableAddress && MmIsAddressValid(g_CurrentWin32pServiceTableAddress))
	{
		for (Ordinal = 0; Ordinal < g_CurrentWin32pServiceTableAddress->Limit; Ordinal++)
		{
			if (_wcsicmp(g_SssdtFunctionName[Ordinal], wzFunctionName) == 0)
			{
				// 拿到了 Ordinal !
				bOk = TRUE;
				break;
			}
		}

		if (bOk)
		{
#ifdef _WIN64
			INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentWin32pServiceTableAddress->Base + Ordinal * 4)) >> 4;    // 带符号位的移位

			FunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + CurrentOffset);
#else
			FunctionAddress = *(UINT32*)((UINT32)g_CurrentWin32pServiceTableAddress->Base + Ordinal * 4);
#endif // !_WIN64
		}
		else
		{
			DbgPrint("Get Sssdt Function Ordinal Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Get Sssdt Address Failed\r\n");
	}
	return FunctionAddress;
}
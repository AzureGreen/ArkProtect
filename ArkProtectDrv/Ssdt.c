#include "Ssdt.h"

extern DYNAMIC_DATA            g_DynamicData;
extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;

PVOID    g_ImageBuffer = NULL;       // 重载内核的基地址
PKSERVICE_TABLE_DESCRIPTOR g_CurrentSsdtAddress = NULL;  // 当前系统运行着的Ntos的Ssdt基地址
PKSERVICE_TABLE_DESCRIPTOR g_ReloadSsdtAddress = NULL;   // 我们重载出来的Ntos的Ssdt基地址
UINT_PTR g_OriginalSsdtFunctionAddress[0x200] = { 0 };   // SsdtFunction原本的地址
UINT32   g_SsdtItem[0x200] = { 0 };                       // Ssdt表里面原始存放的数据
WCHAR    g_SsdtFunctionName[0x200][100] = { 0 };          // Ssdt函数名称表（按序号存放）


/************************************************************************
*  Name : APGetCurrentSsdtAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得SSDT地址 （x86 搜索导出表/x64 硬编码，算偏移）
************************************************************************/
UINT_PTR
APGetCurrentSsdtAddress()
{
	if (g_CurrentSsdtAddress == NULL)
	{
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
				if (v1 == 0x4c && v2 == 0x8d && v3 == 0x15)		// 硬编码  lea r10
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					(UINT_PTR)g_CurrentSsdtAddress = (UINT_PTR)(iOffset + (UINT64)i + 7);
				}
			}
		}

#else

		/*
		kd> dd KeServiceDescriptorTable
		80553fa0  80502b8c 00000000 0000011c 80503000
		*/

		// 在Ntoskrnl.exe的导出表中，获取到KeServiceDescriptorTable地址
		APGetNtosExportVariableAddress(L"KeServiceDescriptorTable", (PVOID*)&g_CurrentSsdtAddress);

#endif
	}

	DbgPrint("SSDTAddress is %p\r\n", g_CurrentSsdtAddress);

	return (UINT_PTR)g_CurrentSsdtAddress;
}


/************************************************************************
*  Name : APInitializeSsdtFunctionName
*  Param: void
*  Ret  : NTSTATUS
*  初始化保存SsdtFunctionNamde的全局数组
************************************************************************/
NTSTATUS
APInitializeSsdtFunctionName()
{
	NTSTATUS  Status = STATUS_SUCCESS;

	PKSERVICE_TABLE_DESCRIPTOR CurrentSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentSsdtAddress();

	if (CurrentSsdtAddress == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (*g_SsdtFunctionName[0] == 0 || *g_SsdtFunctionName[CurrentSsdtAddress->Limit] == 0)
	{
		UINT32    Count = 0;

#ifdef _WIN64

		/* Win7 64bit
		004> u zwopenprocess
		ntdll!ZwOpenProcess:
		00000000`774c1570 4c8bd1          mov     r10,rcx
		00000000`774c1573 b823000000      mov     eax,23h
		00000000`774c1578 0f05            syscall
		00000000`774c157a c3              ret
		00000000`774c157b 0f1f440000      nop     dword ptr [rax+rax]
		*/

		UINT32    SsdtFunctionIndexOffset = 4;

#else

		/* 	Win7 32bit
		kd> u zwopenProcess
		nt!ZwOpenProcess:
		83e9162c b8be000000      mov     eax,0BEh
		83e91631 8d542404        lea     edx,[esp+4]
		83e91635 9c              pushfd
		83e91636 6a08            push    8
		83e91638 e8b1190000      call    nt!KiSystemService (83e92fee)
		83e9163d c21000          ret     10h
		*/

		UINT32    SsdtFunctionIndexOffset = 1;

#endif

		// 1.映射ntdll到内存中
		WCHAR   wzFileFullPath[] = L"\\SystemRoot\\System32\\ntdll.dll";
		PVOID   MappingBaseAddress = NULL;

		Status = APMappingFileInKernelSpace(wzFileFullPath, &MappingBaseAddress);
		if (NT_SUCCESS(Status))
		{
			// 2.读取ntdll的导出表

			PIMAGE_DOS_HEADER       DosHeader = NULL;
			PIMAGE_NT_HEADERS       NtHeader = NULL;

			__try
			{
				DosHeader = (PIMAGE_DOS_HEADER)MappingBaseAddress;
				NtHeader = (PIMAGE_NT_HEADERS)((UINT_PTR)MappingBaseAddress + DosHeader->e_lfanew);
				if (NtHeader && NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
				{
					PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
					PUINT32                 AddressOfFunctions = NULL;      // offset
					PUINT32                 AddressOfNames = NULL;          // offset
					PUINT16                 AddressOfNameOrdinals = NULL;   // Ordinal

					ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PUINT8)MappingBaseAddress + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);		// 导出表地址

					AddressOfFunctions = (PUINT32)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfFunctions);
					AddressOfNames = (PUINT32)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfNames);
					AddressOfNameOrdinals = (PUINT16)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfNameOrdinals);

					// 这里不处理转发，ntdll应该不存在转发
					for (UINT32 i = 0; i < ExportDirectory->NumberOfNames; i++)
					{
						CHAR*                   szFunctionName = NULL;

						szFunctionName = (CHAR*)((PUINT8)MappingBaseAddress + AddressOfNames[i]);   // 获得函数名称

																									// 通过函数名称开头是 ZW 来判断是否是Ssdt函数
						if (szFunctionName[0] == 'Z' && szFunctionName[1] == 'w')
						{
							UINT32   FunctionOrdinal = 0;
							UINT_PTR FunctionAddress = 0;
							INT32    SsdtFunctionIndex = 0;
							WCHAR    wzFunctionName[100] = { 0 };

							FunctionOrdinal = AddressOfNameOrdinals[i];
							FunctionAddress = (UINT_PTR)((PUINT8)MappingBaseAddress + AddressOfFunctions[FunctionOrdinal]);

							SsdtFunctionIndex = *(PUINT32)(FunctionAddress + SsdtFunctionIndexOffset);

							if ((SsdtFunctionIndex >= 0) && (SsdtFunctionIndex < (INT32)CurrentSsdtAddress->Limit))
							{
								APCharToWchar(szFunctionName, wzFunctionName);

								wzFunctionName[0] = 'N';
								wzFunctionName[1] = 't';

								RtlStringCchCopyW(g_SsdtFunctionName[SsdtFunctionIndex], wcslen(wzFunctionName) + 1, wzFunctionName);

								Status = STATUS_SUCCESS;
							}

							Count++;
						}
					}
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("Catch Exception\r\n");
			}

			ZwUnmapViewOfSection(NtCurrentProcess(), MappingBaseAddress);
		}
	}

	return Status;
}


/*
Original
0: kd> dd KeServiceDescriptorTable
83f6f9c0  83e83d9c 00000000 00000191 83e843e4
83f6f9d0  00000000 00000000 00000000 00000000

Before
0: kd> dd AFD6A9C0
afd6a9c0  00000000 00000000 00000000 00000000
afd6a9d0  00000000 00000000 00000000 00000000

After:
1: kd> dd AFD6A9C0
afd6a9c0  afc7ed9c 00000000 00000191 83e843e4
afd6a9d0  00000000 00000000 00000000 00000000

1: kd> dd afc7ed9c
afc7ed9c  afe7ac28 afcc140d afe0ab68 afc2588a
afc7edac  afe7c4ff afcfe3fa afeecb05 afeecb4e

1: kd> u afe7ac28
afe7ac28 8bff            mov     edi,edi
afe7ac2a 55              push    ebp
afe7ac2b 8bec            mov     ebp,esp
afe7ac2d 64a124010000    mov     eax,dword ptr fs:[00000124h]
afe7ac33 66ff8884000000  dec     word ptr [eax+84h]
afe7ac3a 56              push    esi
afe7ac3b 57              push    edi
afe7ac3c 6a01            push    1

*/
/************************************************************************
*  Name : APFixKiServiceTable
*  Param: ImageBase			    新模块加载基地址 （PVOID）
*  Param: OriginalBase		    原模块加载基地址 （PVOID）
*  Ret  : VOID
*  修正SSDT base 以及base里面的函数
************************************************************************/
VOID
APFixKiServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase)
{
	UINT_PTR KrnlOffset = (INT64)((UINT_PTR)ImageBase - (UINT_PTR)OriginalBase);

	DbgPrint("Krnl Offset :%x\r\n", KrnlOffset);

	g_ReloadSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)((UINT_PTR)g_CurrentSsdtAddress + KrnlOffset);
	if (g_ReloadSsdtAddress &&MmIsAddressValid(g_ReloadSsdtAddress))
	{
		// 给SSDT赋值
		g_ReloadSsdtAddress->Base = (PUINT_PTR)((UINT_PTR)(g_CurrentSsdtAddress->Base) + KrnlOffset);
		g_ReloadSsdtAddress->Limit = g_CurrentSsdtAddress->Limit;
		g_ReloadSsdtAddress->Number = g_CurrentSsdtAddress->Number;

		DbgPrint("New KeServiceDescriptorTable:%p\r\n", g_ReloadSsdtAddress);
		DbgPrint("New KeServiceDescriptorTable Base:%p\r\n", g_ReloadSsdtAddress->Base);

		// 给Base里的每个成员赋值（函数地址）
		if (MmIsAddressValid(g_ReloadSsdtAddress->Base))
		{

#ifdef _WIN64

			// 刚开始保存的是函数的真实地址，我们保存在自己的全局数组中
			for (UINT32 i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				g_OriginalSsdtFunctionAddress[i] = *(UINT64*)((ULONG_PTR)g_ReloadSsdtAddress->Base + i * 8);
			}

			for (UINT32 i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				UINT32 Temp = 0;
				Temp = (UINT32)(g_OriginalSsdtFunctionAddress[i] - (UINT64)g_CurrentSsdtAddress->Base);
				Temp += ((UINT64)g_CurrentSsdtAddress->Base & 0xffffffff);
				// 更新Ssdt->base中的成员为相对于Base的偏移
				*(UINT32*)((UINT64)g_ReloadSsdtAddress->Base + i * 4) = (Temp - ((UINT64)g_CurrentSsdtAddress->Base & 0xffffffff)) << 4;
			}

			DbgPrint("CurrentSsdt%p\n", g_CurrentSsdtAddress->Base);
			DbgPrint("ReloaddSsdt%p\n", g_ReloadSsdtAddress->Base);

			for (UINT32 i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				g_SsdtItem[i] = *(UINT32*)((UINT64)g_ReloadSsdtAddress->Base + i * 4);
			}
#else
			for (UINT32 i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				g_OriginalSsdtFunctionAddress[i] = *(UINT32*)(g_ReloadSsdtAddress->Base + i * 4);
				g_SsdtItem[i] = g_OriginalSsdtFunctionAddress[i];
				*(UINT32*)(g_ReloadSsdtAddress->Base + i * 4) += KrnlOffset;      // 将所有Ssdt函数地址转到我们新加载到内存中的地址
			}
#endif // _WIN64

		}
		else
		{
			DbgPrint("New KeServiceDescriptorTable Base is not valid\r\n");
		}
	}
	else
	{
		DbgPrint("New KeServiceDescriptorTable is not valid\r\n");
	}
}


/************************************************************************
*  Name : APReloadNtkrnl
*  Param: VOID
*  Ret  : NTSTATUS
*  重载内核第一模块
************************************************************************/
NTSTATUS
APReloadNtkrnl()
{
	NTSTATUS    Status = STATUS_SUCCESS;

	if (g_ImageBuffer == NULL)
	{
		PLDR_DATA_TABLE_ENTRY NtLdr = NULL;
		PVOID                 FileBuffer = NULL;

		Status = STATUS_UNSUCCESSFUL;

		// 1.获得第一模块信息
		NtLdr = (PLDR_DATA_TABLE_ENTRY)g_PsLoadedModuleList->InLoadOrderLinks.Flink;   // Ntkrnl

		DbgPrint("模块名称:%S\r\n", NtLdr->BaseDllName.Buffer);
		DbgPrint("模块路径:%S\r\n", NtLdr->FullDllName.Buffer);
		DbgPrint("模块地址:%p\r\n", NtLdr->DllBase);
		DbgPrint("模块大小:%x\r\n", NtLdr->SizeOfImage);

		// 2.读取第一模块文件到内存，按内存对齐格式完成PE的IAT，BaseReloc修复
		FileBuffer = APGetFileBuffer(&NtLdr->FullDllName);
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
					g_ImageBuffer = ExAllocatePool(NonPagedPool, NtHeader->OptionalHeader.SizeOfImage);
					if (g_ImageBuffer)
					{
						DbgPrint("New Base::%p\r\n", g_ImageBuffer);

						// 2.1.开始拷贝数据
						RtlZeroMemory(g_ImageBuffer, NtHeader->OptionalHeader.SizeOfImage);
						// 2.1.1.拷贝头
						RtlCopyMemory(g_ImageBuffer, FileBuffer, NtHeader->OptionalHeader.SizeOfHeaders);
						// 2.1.2.拷贝节区
						SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
						for (UINT16 i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
						{
							RtlCopyMemory((PUINT8)g_ImageBuffer + SectionHeader[i].VirtualAddress,
								(PUINT8)FileBuffer + SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData);
						}

						// 2.2.修复导入地址表
						APFixImportAddressTable(g_ImageBuffer);

						// 2.3.修复重定向表
						APFixRelocBaseTable(g_ImageBuffer, NtLdr->DllBase);

						// 2.4.修复SSDT
						APFixKiServiceTable(g_ImageBuffer, NtLdr->DllBase);

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
*  Name : APEnumSsdtHook
*  Param: OutputBuffer            ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举进程模块
************************************************************************/
NTSTATUS
APEnumSsdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    SsdtFunctionCount = (OutputLength - sizeof(SSDT_HOOK_INFORMATION)) / sizeof(SSDT_HOOK_ENTRY_INFORMATION);

	PSSDT_HOOK_INFORMATION shi = (PSSDT_HOOK_INFORMATION)OutputBuffer;

	// 1.获得当前的SSDT
	g_CurrentSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentSsdtAddress();
	if (g_CurrentSsdtAddress && MmIsAddressValid(g_CurrentSsdtAddress))
	{
		// 2.初始化Ssdt函数名称
		Status = APInitializeSsdtFunctionName();
		if (NT_SUCCESS(Status))
		{
			// 3.重载内核SSDT(得到原先的SSDT函数地址数组)
			Status = APReloadNtkrnl();
			if (NT_SUCCESS(Status))
			{
				// 4.对比Original&Current
				for (UINT32 i = 0; i < g_CurrentSsdtAddress->Limit; i++)
				{
					if (SsdtFunctionCount >= shi->NumberOfSsdtFunctions)
					{
#ifdef _WIN64
						// 64位存储的是 偏移（高28位）
						INT32 OriginalOffset = g_SsdtItem[i] >> 4;
						INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentSsdtAddress->Base + i * 4)) >> 4;    // 带符号位的移位

						UINT64 CurrentSsdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentSsdtAddress->Base + CurrentOffset);
						UINT64 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#else
						// 32位存储的是 绝对地址
						UINT32 CurrentSsdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentSsdtAddress->Base + i * 4);
						UINT32 OriginalSsdtFunctionAddress = g_SsdtItem[i];

#endif // _WIN64

						if (OriginalSsdtFunctionAddress != CurrentSsdtFunctionAddress)   // 表明被Hook了
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = FALSE;
						}
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].Ordinal = i;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].CurrentAddress = CurrentSsdtFunctionAddress;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].OriginalAddress = OriginalSsdtFunctionAddress;

						RtlStringCchCopyW(shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].wzFunctionName, wcslen(g_SsdtFunctionName[i]) + 1, g_SsdtFunctionName[i]);

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					shi->NumberOfSsdtFunctions++;
				}
			}
			else
			{
				DbgPrint("Reload Ntkrnl & Ssdt Failed\r\n");
			}
		}
		else
		{
			DbgPrint("Initialize Ssdt Function Name Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Get Current Ssdt Failed\r\n");
	}

	return Status;
}


NTSTATUS
APResumeSsdtHook(IN UINT32 Ordinal)
{
	NTSTATUS       Status = STATUS_UNSUCCESSFUL;
	
	if (Ordinal > g_CurrentSsdtAddress->Limit)
	{
		Status = STATUS_INVALID_PARAMETER;
	}
	else
	{
		// 需要做的是将当前Ssdt中保存的值改为g_SsdtItem[i]中的对应项
		
		*(UINT32*)((UINT_PTR)g_CurrentSsdtAddress->Base + Ordinal * 4) = g_SsdtItem[Ordinal];
				
		Status = STATUS_SUCCESS;
	}
	
	return Status;
}
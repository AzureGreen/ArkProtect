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
*  Name : APMappingFileInKernelSpace
*  Param: wzFileFullPath		文件完整路径
*  Param: MappingBaseAddress	映射后的基地址 (OUT)
*  Ret  : BOOLEAN
*  将PE文件映射到内核空间
************************************************************************/
NTSTATUS
APMappingFileInKernelSpace(IN WCHAR* wzFileFullPath, OUT PVOID* MappingBaseAddress)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	if (wzFileFullPath && MappingBaseAddress)
	{
		UNICODE_STRING    uniFileFullPath = { 0 };
		OBJECT_ATTRIBUTES oa = { 0 };
		IO_STATUS_BLOCK   Iosb = { 0 };
		HANDLE			  FileHandle = NULL;
		HANDLE			  SectionHandle = NULL;

		RtlInitUnicodeString(&uniFileFullPath, wzFileFullPath);		// 常量指针格式化到unicode
		InitializeObjectAttributes(&oa,									// 初始化 oa
			&uniFileFullPath,											// Dll完整路径
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,					// 不区分大小写 | 内核句柄
			NULL,
			NULL);

		Status = IoCreateFile(&FileHandle,								// 获得文件句柄
			GENERIC_READ | SYNCHRONIZE,									// 同步读
			&oa,														// 文件绝对路径
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			CreateFileTypeNone,
			NULL,
			IO_NO_PARAMETER_CHECKING);

		if (NT_SUCCESS(Status))
		{
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

			Status = ZwCreateSection(&SectionHandle,			// 创建节对象,用于后面文件映射 （CreateFileMapping）
				SECTION_QUERY | SECTION_MAP_READ,
				&oa,
				NULL,
				PAGE_WRITECOPY,
				SEC_IMAGE,              // 内存对齐
				FileHandle);

			if (NT_SUCCESS(Status))
			{
				SIZE_T MappingViewSize = 0;

				Status = ZwMapViewOfSection(SectionHandle,
					ZwCurrentProcess(),				// 映射到当前进程的内存空间中 System
					MappingBaseAddress,
					0,
					0,
					0,
					&MappingViewSize,
					ViewUnmap,
					0,
					PAGE_WRITECOPY);

				ZwClose(SectionHandle);
			}
			ZwClose(FileHandle);
		}
	}

	return Status;
}


NTSTATUS
APInitializeSsdtFunctionName()
{
	NTSTATUS  Status = STATUS_SUCCESS;

	if (g_CurrentSsdtAddress == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (*g_SsdtFunctionName[0] == 0 || *g_SsdtFunctionName[g_CurrentSsdtAddress->Limit] == 0)
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

							if ((SsdtFunctionIndex >= 0) && (SsdtFunctionIndex < (INT32)g_CurrentSsdtAddress->Limit))
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


/************************************************************************
*  Name : APGetFileBuffer
*  Param: uniFilePath			文件路径 （PUNICODE_STRING）
*  Ret  : PVOID                 读取文件到内存的首地址
*  读取文件到内存
************************************************************************/
PVOID
APGetFileBuffer(IN PUNICODE_STRING uniFilePath)
{
	NTSTATUS          Status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES oa = { 0 };
	HANDLE            FileHandle = NULL;
	IO_STATUS_BLOCK   IoStatusBlock = { 0 };
	PVOID             FileBuffer = NULL;

	InitializeObjectAttributes(&oa, uniFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	Status = ZwCreateFile(&FileHandle,
		FILE_READ_DATA,
		&oa,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if (NT_SUCCESS(Status))
	{
		FILE_STANDARD_INFORMATION fsi = { 0 };

		// 文件长度
		Status = ZwQueryInformationFile(FileHandle,
			&IoStatusBlock,
			&fsi,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation);
		if (NT_SUCCESS(Status))
		{
			DbgPrint("%d\r\n", IoStatusBlock.Information);
			DbgPrint("%d\r\n", fsi.EndOfFile.LowPart);

			FileBuffer = ExAllocatePool(PagedPool, fsi.EndOfFile.LowPart);
			if (FileBuffer)
			{
				LARGE_INTEGER ReturnLength = { 0 };

				Status = ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, FileBuffer, fsi.EndOfFile.LowPart, &ReturnLength, NULL);
				if (!NT_SUCCESS(Status))
				{
					DbgPrint("KeGetFileData::ZwReadFile Failed\r\n");
				}
			}
			else
			{
				DbgPrint("KeGetFileData::ZwQueryInformationFile Failed\r\n");
			}
		}
		else
		{
			DbgPrint("KeGetFileData::ZwQueryInformationFile Failed\r\n");
		}
		ZwClose(FileHandle);
	}
	else
	{
		DbgPrint("KeGetFileData::ZwCreateFile Failed\r\n");
	}

	return FileBuffer;
}


/************************************************************************
*  Name : APGetModuleHandle
*  Param: szModuleName			模块名称 （PCHAR）
*  Ret  : PVOID                 模块在内存中首地址
*  通过遍历Ldr枚举模块
************************************************************************/
PVOID
APGetModuleHandle(IN PCHAR szModuleName)
{
	ANSI_STRING       ansiModuleName = { 0 };
	WCHAR             Buffer[256] = { 0 };
	UNICODE_STRING    uniModuleName = { 0 };

	// 单字转双字
	RtlInitAnsiString(&ansiModuleName, szModuleName);
	RtlInitEmptyUnicodeString(&uniModuleName, Buffer, sizeof(Buffer));
	RtlAnsiStringToUnicodeString(&uniModuleName, &ansiModuleName, FALSE);

	for (PLIST_ENTRY TravelListEntry = g_PsLoadedModuleList->InLoadOrderLinks.Flink;
		TravelListEntry != (PLIST_ENTRY)g_PsLoadedModuleList;
		TravelListEntry = TravelListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)TravelListEntry;   // 首成员就是InLoadOrderLinks

		if (_wcsicmp(uniModuleName.Buffer, LdrDataTableEntry->BaseDllName.Buffer) == 0)
		{
			DbgPrint("模块名称：%S\r\n", LdrDataTableEntry->BaseDllName.Buffer);
			DbgPrint("模块基址：%p\r\n", LdrDataTableEntry->DllBase);
			return LdrDataTableEntry->DllBase;
		}
	}

	return NULL;
}

/************************************************************************
*  Name : APGetProcAddress
*  Param: ModuleBase			导出模块基地址 （PVOID）
*  Param: szFunctionName		导出函数名称   （PCHAR）
*  Ret  : PVOID                 导出函数地址
*  获得导出函数地址（处理转发）
************************************************************************/
PVOID
APGetProcAddress(IN PVOID ModuleBase, IN PCHAR szFunctionName)
{
	PIMAGE_DOS_HEADER			DosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
	PIMAGE_NT_HEADERS			NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ModuleBase + DosHeader->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY		ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PUINT8)ModuleBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	UINT32	ExportDirectoryRVA = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	UINT32	ExportDirectorySize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	PUINT32	AddressOfFunctions = (PUINT32)((PUINT8)ModuleBase + ExportDirectory->AddressOfFunctions);
	PUINT32	AddressOfNames = (PUINT32)((PUINT8)ModuleBase + ExportDirectory->AddressOfNames);
	PUINT16	AddressOfNameOrdinals = (PUINT16)((PUINT8)ModuleBase + ExportDirectory->AddressOfNameOrdinals);


	for (UINT32 i = 0; i < ExportDirectory->NumberOfFunctions; i++)
	{
		UINT16	Ordinal = 0xffff;
		PCHAR	Name = NULL;

		// 按序号导出        
		if ((UINT_PTR)szFunctionName <= 0xffff)
		{
			Ordinal = (UINT16)(i);		// 序号导出函数，得到的就是序号
		}
		else if ((UINT_PTR)(szFunctionName) > 0xffff && i < ExportDirectory->NumberOfNames)       // 名称导出的都是地址，肯定比0xffff大,而且可以看出名称导出在序号导出之前
		{
			Name = (PCHAR)((PUINT8)ModuleBase + AddressOfNames[i]);
			Ordinal = (UINT16)(AddressOfNameOrdinals[i]);		// 名称导出表中得到名称导出函数的序号 2字节
		}
		else
		{
			return 0;
		}

		if (((UINT_PTR)(szFunctionName) <= 0xffff && (UINT16)((UINT_PTR)szFunctionName) == (Ordinal + ExportDirectory->Base)) ||
			((UINT_PTR)(szFunctionName) > 0xffff && _stricmp(Name, szFunctionName) == 0))
		{
			// 目前不论是序号导出还是名称导出都是对的进这里
			UINT_PTR FunctionAddress = (UINT_PTR)((PUINT8)ModuleBase + AddressOfFunctions[Ordinal]);		// 得到函数的地址（也许不是真实地址）

																											// 检查是不是forwarder export，如果刚得到的函数地址还在导出表范围内，则涉及到转发器（子dll导入父dll导出的函数后再导出----> 转发器）																						
																											// 因为如果是函数真实地址，就已经超出了导出表地址范围
			if (FunctionAddress >= (UINT_PTR)((PUINT8)ModuleBase + ExportDirectoryRVA) &&
				FunctionAddress <= (UINT_PTR)((PUINT8)ModuleBase + ExportDirectoryRVA + ExportDirectorySize))
			{
				CHAR  szForwarderModuleName[100] = { 0 };
				CHAR  szForwarderFunctionName[100] = { 0 };
				PCHAR Pos = NULL;
				PVOID ForwarderModuleBase = NULL;

				RtlCopyMemory(szForwarderModuleName, (CHAR*)FunctionAddress, strlen((CHAR*)FunctionAddress) + 1);  // 模块名称.导出函数名称

				Pos = strchr(szForwarderModuleName, '.');		// 切断字符串，返回后面部分
				if (!Pos)
				{
					return (PVOID)FunctionAddress;
				}
				*Pos = 0;
				RtlCopyMemory(szForwarderFunctionName, Pos + 1, strlen(Pos + 1) + 1);

				RtlStringCchCopyA(szForwarderModuleName, strlen(".dll") + 1, ".dll");

				ForwarderModuleBase = APGetModuleHandle(szForwarderModuleName);
				if (ForwarderModuleBase == NULL)
				{
					return (PVOID)FunctionAddress;
				}
				return APGetProcAddress(ForwarderModuleBase, szForwarderFunctionName);
			}
			// 不是 Forward Export 就直接break退出for循环，返回 导出函数信息，只有函数地址
			return (PVOID)FunctionAddress;
		}
	}
	return NULL;
}

/************************************************************************
*  Name : APFixImportAddressTable
*  Param: ImageBase			    新模块加载基地址 （PVOID）
*  Ret  : VOID
*  修正导入表  IAT 填充函数地址
************************************************************************/
VOID
APFixImportAddressTable(IN PVOID ImageBase)
{
	PIMAGE_DOS_HEADER         DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS         NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ImageBase + DosHeader->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR  ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((PUINT8)ImageBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (ImportDescriptor->Characteristics)
	{
		PCHAR szImportModuleName = (PCHAR)((PUINT8)ImageBase + ImportDescriptor->Name);        // 导入模块
		PVOID ImportModuleBase = APGetModuleHandle(szImportModuleName);    // 遍历List找到导入模块地址

		if (ImportModuleBase)
		{
			PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((PUINT8)ImageBase + ImportDescriptor->FirstThunk);
			PIMAGE_THUNK_DATA OriginalFirstThunk = (PIMAGE_THUNK_DATA)((PUINT8)ImageBase + ImportDescriptor->OriginalFirstThunk);

			// 遍历导入函数名称表
			for (UINT32 i = 0; OriginalFirstThunk->u1.AddressOfData; i++)
			{
				// 在内核模块中，导入表不存在序号导入
				if (!IMAGE_SNAP_BY_ORDINAL(OriginalFirstThunk->u1.Ordinal))
				{
					PIMAGE_IMPORT_BY_NAME ImportByName = (PIMAGE_IMPORT_BY_NAME)((PUINT8)ImageBase + OriginalFirstThunk->u1.AddressOfData);
					PVOID                 FunctionAddress = NULL;
					FunctionAddress = APGetProcAddress(ImportModuleBase, ImportByName->Name);
					if (FunctionAddress)
					{
						FirstThunk[i].u1.Function = (UINT_PTR)FunctionAddress;
					}
					else
					{
						DbgPrint("FixImportAddressTable::No Such Function\r\n");
					}
				}
				// 没有序号导入
				OriginalFirstThunk++;
			}
		}
		else
		{
			DbgPrint("FixImportAddressTable::No Such Module\r\n");
		}

		ImportDescriptor++;    // 下一张导入表
	}
}


/*
1: kd> u PsLookupProcessByProcessId l 10
nt!PsLookupProcessByProcessId:
84061575 8bff            mov     edi,edi
84061577 55              push    ebp
84061578 8bec            mov     ebp,esp
8406157a 83ec0c          sub     esp,0Ch
8406157d 53              push    ebx
8406157e 56              push    esi
8406157f 648b3524010000  mov     esi,dword ptr fs:[124h]
84061586 33db            xor     ebx,ebx
84061588 66ff8e84000000  dec     word ptr [esi+84h]
8406158f 57              push    edi
84061590 ff7508          push    dword ptr [ebp+8]
84061593 8b3d347ff483    mov     edi,dword ptr [nt!PspCidTable (83f47f34)]
84061599 e8d958feff      call    nt!ExMapHandleToPointer (84046e77)
8406159e 8bf8            mov     edi,eax
840615a0 85ff            test    edi,edi
840615a2 747c            je      nt!PsLookupProcessByProcessId+0xab (84061620)
1: kd> u AFE5C575 l 10
afe5c575 8bff            mov     edi,edi
afe5c577 55              push    ebp
afe5c578 8bec            mov     ebp,esp
afe5c57a 83ec0c          sub     esp,0Ch
afe5c57d 53              push    ebx
afe5c57e 56              push    esi
afe5c57f 648b3524010000  mov     esi,dword ptr fs:[124h]
afe5c586 33db            xor     ebx,ebx
afe5c588 66ff8e84000000  dec     word ptr [esi+84h]
afe5c58f 57              push    edi
afe5c590 ff7508          push    dword ptr [ebp+8]
afe5c593 8b3d347ff483    mov     edi,dword ptr [nt!PspCidTable (83f47f34)]
afe5c599 e8d958feff      call    afe41e77
afe5c59e 8bf8            mov     edi,eax
afe5c5a0 85ff            test    edi,edi
afe5c5a2 747c            je      afe5c620
*/
/************************************************************************
*  Name : APFixRelocBaseTable
*  Param: ReloadBase		    新模块加载基地址 （PVOID）
*  Param: OriginalBase		    原模块加载基地址 （PVOID）
*  Ret  : VOID
*  修正重定向表
************************************************************************/
VOID
APFixRelocBaseTable(IN PVOID ReloadBase, IN PVOID OriginalBase)
{
	PIMAGE_DOS_HEADER         DosHeader = (PIMAGE_DOS_HEADER)ReloadBase;
	PIMAGE_NT_HEADERS         NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ReloadBase + DosHeader->e_lfanew);
	PIMAGE_BASE_RELOCATION    BaseRelocation = (PIMAGE_BASE_RELOCATION)((PUINT8)ReloadBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	if (BaseRelocation)
	{
		while (BaseRelocation->SizeOfBlock)
		{
			PUINT16	TypeOffset = (PUINT16)((PUINT8)BaseRelocation + sizeof(IMAGE_BASE_RELOCATION));
			// 计算需要修正的重定向位项的数目
			UINT32	NumberOfRelocations = (BaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16);

			for (UINT32 i = 0; i < NumberOfRelocations; i++)
			{
				if ((TypeOffset[i] >> 12) == IMAGE_REL_BASED_DIR64)
				{
#ifdef _WIN64
					// 调试发现 Win7 x64的全局变量没有能够修复成功
					PUINT64	RelocAddress = (PUINT64)((PUINT8)ReloadBase + BaseRelocation->VirtualAddress + (TypeOffset[i] & 0x0FFF));  // 定位到重定向块
					*RelocAddress = (UINT64)(*RelocAddress + (UINT_PTR)((UINT_PTR)OriginalBase - (UINT_PTR)NtHeader->OptionalHeader.ImageBase));            // 重定向块的数据 + （真实加载地址 - 预加载地址 = Offset）

																																							//DbgPrint("RelocAddress: %p\r\n", RelocAddress);
#endif // _WIN64
				}
				else if ((TypeOffset[i] >> 12) == IMAGE_REL_BASED_HIGHLOW)
				{
#ifndef _WIN64
					PUINT32	RelocAddress = (PUINT32)((PUINT8)ReloadBase + BaseRelocation->VirtualAddress + (TypeOffset[i] & 0x0FFF));
					*RelocAddress = (UINT32)(*RelocAddress + ((PUINT8)OriginalBase - NtHeader->OptionalHeader.ImageBase));

					//DbgPrint("RelocAddress: %p\r\n", RelocAddress);
#endif // !_WIN64
				}
			}
			// 转到下一张重定向表
			BaseRelocation = (PIMAGE_BASE_RELOCATION)((UINT_PTR)BaseRelocation + BaseRelocation->SizeOfBlock);
		}
	}
	else
	{
		DbgPrint("FixRelocBaseTable::No BaseReloc\r\n");
	}
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
						INT32 CurrentOffset = (*(UINT32*)((UINT64)g_CurrentSsdtAddress->Base + i * 4)) >> 4;

						if (OriginalOffset != CurrentOffset)   // 表明被Hook了
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = FALSE;
						}

						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].Ordinal = i;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].CurrentAddress = (UINT_PTR)(g_CurrentSsdtAddress->Base + CurrentOffset);
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].OriginalAddress = g_OriginalSsdtFunctionAddress[i];

						RtlStringCchCopyW(shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].wzFunctionName, wcslen(g_SsdtFunctionName[i]) + 1, g_SsdtFunctionName[i]);

#else




#endif // _WIN64

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

#include "Private.h"


/************************************************************************
*  Name : APGetNtosExportVariableAddress
*  Param: wzVariableName		目标变量名称   （双字）
*  Param: VariableAddress		目标变量地址 （OUT）
*  Ret  : BOOLEAN
*  通过全局变量（函数地址）名称返回Ntos导出表中全局变量（函数地址）地址，这里用于 x86下获得SSDT地址
************************************************************************/
BOOLEAN
APGetNtosExportVariableAddress(IN const WCHAR *wzVariableName, OUT PVOID *VariableAddress)
{
	UNICODE_STRING	uniVariableName = { 0 };

	if (wzVariableName && wcslen(wzVariableName) > 0)
	{
		RtlInitUnicodeString(&uniVariableName, wzVariableName);

		//从Ntoskrnl模块的导出表中获得一个导出变量的地址
		*VariableAddress = MmGetSystemRoutineAddress(&uniVariableName);		// 函数返回值是PVOID，才产生了二维指针
	}

	if (*VariableAddress == NULL)
	{
		return FALSE;
	}

	return TRUE;
}


/************************************************************************
*  Name : APIsUnicodeStringValid
*  Param: uniString		        目标UnicodeString
*  Ret  : BOOLEAN
*  判断Unicode字符串是否合法
************************************************************************/
BOOLEAN
APIsUnicodeStringValid(IN PUNICODE_STRING uniString)
{
	BOOLEAN bOk = FALSE;

	__try
	{
		if (uniString->Length > 0 &&
			uniString->Buffer		&&
			MmIsAddressValid(uniString->Buffer) &&   // 字符串起始地址
			MmIsAddressValid(&uniString->Buffer[uniString->Length / sizeof(WCHAR) - 1])) // 字符串末尾地址
		{
			bOk = TRUE;
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bOk = FALSE;
	}

	return bOk;
}


/************************************************************************
*  Name : APCharToWchar
*  Param: szString		        单字字符串
*  Param: wzString		        双字字符串
*  Ret  : VOID
*  单字转双字
************************************************************************/
VOID
APCharToWchar(IN CHAR* szString, OUT WCHAR* wzString)
{
	if (szString && wzString)
	{
		NTSTATUS          Status = STATUS_UNSUCCESSFUL;
		ANSI_STRING       ansiString = { 0 };
		UNICODE_STRING    uniString = { 0 };

		// 单字转双字
		RtlInitAnsiString(&ansiString, szString);
		Status = RtlAnsiStringToUnicodeString(&uniString, &ansiString, TRUE);
		if (NT_SUCCESS(Status))
		{
			RtlCopyMemory(wzString, uniString.Buffer, uniString.Length);
			RtlFreeUnicodeString(&uniString);
		}
	}
}


/************************************************************************
*  Name : APPageProtectOff
*  Param: void
*  Ret  : VOID
*  关闭页面写保护
************************************************************************/
VOID
APPageProtectOff()
{
#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)
	_disable();
	__writecr0(__readcr0() & (~(0x10000)));
#else
	__asm
	{
		cli;
		mov eax, cr0;
		and eax, not 0x10000;
		mov cr0, eax;
	}
#endif
}


/************************************************************************
*  Name : APPageProtectOn
*  Param: void
*  Ret  : VOID
*  开启页面写保护
************************************************************************/
VOID
APPageProtectOn()
{
#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)
	__writecr0(__readcr0() ^ 0x10000);
	_enable();
#else
	__asm
	{
		mov eax, cr0;
		or  eax, 0x10000;
		mov cr0, eax;
		sti;
	}
#endif
}


/*
UINT32
APGetLogicalDriveStringsW(UINT32 BufferLength, LPWSTR wzBuffer)
{
	UINT32         DriveMap;
	UNICODE_STRING RootName;
	int i;
	PUCHAR Dst;
	DWORD BytesLeft;
	DWORD BytesNeeded;
	BOOLEAN WeFailed;
	WCHAR wszDrive[] = L"A:\\";

	BufferLength = BufferLength * 2;
	BytesNeeded = 0;
	BytesLeft = BufferLength;
	Dst = (PUCHAR)wzBuffer;
	WeFailed = FALSE;

	RtlInitUnicodeString(&RootName, wszDrive);

	DriveMap = GetLogicalDrives();
	for (i = 0; i < MAX_DOS_DRIVES; i++)
	{
		RootName.Buffer[0] = (WCHAR)((CHAR)i + 'A');
		if (DriveMap & (1 << i)) {

			BytesNeeded += RootName.MaximumLength;
			if (BytesNeeded < (USHORT)BytesLeft) {
				RtlMoveMemory(Dst, RootName.Buffer, RootName.MaximumLength);
				Dst += RootName.MaximumLength;
				*(PWSTR)Dst = UNICODE_NULL;
			}
			else {
				WeFailed = TRUE;
			}
		}
	}

	if (WeFailed) {
		BytesNeeded += 2;
	}

	return(BytesNeeded / 2);
}

*/




/*
BOOLEAN
DosPathToNtPath(IN WCHAR* wzDosFullPath, OUT WCHAR* wzNtFullPath)
{
	WCHAR	wzDriveStrings[0x1000] = { 0 };
	int		i = 0;
	WCHAR	wzNtDriveHead[3] = { 0 };
	WCHAR	wzDosDriveHead[0x100] = { 0 };
	int		iLength = 0;
	// 检查参数
	if (!wzDosFullPath || !wzNtFullPath)
	{
		return FALSE;
	}

	// 获取本地磁盘字符串
	if (GetLogicalDriveStrings(sizeof(wzDriveStrings), wzDriveStrings))
	{
		for (i = 0; wzDriveStrings[i]; i += 4)     // A:\\0B:\\0C:\\0D:\\0......所以这里每次 +4
		{
			// A B 盘符是不用的盘符
			if (!lstrcmp(&wzDriveStrings[i], L"A:\\") || !lstrcmp(&wzDriveStrings[i], L"B:\\"))
			{
				continue;
			}
			wzNtDriveHead[0] = wzDriveStrings[i];		// C
			wzNtDriveHead[1] = wzDriveStrings[i + 1];	// :
			wzNtDriveHead[2] = '\0';

			if (!APQueryDosDevice(wzNtDriveHead, wzDosDriveHead, 0x100))	// 查询 Dos 设备名
			{
				return FALSE;
			}
			iLength = lstrlen(wzDosDriveHead);
			if (_tcsnicmp(wzDosFullPath, wzDosDriveHead, iLength) == 0)
			{
				lstrcpy(wzNtFullPath, wzNtDriveHead);	// 复制驱动器
				lstrcat(wzNtFullPath, wzDosFullPath + iLength);	// 复制路径,紧接后面

				return TRUE;
			}
		}
	}
	lstrcpy(wzNtFullPath, wzDosFullPath);
	return FALSE;
}
*/



BOOLEAN
APDosPathToNtPath(IN WCHAR *wzDosFullPath, OUT WCHAR *wzNtFullPath)
{
	WCHAR  wzNtDriveHead[4] = { 0 };
	WCHAR  wzDosDriveHead[64] = { 0 };

	if (!wzNtFullPath || !wzDosFullPath)
	{
		return FALSE;
	}
	// 从 'A'开始
	for (UINT16 i = 65; i < 26 + 65; i++)
	{
		wzNtDriveHead[0] = (WCHAR)i;
		wzNtDriveHead[1] = L':';
		if (APQueryDosDevice(wzNtDriveHead, wzDosDriveHead, 64))
		{
			if (wzDosDriveHead)
			{
				SIZE_T DosDriveHeadLength = 0;
				DosDriveHeadLength = wcslen(wzDosDriveHead);
				if (_wcsnicmp(wzDosDriveHead, wzDosFullPath, DosDriveHeadLength) == 0)
				{
					//wcscpy(wzNtFullPath, wzNtDriveHead);
					//wcscat(wzNtFullPath, wzDosFullPath + DosDriveHeadLength);
					RtlStringCchCopyW(wzNtFullPath, wcslen(wzNtDriveHead), wzNtDriveHead);
					RtlStringCchCatW(wzNtFullPath, wcslen(wzDosFullPath + DosDriveHeadLength) + 1, wzDosFullPath + DosDriveHeadLength);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


UINT32
APQueryDosDevice(WCHAR *DeviceName, WCHAR *TargetPath, UINT32 MaximumLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	UINT32   ucchReturned = 0;

	UNICODE_STRING uniDirectory = { 0 };
	OBJECT_ATTRIBUTES oa = { 0 };
	HANDLE DirectoryHandle = NULL;

	// 初始化并打开目录句柄
	RtlInitUnicodeString(&uniDirectory, L"\\??");
	InitializeObjectAttributes(&oa, &uniDirectory, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenDirectoryObject(&DirectoryHandle, DIRECTORY_QUERY, &oa);
	if (NT_SUCCESS(Status))
	{
		if (DeviceName)   // 如果传入了NT盘符名
		{
			HANDLE SymbolicLinkHandle = NULL;
			UNICODE_STRING uniDosDeviceName = { 0 };

			// 初始化符号链接，打开符号链接对象句柄
			RtlInitUnicodeString(&uniDosDeviceName, DeviceName);
			InitializeObjectAttributes(&oa, &uniDosDeviceName, OBJ_CASE_INSENSITIVE, DirectoryHandle, NULL);
			Status = ZwOpenSymbolicLinkObject(&SymbolicLinkHandle, SYMBOLIC_LINK_QUERY, &oa);
			if (NT_SUCCESS(Status))
			{
				UINT32 ReturnLength = 0;
				UNICODE_STRING uniNtDeviceName = { 0 };

				uniNtDeviceName.Length = 0;
				uniNtDeviceName.MaximumLength = (USHORT)(MaximumLength * sizeof(WCHAR));
				uniNtDeviceName.Buffer = TargetPath;

				// 查询符号链接对象
				Status = ZwQuerySymbolicLinkObject(SymbolicLinkHandle, &uniNtDeviceName, &ReturnLength);
				if (NT_SUCCESS(Status))
				{
					ucchReturned = uniNtDeviceName.Length / sizeof(WCHAR);
					if (ucchReturned < MaximumLength)
					{
						// 名称已经填充完成
						TargetPath[ucchReturned++] = UNICODE_NULL;
					}
					else
					{
						ucchReturned = 0;
					}
				}
				ZwClose(SymbolicLinkHandle);
			}
			else
			{
				DbgPrint("Open SymbolicLink Object Failed\r\n");
			}
		}
		else    // 需要一个一个查询目录对象
		{
			BOOLEAN bRestartScan = TRUE;
			UINT32  Context = 0;
			UINT32  ReturnLength = 0;
			CHAR    szBuffer[512] = { 0 };
			WCHAR   *Pos = TargetPath;     // 定位

			POBJECT_DIRECTORY_INFORMATION odi = (POBJECT_DIRECTORY_INFORMATION)szBuffer;

			while (TRUE)
			{
				Status = ZwQueryDirectoryObject(DirectoryHandle, (PVOID)odi, sizeof(szBuffer), TRUE, bRestartScan, &Context, &ReturnLength);
				if (NT_SUCCESS(Status))
				{
					if (wcscmp(odi->TypeName.Buffer, L"SymbolicLink") == 0)
					{
						UINT32 NameLength = odi->Name.Length / sizeof(WCHAR);
						if (ucchReturned + NameLength + 1 >= MaximumLength)
						{
							ucchReturned = 0;
							break;
						}
						RtlStringCchCopyW(Pos, odi->Name.Length, odi->Name.Buffer);
						Pos += NameLength;
						*Pos++ = UNICODE_NULL;
						ucchReturned += NameLength + 1;
					}
				}
				else
				{
					if (Status == STATUS_NO_MORE_ENTRIES)
					{
						*Pos = UNICODE_NULL;
						ucchReturned++;
						Status = STATUS_SUCCESS;
					}
					else
					{
						ucchReturned = 0;
					}
					break;
				}
				bRestartScan = FALSE;
			}
		}
		ZwClose(DirectoryHandle);
	}
	else
	{
		DbgPrint("Open Directory Failed\r\n");
	}
	return ucchReturned;
}
#include "stdafx.h"
#include "ProcessCore.h"


#include "Global.hpp"


namespace ArkProtect
{
	CProcessCore *CProcessCore::m_Process;

	CProcessCore::CProcessCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_Process = this;
	}


	CProcessCore::~CProcessCore()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessList
	*  Param: ProcessList        ProcessModule对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CProcessCore::InitializeProcessList(CListCtrl *ProcessList)
	{
		ProcessList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ProcessList->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : GetProcessNum
	*  Param: void
	*  Ret  : UINT32
	*  向驱动层发送消息请求当前进程个数
	************************************************************************/
	UINT32 CProcessCore::GetProcessNum()
	{
		BOOL   bOk = FALSE;
		DWORD  dwReturnLength = 0;
		UINT32 ProcessNum = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_PROCESSNUM,
			NULL,		// InputBuffer
			0,
			&ProcessNum,
			sizeof(UINT32),
			&dwReturnLength,
			NULL);
		if (!bOk)
		{
			::MessageBox(NULL, L"Get Process Count Failed", L"ArkProtect", MB_OK | MB_ICONERROR);
		}

		return ProcessNum;
	}


	/************************************************************************
	*  Name : GrantPriviledge
	*  Param: PriviledgeName		需要提升的权限
	*  Param: bEnable				开关
	*  Ret  : BOOL
	*  提升到想要的权限（权限令牌）
	************************************************************************/
	BOOL CProcessCore::GrantPriviledge(IN PWCHAR PriviledgeName, IN BOOL bEnable)
	{
		TOKEN_PRIVILEGES TokenPrivileges, OldPrivileges;
		DWORD			 dwReturnLength = sizeof(OldPrivileges);
		HANDLE			 TokenHandle = NULL;
		LUID			 uID;

		// 打开权限令牌
		if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &TokenHandle))
		{
			if (GetLastError() != ERROR_NO_TOKEN)
			{
				return FALSE;
			}
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
			{
				return FALSE;
			}
		}

		if (!LookupPrivilegeValue(NULL, PriviledgeName, &uID))		// 通过权限名称查找uID
		{
			CloseHandle(TokenHandle);
			TokenHandle = NULL;
			return FALSE;
		}

		TokenPrivileges.PrivilegeCount = 1;		// 要提升的权限个数
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;    // 动态数组，数组大小根据Count的数目
		TokenPrivileges.Privileges[0].Luid = uID;

		// 在这里我们进行调整权限
		if (!AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), &OldPrivileges, &dwReturnLength))
		{
			CloseHandle(TokenHandle);
			TokenHandle = NULL;
			return FALSE;
		}

		// 成功了
		CloseHandle(TokenHandle);
		TokenHandle = NULL;

		return TRUE;
	}


	/************************************************************************
	*  Name : QueryProcessUserAccess
	*  Param: ProcessId		         目标进程id
	*  Ret  : BOOL
	*  通过能否打开进程句柄判断能否让用户访问权限
	************************************************************************/
	BOOL CProcessCore::QueryProcessUserAccess(UINT32 ProcessId)
	{
		GrantPriviledge(SE_DEBUG_NAME, TRUE);

		HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_OPERATION, TRUE, ProcessId);

		GrantPriviledge(SE_DEBUG_NAME, FALSE);
		if (ProcessHandle)
		{
			CloseHandle(ProcessHandle);
			return TRUE;
		}

		return FALSE;
	}


	/************************************************************************
	*  Name : QueryPEFileBit
	*  Param: wzFilePath			进程Id
	*  Ret  : BOOL
	*  判断是否为32为程序(读取文件的方式)
	************************************************************************/
	ePeBit CProcessCore::QueryPEFileBit(const WCHAR *wzFilePath)
	{
		ePeBit  PeBit = pb_Unknown;
		HANDLE	FileHandle = NULL;
		HANDLE	MappingHandle = NULL;

		FileHandle = CreateFileW(wzFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			DWORD dwReturnLength = 0;
			DWORD dwFileSize = GetFileSize(FileHandle, 0);
			PVOID FileBuffer = malloc(dwFileSize);
			if (FileBuffer)
			{
				BOOL bOk = ReadFile(FileHandle, FileBuffer, dwFileSize, &dwReturnLength, nullptr);
				if (bOk)
				{
					PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
					if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
					{
						PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)FileBuffer + DosHeader->e_lfanew);
						if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
						{
							if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
							{
								PeBit = pb_32;
							}
							else if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64)
							{
								PeBit = pb_64;
							}
						}

					}
				}
				free(FileBuffer);
			}
			CloseHandle(FileHandle);
		}
		return PeBit;
	}


	/************************************************************************
	*  Name : DeployProcessInfo
	*  Param: pei			     进程信息结构
	*  Ret  : void
	*  完善进程信息结构
	************************************************************************/
	void CProcessCore::PerfectProcessInfo(PROCESS_ENTRY_INFORMATION pei)
	{
		if (pei.ProcessId == 0)
		{
			// Idle
			StringCchCopyW(pei.wzImageName, wcslen(L"System Idle Process") + 1, L"System Idle Process");
			StringCchCopyW(pei.wzFilePath, wcslen(L"System Idle Process") + 1, L"System Idle Process");
			pei.bUserAccess = FALSE;
			pei.ParentProcessId = 0xffffffff;
			StringCchCopyW(pei.wzCompanyName, wcslen(L" ") + 1, L" ");
		}
		else if (pei.ProcessId == 4)
		{
			// System
			StringCchCopyW(pei.wzImageName, wcslen(L"System") + 1, L"System");

			WCHAR wzFilePath[MAX_PATH] = { 0 };
			//GetEnvironmentVariableW(L"windir", wzFilePath, MAX_PATH);   // 获得WindowsDirectory
			//lstrcat(wzFilePath, L"\\System32\\ntoskrnl.exe");
			GetSystemDirectory(wzFilePath, MAX_PATH);      // 获得System32Directory
			StringCchCatW(wzFilePath, MAX_PATH, L"\\ntoskrnl.exe");

			StringCchCopyW(pei.wzFilePath, wcslen(wzFilePath) + 1, wzFilePath);
			pei.bUserAccess = FALSE;
			pei.ParentProcessId = 0xffffffff;
			StringCchCopyW(pei.wzCompanyName,
				m_Global->GetFileCompanyName(pei.wzFilePath).GetLength() + 1,
				m_Global->GetFileCompanyName(pei.wzFilePath).GetBuffer());
		}
		else
		{
			// Others
			WCHAR *wzImageName = NULL;
			wzImageName = wcsrchr(pei.wzFilePath, '\\');
			wzImageName++;  // 过 '\\'

			StringCchCopyW(pei.wzImageName, wcslen(wzImageName) + 1, wzImageName);

			if (QueryProcessUserAccess(pei.ProcessId) == TRUE)
			{
				pei.bUserAccess = TRUE;
			}
			else
			{
				pei.bUserAccess = FALSE;
			}

			CString strCompanyName = m_Global->GetFileCompanyName(pei.wzFilePath);
			if (strCompanyName.GetLength() == 0)
			{
				strCompanyName = L" ";
			}

			StringCchCopyW(pei.wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());

#ifdef _WIN64
			// 只需要判断32位程序
			if (QueryPEFileBit(pei.wzFilePath) == TRUE)
			{
				StringCchCatW(pei.wzImageName, 100, L" *32");
			}
#endif // _WIN64

		}
	}


	/************************************************************************
	*  Name : EnumProcessInfo
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程及相关信息
	************************************************************************/
	BOOL CProcessCore::EnumProcessInfo()
	{
		BOOL bOk = FALSE;

		m_ProcessEntryVector.clear();

		// 首先我们试探一下有多少个进程
		UINT32 ProcessNum = GetProcessNum();
		if (ProcessNum == 0)
		{
			return FALSE;
		}

		UINT32   Count = ProcessNum + 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_INFORMATION pi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pi)
			{
				free(pi);
				pi = NULL;
			}

			OutputLength = sizeof(PROCESS_INFORMATION) + Count * sizeof(PROCESS_ENTRY_INFORMATION);

			pi = (PPROCESS_INFORMATION)malloc(OutputLength);
			if (!pi)
			{
				break;
			}

			RtlZeroMemory(pi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESS,
				NULL,		// InputBuffer
				0,
				pi,
				OutputLength,
				&dwReturnLength,
				NULL);

			// 这句配合do while循环，恰到好处，如果给的内存不足则重复枚举，如果足够则一次通过
			Count = (UINT32)pi->NumberOfProcesses + 1000;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pi)
		{
			for (UINT32 i = 0; i < pi->NumberOfProcesses; i++)
			{
				// 完善进程信息结构
				PerfectProcessInfo(pi->ProcessEntry[i]);
				m_ProcessEntryVector.push_back(pi->ProcessEntry[i]);
			}
			bOk = TRUE;
		}

		if (pi)
		{
			free(pi);
			pi = NULL;
		}

		if (m_ProcessEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertProcessInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CProcessCore::InsertProcessInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessNum = 0;
		size_t Size = m_ProcessEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_ENTRY_INFORMATION ProcessEntry = m_ProcessEntryVector[i];

			CString strImageName, strProcessId, strParentProcessId, strFilePath, strEProcess, strUserAccess, strCompanyName;

			strImageName = ProcessEntry.wzImageName;
			
			strProcessId.Format(L"%d", ProcessEntry.ProcessId);
			
			if (ProcessEntry.ParentProcessId == 0xffffffff)
			{
				strParentProcessId = L"-";
			}
			else
			{
				strParentProcessId.Format(L"%d", ProcessEntry.ParentProcessId);
			}
			
			strFilePath = ProcessEntry.wzFilePath;
			
			strEProcess.Format(L"0x%p", ProcessEntry.EProcess);
			
			if (ProcessEntry.bUserAccess == TRUE)
			{
				strUserAccess = L"允许";
			}
			else
			{
				strUserAccess = L"拒绝";
			}

			strCompanyName = ProcessEntry.wzCompanyName;
			 
			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strImageName);
			ListCtrl->SetItemText(iItem, pc_ProcessId, strProcessId);
			ListCtrl->SetItemText(iItem, pc_ParentProcessId, strParentProcessId);
			ListCtrl->SetItemText(iItem, pc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, pc_EProcess, strEProcess);
			ListCtrl->SetItemText(iItem, pc_UserAccess, strUserAccess);
			ListCtrl->SetItemText(iItem, pc_Company, strCompanyName);

			ProcessNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Info is loading now, Count:%d", ProcessNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QueryProcessInfo
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CProcessCore::QueryProcessInfo(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessEntryVector.clear();

		if (EnumProcessInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Info Initialize failed");
			return;
		}

		InsertProcessInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessInfoCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程信息的回调
	************************************************************************/
	DWORD CALLBACK CProcessCore::QueryProcessInfoCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Process->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_Process->m_Global->UpdateStatusBarTip(L"Process Info");
		m_Process->m_Global->UpdateStatusBarDetail(L"Process Info is loading now...");
		
		m_Process->QueryProcessInfo(ListCtrl);

		m_Process->m_Global->m_bIsRequestNow = TRUE;

		return 0;
	}

}



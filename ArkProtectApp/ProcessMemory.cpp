#include "stdafx.h"
#include "ProcessMemory.h"
#include "Global.hpp"
#include "ProcessDlg.h"


namespace ArkProtect
{
	CProcessMemory *CProcessMemory::m_ProcessMemory;

	CProcessMemory::CProcessMemory(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_ProcessModule(GlobalObject->ProcessModule())
	{
		m_ProcessMemory = this;
	}


	CProcessMemory::~CProcessMemory()
	{
	}


	/************************************************************************
	*  Name : InitializeVirtualMemoryProtectVector
	*  Param: void        
	*  Ret  : void
	*  初始化虚拟内存页面保护属性Vector的信息
	************************************************************************/
	void CProcessMemory::InitializeVirtualMemoryProtectVector()
	{
		m_VitualProtectTypeVector.clear();

		/*
		#define PAGE_NOACCESS          0x01
		#define PAGE_READONLY          0x02
		#define PAGE_READWRITE         0x04
		#define PAGE_WRITECOPY         0x08
		#define PAGE_EXECUTE           0x10
		#define PAGE_EXECUTE_READ      0x20
		#define PAGE_EXECUTE_READWRITE 0x40
		#define PAGE_EXECUTE_WRITECOPY 0x80
		#pragma endregion
		#define PAGE_GUARD            0x100
		*/

		VIRTUAL_PROTECT_TYPE VitualProtectType = { 0 };

		VitualProtectType.Type = PAGE_NOACCESS;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"No Access") + 1, L"No Access");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_READONLY;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"Read") + 1, L"Read");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_READWRITE;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"ReadWrite") + 1, L"ReadWrite");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_WRITECOPY;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"WriteCopy") + 1, L"WriteCopy");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_EXECUTE;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"Execute") + 1, L"Execute");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_EXECUTE_READ;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"ReadExecute") + 1, L"ReadExecute");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_EXECUTE_READWRITE;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"ReadWriteExecute") + 1, L"ReadWriteExecute");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_EXECUTE_WRITECOPY;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"WriteCopyExecute") + 1, L"WriteCopyExecute");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_GUARD;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"Guard") + 1, L"Guard");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_NOCACHE;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"No Cache") + 1, L"No Cache");
		m_VitualProtectTypeVector.push_back(VitualProtectType);

		RtlZeroMemory(&VitualProtectType, sizeof(VIRTUAL_PROTECT_TYPE));
		VitualProtectType.Type = PAGE_WRITECOMBINE;
		StringCchCopyW(VitualProtectType.wzTypeName, wcslen(L"WriteCombine") + 1, L"WriteCombine");
		m_VitualProtectTypeVector.push_back(VitualProtectType);
	}

	/************************************************************************
	*  Name : InitializeProcessMemoryList
	*  Param: ProcessInfoList        ProcessMemory对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CProcessMemory::InitializeProcessMemoryList(CListCtrl *ListCtrl)
	{
		while (ListCtrl->DeleteColumn(0));
		ListCtrl->DeleteAllItems();

		ListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ListCtrl->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}

		InitializeVirtualMemoryProtectVector();
	}



	/************************************************************************
	*  Name : EnumProcessMemory
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CProcessMemory::EnumProcessMemory()
	{
		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_MEMORY_INFORMATION pmi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pmi)
			{
				free(pmi);
				pmi = NULL;
			}

			OutputLength = sizeof(PROCESS_MEMORY_INFORMATION) + Count * sizeof(PROCESS_MEMORY_ENTRY_INFORMATION);

			pmi = (PPROCESS_MEMORY_INFORMATION)malloc(OutputLength);
			if (!pmi)
			{
				break;
			}

			RtlZeroMemory(pmi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSMEMORY,
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pmi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pmi->NumberOfMemories + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pmi)
		{
			for (UINT32 i = 0; i < pmi->NumberOfMemories; i++)
			{
				// 完善进程信息结构
				m_ProcessMemoryEntryVector.push_back(pmi->MemoryEntry[i]);
			}
			bOk = TRUE;
		}

		if (pmi)
		{
			free(pmi);
			pmi = NULL;
		}

		if (m_ProcessMemoryEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	CString CProcessMemory::GetMemoryProtect(UINT32 Protect)
	{
		BOOL bFirst = TRUE;
		CString strProtect = L"";

		UINT32 Size = m_VitualProtectTypeVector.size();
		for (UINT32 i = 0; i < Size; i++)
		{
			VIRTUAL_PROTECT_TYPE VirtualProtectType = m_VitualProtectTypeVector[i];
			if (VirtualProtectType.Type & Protect)
			{
				if (bFirst == TRUE)
				{
					strProtect = VirtualProtectType.wzTypeName;
					bFirst = FALSE;
				}
				else
				{
					strProtect += L" ";
					strProtect += VirtualProtectType.wzTypeName;
				}
			}
		}

		return strProtect;
	}


	CString CProcessMemory::GetMemoryState(UINT32 State)
	{
		/*
		#define MEM_COMMIT                  0x1000
		#define MEM_RESERVE                 0x2000
		#define MEM_FREE                    0x10000
		*/

		CString strState = L"";

		if (State == MEM_COMMIT)
		{
			strState = L"Commit";
		}
		else if (State == MEM_FREE)
		{
			strState = L"Free";
		}
		else if (State == MEM_RESERVE)
		{
			strState = L"Reserve";
		}
		else if (State == MEM_DECOMMIT)
		{
			strState = L"Decommit";
		}
		else if (State == MEM_RELEASE)
		{
			strState = L"Release";
		}

		return strState;
	}


	CString CProcessMemory::GetMemoryType(UINT32 Type)
	{
		/*
		#define SEC_IMAGE         0x1000000
		#define MEM_MAPPED        0x40000
		#define MEM_PRIVATE       0x20000
		*/

		CString strType = L"";

		if (Type == MEM_PRIVATE)
		{
			strType = L"Private";
		}
		else if (Type == MEM_MAPPED)
		{
			strType = L"Map";
		}
		else if (Type == MEM_IMAGE)
		{
			strType = L"Image";
		}

		return strType;
	}


	CString CProcessMemory::GetModuleImageNameByMemoryBaseAddress(UINT_PTR BaseAddress)
	{
		CString strImageName = L"";

		size_t Size = m_ProcessModule.ProcessModuleEntryVector().size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry = m_ProcessModule.ProcessModuleEntryVector()[i];
			if (BaseAddress >= ModuleEntry.BaseAddress && BaseAddress <= (ModuleEntry.BaseAddress + ModuleEntry.SizeOfImage))
			{
				CString strModulePath = ModuleEntry.wzFilePath;
				strImageName = strModulePath.Right(strModulePath.GetLength() - strModulePath.ReverseFind('\\') - 1);		// ReverseFind最后一次找到 "\\"
				break;
			}
		}

		return strImageName;
	}


	/************************************************************************
	*  Name : InsertProcessMemoryInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CProcessMemory::InsertProcessMemoryInfoList(CListCtrl *ListCtrl)
	{
		size_t Size = m_ProcessMemoryEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MEMORY_ENTRY_INFORMATION MemoryEntry = m_ProcessMemoryEntryVector[i];

			CString strBaseAddress, strRegionSize, strProtect, strType, strImageName, strState;

			strBaseAddress.Format(L"0x%08p", MemoryEntry.BaseAddress);
			strRegionSize.Format(L"0x%X", MemoryEntry.RegionSize);
			strProtect = GetMemoryProtect(MemoryEntry.Protect);
			strState = GetMemoryState(MemoryEntry.State);
			strType = GetMemoryType(MemoryEntry.Type);

			if (MemoryEntry.Type == MEM_IMAGE)    // 对应映射
			{
				strImageName = GetModuleImageNameByMemoryBaseAddress(MemoryEntry.BaseAddress);
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strBaseAddress);
			ListCtrl->SetItemText(iItem, pmc_RegionSize, strRegionSize);
			ListCtrl->SetItemText(iItem, pmc_ProtectType, strProtect);
			ListCtrl->SetItemText(iItem, pmc_State, strState);
			ListCtrl->SetItemText(iItem, pmc_Type, strType);
			ListCtrl->SetItemText(iItem, pmc_ModuleName, strImageName);
			ListCtrl->SetItemData(iItem, iItem);
		}

		CString strNum;
		strNum.Format(L"%d", Size);
		((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(strNum);
	}


	/************************************************************************
	*  Name : QueryProcessMemory
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CProcessMemory::QueryProcessMemory(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessMemoryEntryVector.clear();
		m_ProcessModule.ProcessModuleEntryVector().clear();

		if (EnumProcessMemory() == FALSE)
		{
			((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(L"Process Memory Initialize failed");
			return;
		}

		if (m_ProcessModule.EnumProcessModule() == FALSE)
		{
			((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(L"Process Memory Initialize failed");
			return;
		}

		InsertProcessMemoryInfoList(ListCtrl);
	}



	/************************************************************************
	*  Name : QueryProcessModuleCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CProcessMemory::QueryProcessMemoryCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessMemory->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_ProcessMemory->QueryProcessMemory(ListCtrl);

		m_ProcessMemory->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
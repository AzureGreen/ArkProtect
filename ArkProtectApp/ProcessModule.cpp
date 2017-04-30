#include "stdafx.h"
#include "ProcessModule.h"
#include "Global.hpp"

namespace ArkProtect
{
	CProcessModule *CProcessModule::m_ProcessModule;

	CProcessModule::CProcessModule(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_ProcessModule = this;
	}


	CProcessModule::~CProcessModule()
	{
	}



	/************************************************************************
	*  Name : InitializeProcessModuleList
	*  Param: ProcessInfoList        ProcessModule对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CProcessModule::InitializeProcessModuleList(CListCtrl *ListCtrl)
	{
		while (ListCtrl->DeleteColumn(0));
		ListCtrl->DeleteAllItems();

		ListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ListCtrl->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : PerfectProcessModuleInfo
	*  Param: ModuleEntry			     进程信息结构
	*  Ret  : void
	*  完善进程信息结构
	************************************************************************/
	void CProcessModule::PerfectProcessModuleInfo(PPROCESS_MODULE_ENTRY_INFORMATION ModuleEntry)
	{
		// 修剪模块文件路径
		CString strFullPath = m_Global->TrimPath(ModuleEntry->wzFilePath);
		StringCchCopyW(ModuleEntry->wzFilePath, strFullPath.GetLength() + 1, strFullPath.GetBuffer());
		//wcsncpy_s(ModuleEntry.wzFullPath, MAX_PATH, strFullPath.GetBuffer(), strFullPath.GetLength());

		CString strCompanyName = m_Global->GetFileCompanyName(ModuleEntry->wzFilePath);
		if (strCompanyName.GetLength() == 0)
		{
			strCompanyName = L" ";
		}

		StringCchCopyW(ModuleEntry->wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());
	}
	

	/************************************************************************
	*  Name : EnumProcessModule
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CProcessModule::EnumProcessModule()
	{
		m_ProcessModuleEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_MODULE_INFORMATION pmi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pmi)
			{
				free(pmi);
				pmi = NULL;
			}

			OutputLength = sizeof(PROCESS_MODULE_INFORMATION) + Count * sizeof(PROCESS_MODULE_ENTRY_INFORMATION);

			pmi = (PPROCESS_MODULE_INFORMATION)malloc(OutputLength);
			if (!pmi)
			{
				break;
			}

			RtlZeroMemory(pmi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSMODULE,
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pmi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pmi->NumberOfModules + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pmi)
		{
			for (UINT32 i = 0; i < pmi->NumberOfModules; i++)
			{
				// 完善进程信息结构
				PerfectProcessModuleInfo(&pmi->ModuleEntry[i]);
				m_ProcessModuleEntryVector.push_back(pmi->ModuleEntry[i]);
			}
			bOk = TRUE;
		}

		if (pmi)
		{
			free(pmi);
			pmi = NULL;
		}

		if (m_ProcessModuleEntryVector.empty())
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
	void CProcessModule::InsertProcessModuleInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessModuleNum = 0;
		size_t Size = m_ProcessModuleEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry = m_ProcessModuleEntryVector[i];

			CString strFilePath, strBaseAddress, strImageSize, strCompanyName;
			
			strFilePath = ModuleEntry.wzFilePath;
			strBaseAddress.Format(L"0x%p", ModuleEntry.BaseAddress);
			strImageSize.Format(L"0x%X", ModuleEntry.SizeOfImage);
			strCompanyName = ModuleEntry.wzCompanyName;

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strFilePath);
			ListCtrl->SetItemText(iItem, pmc_BaseAddress, strBaseAddress);
			ListCtrl->SetItemText(iItem, pmc_SizeOfImage, strImageSize);
			ListCtrl->SetItemText(iItem, pmc_Company, strCompanyName);

			ProcessModuleNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Module Info is loading now, Count:%d", ProcessModuleNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process module Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QueryProcessModule
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CProcessModule::QueryProcessModule(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessModuleEntryVector.clear();

		if (EnumProcessModule() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Module Initialize failed");
			return;
		}

		InsertProcessModuleInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessModuleCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CProcessModule::QueryProcessModuleCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessModule->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_ProcessModule->m_Global->UpdateStatusBarTip(L"Process Module");
		m_ProcessModule->m_Global->UpdateStatusBarDetail(L"Process Module is loading now...");

		m_ProcessModule->QueryProcessModule(ListCtrl);

		m_ProcessModule->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
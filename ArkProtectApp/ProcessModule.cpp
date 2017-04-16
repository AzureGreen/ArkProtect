#include "stdafx.h"
#include "ProcessModule.h"

namespace ArkProtect
{
	CProcessModule *CProcessModule::m_ProcessModule;

	CProcessModule::CProcessModule(CGlobal *GlobalObject, PPROCESS_ENTRY_INFORMATION ProcessEntry)
		: m_Global(GlobalObject)
		, m_ProcessEntry(ProcessEntry)
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
	void CProcessModule::InitializeProcessModuleList(CListCtrl *ProcessInfoList)
	{
		ProcessInfoList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ProcessInfoList->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}

	

	/************************************************************************
	*  Name : EnumProcessModule
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CProcessModule::EnumProcessModule()
	{
		BOOL bOk = FALSE;

		m_ProcessModuleEntryVector.clear();

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
				&m_ProcessEntry->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pmi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pmi->NumberOfModules + 1000;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pmi)
		{
			for (UINT32 i = 0; i < pmi->NumberOfModules; i++)
			{
				// 完善进程信息结构
				//PerfectProcessInfo(pmi->ModuleEntry[i]);
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

	//	InsertProcessModuleList(ListCtrl);
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
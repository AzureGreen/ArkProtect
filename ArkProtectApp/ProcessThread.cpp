#include "stdafx.h"
#include "ProcessThread.h"


namespace ArkProtect
{
	CProcessThread *CProcessThread::m_ProcessThread;

	CProcessThread::CProcessThread(CGlobal *GlobalObject, PPROCESS_ENTRY_INFORMATION ProcessEntry)
		: m_Global(GlobalObject)
		, m_ProcessEntry(ProcessEntry)
	{
		m_ProcessThread = this;
	}


	CProcessThread::~CProcessThread()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessThreadList
	*  Param: ProcessInfoList        ProcessThread对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CProcessThread::InitializeProcessThreadList(CListCtrl *ListCtrl)
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
	*  Name : EnumProcessThread
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CProcessThread::EnumProcessThread()
	{
		BOOL bOk = FALSE;

		m_ProcessThreadEntryVector.clear();

		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_THREAD_INFORMATION pti = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pti)
			{
				free(pti);
				pti = NULL;
			}

			OutputLength = sizeof(PROCESS_THREAD_INFORMATION) + Count * sizeof(PROCESS_THREAD_ENTRY_INFORMATION);

			pti = (PPROCESS_THREAD_INFORMATION)malloc(OutputLength);
			if (!pti)
			{
				break;
			}

			RtlZeroMemory(pti, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSTHREAD,
				&m_ProcessEntry->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pti,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pti->NumberOfThreads + 1000;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pti)
		{
			for (UINT32 i = 0; i < pti->NumberOfThreads; i++)
			{
				// 完善进程信息结构
				//PerfectProcessModuleInfo(&pmi->ModuleEntry[i]);
				m_ProcessThreadEntryVector.push_back(pti->ThreadEntry[i]);
			}
			bOk = TRUE;
		}

		if (pti)
		{
			free(pti);
			pti = NULL;
		}

		if (m_ProcessThreadEntryVector.empty())
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
			strStatusContext.Format(L"Process Info is loading now, Count:%d", ProcessModuleNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}




	/************************************************************************
	*  Name : QueryProcessThread
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CProcessThread::QueryProcessThread(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessThreadEntryVector.clear();
		m_ProcessModuleEntryVector.clear();

		if (EnumProcessThread() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Thread Initialize failed");
			return;
		}

		if (EnumProcessModule() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Module Initialize failed");
			return;
		}

		//InsertProcessThreadInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessThreadCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CProcessThread::QueryProcessThreadCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessThread->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_ProcessThread->m_Global->UpdateStatusBarTip(L"Process Thread");
		m_ProcessThread->m_Global->UpdateStatusBarDetail(L"Process Thread is loading now...");

		m_ProcessThread->QueryProcessThread(ListCtrl);

		m_ProcessThread->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
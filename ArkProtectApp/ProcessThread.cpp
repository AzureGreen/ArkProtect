#include "stdafx.h"
#include "ProcessThread.h"
#include "Global.hpp"
#include "ProcessModule.h"

namespace ArkProtect
{
	CProcessThread *CProcessThread::m_ProcessThread;

	CProcessThread::CProcessThread(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_ProcessModule(GlobalObject->ProcessModule())
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
		m_ProcessThreadEntryVector.clear();
		
		BOOL bOk = FALSE;
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
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pti,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pti->NumberOfThreads + 0x20;

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


	CString CProcessThread::GetModulePathByThreadStartAddress(UINT_PTR StartAddress)
	{
		CString strModulePath = L"";

		size_t Size = m_ProcessModule.ProcessModuleEntryVector().size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry = m_ProcessModule.ProcessModuleEntryVector()[i];
			if (StartAddress >= ModuleEntry.BaseAddress && StartAddress <= (ModuleEntry.BaseAddress + ModuleEntry.SizeOfImage))
			{
				strModulePath = ModuleEntry.wzFilePath;
			}	
		}

		// 如果不进入循环 就说明是内核模块
		return strModulePath;
	}


	/************************************************************************
	*  Name : InsertProcessInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CProcessThread::InsertProcessThreadInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessThreadNum = 0;
		size_t Size = m_ProcessThreadEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_THREAD_ENTRY_INFORMATION ThreadEntry = m_ProcessThreadEntryVector[i];

			CString strThreadId, strEThread, strTeb, strPriority, strWin32StartAddress, strContextSwitches, strState, strModulePath;

			strThreadId.Format(L"%d", ThreadEntry.ThreadId);
			strEThread.Format(L"0x%08p", ThreadEntry.EThread);
			if (ThreadEntry.Teb == 0)
			{
				strTeb = L"-";
			}
			else
			{
				strTeb.Format(L"0x%08p", ThreadEntry.Teb);
			}

			strPriority.Format(L"%d", ThreadEntry.Priority);
			strWin32StartAddress.Format(L"0x%08p", ThreadEntry.Win32StartAddress);
			strContextSwitches.Format(L"%d", ThreadEntry.ContextSwitches);

			strModulePath = GetModulePathByThreadStartAddress(ThreadEntry.Win32StartAddress);

			if (strModulePath.GetLength() <= 1)
			{
				strModulePath = L"\\ ";
			}

			WCHAR *Temp = NULL;

			Temp = wcsrchr(strModulePath.GetBuffer(), L'\\');

			if (Temp != NULL)
			{
				Temp++;
			}

			strModulePath = Temp;

			switch (ThreadEntry.State)
			{
			case Initialized:
			{
				strState = L"预置";
				break;
			}
			case Ready:
			{
				strState = L"就绪";
				break;
			}
			case Running:
			{
				strState = L"运行";
				break;
			}
			case Standby:
			{
				strState = L"备用";
				break;
			}
			case Terminated:
			{
				strState = L"终止";
				break;
			}
			case Waiting:
			{
				strState = L"等待";
				break;
			}
			case Transition:
			{
				strState = L"过度";
				break;
			}
			case DeferredReady:
			{
				strState = L"延迟就绪";
				break;
			}
			case GateWait:
			{
				strState = L"门等待";
				break;
			}
			default:
				strState = L"未知";
				break;
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strThreadId);
			ListCtrl->SetItemText(iItem, ptc_EThread, strEThread);
			ListCtrl->SetItemText(iItem, ptc_Teb, strTeb);
			ListCtrl->SetItemText(iItem, ptc_Priority, strPriority);
			ListCtrl->SetItemText(iItem, ptc_Entrance, strWin32StartAddress);
			ListCtrl->SetItemText(iItem, ptc_Module, strModulePath);
			ListCtrl->SetItemText(iItem, ptc_switches, strContextSwitches);
			ListCtrl->SetItemText(iItem, ptc_Status, strState);
			
			ListCtrl->SetItemData(iItem, iItem);

			ProcessThreadNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Info is loading now, Count:%d", ProcessThreadNum);

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
		m_ProcessModule.ProcessModuleEntryVector().clear();

		if (EnumProcessThread() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Thread Initialize failed");
			return;
		}

		if (m_ProcessModule.EnumProcessModule() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Module Initialize failed");
			return;
		}

		InsertProcessThreadInfoList(ListCtrl);
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
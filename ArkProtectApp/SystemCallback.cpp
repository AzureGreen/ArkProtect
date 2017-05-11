#include "stdafx.h"
#include "SystemCallback.h"
#include "Global.hpp"

namespace ArkProtect
{
	CSystemCallback *CSystemCallback::m_SystemCallback;

	CSystemCallback::CSystemCallback(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_SystemCallback = this;
	}


	CSystemCallback::~CSystemCallback()
	{
	}


	/************************************************************************
	*  Name : InitializeCallbackList
	*  Param: ListCtrl               ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CSystemCallback::InitializeCallbackList(CListCtrl *ListCtrl)
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
	*  Name : EnumProcessModule
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CSystemCallback::EnumSystemCallback()
	{
		m_CallbackEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PSYS_CALLBACK_INFORMATION sci = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (sci)
			{
				free(sci);
				sci = NULL;
			}

			OutputLength = sizeof(SYS_CALLBACK_INFORMATION) + Count * sizeof(SYS_CALLBACK_ENTRY_INFORMATION);

			sci = (PSYS_CALLBACK_INFORMATION)malloc(OutputLength);
			if (!sci)
			{
				break;
			}

			RtlZeroMemory(sci, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMSYSCALLBACK,
				NULL,		// InputBuffer
				0,
				sci,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)sci->NumberOfCallbacks + 100;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && sci)
		{
			for (UINT32 i = 0; i < sci->NumberOfCallbacks; i++)
			{
				// 完善进程信息结构
				//PerfectProcessModuleInfo(&pmi->ModuleEntry[i]);
				m_CallbackEntryVector.push_back(sci->CallbackEntry[i]);
			}
			bOk = TRUE;
		}

		if (sci)
		{
			free(sci);
			sci = NULL;
		}

		if (m_CallbackEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertSystemCallbackInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CSystemCallback::InsertSystemCallbackInfoList(CListCtrl *ListCtrl)
	{
		m_NotifyCreateProcess = 0;
		m_NotifyCreateThread = 0;
		m_NotifyLoadImage = 0;
		m_NotifyCmpCallback = 0;
		m_NotifyShutdown = 0;
		m_NotifyLastChanceShutdown = 0;
		m_NotifyCheck = 0;
		m_NotifyCheckReason = 0;
	    
		size_t Size = m_CallbackEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			SYS_CALLBACK_ENTRY_INFORMATION CallbackEntry = m_CallbackEntryVector[i];

			CString strCallbackAddress, strType, strFilePath, strCompany, strDescription;

			strCallbackAddress.Format(L"0x%p", CallbackEntry.CallbackAddress);
			strDescription.Format(L"0x%p", CallbackEntry.Description);

			switch (CallbackEntry.Type)
			{
			case ct_NotifyCreateProcess:
			{
				strType = L"CreateProcess";

				m_NotifyCreateProcess++;
				break;
			}
			case ct_NotifyCreateThread:
			{
				strType = L"CreateThread";

				m_NotifyCreateThread++;
				break;
			}
			case ct_NotifyLoadImage:
			{
				strType = L"LoadImage";

				m_NotifyLoadImage++;
				break;
			}
			case ct_NotifyCmpCallBack:
			{
				strType = L"CmpCallBack";

				m_NotifyCmpCallback++;
				break;
			}
			case ct_NotifyKeBugCheck:
			{
				strType = L"BugCheck";

				m_NotifyCheck++;
				break;
			}
			case ct_NotifyKeBugCheckReason:
			{
				strType = L"BugCheckReason";

				m_NotifyCheckReason++;
				break;
			}
			case ct_NotifyShutdown:
			{
				strType = L"Shutdown";

				m_NotifyShutdown++;
				break;
			}
			case ct_NotifyLastChanceShutdown:
			{
				strType = L"LastChanceShutdown";

				m_NotifyLastChanceShutdown++;
				break;
			}
			
			default:
				break;
			}

			strFilePath = m_DriverCore.GetDriverPathByAddress(CallbackEntry.CallbackAddress);
			strCompany = m_Global->GetFileCompanyName(strFilePath);

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strCallbackAddress);
			ListCtrl->SetItemText(iItem, scc_Type, strType);
			ListCtrl->SetItemText(iItem, scc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, scc_Company, strCompany);
			ListCtrl->SetItemText(iItem, scc_Description, strDescription);

			if (_wcsnicmp(strCompany.GetBuffer(), L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) != 0)
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			CString strStatusContext;
			strStatusContext.Format(L"系统回调正在加载  进程创建：%d，线程创建：%d，加载模块：%d，注册表：%d，错误检测：%d，关机：%d",
				m_NotifyCreateProcess,
				m_NotifyCreateThread,
				m_NotifyLoadImage,
				m_NotifyCmpCallback,
				m_NotifyCheckReason + m_NotifyCheck,
				m_NotifyShutdown + m_NotifyLastChanceShutdown);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"系统回调加载完成  进程创建：%d，线程创建：%d，加载模块：%d，注册表：%d，错误检测：%d，关机：%d",
			m_NotifyCreateProcess,
			m_NotifyCreateThread,
			m_NotifyLoadImage,
			m_NotifyCmpCallback,
			m_NotifyCheckReason + m_NotifyCheck,
			m_NotifyShutdown + m_NotifyLastChanceShutdown);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QuerySystemCallback
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CSystemCallback::QuerySystemCallback(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_CallbackEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumSystemCallback() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"System Callback Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"System Callback Initialize failed");
			return;
		}

		InsertSystemCallbackInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QuerySystemCallbackCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CSystemCallback::QuerySystemCallbackCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_SystemCallback->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_SystemCallback->m_Global->UpdateStatusBarTip(L"System Callback");
		m_SystemCallback->m_Global->UpdateStatusBarDetail(L"System Callback is loading now...");

		m_SystemCallback->QuerySystemCallback(ListCtrl);

		m_SystemCallback->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
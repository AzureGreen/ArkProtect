#include "stdafx.h"
#include "ProcessHandle.h"
#include "Global.hpp"


namespace ArkProtect
{
	CProcessHandle *CProcessHandle::m_ProcessHandle;

	CProcessHandle::CProcessHandle(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_ProcessHandle = this;
	}


	CProcessHandle::~CProcessHandle()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessHandleList
	*  Param: ProcessInfoList        ProcessHandle对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CProcessHandle::InitializeProcessHandleList(CListCtrl *ListCtrl)
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
	*  Name : EnumProcessHandle
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程句柄信息
	************************************************************************/
	BOOL CProcessHandle::EnumProcessHandle()
	{
		m_ProcessHandleEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_HANDLE_INFORMATION phi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (phi)
			{
				free(phi);
				phi = NULL;
			}

			OutputLength = sizeof(PROCESS_HANDLE_INFORMATION) + Count * sizeof(PROCESS_HANDLE_ENTRY_INFORMATION);

			phi = (PPROCESS_HANDLE_INFORMATION)malloc(OutputLength);
			if (!phi)
			{
				break;
			}

			RtlZeroMemory(phi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSHANDLE,
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				phi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)phi->NumberOfHandles + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && phi)
		{
			for (UINT32 i = 0; i < phi->NumberOfHandles; i++)
			{
				m_ProcessHandleEntryVector.push_back(phi->HandleEntry[i]);
			}
			bOk = TRUE;
		}

		if (phi)
		{
			free(phi);
			phi = NULL;
		}

		if (m_ProcessHandleEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertProcessHandleInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CProcessHandle::InsertProcessHandleInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessHandleNum = 0;
		size_t Size = m_ProcessHandleEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_HANDLE_ENTRY_INFORMATION HandleEntry = m_ProcessHandleEntryVector[i];

			CString strHandleType, strHandleName, strHandle, strObject, strReferenceCount;

			strHandleType = HandleEntry.wzHandleType;
			strHandleName = HandleEntry.wzHandleName;
			strHandle.Format(L"0x%04X", HandleEntry.Handle);
			strObject.Format(L"0x%08p", HandleEntry.Object);
			strReferenceCount.Format(L"%d", HandleEntry.ReferenceCount);

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strHandleType);
			ListCtrl->SetItemText(iItem, phc_HandleName, strHandleName);
			ListCtrl->SetItemText(iItem, phc_HandleValue, strHandle);
			ListCtrl->SetItemText(iItem, phc_Object, strObject);
			ListCtrl->SetItemText(iItem, phc_ReferenceCount, strReferenceCount);

			ProcessHandleNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Handle Info is loading now, Count:%d", ProcessHandleNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process Handle Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QueryProcessHandle
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CProcessHandle::QueryProcessHandle(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessHandleEntryVector.clear();

		if (EnumProcessHandle() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Handle Initialize failed");
			return;
		}

		InsertProcessHandleInfoList(ListCtrl);
	}



	/************************************************************************
	*  Name : QueryProcessModuleCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CProcessHandle::QueryProcessHandleCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessHandle->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_ProcessHandle->m_Global->UpdateStatusBarTip(L"Process Module");
		m_ProcessHandle->m_Global->UpdateStatusBarDetail(L"Process Module is loading now...");

		m_ProcessHandle->QueryProcessHandle(ListCtrl);

		m_ProcessHandle->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


}

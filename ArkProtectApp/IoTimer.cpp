#include "stdafx.h"
#include "IoTimer.h"
#include "Global.hpp"



namespace ArkProtect
{
	CIoTimer *CIoTimer::m_IoTimer;

	CIoTimer::CIoTimer(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_IoTimer = this;
	}


	CIoTimer::~CIoTimer()
	{
	}



	/************************************************************************
	*  Name : InitializeIoTimerList
	*  Param: ListCtrl               ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CIoTimer::InitializeIoTimerList(CListCtrl *ListCtrl)
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
	*  Name : EnumIoTimer
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举IoTimer信息
	************************************************************************/
	BOOL CIoTimer::EnumIoTimer()
	{
		m_IoTimerEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PIO_TIMER_INFORMATION iti = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (iti)
			{
				free(iti);
				iti = NULL;
			}

			OutputLength = sizeof(IO_TIMER_INFORMATION) + Count * sizeof(IO_TIMER_ENTRY_INFORMATION);

			iti = (PIO_TIMER_INFORMATION)malloc(OutputLength);
			if (!iti)
			{
				break;
			}

			RtlZeroMemory(iti, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMIOTIMER,
				NULL,		// InputBuffer
				0,
				iti,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = iti->NumberOfIoTimers + 20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && iti)
		{
			for (UINT32 i = 0; i < iti->NumberOfIoTimers; i++)
			{
				m_IoTimerEntryVector.push_back(iti->IoTimerEntry[i]);
			}
			bOk = TRUE;
		}

		if (iti)
		{
			free(iti);
			iti = NULL;
		}

		if (m_IoTimerEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}

	/************************************************************************
	*  Name : InsertIoTimerInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CIoTimer::InsertIoTimerInfoList(CListCtrl *ListCtrl)
	{
		UINT32 IoTimerNum = 0;
		size_t Size = m_IoTimerEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			IO_TIMER_ENTRY_INFORMATION IoTimerEntry = m_IoTimerEntryVector[i];

			CString strTimerObject, strDeviceObject, strFilePath, strStatus, strDispatch, strCompany;

			strTimerObject.Format(L"0x%p", IoTimerEntry.TimerObject);
			strDeviceObject.Format(L"0x%p", IoTimerEntry.DeviceObject);
			strDispatch.Format(L"0x%p", IoTimerEntry.TimeDispatch);
			strFilePath = m_DriverCore.GetDriverPathByAddress(IoTimerEntry.TimeDispatch);
			strCompany = m_Global->GetFileCompanyName(strFilePath);

			if (IoTimerEntry.Status)
			{
				strStatus = L"运行";
			}
			else
			{
				strStatus = L"停止";
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strTimerObject);
			ListCtrl->SetItemText(iItem, itc_Device, strDeviceObject);
			ListCtrl->SetItemText(iItem, itc_Status, strStatus);
			ListCtrl->SetItemText(iItem, itc_Dispatch, strDispatch);
			ListCtrl->SetItemText(iItem, itc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, itc_Company, strCompany);

			if (_wcsnicmp(strCompany.GetBuffer(), L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) != 0)
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			IoTimerNum++;

			CString strStatusContext;
			strStatusContext.Format(L"IoTimer Info is loading now, Count:%d", IoTimerNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"IoTimer Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}




	/************************************************************************
	*  Name : QueryIoTimer
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CIoTimer::QueryIoTimer(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_IoTimerEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumIoTimer() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"IoTimer Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"IoTimer Initialize failed");
			return;
		}

		InsertIoTimerInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryIoTimerCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询 IoTimer 的回调
	************************************************************************/
	DWORD CALLBACK CIoTimer::QueryIoTimerCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_IoTimer->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_IoTimer->m_Global->UpdateStatusBarTip(L"IoTimer");
		m_IoTimer->m_Global->UpdateStatusBarDetail(L"IoTimer is loading now...");

		m_IoTimer->QueryIoTimer(ListCtrl);

		m_IoTimer->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}

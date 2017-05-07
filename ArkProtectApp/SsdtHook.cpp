#include "stdafx.h"
#include "SsdtHook.h"
#include "Global.hpp"

namespace ArkProtect
{
	CSsdtHook *CSsdtHook::m_SsdtHook;
	UINT32    CSsdtHook::m_SsdtFunctionCount;

	CSsdtHook::CSsdtHook(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_SsdtHook = this;
		m_SsdtFunctionCount = 0;
	}


	CSsdtHook::~CSsdtHook()
	{
	}


	/************************************************************************
	*  Name : InitializeSsdtList
	*  Param: ListCtrl               ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CSsdtHook::InitializeSsdtList(CListCtrl *ListCtrl)
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
	*  Name : EnumSsdtHook
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举Ssdt信息
	************************************************************************/
	BOOL CSsdtHook::EnumSsdtHook()
	{
		m_SsdtHookEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x200;
		DWORD	 dwReturnLength = 0;
		PSSDT_HOOK_INFORMATION shi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (shi)
			{
				free(shi);
				shi = NULL;
			}

			OutputLength = sizeof(SSDT_HOOK_INFORMATION) + Count * sizeof(SSDT_HOOK_ENTRY_INFORMATION);

			shi = (PSSDT_HOOK_INFORMATION)malloc(OutputLength);
			if (!shi)
			{
				break;
			}

			RtlZeroMemory(shi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMSSDTHOOK,
				NULL,		// InputBuffer
				0,
				shi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)shi->NumberOfSsdtFunctions + 100;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && shi)
		{
			for (UINT32 i = 0; i < shi->NumberOfSsdtFunctions; i++)
			{
				// 完善进程信息结构
				m_SsdtHookEntryVector.push_back(shi->SsdtHookEntry[i]);
			}

			m_SsdtFunctionCount = shi->NumberOfSsdtFunctions;
			bOk = TRUE;
		}

		if (shi)
		{
			free(shi);
			shi = NULL;
		}

		if (m_SsdtHookEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertSsdtHookInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CSsdtHook::InsertSsdtHookInfoList(CListCtrl *ListCtrl)
	{
		UINT32 SsdtFuncNum = 0;
		UINT32 SsdtHookNum = 0;
		size_t Size = m_SsdtHookEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			SSDT_HOOK_ENTRY_INFORMATION SsdtHookEntry = m_SsdtHookEntryVector[i];

			CString strOrdinal, strFunctionName, strCurrentAddress, strOriginalAddress, strStatus, strFilePath;

			strOrdinal.Format(L"%d", SsdtHookEntry.Ordinal);
			strFunctionName = SsdtHookEntry.wzFunctionName;
			strCurrentAddress.Format(L"0x%p", SsdtHookEntry.CurrentAddress);
			strOriginalAddress.Format(L"0x%p", SsdtHookEntry.OriginalAddress);
			strFilePath = m_DriverCore.GetDriverPathByAddress(SsdtHookEntry.CurrentAddress);

			if (SsdtHookEntry.bHooked)
			{
				strStatus = L"SsdtHook";
				SsdtHookNum++;
			}
			else
			{
				strStatus = L"-";
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strOrdinal);
			ListCtrl->SetItemText(iItem, shc_FunctionName, strFunctionName);
			ListCtrl->SetItemText(iItem, shc_CurrentAddress, strCurrentAddress);
			ListCtrl->SetItemText(iItem, shc_OriginalAddress, strOriginalAddress);
			ListCtrl->SetItemText(iItem, shc_Status, strStatus);
			ListCtrl->SetItemText(iItem, shc_FilePath, strFilePath);

			if (SsdtHookEntry.bHooked)
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			SsdtFuncNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Ssdt Hook Info is loading now, Count:%d", SsdtFuncNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Ssdt函数: %d，被挂钩函数: %d", Size, SsdtHookNum);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QuerySsdtHook
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CSsdtHook::QuerySsdtHook(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_SsdtHookEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumSsdtHook() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Ssdt Hook Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Ssdt Hook Initialize failed");
			return;
		}

		InsertSsdtHookInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QuerySsdtHookCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CSsdtHook::QuerySsdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_SsdtHook->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_SsdtHook->m_Global->UpdateStatusBarTip(L"Ssdt");
		m_SsdtHook->m_Global->UpdateStatusBarDetail(L"Ssdt is loading now...");

		m_SsdtHook->QuerySsdtHook(ListCtrl);

		m_SsdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : UnloadDriver
	*  Param: Ordinal
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程及相关信息
	************************************************************************/
	BOOL CSsdtHook::ResumeSsdtHook(UINT32 Ordinal)
	{
		BOOL   bOk = FALSE;
		DWORD  dwReturnLength = 0;
		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_RESUMESSDTHOOK,
			&Ordinal,		// InputBuffer
			sizeof(UINT32),
			NULL,
			0,
			&dwReturnLength,
			NULL);

		return bOk;
	}


	/************************************************************************
	*  Name : ResumeSsdtHookCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CSsdtHook::ResumeSsdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		int iIndex = ListCtrl->GetSelectionMark();
		if (iIndex < 0)
		{
			return 0;
		}

		UINT32   Ordinal = iIndex;

		UINT_PTR CurrentAddress = 0;
		CString  strCurrentAddress = ListCtrl->GetItemText(iIndex, shc_CurrentAddress);
		swscanf_s(strCurrentAddress.GetBuffer() + 2, L"%p", &CurrentAddress);

		UINT_PTR OriginalAddress = 0;
		CString  strOriginalAddress = ListCtrl->GetItemText(iIndex, shc_OriginalAddress);
		swscanf_s(strOriginalAddress.GetBuffer() + 2, L"%p", &OriginalAddress);

		// 如果没有Hook就直接返回了
		if (OriginalAddress == CurrentAddress || Ordinal < 0 || Ordinal > m_SsdtFunctionCount)
		{
			return 0;
		}

		m_SsdtHook->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		if (m_SsdtHook->ResumeSsdtHook(Ordinal))
		{
			// 刷新列表
			m_SsdtHook->QuerySsdtHook(ListCtrl);
		}

		m_SsdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
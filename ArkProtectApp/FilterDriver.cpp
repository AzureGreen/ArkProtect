#include "stdafx.h"
#include "FilterDriver.h"
#include "Global.hpp"

namespace ArkProtect
{
	CFilterDriver *CFilterDriver::m_FilterDriver;

	CFilterDriver::CFilterDriver(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_FilterDriver = this;
	}


	CFilterDriver::~CFilterDriver()
	{
	}


	/************************************************************************
	*  Name : InitializeFilterDriverList
	*  Param: ListCtrl               ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CFilterDriver::InitializeFilterDriverList(CListCtrl *ListCtrl)
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
	*  Name : EnumFilterDriver
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程模块信息
	************************************************************************/
	BOOL CFilterDriver::EnumFilterDriver()
	{
		m_FilterDriverEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PFILTER_DRIVER_INFORMATION fdi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (fdi)
			{
				free(fdi);
				fdi = NULL;
			}

			OutputLength = sizeof(FILTER_DRIVER_INFORMATION) + Count * sizeof(FILTER_DRIVER_ENTRY_INFORMATION);

			fdi = (PFILTER_DRIVER_INFORMATION)malloc(OutputLength);
			if (!fdi)
			{
				break;
			}

			RtlZeroMemory(fdi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMFILTERDRIVER,
				NULL,		// InputBuffer
				0,
				fdi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = fdi->NumberOfFilterDrivers + 100;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && fdi)
		{
			for (UINT32 i = 0; i < fdi->NumberOfFilterDrivers; i++)
			{
				m_FilterDriverEntryVector.push_back(fdi->FilterDriverEntry[i]);
			}
			bOk = TRUE;
		}

		if (fdi)
		{
			free(fdi);
			fdi = NULL;
		}

		if (m_FilterDriverEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertFilterDriverInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CFilterDriver::InsertFilterDriverInfoList(CListCtrl *ListCtrl)
	{
		UINT32 FilterDriverNum = 0;
		size_t Size = m_FilterDriverEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			FILTER_DRIVER_ENTRY_INFORMATION FilterDriverEntry = m_FilterDriverEntryVector[i];

			CString strFilterType, strFilterDriverName, strFilePath, strFilterDeviceObject, strDeviceName, strAttachedDriverName, strCompany;

			strFilterDriverName = FilterDriverEntry.wzFilterDriverName;
			strFilePath = m_Global->TrimPath(FilterDriverEntry.wzFilePath);
			strFilterDeviceObject.Format(L"0x%08p", FilterDriverEntry.FilterDeviceObject);
			strDeviceName = FilterDriverEntry.wzFilterDeviceName;
			strAttachedDriverName = FilterDriverEntry.wzAttachedDriverName;
			strCompany = m_Global->GetFileCompanyName(strFilePath);

			switch (FilterDriverEntry.FilterType)
			{
			case ft_File:
			{
				strFilterType = L"File";
				break;
			}
			case ft_Disk:
			{
				strFilterType = L"Disk";
				break;
			}
			case ft_Volume:
			{
				strFilterType = L"Volume";
				break;
			}
			case ft_Keyboard:
			{
				strFilterType = L"Keyboard";
				break;
			}
			case ft_Mouse:
			{
				strFilterType = L"Mouse";
				break;
			}
			case ft_I8042prt:
			{
				strFilterType = L"I8042prt";
				break;
			}
			case ft_Tcpip:
			{
				strFilterType = L"Tcpip";
				break;
			}
			case ft_Ndis:
			{
				strFilterType = L"NDIS";
				break;
			}
			case ft_PnpManager:
			{
				strFilterType = L"PnpManager";
				break;
			}
			case ft_Tdx:
			{
				strFilterType = L"Tdx";
				break;
			}
			case ft_Raw:
			{
				strFilterType = L"Raw";
				break;
			}
			default:
				strFilterType = L"Unknown";
				break;
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strFilterType);
			ListCtrl->SetItemText(iItem, fdc_DriverName, strFilterDriverName);
			ListCtrl->SetItemText(iItem, fdc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, fdc_DeviceObject, strFilterDeviceObject);
			ListCtrl->SetItemText(iItem, fdc_DeviceName, strDeviceName);
			ListCtrl->SetItemText(iItem, fdc_AttachedDriverName, strAttachedDriverName);
			ListCtrl->SetItemText(iItem, fdc_Compay, strCompany);

			ListCtrl->SetItemData(iItem, iItem);

			FilterDriverNum++;

			CString strStatusContext;
			strStatusContext.Format(L"FilterDriver Info is loading now, Count:%d", FilterDriverNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"FilterDriver Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}



	/************************************************************************
	*  Name : QueryFilterDriver
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CFilterDriver::QueryFilterDriver(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_FilterDriverEntryVector.clear();

		if (EnumFilterDriver() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Filter Driver Initialize failed");
			return;
		}

		InsertFilterDriverInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryFilterDriverCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程模块的回调
	************************************************************************/
	DWORD CALLBACK CFilterDriver::QueryFilterDriverCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_FilterDriver->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_FilterDriver->m_Global->UpdateStatusBarTip(L"Filter Driver");
		m_FilterDriver->m_Global->UpdateStatusBarDetail(L"Filter Driver is loading now...");

		m_FilterDriver->QueryFilterDriver(ListCtrl);

		m_FilterDriver->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}

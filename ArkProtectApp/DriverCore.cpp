#include "stdafx.h"
#include "DriverCore.h"
#include "Global.hpp"
#include "DriverDlg.h"

namespace ArkProtect
{
	CDriverCore *CDriverCore::m_Driver;

	CDriverCore::CDriverCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_Driver = this;
	}


	CDriverCore::~CDriverCore()
	{
	}


	/************************************************************************
	*  Name : InitializeDriverList
	*  Param: ProcessList           ProcessModule对话框的ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CDriverCore::InitializeDriverList(CListCtrl *DriverList)
	{
		DriverList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			DriverList->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : PerfectDriverInfo
	*  Param: DriverEntry			     驱动模块信息结构
	*  Ret  : void
	*  完善进程信息结构
	************************************************************************/
	void CDriverCore::PerfectDriverInfo(PDRIVER_ENTRY_INFORMATION DriverEntry)
	{
		if (!DriverEntry || wcslen(DriverEntry->wzDriverPath) == 0)
		{
			return;
		}

		//
		// 处理驱动路径
		// 
		WCHAR wzWindowsDirectory[MAX_PATH] = { 0 };
		WCHAR wzDriverDirectory[MAX_PATH] = { 0 };
		WCHAR wzDrivers[] = L"\\System32\\Drivers\\";

		GetWindowsDirectory(wzWindowsDirectory, MAX_PATH - 1);	// 获得Windows目录
		StringCchCopyW(wzDriverDirectory, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
		StringCchCatW(wzDriverDirectory, wcslen(wzDriverDirectory) + wcslen(wzDrivers) + 1 , wzDrivers);

		WCHAR  *wzOriginalDriverPath = DriverEntry->wzDriverPath;
		WCHAR  wzFixedDriverPath[MAX_PATH] = { 0 };
		WCHAR  *wzPos = wcschr(wzOriginalDriverPath, L'\\');

		// 没有目录信息 只有驱动名字 
		if (!wzPos)
		{
			StringCchCopyW(wzFixedDriverPath, wcslen(wzDriverDirectory) + 1, wzDriverDirectory);
			StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath) + 1, wzOriginalDriverPath);
			StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

			wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
		}
		else
		{
			WCHAR wzUnknow[] = L"\\??\\";
			WCHAR wzSystemRoot[] = L"\\SystemRoot";
			WCHAR wzWindows[] = L"\\Windows";
			WCHAR wzWinnt[] = L"\\Winnt";
			size_t   Original = wcslen(wzOriginalDriverPath);

			if (Original >= wcslen(wzUnknow) && _wcsnicmp(wzOriginalDriverPath, wzUnknow, wcslen(wzUnknow)) == 0)
			{
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzUnknow)) + 1, wzOriginalDriverPath + wcslen(wzUnknow));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);
				
				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzSystemRoot) && _wcsnicmp(wzOriginalDriverPath, wzSystemRoot, wcslen(wzSystemRoot)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzSystemRoot)) + 1, wzOriginalDriverPath + wcslen(wzSystemRoot));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzWindows) && _wcsnicmp(wzOriginalDriverPath, wzWindows, wcslen(wzWindows)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzWindows)) + 1, wzOriginalDriverPath + wcslen(wzWindows));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzWinnt) && _wcsnicmp(wzOriginalDriverPath, wzWinnt, wcslen(wzWinnt)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzWinnt)) + 1, wzOriginalDriverPath + wcslen(wzWinnt));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
		}

		// 如果是短文件名
		if (wcschr(wzOriginalDriverPath, '~'))
		{
			WCHAR wzLongPathName[MAX_PATH] = { 0 };
			DWORD dwReturn = GetLongPathName(wzOriginalDriverPath, wzLongPathName, MAX_PATH);
			if (!(dwReturn >= MAX_PATH || dwReturn == 0))
			{
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzLongPathName) + 1, wzLongPathName);

				wzOriginalDriverPath[wcslen(wzLongPathName)] = L'\0';
			}
		}


		//
		// 处理驱动名称
		// 
		WCHAR *wzDriverName = NULL;
		wzDriverName = wcsrchr(DriverEntry->wzDriverPath, '\\');  // 最后出现 \\ 
		wzDriverName++;  // 过 '\\'

		StringCchCopyW(DriverEntry->wzDriverName, wcslen(wzDriverName) + 1, wzDriverName);

		//
		// 处理厂商名称
		//
		CString strCompanyName = m_Global->GetFileCompanyName(DriverEntry->wzDriverPath);
		if (strCompanyName.GetLength() == 0)
		{
			strCompanyName = L"文件不存在";
		}

		StringCchCopyW(DriverEntry->wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());
	}


	/************************************************************************
	*  Name : EnumDriverInfo
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程及相关信息
	************************************************************************/
	BOOL CDriverCore::EnumDriverInfo()
	{
		m_DriverEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PDRIVER_INFORMATION di = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (di)
			{
				free(di);
				di = NULL;
			}

			OutputLength = sizeof(DRIVER_INFORMATION) + Count * sizeof(DRIVER_ENTRY_INFORMATION);

			di = (PDRIVER_INFORMATION)malloc(OutputLength);
			if (!di)
			{
				break;
			}

			RtlZeroMemory(di, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMDRIVER,
				NULL,		// InputBuffer
				0,
				di,
				OutputLength,
				&dwReturnLength,
				NULL);

			// 这句配合do while循环，恰到好处，如果给的内存不足则重复枚举，如果足够则一次通过
			Count = (UINT32)di->NumberOfDrivers + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && di)
		{
			for (UINT32 i = 0; i < di->NumberOfDrivers; i++)
			{
				// 完善进程信息结构
				PerfectDriverInfo(&di->DriverEntry[i]);
				m_DriverEntryVector.push_back(di->DriverEntry[i]);
			}
			bOk = TRUE;
		}

		if (di)
		{
			free(di);
			di = NULL;
		}

		if (m_DriverEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertDriverInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  向ListControl里插入进程信息
	************************************************************************/
	void CDriverCore::InsertDriverInfoList(CListCtrl *ListCtrl)
	{
		UINT32 DriverNum = 0;
		size_t Size = m_DriverEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			DRIVER_ENTRY_INFORMATION DriverEntry = m_DriverEntryVector[i];

			CString strDriverName, strBase, strSize, strObject, strDriverPath, strServiceName, strLoadOrder, strCompanyName, strDriverStart;

			strDriverName = DriverEntry.wzDriverName;
			strDriverPath = DriverEntry.wzDriverPath;
			strServiceName = DriverEntry.wzServiceName;

			strBase.Format(L"0x%p", DriverEntry.BaseAddress);
			strSize.Format(L"0x%X", DriverEntry.Size);
			strLoadOrder.Format(L"%d", DriverEntry.LoadOrder);

			if (DriverEntry.DriverObject)
			{
				strObject.Format(L"0x%p", DriverEntry.DriverObject);
				strDriverStart.Format(L"0x%p", DriverEntry.DirverStartAddress);
			}
			else
			{
				strObject = L"-";
				strDriverStart = L"-";
			}

			strCompanyName = DriverEntry.wzCompanyName;

			// 添加图标
			m_Global->AddFileIcon(DriverEntry.wzDriverPath, &((CDriverDlg*)m_Global->m_DriverDlg)->m_DriverIconList);
			
			int iImageCount = ((CDriverDlg*)m_Global->m_DriverDlg)->m_DriverIconList.GetImageCount() - 1;
			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strDriverName, iImageCount);

			ListCtrl->SetItemText(iItem, dc_BaseAddress, strBase);
			ListCtrl->SetItemText(iItem, dc_Size, strSize);
			ListCtrl->SetItemText(iItem, dc_Object, strObject);
			ListCtrl->SetItemText(iItem, dc_DriverPath, strDriverPath);
			ListCtrl->SetItemText(iItem, dc_ServiceName, strServiceName);
			ListCtrl->SetItemText(iItem, dc_StartAddress, strDriverStart);
			ListCtrl->SetItemText(iItem, dc_LoadOrder, strLoadOrder);
			ListCtrl->SetItemText(iItem, dc_Company, strCompanyName);

			if (_wcsnicmp(strCompanyName, L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) == 0)
			{
				ListCtrl->SetItemData(iItem, 1);
			}

			DriverNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Driver Info is loading now, Count:%d", DriverNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Driver Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}



	/************************************************************************
	*  Name : QueryDriverInfo
	*  Param: ListCtrl
	*  Ret  : void
	*  查询进程信息
	************************************************************************/
	void CDriverCore::QueryDriverInfo(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_DriverEntryVector.clear();

		if (EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Info Initialize failed");
			return;
		}

		InsertDriverInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryDriverInfoCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程信息的回调
	************************************************************************/
	DWORD CALLBACK CDriverCore::QueryDriverInfoCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Driver->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_Driver->m_Global->UpdateStatusBarTip(L"Process Info");
		m_Driver->m_Global->UpdateStatusBarDetail(L"Process Info is loading now...");

		m_Driver->QueryDriverInfo(ListCtrl);

		m_Driver->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : UnloadDriver
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程及相关信息
	************************************************************************/
	BOOL CDriverCore::UnloadDriver(UINT_PTR DriverObject)
	{
		BOOL bOk = FALSE;
		DWORD	 dwReturnLength = 0;
		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_UNLOADRIVER,
			&DriverObject,		// InputBuffer
			sizeof(UINT_PTR),
			NULL,
			0,
			&dwReturnLength,
			NULL);

		return bOk;
	}


	/************************************************************************
	*  Name : UnloadDriverInfoCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  查询进程信息的回调
	************************************************************************/
	DWORD CALLBACK CDriverCore::UnloadDriverInfoCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		int iIndex = ListCtrl->GetSelectionMark();
		if (iIndex < 0)
		{
			return 0;
		}

		UINT_PTR DriverObject = 0;
		CString  strObject = ListCtrl->GetItemText(iIndex, dc_Object);
		swscanf_s(strObject.GetBuffer() + 2, L"%p", &DriverObject);

		if (DriverObject == 0)
		{
			return 0;
		}

		m_Driver->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		if (m_Driver->UnloadDriver(DriverObject))
		{
			// 刷新列表
			m_Driver->QueryDriverInfo(ListCtrl);
		}

		m_Driver->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : GetDriverPathByAddress
	*  Param: Address                  地址
	*  Ret  : CString
	*  通过地址所在地址范围，获得驱动路径
	************************************************************************/
	CString CDriverCore::GetDriverPathByAddress(UINT_PTR Address)
	{
		CString strPath = L"";

		size_t Size = m_DriverEntryVector.size();

		for (int i = 0; i < Size; i++)
		{
			DRIVER_ENTRY_INFORMATION DriverEntry = m_DriverEntryVector[i];

			UINT_PTR BaseAddress = DriverEntry.BaseAddress;
			UINT_PTR EndAddress = DriverEntry.BaseAddress + DriverEntry.Size;

			if (Address >= BaseAddress && Address <= EndAddress)
			{
				strPath = DriverEntry.wzDriverPath;
				break;
			}
		}
		return strPath;
	}
}
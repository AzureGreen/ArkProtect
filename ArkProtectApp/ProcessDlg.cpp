// ProcessDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ProcessDlg.h"
#include "afxdialogex.h"
#include "ArkProtectAppDlg.h"
#include "FileCore.h"

// CProcessDlg 对话框

IMPLEMENT_DYNAMIC(CProcessDlg, CDialogEx)


UINT32 CProcessDlg::m_SortColumn;
BOOL CProcessDlg::m_bSortOrder;

CProcessDlg::CProcessDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_PROCESS_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	// 保存对话框指针
	m_Global->m_ProcessDlg = this;
	m_SortColumn = 0;
	m_bSortOrder = FALSE;
}

CProcessDlg::~CProcessDlg()
{
}

void CProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_LIST, m_ProcessListCtrl);
}


BEGIN_MESSAGE_MAP(CProcessDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROCESS_LIST, &CProcessDlg::OnNMCustomdrawProcessList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROCESS_LIST, &CProcessDlg::OnLvnColumnclickProcessList)
	ON_NOTIFY(NM_RCLICK, IDC_PROCESS_LIST, &CProcessDlg::OnNMRClickProcessList)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_PROCESS_DELETE, &CProcessDlg::OnProcessDelete)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_DELETE, &CProcessDlg::OnUpdateProcessDelete)
	ON_COMMAND(ID_PROCESS_FRESHEN, &CProcessDlg::OnProcessFreshen)
	ON_COMMAND(ID_PROCESS_MODULE, &CProcessDlg::OnProcessModule)
	ON_COMMAND(ID_PROCESS_THREAD, &CProcessDlg::OnProcessThread)
	ON_COMMAND(ID_PROCESS_HANDLE, &CProcessDlg::OnProcessHandle)
	ON_COMMAND(ID_PROCESS_WINDOW, &CProcessDlg::OnProcessWindow)
	ON_COMMAND(ID_PROCESS_MEMORY, &CProcessDlg::OnProcessMemory)
	ON_COMMAND(ID_PROCESS_TERMINATE, &CProcessDlg::OnProcessTerminate)
	ON_COMMAND(ID_PROCESS_FORCE_TERMINATE, &CProcessDlg::OnProcessForceTerminate)
	ON_COMMAND(ID_PROCESS_SUSPEND, &CProcessDlg::OnProcessSuspend)
	ON_COMMAND(ID_PROCESS_RESUME, &CProcessDlg::OnProcessResume)
	ON_COMMAND(ID_PROCESS_PROPERTY, &CProcessDlg::OnProcessProperty)
	ON_COMMAND(ID_PROCESS_LOCATION, &CProcessDlg::OnProcessLocation)
	ON_COMMAND(ID_PROCESS_EXPORT_INFORMATION, &CProcessDlg::OnProcessExportInformation)
END_MESSAGE_MAP()


// CProcessDlg 消息处理程序


BOOL CProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	// 初始化进程列表
	APInitializeProcessList();

	// 创建Icon图标列表
	UINT nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_ProcessIconList.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	ListView_SetImageList(m_ProcessListCtrl.m_hWnd, m_ProcessIconList.GetSafeHandle(), LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CProcessDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	m_Global->iResizeX = cx;
	m_Global->iResizeY = cy;
}


void CProcessDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		m_ProcessListCtrl.MoveWindow(0, 0, m_Global->iResizeX, m_Global->iResizeY);

		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)m_Global->AppDlg)->m_CurrentChildDlg = ArkProtect::cd_ProcessDialog;
		((CArkProtectAppDlg*)m_Global->AppDlg)->m_ProcessButton.EnableWindow(FALSE);

		// 加载进程信息列表
		APLoadProcessList();

		m_ProcessListCtrl.SetFocus();
	}
}


void CProcessDlg::OnNMCustomdrawProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		COLORREF clrNewTextColor, clrNewBkColor;
		BOOL bNotTrust = FALSE;
		int iItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

		clrNewTextColor = RGB(0, 0, 0);
		clrNewBkColor = RGB(255, 255, 255);

		bNotTrust = (BOOL)m_ProcessListCtrl.GetItemData(iItem);
		if (bNotTrust == TRUE)
		{
			clrNewTextColor = RGB(0, 0, 255);
		}

		pLVCD->clrText = clrNewTextColor;
		pLVCD->clrTextBk = clrNewBkColor;

		*pResult = CDRF_DODEFAULT;
	}
}


void CProcessDlg::OnLvnColumnclickProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	m_SortColumn = pNMLV->iSubItem;

	int iItemCount = m_ProcessListCtrl.GetItemCount();
	for (int i = 0; i < iItemCount; i++)
	{
		m_ProcessListCtrl.SetItemData(i, i);	// Set the data of each item to be equal to its index. 
	}

	m_ProcessListCtrl.SortItems((PFNLVCOMPARE)::APProcessListCompareFunc, (DWORD_PTR)&m_ProcessListCtrl);

	if (m_bSortOrder)
	{
		m_bSortOrder = FALSE;
	}
	else
	{
		m_bSortOrder = TRUE;
	}

	for (int i = 0; i < iItemCount; i++)
	{
		if (_wcsnicmp(m_ProcessListCtrl.GetItemText(i, ArkProtect::pc_Company),
			L"Microsoft Corporation",
			wcslen(L"Microsoft Corporation")) != 0)
		{
			m_ProcessListCtrl.SetItemData(i, TRUE);
		}
	}


	*pResult = 0;
}


void CProcessDlg::OnNMRClickProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	CMenu Menu;
	Menu.LoadMenuW(IDR_PROCESS_MENU);
	CMenu* SubMenu = Menu.GetSubMenu(0);	// 子菜单

	CPoint Pt;
	GetCursorPos(&Pt);         // 得到鼠标位置

	int	iCount = SubMenu->GetMenuItemCount();

	// 如果没有选中,除了刷新 其他全部Disable
	if (m_ProcessListCtrl.GetSelectedCount() == 0)
	{
		for (int i = 0; i < iCount; i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED); //菜单全部变灰
		}

		SubMenu->EnableMenuItem(ID_PROCESS_FRESHEN, MF_BYCOMMAND | MF_ENABLED);
	}

	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Pt.x, Pt.y, this);

	*pResult = 0;
}


void CProcessDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CDialogEx::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	// TODO: 在此处添加消息处理程序代码
	if (!bSysMenu && pPopupMenu)
	{
		CCmdUI CmdUI;
		CmdUI.m_pOther = NULL;
		CmdUI.m_pMenu = pPopupMenu;
		CmdUI.m_pSubMenu = NULL;

		UINT nCount = pPopupMenu->GetMenuItemCount();
		CmdUI.m_nIndexMax = nCount;
		for (UINT i = 0; i < nCount; i++)
		{
			UINT nID = pPopupMenu->GetMenuItemID(i);
			if (nID == -1 || nID == 0)
			{
				continue;
			}
			CmdUI.m_nID = nID;
			CmdUI.m_nIndex = i;
			CmdUI.DoUpdate(this, FALSE);
		}
	}
}


void CProcessDlg::OnProcessDelete()
{
	// TODO: 在此添加命令处理程序代码
	m_bDeleteFile = m_bDeleteFile ? FALSE : TRUE;
}


void CProcessDlg::OnUpdateProcessDelete(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(m_bDeleteFile);
}


void CProcessDlg::OnProcessFreshen()
{
	// TODO: 在此添加命令处理程序代码
	// 加载进程信息列表
	APLoadProcessList();
}


void CProcessDlg::OnProcessModule()
{
	// TODO: 在此添加命令处理程序代码

	// 初始化ProcessInfoDlg，传入
	APInitializeProcessInfoDlg(ArkProtect::pik_Module);

}


void CProcessDlg::OnProcessThread()
{
	// TODO: 在此添加命令处理程序代码

	// 初始化ProcessInfoDlg，传入
	APInitializeProcessInfoDlg(ArkProtect::pik_Thread);
}


void CProcessDlg::OnProcessHandle()
{
	// TODO: 在此添加命令处理程序代码
	// 初始化ProcessInfoDlg，传入
	APInitializeProcessInfoDlg(ArkProtect::pik_Handle);
}


void CProcessDlg::OnProcessWindow()
{
	// TODO: 在此添加命令处理程序代码
	// 初始化ProcessInfoDlg，传入
	APInitializeProcessInfoDlg(ArkProtect::pik_Window);
}


void CProcessDlg::OnProcessMemory()
{
	// TODO: 在此添加命令处理程序代码
	// 初始化ProcessInfoDlg，传入
	APInitializeProcessInfoDlg(ArkProtect::pik_Memory);
}


void CProcessDlg::OnProcessTerminate()
{
	// TODO: 在此添加命令处理程序代码

	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	// 结束进程
	HANDLE ThreadHandle = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::TerminateProcessCallback, &m_ProcessListCtrl, 0, NULL);

	// 阻塞操作，等待进程的结束
	WaitForSingleObject(ThreadHandle, INFINITY);
	
	// 判断是否要删除文件
	if (m_bDeleteFile)
	{
		APDeleteFile();
	}
}


void CProcessDlg::OnProcessForceTerminate()
{
	// TODO: 在此添加命令处理程序代码

	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	// 暴力结束进程
	HANDLE ThreadHandle = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::ForceTerminateProcessCallback, &m_ProcessListCtrl, 0, NULL);

	// 阻塞操作，等待进程的结束
	WaitForSingleObject(ThreadHandle, INFINITE);
	
	// 判断是否要删除文件
	if (m_bDeleteFile)
	{
		APDeleteFile();
	}
}


void CProcessDlg::OnProcessSuspend()
{
	// TODO: 在此添加命令处理程序代码
	if (m_bSuspend == FALSE)
	{
		// 获得进程id
		UINT32   ProcessId = 0;
		POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

		while (Pos)
		{
			int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);
			ProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ProcessId).GetBuffer());
		}

		// 获得一个线程Id
		UINT32 ThreadId = 0;
		BOOL bOk = m_Global->ProcessThread().GetThreadIdByProcessId(ProcessId, &ThreadId);
		if (bOk)
		{
			// 挂起该线程
			m_SuspendThreadHandle = ::OpenThread(THREAD_SUSPEND_RESUME, FALSE, ThreadId);

			::SuspendThread(m_SuspendThreadHandle);

			m_bSuspend = TRUE;
		}
	}
	else
	{
		MessageBox(L"已经挂起该进程了！", L"ArkProtect", MB_OK | MB_ICONERROR);
	}
}


void CProcessDlg::OnProcessResume()
{
	// TODO: 在此添加命令处理程序代码
	if (m_bSuspend == TRUE)
	{
		::ResumeThread(m_SuspendThreadHandle);
		::CloseHandle(m_SuspendThreadHandle);
		m_SuspendThreadHandle = NULL;
		m_bSuspend = FALSE;
	}
	else
	{
		MessageBox(L"该进程处于正常状态！", L"ArkProtect", MB_OK | MB_ICONERROR);
	}
}


void CProcessDlg::OnProcessProperty()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		m_Global->CheckFileProperty(strFilePath);
	}
}


void CProcessDlg::OnProcessLocation()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)  	// 绕过Idle 
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		m_Global->LocationInExplorer(strFilePath);
	}
}


void CProcessDlg::OnProcessExportInformation()
{
	// TODO: 在此添加命令处理程序代码

	m_Global->ExportInformationInText(m_ProcessListCtrl);

}


/************************************************************************
*  Name : APInitializeProcessList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessDlg::APInitializeProcessList()
{
	m_Global->ProcessCore().InitializeProcessList(&m_ProcessListCtrl);
}


/************************************************************************
*  Name : APLoadProcessList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessDlg::APLoadProcessList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	while (m_ProcessIconList.Remove(0));

	m_ProcessListCtrl.DeleteAllItems();

	m_ProcessListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::QueryProcessInfoCallback, &m_ProcessListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APDeleteFile
*  Param: void
*  Ret  : void
*  删除文件
************************************************************************/
void CProcessDlg::APDeleteFile()
{
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		m_strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CFileCore::DeleteFileCallback, &m_strFilePath, 0, NULL)
		);
	}
}


/************************************************************************
*  Name : APInitializeProcessInfoDlg
*  Param: ProcessInfoKind            想要打开的进程信息类型
*  Ret  : void
*  构建ProcessEntry，创建新的子对话框用于显示目标进程信息
************************************************************************/
void CProcessDlg::APInitializeProcessInfoDlg(ArkProtect::eProcessInfoKind ProcessInfoKind)
{
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		// 组合，构造 ProcessEntry结构
		UINT32 ProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ProcessId).GetBuffer());
		UINT32 ParentProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ParentProcessId).GetBuffer());
		CString strEProcess = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_EProcess);

		UINT_PTR EProcess = 0;
		strEProcess = strEProcess.GetBuffer() + 2;	 // 绕过0x
		swscanf_s(strEProcess.GetBuffer(), L"%p", &EProcess);

		ArkProtect::PROCESS_ENTRY_INFORMATION ProcessEntry = { 0 };

		ProcessEntry.ProcessId = ProcessId;
		ProcessEntry.ParentProcessId = ParentProcessId;
		ProcessEntry.EProcess = EProcess;

		if (_wcsnicmp(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_UserAccess).GetBuffer(), L"拒绝", wcslen(L"拒绝")) == 0)
		{
			ProcessEntry.bUserAccess = FALSE;
		}
		else
		{
			ProcessEntry.bUserAccess = TRUE;
		}

		StringCchCopyW(ProcessEntry.wzCompanyName,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_Company).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_Company).GetBuffer());
		StringCchCopyW(ProcessEntry.wzImageName,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ImageName).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ImageName).GetBuffer());
		StringCchCopyW(ProcessEntry.wzFilePath,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath).GetBuffer());

		m_Global->ProcessCore().ProcessEntry() = &ProcessEntry;

		m_ProcessInfoDlg = new CProcessInfoDlg(this, ProcessInfoKind, m_Global);
		m_ProcessInfoDlg->DoModal();
	}
}



/************************************************************************
*  Name : APProcessListCompareFunc
*  Param: lParam1                   第一行
*  Param: lParam2                   第二行
*  Param: lParamSort                附加参数（ListControl）
*  Ret  : void
*  查询进程信息的回调
************************************************************************/
int CALLBACK APProcessListCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// 从参数中提取需要比较的两行数据

	int iRow1 = (int)lParam1;
	int iRow2 = (int)lParam2;

	CListCtrl* ListCtrl = (CListCtrl*)lParamSort;

	CString str1 = ListCtrl->GetItemText(iRow1, CProcessDlg::m_SortColumn);
	CString str2 = ListCtrl->GetItemText(iRow2, CProcessDlg::m_SortColumn);

	if (CProcessDlg::m_SortColumn == ArkProtect::pc_ProcessId ||
		CProcessDlg::m_SortColumn == ArkProtect::pc_ParentProcessId)
	{
		// int型比较
		if (CProcessDlg::m_bSortOrder)
		{
			return _ttoi(str1) - _ttoi(str2);
		}
		else
		{
			return _ttoi(str2) - _ttoi(str1);
		}
	}
	else if (CProcessDlg::m_SortColumn == ArkProtect::pc_EProcess)
	{
		UINT_PTR p1 = 0, p2 = 0;

		str1 = str1.GetBuffer() + 2;	// 过0x
		str2 = str2.GetBuffer() + 2;

		swscanf_s(str1.GetBuffer(), L"%P", &p1);
		swscanf_s(str2.GetBuffer(), L"%P", &p2);

		if (CProcessDlg::m_bSortOrder)
		{
			return (int)(p1 - p2);
		}
		else
		{
			return (int)(p2 - p1);
		}
	}
	else
	{
		// 文字型比较
		if (CProcessDlg::m_bSortOrder)
		{
			return str1.CompareNoCase(str2);
		}
		else
		{
			return str2.CompareNoCase(str1);
		}
	}

	return 0;
}


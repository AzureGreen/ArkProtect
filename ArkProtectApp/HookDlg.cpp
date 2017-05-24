// HookDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "HookDlg.h"
#include "afxdialogex.h"

#include "ArkProtectAppDlg.h"

// CHookDlg 对话框

IMPLEMENT_DYNAMIC(CHookDlg, CDialogEx)

CHookDlg::CHookDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_HOOK_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	// 保存对话框指针
	m_Global->m_HookDlg = this;
}

CHookDlg::~CHookDlg()
{
}

void CHookDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOOK_LISTCTRL, m_HookListCtrl);
	DDX_Control(pDX, IDC_HOOK_LISTBOX, m_HookListBox);
	DDX_Control(pDX, IDC_HOOKMODULE_LISTBOX, m_HookModuleListBox);
}


BEGIN_MESSAGE_MAP(CHookDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_LBN_SELCHANGE(IDC_HOOK_LISTBOX, &CHookDlg::OnLbnSelchangeHookListbox)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_HOOK_LISTCTRL, &CHookDlg::OnNMCustomdrawHookListctrl)
	ON_NOTIFY(NM_RCLICK, IDC_HOOK_LISTCTRL, &CHookDlg::OnNMRClickHookListctrl)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_HOOK_ONLY_SHOW_HOOKED, &CHookDlg::OnHookOnlyShowHooked)
	ON_UPDATE_COMMAND_UI(ID_HOOK_ONLY_SHOW_HOOKED, &CHookDlg::OnUpdateHookOnlyShowHooked)
	ON_COMMAND(ID_HOOK_FRESHEN, &CHookDlg::OnHookFreshen)
	ON_COMMAND(ID_HOOK_RESUME, &CHookDlg::OnHookResume)
	ON_COMMAND(ID_HOOK_RESUME_ALL, &CHookDlg::OnHookResumeAll)
	ON_COMMAND(ID_HOOK_PROPERTY, &CHookDlg::OnHookProperty)
	ON_COMMAND(ID_HOOK_LOCATION, &CHookDlg::OnHookLocation)
	ON_COMMAND(ID_HOOK_EXPORT_INFORMATION, &CHookDlg::OnHookExportInformation)
	
	
END_MESSAGE_MAP()


// CHookDlg 消息处理程序


BOOL CHookDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	APInitializeHookItemList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CHookDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()

	CRect   Rect;
	GetClientRect(Rect);
	dc.FillSolidRect(Rect, RGB(255, 255, 255));

	CRect HookListBoxRect;
	CRect HookModuleListBoxRect;
	CRect HookListCtrlRect;

	m_HookListBox.GetWindowRect(&HookListBoxRect);
	ClientToScreen(&Rect);

	HookListBoxRect.left -= Rect.left;
	HookListBoxRect.right -= Rect.left;
	HookListBoxRect.top -= Rect.top;
	HookListBoxRect.bottom = Rect.Height() - 2;

	m_HookListBox.MoveWindow(HookListBoxRect);

	CPoint StartPoint;
	StartPoint.x = (LONG)(HookListBoxRect.right) + 2;
	StartPoint.y = -1;

	CPoint EndPoint;
	EndPoint.x = (LONG)(HookListBoxRect.right) + 2;
	EndPoint.y = Rect.Height() + 2;

	HookListCtrlRect.left = StartPoint.x + 1;
	HookListCtrlRect.right = Rect.Width();
	HookListCtrlRect.top = 0;
	HookListCtrlRect.bottom = Rect.Height();
	m_HookListCtrl.MoveWindow(HookListCtrlRect);

	COLORREF Color(RGB(190, 190, 190));

	CClientDC aDC(this);			//CClientDC的构造函数需要一个参数，这个参数是指向绘图窗口的指针，我们用this指针就可以了
	CPen pen(PS_SOLID, 1, Color);	//建立一个画笔类对象，构造时设置画笔属性
	aDC.SelectObject(&pen);
	aDC.MoveTo(StartPoint);
	aDC.LineTo(EndPoint);

	if (m_HookModuleListBox.IsWindowVisible())
	{
		HookListBoxRect.bottom = Rect.Height() / 2;

		StartPoint.x = -1;
		StartPoint.y = (LONG)(HookListBoxRect.bottom) + 2;
		EndPoint.x = (LONG)(HookListBoxRect.right) + 2;
		EndPoint.y = (LONG)(HookListBoxRect.bottom) + 2;

		m_HookModuleListBox.MoveWindow(
			HookListBoxRect.left,
			HookListBoxRect.bottom + 5,
			HookListBoxRect.Width(),
			Rect.Height() - HookListBoxRect.bottom - 7
		);

		CPen pen2(PS_SOLID, 1, Color);	//建立一个画笔类对象，构造时设置画笔属性
		aDC.SelectObject(&pen2);
		aDC.MoveTo(StartPoint);
		aDC.LineTo(EndPoint);
	}

	m_HookListBox.MoveWindow(&HookListBoxRect);
}


void CHookDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_CurrentChildDlg = ArkProtect::cd_HookDialog;
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_HookButton.EnableWindow(FALSE);

		m_HookListBox.SetCurSel(ArkProtect::hi_Ssdt);

		OnLbnSelchangeHookListbox();

		m_HookListCtrl.SetFocus();

	}
	else
	{
		m_iCurSel = 65535;
	}
}


void CHookDlg::OnLbnSelchangeHookListbox()
{
	// TODO: 在此添加控件通知处理程序代码

	int iCurSel = m_HookListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::hi_Ssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::hi_Ssdt)
		{
			m_HookListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		m_HookModuleListBox.ShowWindow(FALSE);

		// 初始化ListCtrl
		m_Global->SsdtHook().InitializeSsdtList(&m_HookListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSsdtHook::QuerySsdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::hi_Sssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::hi_Sssdt)
		{
			m_HookListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		m_HookModuleListBox.ShowWindow(FALSE);

		// 初始化ListCtrl
		m_Global->SssdtHook().InitializeSssdtList(&m_HookListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSssdtHook::QuerySssdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}

	default:
		break;
	}

	m_HookListCtrl.SetFocus();

}


void CHookDlg::OnNMCustomdrawHookListctrl(NMHDR *pNMHDR, LRESULT *pResult)
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
		BOOL bHooked = 0;
		int iItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

		clrNewTextColor = RGB(0, 0, 0);
		clrNewBkColor = RGB(255, 255, 255);

		bHooked = (BOOL)m_HookListCtrl.GetItemData(iItem);
		if (bHooked == TRUE)
		{
			clrNewTextColor = RGB(255, 0, 0);
		}

		pLVCD->clrText = clrNewTextColor;
		pLVCD->clrTextBk = clrNewBkColor;

		*pResult = CDRF_DODEFAULT;
	}
}


void CHookDlg::OnNMRClickHookListctrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	CMenu Menu;
	Menu.LoadMenuW(IDR_HOOK_MENU);
	CMenu* SubMenu = Menu.GetSubMenu(0);	// 子菜单

	CPoint Pt;
	GetCursorPos(&Pt);         // 得到鼠标位置

	int	iCount = SubMenu->GetMenuItemCount();

	// 如果没有选中,除了刷新 其他全部Disable
	if (m_HookListCtrl.GetSelectedCount() == 0)
	{
		for (int i = 0; i < iCount; i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED); //菜单全部变灰
		}

		SubMenu->EnableMenuItem(ID_HOOK_FRESHEN, MF_BYCOMMAND | MF_ENABLED);
		SubMenu->EnableMenuItem(ID_HOOK_ONLY_SHOW_HOOKED, MF_BYCOMMAND | MF_ENABLED);
		SubMenu->EnableMenuItem(ID_HOOK_EXPORT_INFORMATION, MF_BYCOMMAND | MF_ENABLED);
	}

	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Pt.x, Pt.y, this);

	*pResult = 0;
}


void CHookDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
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
			if (nID  == -1 || nID ==  0)
			{
				continue;
			}
			CmdUI.m_nID = nID;
			CmdUI.m_nIndex = i;
			CmdUI.DoUpdate(this, FALSE);
		}
	}
}


void CHookDlg::OnHookOnlyShowHooked()
{
	// TODO: 在此添加命令处理程序代码

	m_bOnlyShowHooked = m_bOnlyShowHooked ? FALSE : TRUE;

	// 重新加载一遍
	OnHookFreshen();
}


void CHookDlg::OnUpdateHookOnlyShowHooked(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	// 用于控制是否仅显示被挂钩项
	pCmdUI->SetCheck(m_bOnlyShowHooked);
}


void CHookDlg::OnHookFreshen()
{
	// TODO: 在此添加命令处理程序代码

	m_iCurSel = 65535;

	int iCurSel = m_HookListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::hi_Ssdt:
	{
		m_HookListBox.SetCurSel(ArkProtect::hi_Ssdt);
		break;
	}
	case ArkProtect::hi_Sssdt:
	{
		m_HookListBox.SetCurSel(ArkProtect::hi_Sssdt);
		break;
	}

	default:
		break;
	}

	OnLbnSelchangeHookListbox();

	m_HookListCtrl.SetFocus();
}


void CHookDlg::OnHookResume()
{
	// TODO: 在此添加命令处理程序代码

	int iCurSel = m_HookListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::hi_Ssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE)
		{
			return;
		}

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSsdtHook::ResumeSsdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::hi_Sssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE)
		{
			return;
		}

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSssdtHook::ResumeSssdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}
	default:
		break;
	}

}


void CHookDlg::OnHookResumeAll()
{
	// TODO: 在此添加命令处理程序代码

	int iCurSel = m_HookListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::hi_Ssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE)
		{
			return;
		}

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSsdtHook::ResumeAllSsdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::hi_Sssdt:
	{
		if (m_Global->m_bIsRequestNow == TRUE)
		{
			return;
		}

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSssdtHook::ResumeAllSssdtHookCallback, &m_HookListCtrl, 0, NULL)
		);

		break;
	}
	default:
		break;
	}

}


void CHookDlg::OnHookProperty()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_HookListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_HookListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = APGetSelectedFilePath(iItem);

		m_Global->CheckFileProperty(strFilePath);
	}
}


void CHookDlg::OnHookLocation()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_HookListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_HookListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = APGetSelectedFilePath(iItem);

		m_Global->LocationInExplorer(strFilePath);
	}
}


void CHookDlg::OnHookExportInformation()
{
	// TODO: 在此添加命令处理程序代码
	m_Global->ExportInformationInText(m_HookListCtrl);
}


/************************************************************************
*  Name : APInitializeHookItemList
*  Param: void
*  Ret  : void
*  初始化内核钩子的ListBox（内核项）
************************************************************************/
void CHookDlg::APInitializeHookItemList()
{
	m_HookListBox.AddString(L"SSDT");
	m_HookListBox.InsertString(ArkProtect::hi_Sssdt, L"ShadowSSDT");
	m_HookListBox.InsertString(ArkProtect::hi_NtkrnlFunc, L"内核函数");
	m_HookListBox.InsertString(ArkProtect::hi_NtkrnlIAT, L"内核导入表");
	m_HookListBox.InsertString(ArkProtect::hi_NtkrnlEAT, L"内核导出表");

	m_HookListBox.SetItemHeight(-1, (UINT)(16 * (m_Global->iDpiy / 96.0)));
	m_HookModuleListBox.SetItemHeight(-1, (UINT)(16 * (m_Global->iDpiy / 96.0)));
}


/************************************************************************
*  Name : APGetSelectedFilePath
*  Param: iItem
*  Ret  : CString
*  返回选中的文件路径
************************************************************************/
CString CHookDlg::APGetSelectedFilePath(int iItem)
{
	int iCurSel = m_HookListBox.GetCurSel();

	CString strFilePath;

	switch (iCurSel)
	{
	case ArkProtect::hi_Ssdt:
	{
		strFilePath = m_HookListCtrl.GetItemText(iItem, ArkProtect::shc_FilePath);
		break;
	}
	case ArkProtect::hi_Sssdt:
	{
		strFilePath = m_HookListCtrl.GetItemText(iItem, ArkProtect::shc_FilePath);
		break;
	}
	
	default:
		break;
	}

	return strFilePath;
}









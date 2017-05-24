// KernelDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "KernelDlg.h"
#include "afxdialogex.h"

#include "ArkProtectAppDlg.h"

// CKernelDlg 对话框

IMPLEMENT_DYNAMIC(CKernelDlg, CDialogEx)

CKernelDlg::CKernelDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_KERNEL_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	// 保存对话框指针
	m_Global->m_KernelDlg = this;
}

CKernelDlg::~CKernelDlg()
{
}

void CKernelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_KERNEL_LISTBOX, m_KernelListBox);
	DDX_Control(pDX, IDC_KERNEL_LISTCTRL, m_KernelListCtrl);
}


BEGIN_MESSAGE_MAP(CKernelDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_LBN_SELCHANGE(IDC_KERNEL_LISTBOX, &CKernelDlg::OnLbnSelchangeKernelListbox)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_KERNEL_LISTCTRL, &CKernelDlg::OnNMCustomdrawKernelListctrl)
	ON_NOTIFY(NM_RCLICK, IDC_KERNEL_LISTCTRL, &CKernelDlg::OnNMRClickKernelListctrl)
	ON_COMMAND(ID_KERNEL_FRESHEN, &CKernelDlg::OnKernelFreshen)
	ON_COMMAND(ID_KERNEL_DELETE, &CKernelDlg::OnKernelDelete)
	ON_COMMAND(ID_KERNEL_PROPERTY, &CKernelDlg::OnKernelProperty)
	ON_COMMAND(ID_KERNEL_LOCATION, &CKernelDlg::OnKernelLocation)
	ON_COMMAND(ID_KERNEL_EXPORT_INFORMATION, &CKernelDlg::OnKernelExportInformation)
	
END_MESSAGE_MAP()


// CKernelDlg 消息处理程序


BOOL CKernelDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	APInitializeKernelItemList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CKernelDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()

	CRect   Rect;
	GetClientRect(Rect);
	dc.FillSolidRect(Rect, RGB(255, 255, 255));  // 填充颜色

	CRect KernelListBoxRect;
	CRect KernelListCtrlRect;

	// 获得ListBox的Rect
	m_KernelListBox.GetWindowRect(&KernelListBoxRect);
	ClientToScreen(&Rect);
	KernelListBoxRect.left -= Rect.left;
	KernelListBoxRect.right -= Rect.left;
	KernelListBoxRect.top -= Rect.top;
	KernelListBoxRect.bottom = Rect.Height() - 2;

	m_KernelListBox.MoveWindow(KernelListBoxRect);

	CPoint StartPoint;
	StartPoint.x = (LONG)(KernelListBoxRect.right) + 2;
	StartPoint.y = -1;

	CPoint EndPoint;
	EndPoint.x = (LONG)(KernelListBoxRect.right) + 2;
	EndPoint.y = Rect.Height() + 2;

	KernelListCtrlRect.left = StartPoint.x + 1;
	KernelListCtrlRect.right = Rect.Width();
	KernelListCtrlRect.top = 0;
	KernelListCtrlRect.bottom = Rect.Height();
	m_KernelListCtrl.MoveWindow(KernelListCtrlRect);

	COLORREF ColorRef(RGB(190, 190, 190));

	CClientDC ClientDc(this);
	CPen Pen(PS_SOLID, 1, ColorRef);
	ClientDc.SelectObject(&Pen);
	ClientDc.MoveTo(StartPoint);
	ClientDc.LineTo(EndPoint);

}



void CKernelDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_CurrentChildDlg = ArkProtect::cd_KernelDialog;
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_KernelButton.EnableWindow(FALSE);

		m_KernelListBox.SetCurSel(ArkProtect::ki_SysCallback);

		OnLbnSelchangeKernelListbox();

		m_KernelListCtrl.SetFocus();

	}
	else
	{
		m_iCurSel = 65535;
	}
}


void CKernelDlg::OnNMCustomdrawKernelListctrl(NMHDR *pNMHDR, LRESULT *pResult)
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

		bNotTrust = (BOOL)m_KernelListCtrl.GetItemData(iItem);
		if (bNotTrust == TRUE)
		{
			clrNewTextColor = RGB(0, 0, 255);
		}

		pLVCD->clrText = clrNewTextColor;
		pLVCD->clrTextBk = clrNewBkColor;

		*pResult = CDRF_DODEFAULT;
	}
}


void CKernelDlg::OnLbnSelchangeKernelListbox()
{
	// TODO: 在此添加控件通知处理程序代码

	int iCurSel = m_KernelListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::ki_SysCallback:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::ki_SysCallback)
		{
			m_KernelListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		// 初始化ListCtrl
		m_Global->SystemCallback().InitializeCallbackList(&m_KernelListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CSystemCallback::QuerySystemCallbackCallback, &m_KernelListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::ki_FilterDriver:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::ki_FilterDriver)
		{
			m_KernelListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		// 初始化ListCtrl
		m_Global->FilterDriver().InitializeFilterDriverList(&m_KernelListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CFilterDriver::QueryFilterDriverCallback, &m_KernelListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::ki_IoTimer:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::ki_IoTimer)
		{
			m_KernelListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		// 初始化ListCtrl
		m_Global->IoTimer().InitializeIoTimerList(&m_KernelListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CIoTimer::QueryIoTimerCallback, &m_KernelListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::ki_DpcTimer:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::ki_DpcTimer)
		{
			m_KernelListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		// 初始化ListCtrl
		m_Global->DpcTimer().InitializeDpcTimerList(&m_KernelListCtrl);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CDpcTimer::QueryDpcTimerCallback, &m_KernelListCtrl, 0, NULL)
		);

		break;
	}
	case ArkProtect::ki_SysThread:
	{
		if (m_Global->m_bIsRequestNow == TRUE || m_iCurSel == ArkProtect::ki_SysThread)
		{
			m_KernelListBox.SetCurSel(m_iCurSel);
			break;
		}

		m_iCurSel = iCurSel;

		// 初始化ListCtrl
		m_Global->ProcessThread().InitializeProcessThreadList(&m_KernelListCtrl);

		m_Global->ProcessCore().ProcessEntry()->ProcessId = 4;

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CProcessThread::QueryProcessThreadCallback, &m_KernelListCtrl, 0, NULL)
		);

		break;
	}

	default:
		break;
	}

	m_KernelListCtrl.SetFocus();
}


void CKernelDlg::OnNMRClickKernelListctrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	CMenu Menu;
	Menu.LoadMenuW(IDR_KERNEL_MENU);
	CMenu* SubMenu = Menu.GetSubMenu(0);	// 子菜单

	CPoint Pt;
	GetCursorPos(&Pt);         // 得到鼠标位置

	int	iCount = SubMenu->GetMenuItemCount();

	// 如果没有选中,除了刷新 其他全部Disable
	if (m_KernelListCtrl.GetSelectedCount() == 0)
	{
		for (int i = 0; i < iCount; i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED); //菜单全部变灰
		}

		SubMenu->EnableMenuItem(ID_KERNEL_FRESHEN, MF_BYCOMMAND | MF_ENABLED);
	}

	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Pt.x, Pt.y, this);

	*pResult = 0;
}


void CKernelDlg::OnKernelFreshen()
{
	// TODO: 在此添加命令处理程序代码
	m_iCurSel = 65535;

	int iCurSel = m_KernelListBox.GetCurSel();

	switch (iCurSel)
	{
	case ArkProtect::ki_SysCallback:
	{
		m_KernelListBox.SetCurSel(ArkProtect::ki_SysCallback);
		break;
	}
	case ArkProtect::ki_FilterDriver:
	{
		m_KernelListBox.SetCurSel(ArkProtect::ki_FilterDriver);
		break;
	}
	case ArkProtect::ki_IoTimer:
	{
		m_KernelListBox.SetCurSel(ArkProtect::ki_IoTimer);
		break;
	}
	case ArkProtect::ki_DpcTimer:
	{
		m_KernelListBox.SetCurSel(ArkProtect::ki_DpcTimer);
		break;
	}
	case ArkProtect::ki_SysThread:
	{
		m_KernelListBox.SetCurSel(ArkProtect::ki_SysThread);
		break;
	}

	default:
		break;
	}

	OnLbnSelchangeKernelListbox();

	m_KernelListCtrl.SetFocus();
}


void CKernelDlg::OnKernelDelete()
{
	// TODO: 在此添加命令处理程序代码
}


void CKernelDlg::OnKernelProperty()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_KernelListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_KernelListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = APGetSelectedFilePath(iItem);
		
		m_Global->CheckFileProperty(strFilePath);
	}
}


void CKernelDlg::OnKernelLocation()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_KernelListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_KernelListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = APGetSelectedFilePath(iItem);

		m_Global->LocationInExplorer(strFilePath);
	}
}


void CKernelDlg::OnKernelExportInformation()
{
	// TODO: 在此添加命令处理程序代码
	m_Global->ExportInformationInText(m_KernelListCtrl);
}


/************************************************************************
*  Name : APInitializeKernelItemList
*  Param: void
*  Ret  : void
*  初始化内核模块的ListBox（内核项）
************************************************************************/
void CKernelDlg::APInitializeKernelItemList()
{
	m_KernelListBox.AddString(L"系统回调");
	m_KernelListBox.InsertString(ArkProtect::ki_FilterDriver, L"过滤驱动");
	m_KernelListBox.InsertString(ArkProtect::ki_IoTimer, L"IOTimer");
	m_KernelListBox.InsertString(ArkProtect::ki_DpcTimer, L"DPCTimer");
	m_KernelListBox.InsertString(ArkProtect::ki_SysThread, L"系统线程");

	m_KernelListBox.SetItemHeight(-1, (UINT)(16 * (m_Global->iDpiy / 96.0)));
}


/************************************************************************
*  Name : APGetSelectedFilePath
*  Param: iItem
*  Ret  : CString
*  返回选中的文件路径
************************************************************************/
CString CKernelDlg::APGetSelectedFilePath(int iItem)
{
	int iCurSel = m_KernelListBox.GetCurSel();

	CString strFilePath;

	switch (iCurSel)
	{
	case ArkProtect::ki_SysCallback:
	{
		strFilePath = m_KernelListCtrl.GetItemText(iItem, ArkProtect::scc_FilePath);
		break;
	}
	case ArkProtect::ki_FilterDriver:
	{
		strFilePath = m_KernelListCtrl.GetItemText(iItem, ArkProtect::fdc_FilePath);
		break;
	}
	case ArkProtect::ki_IoTimer:
	{
		strFilePath = m_KernelListCtrl.GetItemText(iItem, ArkProtect::itc_FilePath);
		break;
	}
	case ArkProtect::ki_DpcTimer:
	{
		strFilePath = m_KernelListCtrl.GetItemText(iItem, ArkProtect::dtc_FilePath);
		break;
	}
	case ArkProtect::ki_SysThread:
	{
		//	strFilePath = m_KernelListCtrl.GetItemText(iItem, ArkProtect::stc_FilePath);
		break;
	}
	default:
		break;
	}

	return strFilePath;
}













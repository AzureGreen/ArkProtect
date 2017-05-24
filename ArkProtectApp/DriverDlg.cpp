// DriverDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "DriverDlg.h"
#include "afxdialogex.h"

#include "ArkProtectAppDlg.h"

// CDriverDlg 对话框

IMPLEMENT_DYNAMIC(CDriverDlg, CDialogEx)

CDriverDlg::CDriverDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_DRIVER_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	// 保存对话框指针
	m_Global->m_DriverDlg = this;
}

CDriverDlg::~CDriverDlg()
{
}

void CDriverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DRIVER_LIST, m_DriverListCtrl);
}


BEGIN_MESSAGE_MAP(CDriverDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_DRIVER_LIST, &CDriverDlg::OnNMCustomdrawDriverList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DRIVER_LIST, &CDriverDlg::OnLvnItemchangedDriverList)
	ON_NOTIFY(NM_RCLICK, IDC_DRIVER_LIST, &CDriverDlg::OnNMRClickDriverList)
	ON_COMMAND(ID_DRIVER_FRESHEN, &CDriverDlg::OnDriverFreshen)
	ON_COMMAND(ID_DRIVER_DELETE, &CDriverDlg::OnDriverDelete)
	ON_COMMAND(ID_DRIVER_UNLOAD, &CDriverDlg::OnDriverUnload)
	ON_COMMAND(ID_DRIVER_PROPERTY, &CDriverDlg::OnDriverProperty)
	ON_COMMAND(ID_DRIVER_LOCATION, &CDriverDlg::OnDriverLocation)
	ON_COMMAND(ID_DRIVER_EXPORT_INFORMATION, &CDriverDlg::OnDriverExportInformation)
END_MESSAGE_MAP()


// CDriverDlg 消息处理程序


BOOL CDriverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
		
	// 初始化进程列表
	APInitializeDriverList();

	// 创建Icon图标列表
	UINT nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_DriverIconList.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	ListView_SetImageList(m_DriverListCtrl.m_hWnd, m_DriverIconList.GetSafeHandle(), LVSIL_SMALL);


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CDriverDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	m_Global->iResizeX = cx;
	m_Global->iResizeY = cy;
}


void CDriverDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		m_DriverListCtrl.MoveWindow(0, 0, m_Global->iResizeX, m_Global->iResizeY);

		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_CurrentChildDlg = ArkProtect::cd_DriverDialog;
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_DriverButton.EnableWindow(FALSE);

		// 加载进程信息列表
		APLoadDriverList();

		m_DriverListCtrl.SetFocus();
	}
}


void CDriverDlg::OnNMCustomdrawDriverList(NMHDR *pNMHDR, LRESULT *pResult)
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

		bNotTrust = (BOOL)m_DriverListCtrl.GetItemData(iItem);
		if (bNotTrust == TRUE)
		{
			clrNewTextColor = RGB(0, 0, 255);
		}

		pLVCD->clrText = clrNewTextColor;
		pLVCD->clrTextBk = clrNewBkColor;

		*pResult = CDRF_DODEFAULT;
	}
}


void CDriverDlg::OnLvnItemchangedDriverList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码



	*pResult = 0;
}


void CDriverDlg::OnNMRClickDriverList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	CMenu Menu;
	Menu.LoadMenuW(IDR_DRIVER_MENU);
	CMenu* SubMenu = Menu.GetSubMenu(0);	// 子菜单

	CPoint Pt;
	GetCursorPos(&Pt);         // 得到鼠标位置

	int	iCount = SubMenu->GetMenuItemCount();

	// 如果没有选中,除了刷新 其他全部Disable
	if (m_DriverListCtrl.GetSelectedCount() == 0)
	{
		for (int i = 0; i < iCount; i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED); //菜单全部变灰
		}

		SubMenu->EnableMenuItem(ID_DRIVER_FRESHEN, MF_BYCOMMAND | MF_ENABLED);
	}

	int iIndex = m_DriverListCtrl.GetSelectionMark();
	if (iIndex >= 0)
	{
		if (_wcsicmp(L"-", m_DriverListCtrl.GetItemText(iIndex, ArkProtect::dc_Object)) == 0)
		{
			SubMenu->EnableMenuItem(ID_DRIVER_UNLOAD, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);  // 没有Object, Unload也要变灰
		}
	}

	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Pt.x, Pt.y, this);

	*pResult = 0;
}



void CDriverDlg::OnDriverFreshen()
{
	// TODO: 在此添加命令处理程序代码
	// 加载驱动模块信息列表
	APLoadDriverList();

}


void CDriverDlg::OnDriverDelete()
{
	// TODO: 在此添加命令处理程序代码
	// 删除文件
	POSITION Pos = m_DriverListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_DriverListCtrl.GetNextSelectedItem(Pos);

		m_strFilePath = m_DriverListCtrl.GetItemText(iItem, ArkProtect::dc_DriverPath);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CFileCore::DeleteFileCallback, &m_strFilePath, 0, NULL)
		);
	}
}


void CDriverDlg::OnDriverUnload()
{
	// TODO: 在此添加命令处理程序代码

	if (MessageBox(L"卸载驱动模块很危险，很有可能导致蓝屏\r\n您真的还要卸载吗？", L"ArkProtect", MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL)
	{
		return;
	}

	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CDriverCore::UnloadDriverCallback, &m_DriverListCtrl, 0, NULL)
	);

}



void CDriverDlg::OnDriverProperty()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_DriverListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_DriverListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_DriverListCtrl.GetItemText(iItem, ArkProtect::dc_DriverPath);

		m_Global->CheckFileProperty(strFilePath);
	}
}


void CDriverDlg::OnDriverLocation()
{
	// TODO: 在此添加命令处理程序代码
	POSITION Pos = m_DriverListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_DriverListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_DriverListCtrl.GetItemText(iItem, ArkProtect::dc_DriverPath);

		m_Global->LocationInExplorer(strFilePath);
	}
}


void CDriverDlg::OnDriverExportInformation()
{
	// TODO: 在此添加命令处理程序代码
	m_Global->ExportInformationInText(m_DriverListCtrl);
}


/************************************************************************
*  Name : APInitializeDriverList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CDriverDlg::APInitializeDriverList()
{
	m_Global->DriverCore().InitializeDriverList(&m_DriverListCtrl);
}



/************************************************************************
*  Name : APLoadDriverList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CDriverDlg::APLoadDriverList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	while (m_DriverIconList.Remove(0));

	m_DriverListCtrl.DeleteAllItems();

	m_DriverListCtrl.SetSelectedColumn(-1);

	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CDriverCore::QueryDriverInfoCallback, &m_DriverListCtrl, 0, NULL)
	);
}











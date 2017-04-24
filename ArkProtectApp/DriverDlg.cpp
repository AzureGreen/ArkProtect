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
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_DriverrButton.EnableWindow(FALSE);

		// 加载进程信息列表
		APLoadDriverList();

		m_DriverListCtrl.SetFocus();
	}
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

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CDriverCore::QueryDriverInfoCallback, &m_DriverListCtrl, 0, NULL)
	);
}
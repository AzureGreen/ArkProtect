// ProcessDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ProcessDlg.h"
#include "afxdialogex.h"

#include "ArkProtectAppDlg.h"

// CProcessDlg 对话框

IMPLEMENT_DYNAMIC(CProcessDlg, CDialogEx)

CProcessDlg::CProcessDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_PROCESS_DIALOG, pParent)
	, m_Global(GlobalObject)
	, m_Process(GlobalObject)
{

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
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CProcessDlg 消息处理程序


BOOL CProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	// 保存对话框指针
	m_Global->ProcessDlg = this;

	// 创建Icon图标列表
	UINT nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_ProcessIconList.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	ListView_SetImageList(m_ProcessListCtrl.m_hWnd, m_ProcessIconList.GetSafeHandle(), LVSIL_SMALL);

	// 初始化进程列表
	APInitializeProcessList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CProcessDlg::APInitializeProcessList()
{
	m_Process.InitializeProcessList(&m_ProcessListCtrl);
}



void CProcessDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		m_ProcessListCtrl.MoveWindow(0, 0, m_Global->iResizeX, m_Global->iResizeY);

		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_CurrentChildDlg = ArkProtect::cd_ProcessDialog;
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_ProcessButton.EnableWindow(FALSE);

		// 
		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::QueryProcessInfoCallback, &m_ProcessListCtrl, 0, NULL)
		);

		m_ProcessListCtrl.SetFocus();
	}
}

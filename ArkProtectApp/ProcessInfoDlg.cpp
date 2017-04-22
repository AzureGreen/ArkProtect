// ProcessInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ProcessInfoDlg.h"
#include "afxdialogex.h"


// CProcessInfoDlg 对话框

IMPLEMENT_DYNAMIC(CProcessInfoDlg, CDialogEx)

CProcessInfoDlg::CProcessInfoDlg(CWnd* pParent, ArkProtect::eProcessInfoKind ProcessInfoKind,
	ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_PROCESS_INFO_DIALOG, pParent)
	, m_WantedInfoKind(ProcessInfoKind)
	, m_Global(GlobalObject)
{
}

CProcessInfoDlg::~CProcessInfoDlg()
{
}

void CProcessInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_INFO_LIST, m_ProcessInfoListCtrl);
}


BEGIN_MESSAGE_MAP(CProcessInfoDlg, CDialogEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CProcessInfoDlg 消息处理程序


BOOL CProcessInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	// 设置对话框的图标
	SHFILEINFO shFileInfo = { 0 };
	SHGetFileInfo(m_Global->ProcessCore().ProcessEntry()->wzFilePath, FILE_ATTRIBUTE_NORMAL,
		&shFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);

	m_hIcon = shFileInfo.hIcon;

	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化ListControl
	APInitializeProcessInfoList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CProcessInfoDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
}




/************************************************************************
*  Name : APInitializeProcessList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessInfoList()
{
	// 判断当前需要的对话框
	if (m_WantedInfoKind == m_CurrentInfoKind)
	{
		return;
	}

	CString strWindowText = L"";

	switch (m_WantedInfoKind)
	{
	case ArkProtect::pik_Module:
		
		m_CurrentInfoKind = m_WantedInfoKind;

		strWindowText.Format(L"Process Module - %s", m_Global->ProcessCore().ProcessEntry()->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessModuleList();

		APLoadProcessModuleList();

		break;
	case ArkProtect::pik_Thread:

		m_CurrentInfoKind = m_WantedInfoKind;

		strWindowText.Format(L"Process Thread - %s", m_Global->ProcessCore().ProcessEntry()->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessThreadList();

		APLoadProcessThreadList();

		break;
	case ArkProtect::pik_Handle:

		m_CurrentInfoKind = m_WantedInfoKind;

		strWindowText.Format(L"Process Handle - %s", m_Global->ProcessCore().ProcessEntry()->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessHandleList();

		APLoadProcessHandleList();

		break;
	case ArkProtect::pik_Window:

		m_CurrentInfoKind = m_WantedInfoKind;

		strWindowText.Format(L"Process Window - %s", m_Global->ProcessCore().ProcessEntry()->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessWindowList();

		APLoadProcessWindowList();

		break;
	case ArkProtect::pik_Memory:

		m_CurrentInfoKind = m_WantedInfoKind;

		strWindowText.Format(L"Process Memory - %s", m_Global->ProcessCore().ProcessEntry()->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessMemoryList();

		APLoadProcessMemoryList();

		break;
	default:
		break;
	}



}


/************************************************************************
*  Name : APInitializeProcessList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessModuleList()
{
	m_Global->ProcessModule().InitializeProcessModuleList(&m_ProcessInfoListCtrl);
}


/************************************************************************
*  Name : APLoadProcessModuleList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessInfoDlg::APLoadProcessModuleList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	m_ProcessInfoListCtrl.DeleteAllItems();

	m_ProcessInfoListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessModule::QueryProcessModuleCallback, &m_ProcessInfoListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APInitializeProcessThreadList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessThreadList()
{
	m_Global->ProcessThread().InitializeProcessThreadList(&m_ProcessInfoListCtrl);
}


/************************************************************************
*  Name : APLoadProcessThreadList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessInfoDlg::APLoadProcessThreadList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	m_ProcessInfoListCtrl.DeleteAllItems();

	m_ProcessInfoListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessThread::QueryProcessThreadCallback, &m_ProcessInfoListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APInitializeProcessHandleList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessHandleList()
{
	m_Global->ProcessHandle().InitializeProcessHandleList(&m_ProcessInfoListCtrl);
}


/************************************************************************
*  Name : APLoadProcessThreadList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessInfoDlg::APLoadProcessHandleList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	m_ProcessInfoListCtrl.DeleteAllItems();

	m_ProcessInfoListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessHandle::QueryProcessHandleCallback, &m_ProcessInfoListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APInitializeProcessWindowList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessWindowList()
{
	m_Global->ProcessWindow().InitializeProcessWindowList(&m_ProcessInfoListCtrl);
}


/************************************************************************
*  Name : APLoadProcessWindowList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessInfoDlg::APLoadProcessWindowList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	m_ProcessInfoListCtrl.DeleteAllItems();

	m_ProcessInfoListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessWindow::QueryProcessWindowCallback, &m_ProcessInfoListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APInitializeProcessMemoryList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CProcessInfoDlg::APInitializeProcessMemoryList()
{
	m_Global->ProcessMemory().InitializeProcessMemoryList(&m_ProcessInfoListCtrl);
}


/************************************************************************
*  Name : APLoadProcessMemoryList
*  Param: void
*  Ret  : void
*  加载进程信息到ListControl
************************************************************************/
void CProcessInfoDlg::APLoadProcessMemoryList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	m_ProcessInfoListCtrl.DeleteAllItems();

	m_ProcessInfoListCtrl.SetSelectedColumn(-1);

	// 加载进程信息列表
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessMemory::QueryProcessMemoryCallback, &m_ProcessInfoListCtrl, 0, NULL)
	);
}
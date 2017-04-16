// ProcessInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ProcessInfoDlg.h"
#include "afxdialogex.h"


// CProcessInfoDlg 对话框

IMPLEMENT_DYNAMIC(CProcessInfoDlg, CDialogEx)

CProcessInfoDlg::CProcessInfoDlg(CWnd* pParent, ArkProtect::eProcessInfoKind ProcessInfoKind,
	ArkProtect::CGlobal *GlobalObject, ArkProtect::PPROCESS_ENTRY_INFORMATION ProcessEntry)
	: CDialogEx(IDD_PROCESS_INFO_DIALOG, pParent)
	, m_WantedInfoKind(ProcessInfoKind)
	, m_Global(GlobalObject)
	, m_ProcessModule(GlobalObject, ProcessEntry)
	, m_ProcessEntry(ProcessEntry)
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
	SHGetFileInfo(m_ProcessEntry->wzFilePath, FILE_ATTRIBUTE_NORMAL,
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

		strWindowText.Format(L"Process Module - %s", m_ProcessEntry->wzImageName);

		SetWindowText(strWindowText.GetBuffer());

		APInitializeProcessModuleList();

		APLoadProcessModuleList();

		break;
	case ArkProtect::pik_Thread:
		break;
	case ArkProtect::pik_Handle:
		break;
	case ArkProtect::pik_Window:
		break;
	case ArkProtect::pik_Memory:
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
	m_ProcessModule.InitializeProcessModuleList(&m_ProcessInfoListCtrl);
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
// RegistryDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "RegistryDlg.h"
#include "afxdialogex.h"

#include "ArkProtectAppDlg.h"

// CRegistryDlg 对话框

IMPLEMENT_DYNAMIC(CRegistryDlg, CDialogEx)

CRegistryDlg::CRegistryDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_REGISTRY_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	m_Global->m_RegistryDlg = this;
}

CRegistryDlg::~CRegistryDlg()
{
}

void CRegistryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REGISTRY_LIST, m_RegistryListCtrl);
	DDX_Control(pDX, IDC_REGISTRY_TREE, m_RegistryTreeCtrl);
}


BEGIN_MESSAGE_MAP(CRegistryDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	
END_MESSAGE_MAP()


// CRegistryDlg 消息处理程序


BOOL CRegistryDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	// 初始化注册表列表

	// 1.初始化 ControlTree
	APInitializeRegistryTree();
	// 2.初始化 ListControl
	APInitializeRegistryList();


	//导入Icon资源
	HICON hIcon[3];
	hIcon[0] = AfxGetApp()->LoadIcon(IDI_COMPUTER_ICON);
	hIcon[1] = AfxGetApp()->LoadIcon(IDI_CLOSE_DIR_ICON);
	hIcon[2] = AfxGetApp()->LoadIcon(IDI_OPEN_DIR_ICON);

	UINT nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_RegistryIconTree.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	for (int i = 0; i < 3; i++)
	{
		m_RegistryIconTree.Add(hIcon[i]);
		m_RegistryTreeCtrl.SetImageList(&m_RegistryIconTree, LVSIL_NORMAL);
	//ListView_SetImageList(m_RegistryTreeCtrl.m_hWnd, m_RegistryIconTree.GetSafeHandle(), LVSIL_NORMAL);
	}

	hIcon[0] = AfxGetApp()->LoadIcon(IDI_REG_SZ_ICON);
	hIcon[1] = AfxGetApp()->LoadIcon(IDI_REG_DWORD_ICON);
	nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_RegistryIconList.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	for (int i = 0; i < 2; i++)
	{
		m_RegistryIconList.Add(hIcon[i]);
		m_RegistryListCtrl.SetImageList(&m_RegistryIconList, LVSIL_SMALL);
	//	ListView_SetImageList(m_RegistryListCtrl.m_hWnd, m_RegistryIconList.GetSafeHandle(), LVSIL_SMALL);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}



void CRegistryDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	m_Global->iResizeX = cx;
	m_Global->iResizeY = cy;
}



void CRegistryDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()

	CRect   Rect;
	GetClientRect(Rect);
	dc.FillSolidRect(Rect, RGB(255, 255, 255));  // 填充颜色

	CRect RegistryListTreeRect;
	CRect RegistryListCtrlRect;

	// 获得ListBox的Rect
	m_RegistryTreeCtrl.GetWindowRect(&RegistryListTreeRect);
	ClientToScreen(&Rect);
	RegistryListTreeRect.left -= Rect.left;
	RegistryListTreeRect.right -= Rect.left;
	RegistryListTreeRect.top -= Rect.top;
	RegistryListTreeRect.bottom = Rect.Height() - 2;

	m_RegistryTreeCtrl.MoveWindow(RegistryListTreeRect);

	CPoint StartPoint;
	StartPoint.x = (LONG)(RegistryListTreeRect.right) + 2;
	StartPoint.y = -1;

	CPoint EndPoint;
	EndPoint.x = (LONG)(RegistryListTreeRect.right) + 2;
	EndPoint.y = Rect.Height() + 2;

	RegistryListCtrlRect.left = StartPoint.x + 1;
	RegistryListCtrlRect.right = Rect.Width();
	RegistryListCtrlRect.top = 0;
	RegistryListCtrlRect.bottom = Rect.Height();
	m_RegistryListCtrl.MoveWindow(RegistryListCtrlRect);

	COLORREF ColorRef(RGB(190, 190, 190));

	CClientDC ClientDc(this);
	CPen Pen(PS_SOLID, 1, ColorRef);
	ClientDc.SelectObject(&Pen);
	ClientDc.MoveTo(StartPoint);
	ClientDc.LineTo(EndPoint);

}



void CRegistryDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (bShow == TRUE)
	{
		//m_RegistryTreeCtrl.MoveWindow(0, 0, m_Global->iResizeX, m_Global->iResizeY);

		// 更新父窗口信息 CurrentChildDlg 并 禁用当前子窗口的button
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_CurrentChildDlg = ArkProtect::cd_RegistryDialog;
		((CArkProtectAppDlg*)(m_Global->AppDlg))->m_RegistryButton.EnableWindow(FALSE);

		// 加载进程信息列表
		//APLoadRegistryInfo();

	}




}


/************************************************************************
*  Name : APInitializeRegistryTree
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CRegistryDlg::APInitializeRegistryTree()
{
	m_Global->RegistryCore().InitializeRegistryTree(&m_RegistryTreeCtrl);
}


/************************************************************************
*  Name : APInitializeRegistryList
*  Param: void
*  Ret  : void
*  初始化ListControl
************************************************************************/
void CRegistryDlg::APInitializeRegistryList()
{
	m_Global->RegistryCore().InitializeRegistryList(&m_RegistryListCtrl);
}






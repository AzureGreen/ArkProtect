
// ArkProtectAppDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ArkProtectAppDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  WM_ICONNOTIFY        WM_USER + 0x100    // 自定义消息
#define  WM_STATUSBARTIP      WM_USER + 0x101    // 自定义消息
#define  WM_STATUSBARDETAIL   WM_USER + 0x102    // 自定义消息

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CArkProtectAppDlg 对话框



CArkProtectAppDlg::CArkProtectAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ARKPROTECTAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CArkProtectAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_STATIC, m_ProcessButton);
	DDX_Control(pDX, IDC_DRIVER_STATIC, m_DriverButton);
	DDX_Control(pDX, IDC_KERNEL_STATIC, m_KernelButton);
	DDX_Control(pDX, IDC_HOOK_STATIC, m_HookButton);
	DDX_Control(pDX, IDC_REGISTRY_STATIC, m_RegistryButton);
	DDX_Control(pDX, IDC_APP_TAB, m_AppTab);
	DDX_Control(pDX, IDC_ABOUT_STATIC, m_AboutButton);
}

BEGIN_MESSAGE_MAP(CArkProtectAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_ICONNOTIFY, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnIconNotify)
	ON_MESSAGE(WM_STATUSBARTIP, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnUpdateStatusBarTip)
	ON_MESSAGE(WM_STATUSBARDETAIL, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnUpdateStatusBarDetail)
	ON_COMMAND(ID_ICONNOTIFY_DISPLAY, &CArkProtectAppDlg::OnIconnotifyDisplay)
	ON_COMMAND(ID_ICONNOTIFY_HIDE, &CArkProtectAppDlg::OnIconnotifyHide)
	ON_COMMAND(ID_ICONNOTIFY_EXIT, &CArkProtectAppDlg::OnIconnotifyExit)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
	ON_STN_CLICKED(IDC_PROCESS_STATIC, &CArkProtectAppDlg::OnStnClickedProcessStatic)
	ON_STN_CLICKED(IDC_DRIVER_STATIC, &CArkProtectAppDlg::OnStnClickedDriverStatic)
	ON_STN_CLICKED(IDC_KERNEL_STATIC, &CArkProtectAppDlg::OnStnClickedKernelStatic)
	ON_STN_CLICKED(IDC_HOOK_STATIC, &CArkProtectAppDlg::OnStnClickedHookStatic)
	ON_STN_CLICKED(IDC_REGISTRY_STATIC, &CArkProtectAppDlg::OnStnClickedRegistryStatic)



END_MESSAGE_MAP()


// CArkProtectAppDlg 消息处理程序

BOOL CArkProtectAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	// 保存窗口指针
	m_Global.AppDlg = this;

	// 初始化托盘
	APInitializeTray();

	CPaintDC Dc(this);
	m_Global.iDpix = Dc.GetDeviceCaps(LOGPIXELSX);
	m_Global.iDpiy = Dc.GetDeviceCaps(LOGPIXELSY);

	// 将对话框底部多加部分
	CRect	Rect;
	GetWindowRect(&Rect);
	Rect.bottom += (LONG)(1 + 21 * (m_Global.iDpiy / 96.0));
	MoveWindow(&Rect);

	// 添加状态栏
	m_StatusBar = new CStatusBarCtrl;
	m_StatusBar->Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW, CRect(0, 0, 0, 0), this, 0);

	GetClientRect(&Rect);

	int iPartBlock[3] = { 0 };
	iPartBlock[1] = Rect.right - (LONG)(1 + 21 * (m_Global.iDpix / 96.0));
	iPartBlock[0] = iPartBlock[1] - (int)(120 * (m_Global.iDpix / 96.0));

	m_StatusBar->SetParts(3, iPartBlock);
	m_StatusBar->SetText(L"All Ready", 0, 0);

	// 设置工具栏

	//	m_btnHomePage.MoveWindow(iLeftPops + (70 * 0), 0, 70, 94);
	//	m_btnProcess.MoveWindow(iLeftPops + (70 * 1), 0, 70, 94);
	//	m_btnModules.MoveWindow(iLeftPops + (70 * 2), 0, 70, 94);
	//	m_btnKernel.MoveWindow(iLeftPops + (70 * 3), 0, 70, 94);
	//	m_btnHooks.MoveWindow(iLeftPops + (70 * 4), 0, 70, 94);

	//	Rect.top = 94 + 2;
	//	Rect.bottom -= (LONG)(1 + 21 * (GlobalObject.iDpiy / 96.0));


	GetClientRect(&Rect);
	Rect.top = 2;
	Rect.bottom -= (LONG)(1 + 21 * (m_Global.iDpiy / 96.0));
	Rect.left += 94;
	m_AppTab.MoveWindow(Rect);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CArkProtectAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CArkProtectAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CArkProtectAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



/************************************************************************
*  Name : PhInitTray
*  Param: void
*  Ret  : void
*  初始化托盘
************************************************************************/
void CArkProtectAppDlg::APInitializeTray()
{
	m_NotifyIcon.cbSize = sizeof(NOTIFYICONDATA);			// 大小赋值
	m_NotifyIcon.hWnd = m_hWnd;							// 父窗口
	m_NotifyIcon.uID = IDR_MAINFRAME;						// 这里的宏是图标ID
	m_NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;// 托盘所拥有的状态
	m_NotifyIcon.uCallbackMessage = WM_ICONNOTIFY;			// 自定义回调消息   在托盘上处理鼠标动作  #define  WM_ICONNOTIFY   WM_USER + 0x100
	m_NotifyIcon.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));	// 调用全局函数来加载图标	// MAKEINTRESOURCE 数字类型转换成指针类型

	lstrcpy(m_NotifyIcon.szTip, L"ArkProtect");				// 当鼠标放在上面时，所显示的内容 
	Shell_NotifyIcon(NIM_ADD, &m_NotifyIcon);				// 在托盘区添加图标 // 向任务栏发送一个消息，向托盘区域添加一个图标
}


/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  创建图标菜单
************************************************************************/
LRESULT CArkProtectAppDlg::OnIconNotify(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		// 创建菜单栏
		CMenu Menu;
		Menu.LoadMenuW(IDR_ICONNOTIFY_MENU);

		if (IsWindowVisible())
		{
			Menu.DeleteMenu(ID_ICONNOTIFY_DISPLAY, MF_BYCOMMAND);	// 通过指定id删除菜单项
		}
		else
		{
			Menu.DeleteMenu(ID_ICONNOTIFY_HIDE, MF_BYCOMMAND);
		}

		CPoint Pt;
		GetCursorPos(&Pt);        // 得到鼠标位置
		SetForegroundWindow();    // 设置当前窗口 	

		// 在指定位置显示快捷菜单，并跟踪菜单项的选择
		Menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON, Pt.x, Pt.y, this);

		Menu.DestroyMenu();

		break;
	}
	default:
		break;
	}
	return 0;
}


void CArkProtectAppDlg::OnIconnotifyDisplay()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_SHOWNORMAL);
}


void CArkProtectAppDlg::OnIconnotifyHide()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_HIDE);
}


void CArkProtectAppDlg::OnIconnotifyExit()
{
	// TODO: 在此添加命令处理程序代码
	SendMessage(WM_CLOSE);              // 关闭窗口：CLOSE--->DESTROY--->QUIT
}


void CArkProtectAppDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	Shell_NotifyIcon(NIM_DELETE, &m_NotifyIcon);    // 关闭IconNotify
	CDialogEx::OnClose();
}


void CArkProtectAppDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	
	// 关闭句柄！！！
	CloseHandle(m_Global.m_DeviceHandle);

	// 卸载驱动
	m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // 释放资源
}


void CArkProtectAppDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	// 一开始显示ProcessModule
	if (bShow)
	{
		// 启动的时候，首先显示进程模块
		OnStnClickedProcessStatic();
	}

}


int CArkProtectAppDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	WCHAR wzDriverFullPath[MAX_PATH] = { 0 };
	WCHAR *Pos = NULL;

	HMODULE Module = GetModuleHandle(0);
	GetModuleFileName(Module, wzDriverFullPath, sizeof(wzDriverFullPath));
	Pos = wcsrchr(wzDriverFullPath, L'\\');
	*Pos = 0;   // 截断
	StringCchCatW(wzDriverFullPath, MAX_PATH, L"\\ArkProtectDrv.sys");

	// 启动服务，加载驱动
	BOOL bOk = m_Global.LoadNtDriver(DRIVER_SERVICE_NAME, wzDriverFullPath);
	if (bOk)
	{
		m_Global.m_DeviceHandle = CreateFileW(LINK_NAME, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (m_Global.m_DeviceHandle == INVALID_HANDLE_VALUE)
		{
			m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // 释放资源
			return -1;
		}
	}
	else
	{
		// 驱动加载失败了
		m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // 释放资源
		return -1;
	}

	return 0;
}


void CArkProtectAppDlg::OnStnClickedProcessStatic()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_ProcessDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_ProcessDialog);
	}

}


void CArkProtectAppDlg::OnStnClickedDriverStatic()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_DriverDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_DriverDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedKernelStatic()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_KernelDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_KernelDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedHookStatic()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_HookDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_HookDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedRegistryStatic()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_RegistryDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_RegistryDialog);
	}
}



/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  创建图标菜单
************************************************************************/
LRESULT CArkProtectAppDlg::OnUpdateStatusBarTip(WPARAM wParam, LPARAM lParam)
{
	m_StatusBar->SetText((LPCWSTR)lParam, 0, 0);
	return TRUE;
}


/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  创建图标菜单
************************************************************************/
LRESULT CArkProtectAppDlg::OnUpdateStatusBarDetail(WPARAM wParam, LPARAM lParam)
{
	m_StatusBar->SetText((LPCWSTR)lParam, 1, 0);
	return TRUE;
}






/************************************************************************
*  Name : APEnableCurrentButton
*  Param: CurrentChildDlg         当前子对话框（eChildDlg）
*  Ret  : void
*  启动当前自对话框对应的Button
************************************************************************/
void CArkProtectAppDlg::APEnableCurrentButton(ArkProtect::eChildDlg CurrentChildDlg)
{
	switch (CurrentChildDlg)
	{
	case ArkProtect::cd_ProcessDialog:
	{
		m_ProcessButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_DriverDialog:
	{
		m_DriverButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_KernelDialog:
	{
		m_KernelButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_HookDialog:
	{
		m_HookButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_RegistryDialog:
	{
	 	m_RegistryButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_AboutDialog:
	{
		m_AboutButton.EnableWindow(TRUE);
		break;
	}
	default:
		break;
	}
}


/************************************************************************
*  Name : APShowChildWindow
*  Param: TargetChildDlg        目标生成子对话框（eChildDlg）
*  Ret  : void
*  显示/生成目标的自对话框
************************************************************************/
void CArkProtectAppDlg::APShowChildWindow(ArkProtect::eChildDlg TargetChildDlg)
{
	switch (TargetChildDlg)
	{
	case ArkProtect::cd_ProcessDialog:
	{
		if (m_ProcessDlg == NULL)
		{
			m_ProcessDlg = new CProcessDlg(this, &m_Global);

			// 绑定对话框
			m_ProcessDlg->Create(IDD_PROCESS_DIALOG, GetDlgItem(IDC_APP_TAB));

			// 移动窗口位置
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_ProcessDlg->MoveWindow(&Rect);
		}

		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(TRUE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		//if (m_KrnlHookDlg) m_KrnlHookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_DriverDialog:
	{
		if (m_DriverDlg == NULL)
		{
			m_DriverDlg = new CDriverDlg(this, &m_Global);

			// 绑定对话框
			m_DriverDlg->Create(IDD_DRIVER_DIALOG, GetDlgItem(IDC_APP_TAB));

			// 移动窗口位置
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_DriverDlg->MoveWindow(&Rect);
		}

		if (m_DriverDlg) m_DriverDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
	//	if (m_KrnlHookDlg) m_KrnlHookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_KernelDialog:
	{
		if (m_KernelDlg == NULL)
		{
			m_KernelDlg = new CKernelDlg(this, &m_Global);

			// 绑定对话框
			m_KernelDlg->Create(IDD_KERNEL_DIALOG, GetDlgItem(IDC_APP_TAB));

			// 移动窗口位置
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_KernelDlg->MoveWindow(&Rect);
		}

		if (m_KernelDlg) m_KernelDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
	//	if (m_KrnlHookDlg) m_KrnlHookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_HookDialog:
	{
	//	if (m_KrnlHookDlg == NULL)
	//	{
	//		m_KrnlHookDlg = new CKrnlHookDlg(this);

	//		// 绑定对话框
	//		m_KrnlHookDlg->Create(IDD_DIALOG_KRNLHOOK, GetDlgItem(IDC_TAB_MAIN));

	//		// 移动窗口位置
	//		CRect	Rect;
	//		m_MainTab.GetClientRect(&Rect);
	//		m_KrnlHookDlg->MoveWindow(&Rect);
	//	}

	//	if (m_KrnlHookDlg) m_KrnlHookDlg->ShowWindow(TRUE);
	//	if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
	//	if (m_ModuleDlg) m_ModuleDlg->ShowWindow(FALSE);
	//	if (m_KernelSysDlg) m_KernelSysDlg->ShowWindow(FALSE);


	//	MessageBox(0, 0, 0);
		break;
	}
	case ArkProtect::cd_RegistryDialog:
	{
		if (m_RegistryDlg == NULL)
		{
			m_RegistryDlg = new CRegistryDlg(this, &m_Global);

			// 绑定对话框
			m_RegistryDlg->Create(IDD_REGISTRY_DIALOG, GetDlgItem(IDC_APP_TAB));

			// 移动窗口位置
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_RegistryDlg->MoveWindow(&Rect);
		}

		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		//if (m_KrnlHookDlg) m_KrnlHookDlg->ShowWindow(FALSE);


		break;
	}

	default:
		break;
	}
}







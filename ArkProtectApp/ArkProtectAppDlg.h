
// ArkProtectAppDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "Global.hpp"
#include "afxcmn.h"
#include "ProcessDlg.h"

// CArkProtectAppDlg 对话框
class CArkProtectAppDlg : public CDialogEx
{
// 构造
public:
	CArkProtectAppDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ARKPROTECTAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnIconNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateStatusBarTip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateStatusBarDetail(WPARAM wParam, LPARAM lParam);
	afx_msg void OnIconnotifyDisplay();
	afx_msg void OnIconnotifyHide();
	afx_msg void OnIconnotifyExit();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
public:

	void APInitializeTray();

	void APEnableCurrentButton(ArkProtect::eChildDlg ChildDlg);

	void APShowChildWindow(ArkProtect::eChildDlg ChildDlg);

	CTabCtrl            m_AppTab;
	CStatic             m_ProcessButton;
	CStatic             m_DriverrButton;
	CStatic             m_KernelButton;
	CStatic             m_HookButton;
	CStatic             m_AboutButton;
	CStatusBarCtrl      *m_StatusBar;
	NOTIFYICONDATA	    m_NotifyIcon = { 0 };   // 任务栏图标
	
	
	ArkProtect::CGlobal   m_Global;
	ArkProtect::eChildDlg m_CurrentChildDlg;    // 子对话框
	CProcessDlg           *m_ProcessDlg;
	
};

#pragma once

#include "Global.hpp"
#include "afxcmn.h"
#include "afxwin.h"

// CHookDlg 对话框

class CHookDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHookDlg)

public:
	CHookDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // 标准构造函数
	virtual ~CHookDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HOOK_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnLbnSelchangeHookListbox();
	afx_msg void OnNMCustomdrawHookListctrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickHookListctrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUpdateHookOnlyShowHooked(CCmdUI *pCmdUI);
	afx_msg void OnHookFreshen();
	afx_msg void OnHookResume();
	afx_msg void OnHookResumeAll();
	afx_msg void OnHookProperty();
	afx_msg void OnHookLocation();
	afx_msg void OnHookExportInformation();
	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeHookItemList();

	CString APGetSelectedFilePath(int iItem);

	CListCtrl m_HookListCtrl;
	CListBox m_HookListBox;
	CListBox m_HookModuleListBox;

	ArkProtect::CGlobal  *m_Global;
	
	int                  m_iCurSel = 65535;
	BOOL                 m_bOnlyShowHooked = TRUE;

	afx_msg void OnHookOnlyShowHooked();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
};

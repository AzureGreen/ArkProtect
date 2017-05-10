#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Global.hpp"

// CKernelDlg 对话框

class CKernelDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CKernelDlg)

public:
	CKernelDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // 标准构造函数
	virtual ~CKernelDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KERNEL_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnLbnSelchangeKernelListbox();
	
	DECLARE_MESSAGE_MAP()
public:

	void APInitializeKernelItemList();




	CImageList           m_KernelIconList;
	CListBox             m_KernelListBox;
	CListCtrl            m_KernelListCtrl;

	ArkProtect::CGlobal  *m_Global;

	int                  m_iCurSel = 65535;


	
	afx_msg void OnNMCustomdrawKernelListctrl(NMHDR *pNMHDR, LRESULT *pResult);
};

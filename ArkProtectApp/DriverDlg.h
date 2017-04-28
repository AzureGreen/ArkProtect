#pragma once
#include "Global.hpp"
#include "afxcmn.h"

// CDriverDlg 对话框

class CDriverDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDriverDlg)

public:
	CDriverDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // 标准构造函数
	virtual ~CDriverDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DRIVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnNMCustomdrawDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDriverFreshen();
	afx_msg void OnDriverDelete();
	afx_msg void OnDriverUnload();

	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeDriverList();

	void APLoadDriverList();


	CImageList m_DriverIconList;   // 进程图标
	CListCtrl  m_DriverListCtrl;

	ArkProtect::CGlobal      *m_Global;
	
	



};

#pragma once

#include <vector>
#include "Global.hpp"
#include "afxcmn.h"



// CRegistryDlg 对话框

class CRegistryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegistryDlg)

public:
	CRegistryDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // 标准构造函数
	virtual ~CRegistryDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REGISTRY_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	
	

	DECLARE_MESSAGE_MAP()


public:

	void APInitializeRegistryTree();

	void APInitializeRegistryList();



	ArkProtect::CGlobal *m_Global;
	CImageList          m_RegistryIconList;
	CImageList          m_RegistryIconTree;
	CListCtrl           m_RegistryListCtrl;
	CTreeCtrl           m_RegistryTreeCtrl;

	
};

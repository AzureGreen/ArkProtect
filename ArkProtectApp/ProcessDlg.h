#pragma once
#include "afxcmn.h"
#include "Global.hpp"
#include "ProcessCore.h"


// CProcessDlg 对话框

class CProcessDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessDlg)

public:
	CProcessDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // 标准构造函数
	virtual ~CProcessDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	

	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeProcessList();


	CImageList m_ProcessIconList;   // 进程图标
	CListCtrl  m_ProcessListCtrl;   // ListControl

	ArkProtect::CGlobal      *m_Global;
	ArkProtect::CProcessCore m_Process;

	
};

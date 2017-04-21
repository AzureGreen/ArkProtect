#pragma once
#include "afxcmn.h"
#include "Global.hpp"
#include "ProcessCore.h"
#include "ProcessInfoDlg.h"

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
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnLvnColumnclickProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnProcessFreshen();
	afx_msg void OnProcessModule();
	afx_msg void OnProcessThread();

	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeProcessList();

	void APLoadProcessList();

	void APInitializeProcessInfoDlg(ArkProtect::eProcessInfoKind ProcessInfoKind);


	


	CImageList m_ProcessIconList;   // 进程图标
	CListCtrl  m_ProcessListCtrl;   // ListControl

	

	ArkProtect::CGlobal      *m_Global;
	
	static UINT32     m_SortColumn;
	static BOOL       m_bSortOrder;  // 记录排序顺序
	
	
	
	
	
	
	afx_msg void OnProcessHandle();
};


int CALLBACK APProcessListCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
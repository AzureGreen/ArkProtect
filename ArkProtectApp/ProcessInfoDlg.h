#pragma once
#include "afxcmn.h"
#include "Define.h"
#include "Global.hpp"

// CProcessInfoDlg 对话框

class CProcessInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessInfoDlg)

public:
	CProcessInfoDlg(CWnd* pParent, ArkProtect::eProcessInfoKind ProcessInfoKind,
		ArkProtect::CGlobal *GlobalObject);   // 标准构造函数
	virtual ~CProcessInfoDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESS_INFO_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	

	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeProcessInfoList();

	void APInitializeProcessModuleList();

	void APLoadProcessModuleList();

	void APInitializeProcessThreadList();

	void APLoadProcessThreadList();



	CListCtrl m_ProcessInfoListCtrl;
	HICON     m_hIcon;

	UINT32    m_ProcessId = 0;

	ArkProtect::CGlobal                     *m_Global;
	ArkProtect::eProcessInfoKind            m_WantedInfoKind;
	ArkProtect::eProcessInfoKind            m_CurrentInfoKind = (ArkProtect::eProcessInfoKind)(-1);

};

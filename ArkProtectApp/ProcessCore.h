#pragma once

#include <vector>
#include <strsafe.h>
#include "Define.h"
#include "Global.hpp"

namespace ArkProtect
{
	enum eProcessColumn
	{
		pc_ImageName,     // 进程名称
		pc_ProcessId,     // 进程ID
		pc_ParentProcessId,// 父进程ID		
		pc_FilePath,      // 进程完整路径
		pc_EProcess,      // 进程结构体
		pc_UserAccess,    // 应用层访问
		pc_Company        // 文件厂商
	};

	typedef struct _PROCESS_ENTRY_INFORMATION
	{
		WCHAR     wzImageName[100];
		UINT32	  ProcessId;
		UINT32	  ParentProcessId;
		WCHAR     wzFilePath[MAX_PATH];
		UINT_PTR  EProcess;
		BOOL      bUserAccess;
		WCHAR     wzCompanyName[MAX_PATH];
	} PROCESS_ENTRY_INFORMATION, *PPROCESS_ENTRY_INFORMATION;

	typedef struct _PROCESS_INFORMATION
	{
		UINT32                    NumberOfProcesses;
		PROCESS_ENTRY_INFORMATION ProcessEntry[1];
	} PROCESS_INFORMATION, *PPROCESS_INFORMATION;


	class CProcessCore
	{
	public:
		CProcessCore(CGlobal *GlobalObject);
		~CProcessCore();

		void InitializeProcessList(CListCtrl *ProcessList);

		UINT32 GetProcessNum();

		BOOL GrantPriviledge(IN PWCHAR PriviledgeName, IN BOOL bEnable);

		BOOL QueryProcessUserAccess(UINT32 ProcessId);

		ePeBit QueryPEFileBit(const WCHAR * wzFilePath);

		void PerfectProcessInfo(PPROCESS_ENTRY_INFORMATION ProcessEntry);

		BOOL EnumProcessInfo();

		void AddProcessFileIcon(WCHAR * wzProcessPath);

		void InsertProcessInfoList(CListCtrl * ListCtrl);

		void QueryProcessInfo(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessInfoCallback(LPARAM lParam);





	private:
		
		UINT32        m_ProcessCount = 0;
		int           m_iColumnCount = 7;		// 进程列表数
		COLUMN_STRUCT m_ColumnStruct[7] = {
			{ L"映像名称",			160 },
			{ L"进程ID",			65 },
			{ L"父进程ID",			65 },
			{ L"映像路径",			230 },
			{ L"EPROCESS",			125 },
			{ L"应用层访问",		75 },
			{ L"文件厂商",			122 }};


		std::vector<PROCESS_ENTRY_INFORMATION> m_ProcessEntryVector;



		CGlobal             *m_Global;
		static CProcessCore *m_Process;
		
	};

	

}


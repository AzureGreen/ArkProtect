#pragma once
#include <vector>
#include "Define.h"
#include "NtStructs.h"

typedef NTSTATUS
(NTAPI * pfnZwQuerySystemInformation)(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN UINT32 SystemInformationLength,
	OUT PUINT32 ReturnLength OPTIONAL);

namespace ArkProtect
{
	enum eProcessThreadColumn
	{
		ptc_ThreadId,        // 线程id
		ptc_EThread,
		ptc_Teb,
		ptc_Priority,
		ptc_Entrance,
		ptc_Module,
		ptc_switches,
		ptc_Status
	};

	typedef enum _KTHREAD_STATE 
	{
		Initialized,
		Ready,
		Running,
		Standby,
		Terminated,
		Waiting,
		Transition,
		DeferredReady,
		GateWait
	} KTHREAD_STATE;

	typedef struct _PROCESS_THREAD_ENTRY_INFORMATION
	{
		UINT_PTR EThread;
		UINT32   ThreadId;
		UINT_PTR Teb;
		UINT8    Priority;
		UINT_PTR Win32StartAddress;
		UINT32   ContextSwitches;
		UINT8    State;
	} PROCESS_THREAD_ENTRY_INFORMATION, *PPROCESS_THREAD_ENTRY_INFORMATION;


	typedef struct _PROCESS_THREAD_INFORMATION
	{
		UINT32                           NumberOfThreads;
		PROCESS_THREAD_ENTRY_INFORMATION ThreadEntry[1];
	} PROCESS_THREAD_INFORMATION, *PPROCESS_THREAD_INFORMATION;


	class CProcessThread
	{
	public:
		CProcessThread(class CGlobal *GlobalObject);
		~CProcessThread();
		void InitializeProcessThreadList(CListCtrl * ListCtrl);




		BOOL EnumProcessThread();

		CString GetModulePathByThreadStartAddress(UINT_PTR StartAddress);

		void InsertProcessThreadInfoList(CListCtrl * ListCtrl);

		void QueryProcessThread(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessThreadCallback(LPARAM lParam);

		BOOL GetThreadIdByProcessId(UINT32 ProcessId, PUINT32 ThreadId);





	private:
		int           m_iColumnCount = 8;		// 进程列表数
		COLUMN_STRUCT m_ColumnStruct[8] = {
			{ L"线程ID",			50 },
			{ L"ETHREAD",			125 },
			{ L"Teb",				125 },
			{ L"优先级",			54 },
			{ L"线程入口",			125 },
			{ L"模块",				90 },
			{ L"切换次数",			68 },
			{ L"状态",				50 } };



		std::vector<PROCESS_THREAD_ENTRY_INFORMATION> m_ProcessThreadEntryVector;

		class CGlobal              *m_Global;

		class CProcessModule       &m_ProcessModule;

		static CProcessThread      *m_ProcessThread;

	};



}




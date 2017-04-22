#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eProcessWindowColumn
	{
		pwc_WindowHandle,        // 窗口句柄
		pwc_WindowText,          // 窗口标题
		pwc_WindowClass,         // 窗口类名
		pwc_WindowVisibal,       // 窗口可见性
		pwc_ProcessId,           // 进程id
		pwc_ThreadId,            // 线程id           
	};

	typedef struct _PROCESS_WINDOW_ENTRY_INFORMATION
	{
		HWND   hWnd;
		WCHAR  wzWindowText;
		WCHAR  wzWindowClass;
		BOOL   bVisibal;
		UINT32 ProcessId;
		UINT32 ThreadId;
	} PROCESS_WINDOW_ENTRY_INFORMATION, *PPROCESS_WINDOW_ENTRY_INFORMATION;

	typedef struct _PROCESS_WINDOW_INFORMATION
	{
		UINT32                            NumberOfWindows;
		PROCESS_WINDOW_ENTRY_INFORMATION  WindowEntry[1];
	} PROCESS_WINDOW_INFORMATION, *PPROCESS_WINDOW_INFORMATION;


	class CProcessWindow
	{
	public:
		CProcessWindow(class CGlobal *GlobalObject);
		~CProcessWindow();
		void InitializeProcessWindowList(CListCtrl * ListCtrl);

		BOOL EnumProcessWindow();

		void QueryProcessWindow(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessWindowCallback(LPARAM lParam);







	private:
		int           m_iColumnCount = 6;		// 进程列表数
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"窗口句柄",				80 },
			{ L"窗口标题",				140 },
			{ L"窗口类名",				140 },
			{ L"窗口可见性",			90 },
			{ L"进程Id",				70 },
			{ L"线程Id",				70 } };



		std::vector<PROCESS_WINDOW_ENTRY_INFORMATION> m_ProcessWindowEntryVector;


		class CGlobal         *m_Global;

		static CProcessWindow *m_ProcessWindow;



	};



}

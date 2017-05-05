#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eSsdtHookColumn
	{
		shc_Ordinal,         // 序号
		shc_FunctionName,    // 函数名称
		shc_CurrentAddress,  // 函数当前地址
		shc_OriginalAddress, // 函数原始地址
		shc_Status,          // 状态
		shc_FilePath         // 当前函数所在模块
	};

	typedef struct _SSDT_HOOK_ENTRY_INFORMATION
	{
		UINT32	    Ordinal;
		BOOL        bHooked;
		UINT_PTR	CurrentAddress;
		UINT_PTR	OriginalAddress;
		WCHAR	    wzFunctionName[100];
	} SSDT_HOOK_ENTRY_INFORMATION, *PSSDT_HOOK_ENTRY_INFORMATION;

	typedef struct _SSDT_HOOK_INFORMATION
	{
		UINT32                        NumberOfSsdtFunctions;
		SSDT_HOOK_ENTRY_INFORMATION   SsdtHookEntry[1];
	} SSDT_HOOK_INFORMATION, *PSSDT_HOOK_INFORMATION;


	class CSsdtHook
	{
	public:
		CSsdtHook(class CGlobal *GlobalObject);
		~CSsdtHook();

		void InitializeSsdtList(CListCtrl * ListCtrl);

		BOOL EnumSsdtHook();

		void InsertSsdtHookInfoList(CListCtrl * ListCtrl);

		void QuerySsdtHook(CListCtrl * ListCtrl);

		static DWORD CALLBACK QuerySsdtHookCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"序号",					50 },
			{ L"函数名称",				145 },
			{ L"函数当前地址",			125 },
			{ L"函数原始地址",			125 },
			{ L"状态",					85 },
			{ L"当前函数所在模块",		195 } };

		std::vector<SSDT_HOOK_ENTRY_INFORMATION> m_SsdtHookEntryVector;

		class CGlobal      *m_Global;

		class CDriverCore  &m_DriverCore;

		static CSsdtHook   *m_SsdtHook;
	};
}


#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eProcessMemoryColumn
	{
		pmc_MemoryAddress,    // 内存基地址
		pmc_RegionSize,     // 内存大小
		pmc_ProtectType,    // 保护属性
		pmc_State,          // 状态
		pmc_Type,           // 内存类型 
		pmc_ModuleName      // 模块名称
	};

	typedef struct _VIRTUAL_PROTECT_TYPE
	{
		UINT32 Type;
		WCHAR  wzTypeName[MAX_PATH];
	} VIRTUAL_PROTECT_TYPE, *PVIRTUAL_PROTECT_TYPE;

	typedef struct _PROCESS_MEMORY_ENTRY_INFORMATION
	{
		UINT_PTR	BaseAddress;
		UINT_PTR	RegionSize;
		UINT32		Protect;
		UINT32		State;
		UINT32		Type;
	} PROCESS_MEMORY_ENTRY_INFORMATION, *PPROCESS_MEMORY_ENTRY_INFORMATION;

	typedef struct _PROCESS_MEMORY_INFORMATION
	{
		UINT32								NumberOfMemories;
		PROCESS_MEMORY_ENTRY_INFORMATION	MemoryEntry[1];
	}PROCESS_MEMORY_INFORMATION, *PPROCESS_MEMORY_INFORMATION;


	class CProcessMemory
	{
	public:
		CProcessMemory(class CGlobal *GlobalObject);
		~CProcessMemory();

		void InitializeVirtualMemoryProtectVector();

		void InitializeProcessMemoryList(CListCtrl * ListCtrl);

		BOOL EnumProcessMemory();

		CString GetMemoryProtect(UINT32 Protect);

		CString GetMemoryState(UINT32 State);

		CString GetMemoryType(UINT32 Type);

		CString GetModuleImageNameByMemoryBaseAddress(UINT_PTR BaseAddress);

		void InsertProcessMemoryInfoList(CListCtrl * ListCtrl);

		void QueryProcessMemory(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessMemoryCallback(LPARAM lParam);



	private:
		int           m_iColumnCount = 6;		// 进程列表数
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"内存地址",			125 },
			{ L"内存大小",			80 },
			{ L"保护属性",			100 },
			{ L"状态",				80 },
			{ L"类型",				80 },
			{ L"模块名",			125 } };


		std::vector<VIRTUAL_PROTECT_TYPE> m_VitualProtectTypeVector;

		std::vector<PROCESS_MEMORY_ENTRY_INFORMATION> m_ProcessMemoryEntryVector;


		class CGlobal         *m_Global;

		class CProcessModule  &m_ProcessModule;

		static CProcessMemory *m_ProcessMemory;

	};


}

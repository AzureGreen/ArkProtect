#pragma once
#include <vector>
#include <strsafe.h>
#include "Define.h"

namespace ArkProtect
{
	enum eDriverColumn
	{
		dc_DriverName,     // 驱动名
		dc_BaseAddress,    // 基地址ID
		dc_Size,           // 大小		
		dc_Object,         // 驱动对象
		dc_DriverPath,     // 驱动路径
		dc_ServiceName,    // 服务名
		dc_StartAddress,   // 启动入口
		dc_LoadOrder,      // 加载顺序
		dc_Company         // 文件厂商
	};

	typedef struct _DRIVER_ENTRY_INFORMATION
	{
		UINT_PTR  BaseAddress;
		UINT_PTR  Size;
		UINT_PTR  DriverObject;
		UINT_PTR  DirverStartAddress;
		UINT_PTR  LoadOrder;
		WCHAR     wzDriverName[100];
		WCHAR     wzDriverPath[MAX_PATH];
		WCHAR     wzServiceName[MAX_PATH];
		WCHAR     wzCompanyName[MAX_PATH];
	} DRIVER_ENTRY_INFORMATION, *PDRIVER_ENTRY_INFORMATION;

	typedef struct _DRIVER_INFORMATION
	{
		UINT32                          NumberOfDrivers;
		DRIVER_ENTRY_INFORMATION        DriverEntry[1];
	} DRIVER_INFORMATION, *PDRIVER_INFORMATION;


	class CDriverCore
	{
	public:
		CDriverCore(class CGlobal *GlobalObject);
		~CDriverCore();

		void InitializeDriverList(CListCtrl * DriverList);

		void PerfectDriverInfo(PDRIVER_ENTRY_INFORMATION DriverEntry);

		BOOL EnumDriverInfo();

		void InsertDriverInfoList(CListCtrl * ListCtrl);

		void QueryDriverInfo(CListCtrl * ListCtrl);

		BOOL UnloadDriver(UINT_PTR DriverObject);

		static DWORD CALLBACK QueryDriverInfoCallback(LPARAM lParam);

		static DWORD CALLBACK UnloadDriverCallback(LPARAM lParam);

		CString GetDriverPathByAddress(UINT_PTR Address);


		//
		// 返回变量Interface
		//
		inline std::vector<DRIVER_ENTRY_INFORMATION>& DriverEntryVector() { return m_DriverEntryVector; }


	private:

		UINT32        m_DriverCount = 0;
		int           m_iColumnCount = 9;		// 进程列表数
		COLUMN_STRUCT m_ColumnStruct[9] = {
			{ L"驱动名",		130 },
			{ L"基地址",		125 },
			{ L"大小",			70 },
			{ L"驱动对象",		125 },
			{ L"驱动路径",		200 },
			{ L"服务名",		80 },
			{ L"启动入口",		125 },
			{ L"加载顺序",		65 },
			{ L"文件厂商",		120 } };


		std::vector<DRIVER_ENTRY_INFORMATION> m_DriverEntryVector;


		class CGlobal       *m_Global;
		static CDriverCore  *m_Driver;

	};




}
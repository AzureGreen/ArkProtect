#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eDpcTimerColumn
	{
		dtc_Object,                   // Timer对象
		dtc_Device,                   // 设备对象
		dtc_Cycle,                    // 周期
		dtc_Dispatch,                 // 函数入口
		dtc_FilePath,                 // 描述
		dtc_Company                   // 出品厂商
	};

	typedef struct _DPC_TIMER_ENTRY_INFORMATION
	{
		UINT_PTR TimerObject;
		UINT_PTR RealDpc;
		UINT_PTR Cycle;       // 周期
		UINT_PTR TimeDispatch;
	} DPC_TIMER_ENTRY_INFORMATION, *PDPC_TIMER_ENTRY_INFORMATION;

	typedef struct _DPC_TIMER_INFORMATION
	{
		UINT32                      NumberOfDpcTimers;
		DPC_TIMER_ENTRY_INFORMATION DpcTimerEntry[1];
	} DPC_TIMER_INFORMATION, *PDPC_TIMER_INFORMATION;

	class CDpcTimer
	{
	public:
		CDpcTimer(class CGlobal *GlobalObject);
		~CDpcTimer();

		void InitializeDpcTimerList(CListCtrl * ListCtrl);

		BOOL EnumDpcTimer();

		void InsertDpcTimerInfoList(CListCtrl * ListCtrl);

		void QueryDpcTimer(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryDpcTimerCallback(LPARAM lParam);

	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"定时器对象",			125 },
			{ L"设备对象",				125 },
			{ L"触发周期(s)",			70 },
			{ L"函数入口",				125 },
			{ L"模块文件",				155 },
			{ L"出品厂商",				125 } };

		std::vector<DPC_TIMER_ENTRY_INFORMATION> m_DpcTimerEntryVector;


		class CGlobal     *m_Global;

		class CDriverCore &m_DriverCore;

		static CDpcTimer  *m_DpcTimer;

	};

}

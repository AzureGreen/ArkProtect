#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eIoTimerColumn
	{
		itc_Object,                   // Timer对象
		itc_Device,                   // 设备对象
		itc_Status,                   // 状态
		itc_Dispatch,                 // 函数入口
		itc_FilePath,                 // 描述
		itc_Company                   // 出品厂商
	};

	typedef struct _IO_TIMER_ENTRY_INFORMATION
	{
		UINT_PTR TimerObject;
		UINT_PTR DeviceObject;
		UINT_PTR TimeDispatch;
		UINT_PTR TimerEntry;		// 与ListCtrl的Item关联，便于判断
		UINT32   Status;
	} IO_TIMER_ENTRY_INFORMATION, *PIO_TIMER_ENTRY_INFORMATION;

	typedef struct _IO_TIMER_INFORMATION
	{
		UINT_PTR                   NumberOfIoTimers;
		IO_TIMER_ENTRY_INFORMATION IoTimerEntry[1];
	} IO_TIMER_INFORMATION, *PIO_TIMER_INFORMATION;


	class CIoTimer
	{
	public:
		CIoTimer(class CGlobal *GlobalObject);
		~CIoTimer();

		void InitializeIoTimerList(CListCtrl * ListCtrl);

		BOOL EnumIoTimer();

		void InsertIoTimerInfoList(CListCtrl * ListCtrl);

		void QueryIoTimer(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryIoTimerCallback(LPARAM lParam);



	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"定时器对象",			125 },
			{ L"设备对象",				125 },
			{ L"状态",					45 },
			{ L"函数入口",				125 },
			{ L"模块路径",				180 },
			{ L"出品厂商",				125 } };

		std::vector<IO_TIMER_ENTRY_INFORMATION> m_IoTimerEntryVector;


		class CGlobal     *m_Global;

		class CDriverCore &m_DriverCore;

		static CIoTimer   *m_IoTimer;

	};



}
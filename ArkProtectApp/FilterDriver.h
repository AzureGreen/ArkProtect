#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eFilterDriverColumn
	{
		fdc_Type,                  // 类型
		fdc_DriverName,            // 驱动名称
		fdc_FilePath,              // 文件路径
		fdc_DeviceObject,          // 设备对象
		fdc_DeviceName,            // 设备名称
		fdc_AttachedDriverName,    // 宿主驱动名称
		fdc_Compay                 // 厂商
	};


	enum eFilterType
	{
		ft_Unkonw,
		ft_File,				// 文件------------
		ft_Disk,                // 磁盘    文件相关
		ft_Volume,		        // 卷  ------------- 
		ft_Keyboard,            // 键盘
		ft_Mouse,				// 鼠标            硬件接口
		ft_I8042prt,			// 键盘驱动
		ft_Tcpip,				// tcpip-------------------
		ft_Ndis,				// 网络驱动接口
		ft_PnpManager,          // 即插即用管理器       网络相关
		ft_Tdx,				    // 网络相关
		ft_Raw
	};


	typedef struct _FILTER_DRIVER_ENTRY_INFORMATION
	{
		eFilterType FilterType;
		UINT_PTR    FilterDeviceObject;
		WCHAR       wzFilterDriverName[MAX_PATH];
		WCHAR       wzFilterDeviceName[MAX_PATH];
		WCHAR       wzAttachedDriverName[MAX_PATH];
		WCHAR       wzFilePath[MAX_PATH];
	} FILTER_DRIVER_ENTRY_INFORMATION, *PFILTER_DRIVER_ENTRY_INFORMATION;

	typedef struct _FILTER_DRIVER_INFORMATION
	{
		UINT32                          NumberOfFilterDrivers;
		FILTER_DRIVER_ENTRY_INFORMATION FilterDriverEntry[1];
	} FILTER_DRIVER_INFORMATION, *PFILTER_DRIVER_INFORMATION;

	class CFilterDriver
	{
	public:
		CFilterDriver(class CGlobal *GlobalObject);
		~CFilterDriver();

		void InitializeFilterDriverList(CListCtrl * ListCtrl);

		BOOL EnumFilterDriver();

		void InsertFilterDriverInfoList(CListCtrl * ListCtrl);

		void QueryFilterDriver(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryFilterDriverCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 7;
		COLUMN_STRUCT m_ColumnStruct[7] = {
			{ L"类型",					80 },
			{ L"驱动对象名称",			125 },
			{ L"过滤驱动路径", 			220 },
			{ L"过滤设备对象",			125 },
			{ L"过滤设备名称",			125 },
			{ L"宿主驱动名称",			125 },
			{ L"出品厂商",				125 } };

		std::vector<FILTER_DRIVER_ENTRY_INFORMATION> m_FilterDriverEntryVector;

		class CGlobal          *m_Global;




		static CFilterDriver *m_FilterDriver;

	};

}

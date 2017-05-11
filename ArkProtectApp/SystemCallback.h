#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eSystemCallbackColumn
	{
		scc_Address,                  // 回调地址
		scc_Type,                     // 回调类型
		scc_FilePath,                 // 所在模块文件
		scc_Company,                  // 出品厂商
		scc_Description               // 描述
	};

	enum eCallbackType
	{
		ct_NotifyCreateProcess,
		ct_NotifyCreateThread,
		ct_NotifyLoadImage,
		ct_NotifyCmpCallBack,
		ct_NotifyKeBugCheck,
		ct_NotifyKeBugCheckReason,
		ct_NotifyShutdown,
		ct_NotifyLastChanceShutdown
	};

	typedef struct _SYS_CALLBACK_ENTRY_INFORMATION
	{
		eCallbackType Type;
		UINT_PTR      CallbackAddress;
		UINT_PTR      Description;
	} SYS_CALLBACK_ENTRY_INFORMATION, *PSYS_CALLBACK_ENTRY_INFORMATION;

	typedef struct _SYS_CALLBACK_INFORMATION
	{
		UINT_PTR                       NumberOfCallbacks;
		SYS_CALLBACK_ENTRY_INFORMATION CallbackEntry[1];
	} SYS_CALLBACK_INFORMATION, *PSYS_CALLBACK_INFORMATION;


	class CSystemCallback
	{
	public:
		CSystemCallback(class CGlobal *GlobalObject);
		~CSystemCallback();

		void InitializeCallbackList(CListCtrl * ListCtrl);

		BOOL EnumSystemCallback();

		void InsertSystemCallbackInfoList(CListCtrl * ListCtrl);

		void QuerySystemCallback(CListCtrl * ListCtrl);

		static DWORD CALLBACK QuerySystemCallbackCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 5;
		COLUMN_STRUCT m_ColumnStruct[5] = {
			{ L"回调地址",				125 },
			{ L"回调类型",				125 },
			{ L"模块文件",				225 },
			{ L"出品厂商",				125 },
			{ L"描述",					125 } };

		std::vector<SYS_CALLBACK_ENTRY_INFORMATION> m_CallbackEntryVector;
		
		UINT32   m_NotifyCreateProcess = 0;
		UINT32   m_NotifyCreateThread = 0;
		UINT32   m_NotifyLoadImage = 0;
		UINT32   m_NotifyCmpCallback = 0;
		UINT32   m_NotifyCheck = 0;
		UINT32   m_NotifyCheckReason = 0;
		UINT32   m_NotifyShutdown = 0;
		UINT32   m_NotifyLastChanceShutdown = 0;

		class CGlobal          *m_Global;

		class CDriverCore      &m_DriverCore;

		static CSystemCallback *m_SystemCallback;

	};


}
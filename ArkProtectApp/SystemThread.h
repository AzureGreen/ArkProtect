#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{


	class CSystemThread
	{
	public:
		CSystemThread(class CGlobal *GlobalObject);
		~CSystemThread();




	private:
		int           m_iColumnCount = 9;
		COLUMN_STRUCT m_ColumnStruct[9] = {
			{ L"线程ID",				50 },
			{ L"线程对象",				125 },
			{ L"Peb",					35 },
			{ L"优先级",				55 },
			{ L"线程入口",				125 },
			{ L"切换次数",				65 },
			{ L"线程状态",				100 },
			{ L"模块文件",				190 },
			{ L"出品厂商",				125 } };

//		std::vector<SYS_CALLBACK_ENTRY_INFORMATION> m_CallbackEntryVector;

		class CGlobal          *m_Global;
	};

}


#pragma once
#include <vector>
#include <strsafe.h>
#include "Define.h"


namespace ArkProtect
{

	enum eRegistryColumn
	{
		rc_Name,     // 名称
		rc_Type,     // 类型
		rc_Data      // 数据
	};


	class CRegistryCore
	{
	public:
		CRegistryCore(class CGlobal *GlobalObject);
		~CRegistryCore();

		void InitializeRegistryTree(CTreeCtrl * RegistryTree);

		void InitializeRegistryList(CListCtrl * RegistryList);





	private:
		int           m_iListColumnCount = 3;		
		COLUMN_STRUCT m_ListColumnStruct[3] = {
			{ L"名称",			200 },
			{ L"类型",			130 },
			{ L"数据",			590 }};


		class CGlobal       *m_Global;

	};

}

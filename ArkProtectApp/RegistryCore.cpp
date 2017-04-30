#include "stdafx.h"
#include "RegistryCore.h"
#include "Global.hpp"
#include "resource.h"

namespace ArkProtect
{
	CRegistryCore::CRegistryCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
	}


	CRegistryCore::~CRegistryCore()
	{
	}


	/************************************************************************
	*  Name : InitializeRegistryTree
	*  Param: RegistryList           ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CRegistryCore::InitializeRegistryTree(CTreeCtrl *RegistryTree)
	{
		INT32 iStyle = GetWindowLong(RegistryTree->m_hWnd, GWL_STYLE);
		iStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
		SetWindowLong(RegistryTree->m_hWnd, GWL_STYLE, iStyle);

		HTREEITEM Computer = RegistryTree->InsertItem(L"我的电脑", 0, 0);
		HTREEITEM hTreeItem0 = RegistryTree->InsertItem(L"HKEY_CLASSES_ROOT", 1, 2, Computer, TVI_LAST);
		HTREEITEM hTreeItem1 = RegistryTree->InsertItem(L"HKEY_CURRENT_USER", 1, 2, Computer, TVI_LAST);
		HTREEITEM hTreeItem2 = RegistryTree->InsertItem(L"HKEY_LOCAL_MACHINE", 1, 2, Computer, TVI_LAST);
		HTREEITEM hTreeItem3 = RegistryTree->InsertItem(L"HKEY_USERS", 1, 2, Computer, TVI_LAST);
		HTREEITEM hTreeItem4 = RegistryTree->InsertItem(L"HKEY_CURRENT_CONFIG", 1, 2, Computer, TVI_LAST);

		RegistryTree->Expand(Computer, TVE_EXPAND);
	}


	/************************************************************************
	*  Name : InitializeRegistryList
	*  Param: RegistryList           ListControl控件
	*  Ret  : void
	*  初始化ListControl的信息
	************************************************************************/
	void CRegistryCore::InitializeRegistryList(CListCtrl *RegistryList)
	{
		RegistryList->SetExtendedStyle(LVS_EX_FULLROWSELECT);

		for (int i = 0; i < m_iListColumnCount; i++)
		{
			RegistryList->InsertColumn(i, m_ListColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ListColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}

}
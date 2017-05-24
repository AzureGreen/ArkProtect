#include "stdafx.h"
#include "FileCore.h"
#include "Global.hpp"


namespace ArkProtect
{
	CFileCore *CFileCore::m_File;

	CFileCore::CFileCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_File = this;
	}


	CFileCore::~CFileCore()
	{
	}


	/************************************************************************
	*  Name : DeleteFile
	*  Param: void
	*  Ret  : BOOL
	*  与驱动层通信，枚举进程及相关信息
	************************************************************************/
	BOOL CFileCore::DeleteFile(CString strFilePath)
	{
		if (strFilePath.GetBuffer() == NULL)
		{
			::MessageBox(NULL, L"Invalid file path", L"ArkProtect", MB_OK | MB_ICONERROR);
			return FALSE;
		}

		BOOL  bOk = FALSE;
		DWORD dwReturnLength = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_DELETEFILE,
			strFilePath.GetBuffer(),		// InputBuffer
			strFilePath.GetLength(),
			NULL,
			0,
			&dwReturnLength,
			NULL);
		if (bOk == FALSE)
		{
			::MessageBox(NULL, L"Delete file path failed", L"ArkProtect", MB_OK | MB_ICONERROR);
		}
		
		return bOk;
	}


	/************************************************************************
	*  Name : DeleteFileCallback
	*  Param: lParam （ListCtrl）
	*  Ret  : DWORD
	*  结束目标进程
	************************************************************************/
	DWORD CALLBACK CFileCore::DeleteFileCallback(LPARAM lParam)
	{
		CString strFilePath = *(CString*)lParam;

		Sleep(2000);    // 暂时通过这种方法，让那边结束进程或者卸载驱动先完成了，再进行文件删除!!

		m_File->m_Global->m_bIsRequestNow = TRUE;      // 置TRUE，当驱动还没有返回前，阻止其他与驱动通信的操作

		m_File->DeleteFile(strFilePath);

		m_File->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
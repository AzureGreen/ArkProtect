#pragma once
#include <Windows.h>
#include <strsafe.h>
#include <winsvc.h>     // 服务需要
#include "afxcmn.h"
#include "Define.h"
#pragma comment(lib, "Version.lib")      // GetFileVersionInfo 需要链接此库

namespace ArkProtect 
{
	typedef BOOL(WINAPI *pfnIsWow64Process) (HANDLE, PBOOL);

	class CGlobal
	{
	public:
		CGlobal() {};
		~CGlobal() {};

		//////////////////////////////////////////////////////////////////////////
		// 通用函数


		BOOL QueryOSBit()
		{
#if defined(_WIN64)
			return TRUE;  // 64位程序只在64bit系统中运行
#elif defined(_WIN32)
			// 32位程序在32/64位系统中运行。
			// 所以必须判断
			BOOL bIs64 = FALSE;
			pfnIsWow64Process fnIsWow64Process;

			fnIsWow64Process = (pfnIsWow64Process)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
			if (fnIsWow64Process != NULL)
			{
				return fnIsWow64Process(GetCurrentProcess(), &bIs64) && bIs64;
			}
			return FALSE;
#else
			return FALSE; // Win64不支持16位系统
#endif
		}


		BOOL LoadNtDriver(WCHAR *wzServiceName, WCHAR *wzDriverPath)
		{
			WCHAR wzDriverFullPath[MAX_PATH] = { 0 };
			GetFullPathName(wzDriverPath, MAX_PATH, wzDriverFullPath, NULL);
		
			// 建立到服务控制管理器的连接，并打开指定的数据库
			m_ManagerHandle = OpenSCManagerW(NULL,			// 指定计算机的名称 NULL ---> 连接带本地计算机的服务控制管理器
				NULL,										// 指定要打开的服务控制管理数据库的名称 NULL ---> SERVICES_ACTIVE_DATABASE数据库
				SC_MANAGER_ALL_ACCESS);						// 指定服务访问控制管理器的权限
			if (m_ManagerHandle == NULL)						// 返回指定的服务控制管理器数据库的句柄
			{
				return FALSE;
			}

			// 创建一个服务对象，并将其添加到指定的服务控制管理器数据库
			m_ServiceHandle = CreateServiceW(m_ManagerHandle,		// 服务控制管理程序维护的登记数据库的句柄
				wzServiceName,									// 服务名，用于创建登记数据库中的关键字
				wzServiceName,									// 服务名，用于用户界面标识服务
				SERVICE_ALL_ACCESS,								// 服务返回类型		服务权限
				SERVICE_KERNEL_DRIVER,							// 服务类型			（驱动服务程序）
				SERVICE_DEMAND_START,							// 服务何时启动		（由服务控制器SCM启动的服务）
				SERVICE_ERROR_NORMAL,							// 服务启动失败的严重程度
				wzDriverFullPath,								// 服务程序二进制文件路径
				NULL, NULL, NULL, NULL, NULL);
			if (m_ServiceHandle == NULL)							// 返回服务句柄
			{
				if (ERROR_SERVICE_EXISTS == GetLastError())		// 已存在相同的服务
				{
					// 那就打开服务
					m_ServiceHandle = OpenServiceW(m_ManagerHandle, wzServiceName, SERVICE_ALL_ACCESS);
					if (m_ServiceHandle == NULL)
					{
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}

			// 开启服务
			BOOL bOk = StartServiceW(m_ServiceHandle, 0, NULL);
			if (!bOk)
			{
				if ((GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
					|| GetLastError() == ERROR_IO_PENDING)
				{
					return FALSE;
				}
			}

			m_bDriverService = TRUE;

			return TRUE;
		}


		// 卸载驱动程序
		void UnloadNTDriver(WCHAR *wzServiceName)
		{
			if (m_ServiceHandle)
			{
				SERVICE_STATUS ServiceStatus;

				// 停止服务
				BOOL bOk = ControlService(m_ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);
				if (bOk && m_bDriverService)
				{
					// 删除服务
					bOk = DeleteService(m_ServiceHandle);
				}

				CloseServiceHandle(m_ServiceHandle);
			}		

			if (m_ManagerHandle)
			{
				CloseServiceHandle(m_ManagerHandle);
			}
		}


		void UpdateStatusBarTip(LPCWSTR wzBuffer)
		{
			::SendMessage(this->AppDlg->m_hWnd, sb_Tip, 0, (LPARAM)wzBuffer);
		}


		void UpdateStatusBarDetail(LPCWSTR wzBuffer)
		{
			::SendMessage(this->AppDlg->m_hWnd, sb_Tip, 0, (LPARAM)wzBuffer);
		}


		// 获得文件厂商

		CString GetFileCompanyName(CString strFilePath)
		{
			CString strCompanyName = 0;;

			if (strFilePath.IsEmpty())
			{
				return NULL;
			}

			// 不处理Idle System
			if (!strFilePath.CompareNoCase(L"Idle") || !strFilePath.CompareNoCase(L"System"))
			{
				return NULL;
			}

			struct LANGANDCODEPAGE {
				WORD wLanguage;
				WORD wCodePage;
			} *lpTranslate;

			LPWSTR lpstrFilename = strFilePath.GetBuffer();
			DWORD  dwHandle = 0;
			DWORD  dwVerInfoSize = GetFileVersionInfoSizeW(lpstrFilename, &dwHandle);

			if (dwVerInfoSize)
			{
				LPVOID Buffer = malloc(sizeof(UINT8) * dwVerInfoSize);

				if (Buffer)
				{
					if (GetFileVersionInfo(lpstrFilename, dwHandle, dwVerInfoSize, Buffer))
					{
						UINT cbTranslate = 0;

						if (VerQueryValue(Buffer, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
						{
							LPCWSTR lpwszBlock = 0;
							UINT    cbSizeBuf = 0;
							WCHAR   wzSubBlock[MAX_PATH] = { 0 };

							if ((cbTranslate / sizeof(struct LANGANDCODEPAGE)) > 0)
							{
								StringCchPrintf(wzSubBlock, sizeof(wzSubBlock) / sizeof(WCHAR),
									L"\\StringFileInfo\\%04x%04x\\CompanyName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
							}

							if (VerQueryValue(Buffer, wzSubBlock, (LPVOID*)&lpwszBlock, &cbSizeBuf))
							{
								WCHAR wzCompanyName[MAX_PATH] = { 0 };

								StringCchCopy(wzCompanyName, MAX_PATH / sizeof(WCHAR), (LPCWSTR)lpwszBlock);   // 将系统中内存的数据拷贝到我们自己内存当中
								strCompanyName = wzCompanyName;
							}
						}
					}
					free(Buffer);
				}
			}

			return strCompanyName;
		}


		CString GetLongPath(CString strFilePath)
		{
			if (strFilePath.Find(L'~') != -1)
			{
				WCHAR wzLongPath[MAX_PATH] = { 0 };
				DWORD dwReturn = GetLongPathName(strFilePath, wzLongPath, MAX_PATH);
				if (dwReturn < MAX_PATH && dwReturn != 0)
				{
					strFilePath = wzLongPath;
				}
			}

			return strFilePath;
		}


		CString TrimPath(WCHAR *wzFilePath)
		{
			CString strFilePath;

			// 比如："C:\\"
			if (wzFilePath[1] == ':' && wzFilePath[2] == '\\')
			{
				strFilePath = wzFilePath;
			}
			else if (wcslen(wzFilePath) > wcslen(L"\\SystemRoot\\") &&
				!_wcsnicmp(wzFilePath, L"\\SystemRoot\\", wcslen(L"\\SystemRoot\\")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				GetWindowsDirectory(wzSystemDirectory, MAX_PATH);
				strFilePath.Format(L"%s\\%s", wzSystemDirectory, wzFilePath + wcslen(L"\\SystemRoot\\"));
			}
			else if (wcslen(wzFilePath) > wcslen(L"system32\\") &&
				!_wcsnicmp(wzFilePath, L"system32\\", wcslen(L"system32\\")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				GetWindowsDirectory(wzSystemDirectory, MAX_PATH);
				strFilePath.Format(L"%s\\%s", wzSystemDirectory, wzFilePath/* + wcslen(L"system32\\")*/);
			}
			else if (wcslen(wzFilePath) > wcslen(L"\\??\\") &&
				!_wcsnicmp(wzFilePath, L"\\??\\", wcslen(L"\\??\\")))
			{
				strFilePath = wzFilePath + wcslen(L"\\??\\");
			}
			else if (wcslen(wzFilePath) > wcslen(L"%ProgramFiles%") &&
				!_wcsnicmp(wzFilePath, L"%ProgramFiles%", wcslen(L"%ProgramFiles%")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				if (GetWindowsDirectory(wzSystemDirectory, MAX_PATH) != 0)
				{
					strFilePath = wzSystemDirectory;
					strFilePath = strFilePath.Left(strFilePath.Find('\\'));
					strFilePath += L"\\Program Files";
					strFilePath += wzFilePath + wcslen(L"%ProgramFiles%");
				}
			}
			else
			{
				strFilePath = wzFilePath;
			}

			strFilePath = GetLongPath(strFilePath);

			return strFilePath;
		}



		//////////////////////////////////////////////////////////////////////////


		CWnd *AppDlg = NULL;         // 保存主窗口指针
		CWnd *ProcessDlg = NULL;     // 保存进程模块窗口指针


		int iDpix = 0;               // Logical pixels/inch in X
		int iDpiy = 0;               // Logical pixels/inch in Y

		int iResizeX = 0;
		int iResizeY = 0;

		BOOL      m_bIsRequestNow = FALSE;    // 当前是否在向驱动层发送请求
		HANDLE    m_DeviceHandle = NULL;    // 我们的驱动设备对象句柄
		SC_HANDLE m_ManagerHandle = NULL;	// SCM管理器的句柄
		SC_HANDLE m_ServiceHandle = NULL;	// NT驱动程序的服务句柄
		BOOL      m_bDriverService = FALSE; // 指示加载驱动服务是否开启了
	};
}


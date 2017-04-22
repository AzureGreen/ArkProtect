#ifndef CXX_ProcessWindow_H
#define CXX_ProcessWindow_H

#include <ntifs.h>
#include "Private.h"
#include "ProcessCore.h"

typedef struct _PROCESS_WINDOW_ENTRY_INFORMATION
{
	HWND   hWnd;
	WCHAR  wzWindowText;
	WCHAR  wzWindowClass;
	BOOL   bVisibal;
	UINT32 ProcessId;
	UINT32 ThreadId;
} PROCESS_WINDOW_ENTRY_INFORMATION, *PPROCESS_WINDOW_ENTRY_INFORMATION;

typedef struct _PROCESS_WINDOW_INFORMATION
{
	UINT32                            NumberOfWindows;
	PROCESS_WINDOW_ENTRY_INFORMATION  WindowEntry[1];
} PROCESS_WINDOW_INFORMATION, *PPROCESS_WINDOW_INFORMATION;



NTSTATUS 
APEnumProcessWindowByNtUserBuildHwndList(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_WINDOW_INFORMATION pwi, IN UINT32 WindowCount);

NTSTATUS
APEnumProcessWindow(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength);



#endif // !CXX_ProcessWindow_H



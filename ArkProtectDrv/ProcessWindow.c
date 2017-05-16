#include "ProcessWindow.h"

typedef
NTSTATUS
(*pfnNtUserBuildHwndList)(
	IN HDESK hdesk,
	IN HWND hwndNext,
	IN BOOL fEnumChildren,
	IN DWORD idThread,
	IN UINT cHwndMax,
	OUT HWND *phwndFirst,
	OUT PUINT pcHwndNeeded);

typedef
//W32KAPI
HANDLE
(*pfnNtUserQueryWindow)(
	IN HWND hwnd,
	IN DWORD WindowInfo);


NTSTATUS
APEnumProcessWindowByNtUserBuildHwndList(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_WINDOW_INFORMATION pwi, IN UINT32 WindowCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	
	// 获得 NtUserBuildHwndList 地址
	pfnNtUserBuildHwndList NtUserBuildHwndList = (pfnNtUserBuildHwndList)APGetSssdtFunctionAddress(L"NtUserBuildHwndList");
	pfnNtUserQueryWindow   NtUserQueryWindow = (pfnNtUserQueryWindow)APGetSssdtFunctionAddress(L"NtUserQueryWindow");
	if (NtUserBuildHwndList && MmIsAddressValid((PVOID)NtUserBuildHwndList) &&
		NtUserQueryWindow && MmIsAddressValid((PVOID)NtUserQueryWindow))
	{
		Status = NtUserBuildHwndList(NULL, NULL, FALSE, 0, WindowCount, (HWND*)(pwi->WindowEntry), &pwi->NumberOfWindows);
		if (NT_SUCCESS(Status))
		{
			HWND* Wnds = (HWND*)ExAllocatePool(NonPagedPool, sizeof(HWND) * pwi->NumberOfWindows);
			if (Wnds)
			{
				UINT32 NumberOfWindows = 0;
				RtlCopyMemory(Wnds, &pwi->WindowEntry, sizeof(HWND) * pwi->NumberOfWindows);

				RtlZeroMemory(&pwi->WindowEntry, sizeof(HWND) * pwi->NumberOfWindows);

				for (UINT32 i = 0; i < pwi->NumberOfWindows; i++)
				{
					UINT32 ThreadId = 0, WndProcessId = 0;

					WndProcessId = (UINT32)(UINT_PTR)NtUserQueryWindow(Wnds[i], 0);

					ThreadId = (UINT32)(UINT_PTR)NtUserQueryWindow(Wnds[i], 2);
					
					if (WndProcessId == ProcessId)
					{
						if (WindowCount >= NumberOfWindows)
						{
							pwi->WindowEntry[NumberOfWindows].hWnd = Wnds[i];
							pwi->WindowEntry[NumberOfWindows].ProcessId = ProcessId;
							pwi->WindowEntry[NumberOfWindows].ThreadId = ThreadId;
						}
						NumberOfWindows++;
					}			
				}
				ExFreePool(Wnds);
				pwi->NumberOfWindows = NumberOfWindows;
			}
		}
	}
	else
	{
		DbgPrint("Get NtUserBuildHwndList Failed\r\n");
	}
	return Status;
}



/************************************************************************
*  Name : APEnumProcessWindow
*  Param: ProcessId				进程Id				 （IN）
*  Param: OutputBuffer  		Ring3层需要的内存信息（OUT）
*  Param: OutputLength			Ring3层传递的返出长度（IN）
*  Ret  : NTSTATUS
*  枚举目标进程的句柄信息，存入Ring3提供结构体
************************************************************************/
NTSTATUS
APEnumProcessWindow(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    WindowCount = (OutputLength - sizeof(PROCESS_WINDOW_INFORMATION)) / sizeof(PROCESS_WINDOW_ENTRY_INFORMATION);
	PEPROCESS EProcess = NULL;

	if (ProcessId == 0)
	{
		return Status;
	}
	else
	{
		Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &EProcess);
	}

	if (NT_SUCCESS(Status) && APIsValidProcess(EProcess))
	{
		PPROCESS_WINDOW_INFORMATION pwi = (PPROCESS_WINDOW_INFORMATION)OutputBuffer;
		if (pwi)
		{
			Status = APEnumProcessWindowByNtUserBuildHwndList(ProcessId, EProcess, pwi, WindowCount);
			if (NT_SUCCESS(Status))
			{
				if (WindowCount >= pwi->NumberOfWindows)
				{
					Status = STATUS_SUCCESS;
				}
				else
				{
					Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
				}
			}
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}







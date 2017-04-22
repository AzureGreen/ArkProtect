#include "ProcessWindow.h"


NTSTATUS
APEnumProcessWindowByNtUserBuildHwndList(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_WINDOW_INFORMATION pwi, IN UINT32 WindowCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	/*
	Status = NtUserBuildHwndList(NULL, NULL, FALSE, 0, WindowCount, (HWND*)(pwi->WindowEntry), &pwi->NumberOfWindows);
	if (NT_SUCCESS(Status))
	{
		UINT32 Count = pwi->NumberOfWindows;
		ULONG i = 0;
		HWND* WndBuffer = (HWND*)ExAllocatePool(NonPagedPool, sizeof(HWND) * Count);
		if (WndBuffer)
		{
			//	memcpy(WndBuffer, (PVOID)((ULONG)pwi + sizeof(UINT32)), sizeof(HWND) * Count);

			for (i = 0; i < Count; i++)
			{
				UINT32 ThreadId = 0, ProcessId = 0;
				HWND hWnd = WndBuffer[i];

				ProcessId = NtUserQueryWindow(hWnd, 0);

				ThreadId = NtUserQueryWindow(hWnd, 2);

				pwi->Wnds[i].hWnd = hWnd;
				pwi->Wnds[i].ProcessId = ProcessId;
				pwi->Wnds[i].ThreadId = ThreadId;
			}
			ExFreePool(WndBuffer);
		}
	}
	*/

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
		PPROCESS_WINDOW_INFORMATION pwi = (PPROCESS_WINDOW_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (pwi)
		{
			RtlZeroMemory(pwi, OutputLength);

			Status = APEnumProcessWindowByNtUserBuildHwndList(ProcessId, EProcess, pwi, WindowCount);
			if (NT_SUCCESS(Status))
			{
				if (WindowCount >= pwi->NumberOfWindows)
				{
					RtlCopyMemory(OutputBuffer, pwi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_WINDOW_INFORMATION)OutputBuffer)->NumberOfWindows = pwi->NumberOfWindows;    // 让Ring3知道需要多少个
					Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
				}
			}

			ExFreePool(pwi);
			pwi = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}







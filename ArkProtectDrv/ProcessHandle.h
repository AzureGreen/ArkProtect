#ifndef CXX_ProcessHandle_H
#define CXX_ProcessHandle_H

#include <ntifs.h>
#include <strsafe.h>
#include "Private.h"
#include "ProcessCore.h"



typedef struct _PROCESS_HANDLE_ENTRY_INFORMATION
{
	HANDLE		Handle;
	PVOID		Object;
	UINT32		ReferenceCount;		    // 引用计数
	WCHAR		wzHandleType[MAX_PATH];
	WCHAR		wzHandleName[MAX_PATH];
} PROCESS_HANDLE_ENTRY_INFORMATION, *PPROCESS_HANDLE_ENTRY_INFORMATION;

typedef struct _PROCESS_HANDLE_INFORMATION
{
	UINT32								NumberOfHandles;
	PROCESS_HANDLE_ENTRY_INFORMATION	HandleEntry[1];
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;



VOID 
APGetHandleType(IN HANDLE Handle, OUT PWCHAR wzHandleType);

VOID
APGetHandleName(IN HANDLE Handle, OUT PWCHAR wzHandleName);

VOID 
APGetProcessHandleInfo(IN PEPROCESS EProcess, IN HANDLE Handle, IN PVOID Object, OUT PPROCESS_HANDLE_INFORMATION phi);

NTSTATUS 
APEnumProcessHandleByZwQuerySystemInformation(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_HANDLE_INFORMATION phi, IN UINT32 HandleCount);

NTSTATUS 
APEnumProcessHandle(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength);



#endif // !CXX_ProcessHandle_H



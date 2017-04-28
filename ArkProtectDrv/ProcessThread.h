#ifndef CXX_ProcessThread_H
#define CXX_ProcessThread_H

#include <ntifs.h>
#include <strsafe.h>
#include "Private.h"
#include "ProcessCore.h"


typedef struct _PROCESS_THREAD_ENTRY_INFORMATION
{
	UINT_PTR EThread;
	UINT32   ThreadId;
	UINT_PTR Teb;
	UINT8    Priority;
	UINT_PTR Win32StartAddress;
	UINT32   ContextSwitches;
	UINT8    State;
} PROCESS_THREAD_ENTRY_INFORMATION, *PPROCESS_THREAD_ENTRY_INFORMATION;


typedef struct _PROCESS_THREAD_INFORMATION
{
	UINT32                           NumberOfThreads;
	PROCESS_THREAD_ENTRY_INFORMATION ThreadEntry[1];
} PROCESS_THREAD_INFORMATION, *PPROCESS_THREAD_INFORMATION;


BOOLEAN 
APIsThreadInList(IN PETHREAD EThread, IN PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount);

UINT_PTR 
APGetThreadStartAddress(IN PETHREAD EThread);

VOID 
APGetProcessThreadInfo(IN PETHREAD EThread, IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount);

NTSTATUS
APEnumProcessThreadByTravelThreadListHead(IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount);

NTSTATUS 
APEnumProcessThread(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength);

UINT_PTR 
GetPspTerminateThreadByPointerAddress();

NTSTATUS 
APTerminateProcessByTravelThreadListHead(IN PEPROCESS EProcess);



#endif // !CXX_ProcessThread_H


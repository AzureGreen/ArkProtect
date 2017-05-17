#ifndef CXX_ProcessCore_H
#define CXX_ProcessCore_H

#include <ntifs.h>
#include "Private.h"
#include "Imports.h"
#include "NtStructs.h"
#include "PspCidTable.h"
#include "ProcessThread.h"



#define MAX_PROCESS_COUNT  100000

typedef struct _PROCESS_ENTRY_INFORMATION
{
	WCHAR     wzImageName[100];
	UINT32	  ProcessId;
	UINT32	  ParentProcessId;
	WCHAR     wzFilePath[MAX_PATH];
	UINT_PTR  EProcess;
	BOOL      bUserAccess;
	WCHAR     wzCompanyName[MAX_PATH];
} PROCESS_ENTRY_INFORMATION, *PPROCESS_ENTRY_INFORMATION;

typedef struct _PROCESS_INFORMATION
{
	UINT32                    NumberOfProcesses;
	PROCESS_ENTRY_INFORMATION ProcessEntry[1];
} PROCESS_INFORMATION, *PPROCESS_INFORMATION;

UINT_PTR
APGetPsIdleProcess();

UINT_PTR
APGetObjectType(IN PVOID Object);

BOOLEAN
APIsActiveProcess(IN PEPROCESS EProcess);

BOOLEAN
APIsValidProcess(IN PEPROCESS EProcess);

NTSTATUS
APGetProcessNum(OUT PVOID OutputBuffer);

UINT_PTR
APGetParentProcessId(IN PEPROCESS EProcess);

NTSTATUS
APGetProcessFullPath(IN PEPROCESS EProcess, OUT PWCHAR ProcessFullPath);

VOID 
APGetProcessInfo(IN PEPROCESS EProcess, OUT PPROCESS_INFORMATION pi, IN UINT32 ProcessCount);

VOID 
APEnumProcessInfoByIterateFirstLevelHandleTable(IN UINT_PTR TableCode, OUT PPROCESS_INFORMATION pi, IN UINT32 ProcessCount);

VOID 
APEnumProcessInfoByIterateSecondLevelHandleTable(IN UINT_PTR TableCode, OUT PPROCESS_INFORMATION pi, IN UINT32 ProcessCount);

VOID
APEnumProcessInfoByIterateThirdLevelHandleTable(IN UINT_PTR TableCode, OUT PPROCESS_INFORMATION pi, IN UINT32 ProcessCount);

NTSTATUS 
APEnumProcessInfoByIteratePspCidTable(OUT PPROCESS_INFORMATION pi, IN UINT32 ProcessCount);

NTSTATUS
APEnumProcessInfo(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

NTSTATUS 
APTerminateProcess(IN UINT32 ProcessId);

PEPROCESS 
APGetGuiProcess();


#endif // !CXX_ProcessCore_H




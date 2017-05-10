#ifndef CXX_Sssdt_H
#define CXX_Sssdt_H

#include <ntifs.h>
#include "Private.h"
#include "NtStructs.h"
#include "PeLoader.h"
#include "DriverCore.h"
#include "ProcessCore.h"

typedef struct _SSSDT_HOOK_ENTRY_INFORMATION
{
	UINT32	    Ordinal;
	BOOL        bHooked;
	UINT_PTR	CurrentAddress;
	UINT_PTR	OriginalAddress;
	WCHAR	    wzFunctionName[100];
} SSSDT_HOOK_ENTRY_INFORMATION, *PSSSDT_HOOK_ENTRY_INFORMATION;

typedef struct _SSSDT_HOOK_INFORMATION
{
	UINT32                         NumberOfSssdtFunctions;
	SSSDT_HOOK_ENTRY_INFORMATION   SssdtHookEntry[1];
} SSSDT_HOOK_INFORMATION, *PSSSDT_HOOK_INFORMATION;



UINT_PTR 
APGetCurrentSssdtAddress();

UINT_PTR 
APGetCurrentWin32pServiceTable();

VOID 
APFixWin32pServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase);

NTSTATUS
APReloadWin32k();

NTSTATUS 
APEnumSssdtHookByReloadWin32k(OUT PSSSDT_HOOK_INFORMATION shi, IN UINT32 SssdtFunctionCount);

NTSTATUS
APEnumSssdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

NTSTATUS 
APResumeSssdtHook(IN UINT32 Ordinal);

#endif // !CXX_Sssdt_H



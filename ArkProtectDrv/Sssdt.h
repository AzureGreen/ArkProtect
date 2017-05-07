#ifndef CXX_Sssdt_H
#define CXX_Sssdt_H

#include <ntifs.h>
#include "Private.h"

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



NTSTATUS
APEnumSssdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

#endif // !CXX_Sssdt_H



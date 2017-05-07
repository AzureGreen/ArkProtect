#ifndef CXX_Ssdt_H
#define CXX_Ssdt_H

#include <ntifs.h>
#include <ntimage.h>
#include <ntstrsafe.h>
#include "Private.h"
#include "PeLoader.h"

typedef struct _SSDT_HOOK_ENTRY_INFORMATION
{
	UINT32	    Ordinal;
	BOOL        bHooked;
	UINT_PTR	CurrentAddress;
	UINT_PTR	OriginalAddress;
	WCHAR	    wzFunctionName[100];
} SSDT_HOOK_ENTRY_INFORMATION, *PSSDT_HOOK_ENTRY_INFORMATION;

typedef struct _SSDT_HOOK_INFORMATION
{
	UINT32                        NumberOfSsdtFunctions;
	SSDT_HOOK_ENTRY_INFORMATION   SsdtHookEntry[1];
} SSDT_HOOK_INFORMATION, *PSSDT_HOOK_INFORMATION;


UINT_PTR
APGetCurrentSsdtAddress();

NTSTATUS
APInitializeSsdtFunctionName();

VOID 
APFixKiServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase);

NTSTATUS 
APReloadNtkrnl();

NTSTATUS
APEnumSsdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

NTSTATUS 
APResumeSsdtHook(IN UINT32 Ordinal);

#endif // !CXX_Ssdt_H



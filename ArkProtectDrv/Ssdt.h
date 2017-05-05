#ifndef CXX_Ssdt_H
#define CXX_Ssdt_H

#include <ntifs.h>
#include <ntimage.h>
#include <ntstrsafe.h>
#include "Private.h"

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
APMappingFileInKernelSpace(IN WCHAR * wzFileFullPath, OUT PVOID * MappingBaseAddress);

NTSTATUS
APInitializeSsdtFunctionName();

PVOID 
APGetFileBuffer(IN PUNICODE_STRING uniFilePath);

PVOID 
APGetModuleHandle(IN PCHAR szModuleName);

PVOID 
APGetProcAddress(IN PVOID ModuleBase, IN PCHAR szFunctionName);

VOID
APFixImportAddressTable(IN PVOID ImageBase);

VOID
APFixRelocBaseTable(IN PVOID ImageBase, IN PVOID OriginalBase);

NTSTATUS
APEnumSsdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

#endif // !CXX_Ssdt_H



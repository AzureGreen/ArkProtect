#ifndef CXX_Private_H
#define CXX_Private_H

#include <ntifs.h>
#include <windef.h>
#include <ntstrsafe.h>
#include "NtStructs.h"
#include "Imports.h"

//#define MAX_PATH 260
#define MAX_DOS_DRIVES 26
#define RESUME_ALL_HOOKS 8080

typedef enum _eWinVersion {
	WINVER_7 = 0x0610,
	WINVER_7_SP1 = 0x0611,
	WINVER_8 = 0x0620,
	WINVER_81 = 0x0630,
	WINVER_10 = 0x0A00,
	WINVER_10_AU = 0x0A01,
} eWinVersion;

typedef struct _DYNAMIC_DATA
{
	eWinVersion	WinVersion;

	//////////////////////////////////////////////////////////////////////////
	//
	// Process
	//

	UINT32      ThreadListHead_KPROCESS;    // KPROCESS::ThreadListHead

	UINT32		ObjectTable;				// EPROCESS::ObjectTable

	UINT32		SectionObject;				// EPROCESS::SectionObject

	UINT32		InheritedFromUniqueProcessId;// EPROCESS::InheritedFromUniqueProcessId

	UINT32      ThreadListHead_EPROCESS;      // EPROCESS::ThreadListHead

	//////////////////////////////////////////////////////////////////////////
	//
	// Thread
	//

	UINT32      Priority;                   // KTHREAD::Priority

	UINT32      Teb;                        // KTHREAD::Teb

	UINT32      ContextSwitches;            // KTHREAD::ContextSwitches

	UINT32      State;                      // KTHREAD::State

	UINT32		PreviousMode;				// KTHREAD::PreviousMode

	UINT32      Process;                    // KTHREAD::Process	

	UINT32      ThreadListEntry_KTHREAD;    // KTHREAD::ThreadListEntry

	UINT32      StartAddress;               // ETHREAD::StartAddress

	UINT32      Cid;                        // ETHREAD::Cid

	UINT32      Win32StartAddress;          // ETHREAD::Win32StartAddress

	UINT32      ThreadListEntry_ETHREAD;    // ETHREAD::ThreadListEntry

	UINT32      SameThreadApcFlags;         // ETHREAD::SameThreadApcFlags

	//////////////////////////////////////////////////////////////////////////

	UINT32		SizeOfObjectHeader;			// Size of ObjectHeader;

	UINT32		HandleTableEntryOffset;		// HandleTableEntry Offset To TableCode

	//////////////////////////////////////////////////////////////////////////

	UINT_PTR	MaxUserSpaceAddress;		// Max Address Of Ring3 Can Visit

	UINT_PTR    MinKernelSpaceAddress;		// Min Address Of Kernel Space

	//////////////////////////////////////////////////////////////////////////

	UINT32		NtQueryVirtualMemoryIndex;	// NtQueryVirtualMemory Index In SSDT

	UINT32      NtOpenDirectoryObjectIndex; // NtOpenDirectoryObject Index In SSDT

	UINT32		NtProtectVirtualMemoryIndex; // NtProtectVirtualMemory Index In SSDT

	UINT32		NtReadVirtualMemoryIndex;	// NtReadVirtualMemory Index In SSDT

	UINT32		NtWriteVirtualMemoryIndex;	// NtWriteVirtualMemory Index In SSDT

	UINT32      NtUserBuildHwndListIndex;   // NtUserBuildHwndList Index In SSSDT

	UINT32      NtUserQueryWindowIndex;     // NtUserQueryWindow Index In SSSDT

} DYNAMIC_DATA, *PDYNAMIC_DATA;


BOOLEAN
APGetNtosExportVariableAddress(IN const WCHAR *wzVariableName, OUT PVOID *VariableAddress);

BOOLEAN 
APIsUnicodeStringValid(IN PUNICODE_STRING uniString);

VOID 
APCharToWchar(IN CHAR * szString, OUT WCHAR * wzString);

VOID 
APPageProtectOff();

VOID 
APPageProtectOn();

BOOLEAN 
APDosPathToNtPath(IN WCHAR * wzDosFullPath, OUT WCHAR * wzNtFullPath);

UINT32
APQueryDosDevice(WCHAR *DeviceName, WCHAR *TargetPath, UINT32 MaximumLength);

#endif // !CXX_Private_H

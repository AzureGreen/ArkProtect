#ifndef CXX_Private_H

#include <ntifs.h>

#define MAX_PATH 260

typedef enum _eWinVersion {
	WINVER_7     = 0x0610,
	WINVER_7_SP1 = 0x0611,
	WINVER_8     = 0x0620,
	WINVER_81    = 0x0630,
	WINVER_10    = 0x0A00,
	WINVER_10_AU = 0x0A01,
} eWinVersion;

typedef struct _DYNAMIC_DATA
{
	eWinVersion	WinVersion;					// ϵͳ�汾

	//////////////////////////////////////////////////////////////////////////
	// Process

	UINT32      ThreadListHead_KPROCESS;    // KPROCESS::ThreadListHead

	UINT32		ObjectTable;				// EPROCESS::ObjectTable

	UINT32		SectionObject;				// EPROCESS::SectionObject

	UINT32		InheritedFromUniqueProcessId;// EPROCESS::InheritedFromUniqueProcessId

	UINT32      ThreadListHead_EPROCESS;      // EPROCESS::ThreadListHead

	//////////////////////////////////////////////////////////////////////////
	// Thread

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

	//////////////////////////////////////////////////////////////////////////

	UINT_PTR	UserEndAddress;				// Max Address Of Ring3 Can Visit

	UINT32		NtQueryVirtualMemoryIndex;	// NtQueryVirtualMemory Index In SSDT

	//////////////////////////////////////////////////////////////////////////

	UINT_PTR    KernelStartAddress;			// Start Address Of System

	UINT32		HandleTableEntryOffset;		// HandleTableEntry Offset To TableCode

	UINT32      NtOpenDirectoryObjectIndex; // NtOpenDirectoryObject Index In SSDT

	UINT32		NtProtectVirtualMemoryIndex; // NtProtectVirtualMemory Index In SSDT

	UINT32		NtReadVirtualMemoryIndex;	// NtReadVirtualMemory Index In SSDT

	UINT32		NtWriteVirtualMemoryIndex;	// NtWriteVirtualMemory Index In SSDT

	UINT32      NtUserBuildHwndListIndex;   // NtUserBuildHwndList Index In SSSDT

	UINT32      NtUserQueryWindowIndex;     // NtUserQueryWindow Index In SSSDT

} DYNAMIC_DATA, *PDYNAMIC_DATA;


BOOLEAN
APGetNtosExportVariableAddress(IN const WCHAR *wzVariableName, OUT PVOID *VariableAddress);

#endif // !CXX_Private_H

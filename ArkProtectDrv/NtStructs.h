#ifndef CXX_NtStructs_H
#define CXX_NtStructs_H
#include <ntifs.h>


//////////////////////////////////////////////////////////////////////////
// Section

typedef struct _CONTROL_AREA
{
	struct _SEGMENT*	Segment;
	LIST_ENTRY			DereferenceList;
	UINT_PTR			NumberOfSectionReferences;
	UINT_PTR			NumberOfPfnReferences;
	UINT_PTR			NumberOfMappedViews;
	UINT_PTR			NumberOfUserReferences;
	UINT32				u;
	UINT32				FlushInProgressCount;
	PFILE_OBJECT		FilePointer;
	UINT32				ControlAreaLock;
	UINT32				ModifiedWriteCount;
	UINT_PTR			WaitingForDeletion;
	UINT_PTR			u2;
	UINT64				Padding;
	UINT_PTR			LockedPages;
	LIST_ENTRY			ViewList;
} CONTROL_AREA, *PCONTROL_AREA;


typedef struct _SEGMENT
{
	PCONTROL_AREA	ControlArea;
	UINT32			TotalNumberOfPtes;
	UINT32			SegmentFlags;
	UINT_PTR		NumberOfCommittedPages;
	UINT64			SizeOfSegment;
	union
	{
		UINT_PTR ExtendInfo;
		UINT_PTR BasedAddress;
	} u;
	UINT_PTR	SegmentLock;
	UINT_PTR	u1;
	UINT_PTR	u2;
	UINT_PTR	PrototypePte;
#ifndef _WIN64
	UINT32		Padding;
#endif // !_WIN64
	UINT_PTR	ThePtes;
} SEGMENT, *PSEGMENT;


typedef struct _SECTION_OBJECT
{
	PVOID	 StartingVa;
	PVOID	 EndingVa;
	PVOID	 Parent;
	PVOID	 LeftChild;
	PVOID	 RightChild;
	PSEGMENT Segment;
} SECTION_OBJECT, *PSECTION_OBJECT;


//////////////////////////////////////////////////////////////////////////
// Peb
typedef struct _PEB_LDR_DATA32
{
	UINT32			Length;
	UINT8			Initialized;
	UINT32			SsHandle;
	LIST_ENTRY32	InLoadOrderModuleList;
	LIST_ENTRY32	InMemoryOrderModuleList;
	LIST_ENTRY32	InInitializationOrderModuleList;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
	LIST_ENTRY32	 InLoadOrderLinks;
	LIST_ENTRY32	 InMemoryOrderLinks;
	LIST_ENTRY32	 InInitializationOrderLinks;
	UINT32			 DllBase;
	UINT32			 EntryPoint;
	UINT32			 SizeOfImage;
	UNICODE_STRING32 FullDllName;
	UNICODE_STRING32 BaseDllName;
	UINT32			 Flags;
	UINT16			 LoadCount;
	UINT16			 TlsIndex;
	LIST_ENTRY32	 HashLinks;
	UINT32			 TimeDateStamp;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

typedef struct _PEB32
{
	UINT8 InheritedAddressSpace;
	UINT8 ReadImageFileExecOptions;
	UINT8 BeingDebugged;
	UINT8 BitField;
	UINT32 Mutant;
	UINT32 ImageBaseAddress;
	UINT32 Ldr;
	UINT32 ProcessParameters;
	UINT32 SubSystemData;
	UINT32 ProcessHeap;
	UINT32 FastPebLock;
	UINT32 AtlThunkSListPtr;
	UINT32 IFEOKey;
	UINT32 CrossProcessFlags;
	UINT32 UserSharedInfoPtr;
	UINT32 SystemReserved;
	UINT32 AtlThunkSListPtr32;
	UINT32 ApiSetMap;
} PEB32, *PPEB32;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY		InLoadOrderLinks;
	LIST_ENTRY		InMemoryOrderLinks;
	LIST_ENTRY		InInitializationOrderLinks;
	PVOID			DllBase;
	PVOID			EntryPoint;
	UINT32			SizeOfImage;
	UNICODE_STRING  FullDllName;
	UNICODE_STRING  BaseDllName;
	UINT32			Flags;
	UINT16			LoadCount;
	UINT16			TlsIndex;
	LIST_ENTRY		HashLinks;
	UINT32			TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA
{
	UINT32 Length;
	UINT8 Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _PEB
{
	UINT8 InheritedAddressSpace;
	UINT8 ReadImageFileExecOptions;
	UINT8 BeingDebugged;
	UINT8 BitField;
	PVOID Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PVOID ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PVOID FastPebLock;
	PVOID AtlThunkSListPtr;
	PVOID IFEOKey;
	PVOID CrossProcessFlags;
	PVOID UserSharedInfoPtr;
	UINT32 SystemReserved;
	UINT32 AtlThunkSListPtr32;
	PVOID ApiSetMap;
} PEB, *PPEB;

//////////////////////////////////////////////////////////////////////////
//
#define SEC_IMAGE            0x1000000
#define MEM_IMAGE            SEC_IMAGE
/*
typedef enum _MEMORY_INFORMATION_CLASS
{
MemoryBasicInformation,
MemoryWorkingSetList,
MemorySectionName,
MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;
*/

#define MemorySectionName 2

//
// Memory Information Structures for NtQueryVirtualMemory
//
typedef struct _MEMORY_SECTION_NAME
{
	UNICODE_STRING SectionFileName;
	WCHAR NameBuffer[ANYSIZE_ARRAY];
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;


//
// ObjectDirectory Information Structures for NtQueryDirectoryObject
//
typedef struct _OBJECT_DIRECTORY_INFORMATION {
	UNICODE_STRING Name;
	UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;


#endif // !CXX_NtStructs_H

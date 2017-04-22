#ifndef CXX_ProcessMemory_H
#define CXX_ProcessMemory_H

#include <ntifs.h>
#include "Private.h"
#include "ProcessCore.h"


typedef struct _PROCESS_MEMORY_ENTRY_INFORMATION
{
	UINT_PTR	BaseAddress;
	UINT_PTR	RegionSize;
	UINT32		Protect;
	UINT32		State;
	UINT32		Type;
} PROCESS_MEMORY_ENTRY_INFORMATION, *PPROCESS_MEMORY_ENTRY_INFORMATION;

typedef struct _PROCESS_MEMORY_INFORMATION
{
	UINT32								NumberOfMemories;
	PROCESS_MEMORY_ENTRY_INFORMATION	MemoryEntry[1];
}PROCESS_MEMORY_INFORMATION, *PPROCESS_MEMORY_INFORMATION;


NTSTATUS
APEnumProcessMemoryByZwQueryVirtualMemory(IN PEPROCESS EProcess, OUT PPROCESS_MEMORY_INFORMATION pmi, IN UINT32 MemoryCount);

NTSTATUS 
APEnumProcessMemory(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength);



#endif // !CXX_ProcessMemory_H


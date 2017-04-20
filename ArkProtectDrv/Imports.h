#ifndef CXX_Imports_H

#include <ntifs.h>


NTKERNELAPI
UCHAR *
PsGetProcessImageFileName(PEPROCESS Process);

NTKERNELAPI
PPEB
PsGetProcessPeb(IN PEPROCESS Process);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWow64Process(IN PEPROCESS Process);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointer(
	IN PVOID Object,
	IN ULONG HandleAttributes,
	IN PACCESS_STATE PassedAccessState OPTIONAL,
	IN ACCESS_MASK DesiredAccess OPTIONAL,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	IN KPROCESSOR_MODE AccessMode,
	OUT PHANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
	__in HANDLE DirectoryHandle,
	__out_bcount_opt(Length) PVOID Buffer,
	__in ULONG Length,
	__in BOOLEAN ReturnSingleEntry,
	__in BOOLEAN RestartScan,
	__inout PULONG Context,
	__out_opt PULONG ReturnLength);


#endif // !CXX_Imports_H

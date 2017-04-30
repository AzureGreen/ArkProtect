#ifndef CXX_Imports_H

#include <ntifs.h>


//////////////////////////////////////////////////////////////////////////
// Undocument Import

extern
POBJECT_TYPE* IoDriverObjectType;		// 驱动对象类型

extern
POBJECT_TYPE* IoDeviceObjectType;       // 设备对象类型


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


NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN UINT32 SystemInformationLength,
	OUT PUINT32 ReturnLength OPTIONAL);


NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID *Object
);


#endif // !CXX_Imports_H

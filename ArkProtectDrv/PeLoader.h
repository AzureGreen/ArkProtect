#ifndef CXX_PeLoader_H
#define CXX_PeLoader_H

#include <ntifs.h>
#include <ntimage.h>
#include <ntstrsafe.h>
#include "NtStructs.h"

NTSTATUS
APMappingFileInKernelSpace(IN WCHAR * wzFileFullPath, OUT PVOID * MappingBaseAddress);

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


#endif // !CXX_PeLoader_H
#ifndef CXX_FileCore_H
#define CXX_FileCore_H

#include <ntifs.h>
#include "Private.h"


NTSTATUS
APIrpCompleteRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

NTSTATUS 
APDeleteFileByIrp(IN WCHAR * wzFilePath);

NTSTATUS 
APDeleteFile(IN WCHAR * wzFilePath);


#endif // !CXX_FileCore_H



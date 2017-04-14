#ifndef CXX_Imports_H

#include <ntifs.h>

PEPROCESS PsIdleProcess;     // Idle 的 EProcess，导出全局变量


NTKERNELAPI
UCHAR *
PsGetProcessImageFileName(
	__in PEPROCESS Process
);

#endif // !CXX_Imports_H

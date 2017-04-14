#include "Private.h"


/************************************************************************
*  Name : APGetNtosExportVariableAddress
*  Param: wzVariableName		目标变量名称   （双字）
*  Param: VariableAddress		目标变量地址 （OUT）
*  Ret  : BOOLEAN
*  通过全局变量（函数地址）名称返回Ntos导出表中全局变量（函数地址）地址，这里用于 x86下获得SSDT地址
************************************************************************/
BOOLEAN
APGetNtosExportVariableAddress(IN const WCHAR *wzVariableName, OUT PVOID *VariableAddress)
{
	UNICODE_STRING	uniVariableName = { 0 };

	if (wzVariableName && wcslen(wzVariableName) > 0)
	{
		RtlInitUnicodeString(&uniVariableName, wzVariableName);

		//从Ntoskrnl模块的导出表中获得一个导出变量的地址
		*VariableAddress = MmGetSystemRoutineAddress(&uniVariableName);		// 函数返回值是PVOID，才产生了二维指针
	}

	if (*VariableAddress == NULL)
	{
		return FALSE;
	}

	return TRUE;
}
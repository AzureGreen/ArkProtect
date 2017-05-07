#include "Sssdt.h"






/************************************************************************
*  Name : APEnumSssdtHook
*  Param: OutputBuffer            ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举进程模块
************************************************************************/
NTSTATUS
APEnumSssdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    SssdtFunctionCount = (OutputLength - sizeof(SSSDT_HOOK_INFORMATION)) / sizeof(SSSDT_HOOK_ENTRY_INFORMATION);

	PSSSDT_HOOK_INFORMATION shi = (PSSSDT_HOOK_INFORMATION)OutputBuffer;

/*	// 1.获得当前的SSDT
	g_CurrentSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentSsdtAddress();
	if (g_CurrentSsdtAddress && MmIsAddressValid(g_CurrentSsdtAddress))
	{
		// 2.初始化Ssdt函数名称
		Status = APInitializeSsdtFunctionName();
		if (NT_SUCCESS(Status))
		{
			// 3.重载内核SSDT(得到原先的SSDT函数地址数组)
			Status = APReloadNtkrnl();
			if (NT_SUCCESS(Status))
			{
				// 4.对比Original&Current
				for (UINT32 i = 0; i < g_CurrentSsdtAddress->Limit; i++)
				{
					if (SsdtFunctionCount >= shi->NumberOfSsdtFunctions)
					{
#ifdef _WIN64
						// 64位存储的是 偏移（高28位）
						INT32 OriginalOffset = g_SsdtItem[i] >> 4;
						INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentSsdtAddress->Base + i * 4)) >> 4;    // 带符号位的移位

						UINT64 CurrentSsdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentSsdtAddress->Base + CurrentOffset);
						UINT64 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#else
						// 32位存储的是 绝对地址
						UINT32 CurrentSsdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentSsdtAddress->Base + i * 4);
						UINT32 OriginalSsdtFunctionAddress = g_SsdtItem[i];

#endif // _WIN64

						if (OriginalSsdtFunctionAddress != CurrentSsdtFunctionAddress)   // 表明被Hook了
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = FALSE;
						}
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].Ordinal = i;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].CurrentAddress = CurrentSsdtFunctionAddress;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].OriginalAddress = OriginalSsdtFunctionAddress;

						RtlStringCchCopyW(shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].wzFunctionName, wcslen(g_SsdtFunctionName[i]) + 1, g_SsdtFunctionName[i]);

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					shi->NumberOfSsdtFunctions++;
				}
			}
			else
			{
				DbgPrint("Reload Ntkrnl & Ssdt Failed\r\n");
			}
		}
		else
		{
			DbgPrint("Initialize Ssdt Function Name Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Get Current Ssdt Failed\r\n");
	}
*/
	return Status;
}


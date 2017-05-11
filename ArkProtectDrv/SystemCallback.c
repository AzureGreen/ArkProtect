#include "SystemCallback.h"

extern DYNAMIC_DATA g_DynamicData;



/************************************************************************
*  Name : APGetPspCreateProcessNotifyRoutineAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得PspCreateProcessNotifyRoutine
************************************************************************/
UINT_PTR
APGetPspCreateProcessNotifyRoutineAddress()
{
	UINT_PTR PspCreateProcessNotifyRoutine = 0;
	UINT_PTR PsSetCreateProcessNotifyRoutine = 0;
	UINT_PTR PspSetCreateProcessNotifyRoutine = 0;

	PUINT8	StartSearchAddress = 0, EndSearchAddress = 0;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0, v3 = 0;
	INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型

	APGetNtosExportVariableAddress(L"PsSetCreateProcessNotifyRoutine", (PVOID*)&PsSetCreateProcessNotifyRoutine);
	DbgPrint("%p\r\n", PsSetCreateProcessNotifyRoutine);

	/*
	Win7x64
	kd> u PsSetCreateProcessNotifyRoutine
	nt!PsSetCreateProcessNotifyRoutine:
	fffff800`042cd400 4533c0          xor     r8d,r8d
	fffff800`042cd403 e9e8fdffff      jmp     nt!PspSetCreateProcessNotifyRoutine (fffff800`042cd1f0)
	
	Win7x86
	1: kd> u PsSetCreateProcessNotifyRoutine
	nt!PsSetCreateProcessNotifyRoutine:
	83fd2899 8bff            mov     edi,edi
	......
	83fac8a3 ff7508          push    dword ptr [ebp+8]
	83fac8a6 e809000000      call    nt!PspSetCreateProcessNotifyRoutine (83fac8b4)
	83fac8ab 5d              pop     ebp

	发现PsSetCreateProcessNotifyRoutine是一个跳板，真实函数是PspSetCreateProcessNotifyRoutine
	win7x64:PsSetCreateProcessNotifyRoutine过4字节是跳转的偏移，先计算出PspSetCreateProcessNotifyRoutine函数地址
	win7x86:里面有直接调用PspSetCreateProcessNotifyRoutine，硬编码寻找
	*/

	if (PsSetCreateProcessNotifyRoutine)
	{
#ifdef _WIN64
		PspSetCreateProcessNotifyRoutine = (*(INT32*)(PsSetCreateProcessNotifyRoutine + 4) + PsSetCreateProcessNotifyRoutine + 3);
#else
		StartSearchAddress = (PUINT8)PsSetCreateProcessNotifyRoutine;
		EndSearchAddress = StartSearchAddress + 0x500;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5))
			{
				v1 = *i;
				v2 = *(i + 5);
				if (v1 == 0xe8 && v2 == 0x5d)		//  call offset pop     ebp
				{
					RtlCopyMemory(&iOffset, i + 1, 4);
					PspSetCreateProcessNotifyRoutine = (UINT_PTR)(iOffset + (UINT32)i + 5);;
					break;
				}
			}
		}
#endif // _WIN64
	}

	// 在 PsSetCreateThreadNotifyRoutine 中使用了 PspCreateThreadNotifyRoutine 硬编码加偏移获得
	/*
	Win7 64:
	kd> u PspSetCreateProcessNotifyRoutine l 20
	nt!PspSetCreateProcessNotifyRoutine:
	fffff800`042cd1f0 48895c2408      mov     qword ptr [rsp+8],rbx
	......
	fffff800`042cd236 4c8d3563bdd6ff  lea     r14,[nt!PspCreateProcessNotifyRoutine (fffff800`04038fa0)]
	fffff800`042cd23d 418bc4          mov     eax,r12d

	Win7 32:
	3: kd> u 83fac8b4 l 10
	nt!PspSetCreateProcessNotifyRoutine:
	83fac8b4 8bff            mov     edi,edi
	......
	83fac8d6 c7450ca0a9f583  mov     dword ptr [ebp+0Ch],offset nt!PspCreateProcessNotifyRoutine (83f5a9a0)
	*/

	if (PspSetCreateProcessNotifyRoutine)
	{
		StartSearchAddress = (PUINT8)PspSetCreateProcessNotifyRoutine;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
#ifdef _WIN64
				if (v1 == 0x4c && v2 == 0x8d && v3 == 0x35)		// 硬编码  lea r14
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					PspCreateProcessNotifyRoutine = (UINT_PTR)(iOffset + (UINT64)i + 7);
					break;
				}
#else
				if (v1 == 0xc7 && v2 == 0x45 && v3 == 0x0c)		// mov     dword ptr [ebp+0Ch]
				{
					RtlCopyMemory(&PspCreateProcessNotifyRoutine, i + 3, 4);
					break;
				}
#endif // _WIN64

			}
		}
	}
	return PspCreateProcessNotifyRoutine;
}


/************************************************************************
*  Name : APGetCreateProcessCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举创建进程回调
************************************************************************/
BOOLEAN
APGetCreateProcessCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR PspCreateProcessNotifyRoutine = APGetPspCreateProcessNotifyRoutineAddress();

	DbgPrint("%p\r\n", PspCreateProcessNotifyRoutine);

	if (!PspCreateProcessNotifyRoutine)
	{
		DbgPrint("PspCreateProcessNotifyRoutineAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		UINT32 i = 0;
		for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++)
		{
			UINT_PTR NotifyItem = 0;		// PspCreateThreadNotifyRoutine数组每一项成员
			UINT_PTR CallbackAddress = 0;	// 真实的回调例程地址

			if (MmIsAddressValid((PVOID)(PspCreateProcessNotifyRoutine + i * sizeof(UINT_PTR))))
			{
				NotifyItem = *(PUINT_PTR)(PspCreateProcessNotifyRoutine + i * sizeof(UINT_PTR));

				if (NotifyItem > g_DynamicData.MinKernelSpaceAddress &&
					MmIsAddressValid((PVOID)(NotifyItem & ~7)))
				{
#ifdef _WIN64
					CallbackAddress = *(PUINT_PTR)(NotifyItem & ~7);
#else
					CallbackAddress = *(PUINT_PTR)((NotifyItem & ~7) + sizeof(UINT32));
#endif // _WIN64
					if (CallbackAddress && MmIsAddressValid((PVOID)CallbackAddress))
					{
						if (CallbackCount > sci->NumberOfCallbacks)
						{
							sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyCreateProcess;
							sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = CallbackAddress;
							sci->CallbackEntry[sci->NumberOfCallbacks].Description = NotifyItem;
						}
						sci->NumberOfCallbacks++;
					}
				}
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APGetPspCreateThreadNotifyRoutineAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得PspCreateThreadNotifyRoutine
************************************************************************/
UINT_PTR
APGetPspCreateThreadNotifyRoutineAddress()
{
	// 在 PsSetCreateThreadNotifyRoutine 中使用了 PspCreateThreadNotifyRoutine 硬编码加偏移获得
	/*
	Win7x64:
	2: kd> u PsSetCreateThreadNotifyRoutine l 10
	nt!PsSetCreateThreadNotifyRoutine:
	fffff800`0428cbf0 48895c2408      mov     qword ptr [rsp+8],rbx
	......
	fffff800`0428cc12 488d0d67c1d9ff  lea     rcx,[nt!PspCreateThreadNotifyRoutine (fffff800`04028d80)]

	Win7x86:
	0: kd> u PsSetCreateThreadNotifyRoutine l 20
	nt!PsSetCreateThreadNotifyRoutine:
	840f78e7 8bff            mov     edi,edi
	......
	840f7906 56              push    esi
	840f7907 be80a8f583      mov     esi,offset nt!PspCreateThreadNotifyRoutine (83f5a880)
	840f790c 6a00            push    0
	*/

	UINT_PTR PspCreateThreadNotifyRoutine = 0;
	UINT_PTR PsSetCreateThreadNotifyRoutine = 0;

	APGetNtosExportVariableAddress(L"PsSetCreateThreadNotifyRoutine", (PVOID*)&PsSetCreateThreadNotifyRoutine);
	DbgPrint("%p\r\n", PsSetCreateThreadNotifyRoutine);

	if (PsSetCreateThreadNotifyRoutine)
	{
		PUINT8	StartSearchAddress = (PUINT8)PsSetCreateThreadNotifyRoutine;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					PspCreateThreadNotifyRoutine = (UINT_PTR)(iOffset + (UINT64)i + 7);
					break;
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 6);
				if (v1 == 0x56 && v2 == 0xbe && v3 == 0x6a)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&PspCreateThreadNotifyRoutine, i + 2, 4);
					break;
				}
			}
#endif // _WIN64
		}
	}
	return PspCreateThreadNotifyRoutine;
}


/************************************************************************
*  Name : APGetCreateThreadCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举创建线程回调
************************************************************************/
BOOLEAN
APGetCreateThreadCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR PspCreateThreadNotifyRoutine = APGetPspCreateThreadNotifyRoutineAddress();

	DbgPrint("%p\r\n", PspCreateThreadNotifyRoutine);

	if (!PspCreateThreadNotifyRoutine)
	{
		DbgPrint("PspCreateThreadNotifyRoutineAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		UINT32 i = 0;
		for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++)
		{
			UINT_PTR NotifyItem = 0;		// PspCreateThreadNotifyRoutine数组每一项成员
			UINT_PTR CallbackAddress = 0;	// 真实的回调例程地址

			if (MmIsAddressValid((PVOID)(PspCreateThreadNotifyRoutine + i * sizeof(UINT_PTR))))
			{
				NotifyItem = *(PUINT_PTR)(PspCreateThreadNotifyRoutine + i * sizeof(UINT_PTR));

				if (NotifyItem > g_DynamicData.MinKernelSpaceAddress &&
					MmIsAddressValid((PVOID)(NotifyItem & ~7)))
				{
#ifdef _WIN64
					CallbackAddress = *(PUINT_PTR)(NotifyItem & ~7);
#else
					CallbackAddress = *(PUINT_PTR)((NotifyItem & ~7) + sizeof(UINT32));
#endif // _WIN64

					if (CallbackAddress && MmIsAddressValid((PVOID)CallbackAddress))
					{
						if (CallbackCount > sci->NumberOfCallbacks)
						{
							sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyCreateThread;
							sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = CallbackAddress;
							sci->CallbackEntry[sci->NumberOfCallbacks].Description = NotifyItem;
						}
						sci->NumberOfCallbacks++;
					}
				}
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APGetPspLoadImageNotifyRoutineAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得PspLoadImageNotifyRoutine
************************************************************************/
UINT_PTR
APGetPspLoadImageNotifyRoutineAddress()
{
	// 在 PsSetLoadImageNotifyRoutine 中使用了 PspLoadImageNotifyRoutine 硬编码加偏移获得
	/*
	Win7 x64:
	0: kd> u PsSetLoadImageNotifyRoutine l 10
	nt!PsSetLoadImageNotifyRoutine:
	fffff800`0429cb70 48895c2408      mov     qword ptr [rsp+8],rbx
	......
	fffff800`0429cb92 488d0d87c1d9ff  lea     rcx,[nt!PspLoadImageNotifyRoutine (fffff800`04038d20)]
	fffff800`0429cb99 4533c0          xor     r8d,r8d

	Win7 x86:
	0: kd> u PsSetLoadImageNotifyRoutine l 10
	nt!PsSetLoadImageNotifyRoutine:
	83f889ca 8bff            mov     edi,edi
	......
	83f889e1 7425            je      nt!PsSetLoadImageNotifyRoutine+0x3e (83f88a08)
	83f889e3 be40a8f583      mov     esi,offset nt!PspLoadImageNotifyRoutine (83f5a840)
	83f889e8 6a00            push    0
	*/

	UINT_PTR PspLoadImageNotifyRoutine = 0;
	UINT_PTR PsSetLoadImageNotifyRoutine = 0;

	APGetNtosExportVariableAddress(L"PsSetLoadImageNotifyRoutine", (PVOID*)&PsSetLoadImageNotifyRoutine);
	DbgPrint("%p\r\n", PsSetLoadImageNotifyRoutine);

	if (PsSetLoadImageNotifyRoutine)
	{
		PUINT8	StartSearchAddress = (PUINT8)PsSetLoadImageNotifyRoutine;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					PspLoadImageNotifyRoutine = (UINT_PTR)(iOffset + (UINT64)i + 7);
					break;
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 5);
				v3 = *(i + 6);
				if (v1 == 0xbe && v2 == 0x6a && v3 == 0x00)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&PspLoadImageNotifyRoutine, i + 1, 4);
					break;
				}
			}
#endif // _WIN64

		}
	}
	return PspLoadImageNotifyRoutine;
}


/************************************************************************
*  Name : APGetLoadImageCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举加载模块回调
************************************************************************/
BOOLEAN
APGetLoadImageCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR PspLoadImageNotifyRoutine = APGetPspLoadImageNotifyRoutineAddress();

	DbgPrint("%p\r\n", PspLoadImageNotifyRoutine);

	if (!PspLoadImageNotifyRoutine)
	{
		DbgPrint("PspLoadImageNotifyRoutineAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		UINT32 i = 0;
		for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++)		// 64位数组大小为64
		{
			UINT_PTR NotifyItem = 0;		// PspLoadImageNotifyRoutine数组每一项成员
			UINT_PTR CallbackAddress = 0;	// 真实的回调例程地址

			if (MmIsAddressValid((PVOID)(PspLoadImageNotifyRoutine + i * sizeof(UINT_PTR))))
			{
				NotifyItem = *(PUINT_PTR)(PspLoadImageNotifyRoutine + i * sizeof(UINT_PTR));

				if (NotifyItem > g_DynamicData.MinKernelSpaceAddress &&
					MmIsAddressValid((PVOID)(NotifyItem & ~7)))
				{
#ifdef _WIN64
					CallbackAddress = *(PUINT_PTR)(NotifyItem & ~7);
#else
					CallbackAddress = *(PUINT_PTR)((NotifyItem & ~7) + sizeof(UINT32));
#endif // _WIN64

					if (CallbackAddress && MmIsAddressValid((PVOID)CallbackAddress))
					{
						if (CallbackCount > sci->NumberOfCallbacks)
						{
							sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyLoadImage;
							sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = CallbackAddress;
							sci->CallbackEntry[sci->NumberOfCallbacks].Description = NotifyItem;
						}
						sci->NumberOfCallbacks++;
					}
				}
			}
		}
	}

	return TRUE;
}


//
// Win7之后 都是ListEntry结构 xp之前 都是Vector数组结构
//
/************************************************************************
*  Name : APGetCallbackListHeadAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得CallbackListHead
************************************************************************/
UINT_PTR
APGetCallbackListHeadAddress()
{
	/*
	Win7 x64:
	0: kd> u CmUnRegisterCallback l 30
	nt!CmUnRegisterCallback:
	fffff800`042c67d0 48894c2408      mov     qword ptr [rsp+8],rcx
	......
	fffff800`042c689e 488d0dcb90dcff  lea     rcx,[nt!CallbackListHead (fffff800`0408f970)]

	Win7 x86:
	0: kd> u CmUnRegisterCallback l 30
	nt!CmUnRegisterCallback:
	840b6881 6a38            push    38h
	......
	840b690d 8d4dd4          lea     ecx,[ebp-2Ch]
	840b6910 bf900ef883      mov     edi,offset nt!CallbackListHead (83f80e90)
	840b6915 8bc7            mov     eax,edi
	*/

	UINT_PTR CallbackListHead = 0;
	UINT_PTR CmUnRegisterCallback = 0;

	APGetNtosExportVariableAddress(L"CmUnRegisterCallback", (PVOID*)&CmUnRegisterCallback);
	DbgPrint("%p\r\n", CmUnRegisterCallback);

	if (CmUnRegisterCallback)
	{
		PUINT8	StartSearchAddress = (PUINT8)CmUnRegisterCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					// 前面也出现了 lea rcx,所有需要多加一次判断
					j = i - 5;
					if (MmIsAddressValid(j) && MmIsAddressValid(j + 1) && MmIsAddressValid(j + 2))
					{
						v1 = *j;
						v2 = *(j + 1);
						v3 = *(j + 2);
						if (v1 == 0x48 && v2 == 0x8d && v3 == 0x54)		// 硬编码  lea rdx
						{
							RtlCopyMemory(&iOffset, i + 3, 4);
							CallbackListHead = (UINT_PTR)(iOffset + (UINT64)i + 7);
							break;
						}
					}
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 5);
				v3 = *(i + 6);
				if (v1 == 0xbf && v2 == 0x8b && v3 == 0xc7)		// mov     edi ...
				{
					RtlCopyMemory(&CallbackListHead, i + 1, 4);
					break;
				}
			}
#endif // _WIN64
		}
	}

	return CallbackListHead;
}


/************************************************************************
*  Name : APGetRegisterCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举注册表回调
************************************************************************/
BOOLEAN
APGetRegisterCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR CallbackListHead = APGetCallbackListHeadAddress();

	DbgPrint("%p\r\n", CallbackListHead);

	if (!CallbackListHead)
	{
		DbgPrint("CallbackListHeadAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		PCM_NOTIFY_ENTRY	Notify = NULL;
		
		/*
		WinDbg调试
		3: kd> dt _list_entry fffff800`0408f970
		nt!_LIST_ENTRY
		[ 0xfffff800`0408f970 - 0xfffff800`0408f970 ]
		+0x000 Flink            : 0xfffff800`0408f970 _LIST_ENTRY [ 0xfffff800`0408f970 - 0xfffff800`0408f970 ]
		+0x008 Blink            : 0xfffff800`0408f970 _LIST_ENTRY [ 0xfffff800`0408f970 - 0xfffff800`0408f970 ]

		3: kd> dq fffff800`0408f970
		fffff800`0408f970  fffff800`0408f970 fffff800`0408f970
		fffff800`0408f980  00000000`00000000 00000000`00000000
		fffff800`0408f990  00000000`00000000 00000000`00000000

		目前系统没有这种回调
		*/

		for (PLIST_ENTRY NotifyListEntry = ((PLIST_ENTRY)CallbackListHead)->Flink;
			NotifyListEntry != (PLIST_ENTRY)CallbackListHead;
			NotifyListEntry = NotifyListEntry->Flink)
		{
			Notify = (PCM_NOTIFY_ENTRY)NotifyListEntry;
			if (MmIsAddressValid(Notify))
			{
				if (MmIsAddressValid((PVOID)(Notify->Function)) && Notify->Function > g_DynamicData.MinKernelSpaceAddress)
				{
					if (CallbackCount > sci->NumberOfCallbacks)
					{
						sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyCmpCallBack;
						sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = (UINT_PTR)Notify->Function;
						sci->CallbackEntry[sci->NumberOfCallbacks].Description = (UINT_PTR)Notify->Cookie.QuadPart;
					}
					sci->NumberOfCallbacks++;
				}
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APGetKeBugCheckCallbackListHeadAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得KeBugCheckCallbackListHead
************************************************************************/
UINT_PTR
APGetKeBugCheckCallbackListHeadAddress()
{
	/*
	Win7 x64:
	0: kd> u KeRegisterBugCheckCallback l 50
	nt!KeRegisterBugCheckCallback:
	fffff800`03f390b0 48895c2420      mov     qword ptr [rsp+20h],rbx
	......
	fffff800`03f391bc 488d0d7d111500  lea     rcx,[nt!KeBugCheckCallbackListHead (fffff800`0408a340)]

	Win7 x86:
	2: kd>  u KeRegisterBugCheckCallback l 50
	nt!KeRegisterBugCheckCallback:
	83e202c7 8bff            mov     edi,edi
	......
	83e20315 c6401c01        mov     byte ptr [eax+1Ch],1
	83e20319 8b0da0d7f783    mov     ecx,dword ptr [nt!KeBugCheckCallbackListHead (83f7d7a0)]
	83e2031f 8908            mov     dword ptr [eax],ecx

	*/

	UINT_PTR KeBugCheckCallbackListHead = 0;
	UINT_PTR KeRegisterBugCheckCallback = 0;

	APGetNtosExportVariableAddress(L"KeRegisterBugCheckCallback", (PVOID*)&KeRegisterBugCheckCallback);
	DbgPrint("%p\r\n", KeRegisterBugCheckCallback);

	if (KeRegisterBugCheckCallback)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeRegisterBugCheckCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					j = i - 3;
					if (MmIsAddressValid(j) && MmIsAddressValid(j + 1) && MmIsAddressValid(j + 2))
					{
						v1 = *j;
						v2 = *(j + 1);
						v3 = *(j + 2);
						if (v1 == 0x48 && v2 == 0x03 && v3 == 0xc1)		// 硬编码  add rax, rcx
						{
							RtlCopyMemory(&iOffset, i + 3, 4);
							KeBugCheckCallbackListHead = (UINT_PTR)(iOffset + (UINT64)i + 7);
							break;
						}
					}
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 6);
				if (v1 == 0x8b && v2 == 0x0d && v3 == 0x89)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&KeBugCheckCallbackListHead, i + 2, 4);
					break;
				}
			}
#endif // _WIN64

		}
	}
	return KeBugCheckCallbackListHead;
}


/************************************************************************
*  Name : APGetBugCheckCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举错误检测回调
************************************************************************/
BOOLEAN
APGetBugCheckCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR KeBugCheckCallbackListHead = APGetKeBugCheckCallbackListHeadAddress();

	DbgPrint("%p\r\n", KeBugCheckCallbackListHead);

	if (!KeBugCheckCallbackListHead)
	{
		DbgPrint("KeBugCheckCallbackListHeadAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		/*
		2: kd> dt _list_entry fffff800`0407a340
		nt!_LIST_ENTRY
		[ 0xfffffa80`198d7ea0 - 0xfffff800`04413400 ]
		+0x000 Flink            : 0xfffffa80`198d7ea0 _LIST_ENTRY [ 0xfffffa80`1989eea0 - 0xfffff800`0407a340 ]
		+0x008 Blink            : 0xfffff800`04413400 _LIST_ENTRY [ 0xfffff800`0407a340 - 0xfffffa80`1976aea0 ]

		2: kd> dq 0xfffffa80`198d7ea0
		fffffa80`198d7ea0  fffffa80`1989eea0 fffff800`0407a340
		fffffa80`198d7eb0  fffff880`014f1b00 fffffa80`198d71a0

		2: kd> u fffff880`014f1b00
		fffff880`014f1b00 4883ec28        sub     rsp,28h		; 开堆栈，是函数的标志
		fffff880`014f1b04 81faa0160000    cmp     edx,16A0h
		......
		*/

		UINT_PTR     Dispatch = 0;

		for (PLIST_ENTRY TravelListEntry = ((PLIST_ENTRY)KeBugCheckCallbackListHead)->Flink;
			TravelListEntry != (PLIST_ENTRY)KeBugCheckCallbackListHead;
			TravelListEntry = TravelListEntry->Flink)
		{
			Dispatch = *(PUINT_PTR)((PUINT8)TravelListEntry + sizeof(LIST_ENTRY));	// 过ListEntry,后面就是函数

			if (Dispatch && MmIsAddressValid((PVOID)Dispatch))
			{
				if (CallbackCount > sci->NumberOfCallbacks)
				{
					sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyKeBugCheck;
					sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = Dispatch;
					sci->CallbackEntry[sci->NumberOfCallbacks].Description = (UINT_PTR)TravelListEntry;
				}
				sci->NumberOfCallbacks++;
			}
		}
	}

	return TRUE;
}

//
// 发现 KeBugCheckCallbackListHead 刚好就在 KeBugCheckReasonCallbackListHead的后面
//


/************************************************************************
*  Name : APGetKeBugCheckReasonCallbackListHeadAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得KeBugCheckReasonCallbackListHead
************************************************************************/
UINT_PTR
APGetKeBugCheckReasonCallbackListHeadAddress()
{
	/*
	Win7 x64:
	0: kd> u KeRegisterBugCheckReasonCallback l 50
	nt!KeRegisterBugCheckReasonCallback:
	fffff800`03f38da0 48895c2418      mov     qword ptr [rsp+18h],rbx
	......
	fffff800`03f38ea8 488b0581141500  mov     rax,qword ptr [nt!KeBugCheckReasonCallbackListHead (fffff800`0408a330)]
	fffff800`03f38eaf 488d0d7a141500  lea     rcx,[nt!KeBugCheckReasonCallbackListHead (fffff800`0408a330)]

	Win7 x86:
	2: kd> u KeRegisterBugCheckReasonCallback l 50
	nt!KeRegisterBugCheckReasonCallback:
	83e24818 8bff            mov     edi,edi
	......
	83e24863 7419            je      nt!KeRegisterBugCheckReasonCallback+0x66 (83e2487e)
	83e24865 8b0d98d7f783    mov     ecx,dword ptr [nt!KeBugCheckReasonCallbackListHead (83f7d798)]
	83e2486b 8908            mov     dword ptr [eax],ecx

	*/

	UINT_PTR KeBugCheckReasonCallbackListHead = 0;
	UINT_PTR KeRegisterBugCheckReasonCallback = 0;

	APGetNtosExportVariableAddress(L"KeRegisterBugCheckReasonCallback", (PVOID*)&KeRegisterBugCheckReasonCallback);
	DbgPrint("%p\r\n", KeRegisterBugCheckReasonCallback);

	if (KeRegisterBugCheckReasonCallback)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeRegisterBugCheckReasonCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					j = i - 7;
					if (MmIsAddressValid(j) && MmIsAddressValid(j + 1) && MmIsAddressValid(j + 2))
					{
						v1 = *j;
						v2 = *(j + 1);
						v3 = *(j + 2);
						if (v1 == 0x48 && v2 == 0x8b && v3 == 0x05)		// 硬编码  mov rax
						{
							RtlCopyMemory(&iOffset, i + 3, 4);
							KeBugCheckReasonCallbackListHead = (UINT_PTR)(iOffset + (UINT64)i + 7);
							break;
						}
					}
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 6);
				if (v1 == 0x8b && v2 == 0x0d && v3 == 0x89)
				{
					RtlCopyMemory(&KeBugCheckReasonCallbackListHead, i + 2, 4);
					break;
				}
			}
#endif // _WIN64

		}
	}
	return KeBugCheckReasonCallbackListHead;
}


/************************************************************************
*  Name : APGetBugCheckReasonCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举带原因的错误检测回调
************************************************************************/
BOOLEAN
APGetBugCheckReasonCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR KeBugCheckReasonCallbackListHead = APGetKeBugCheckReasonCallbackListHeadAddress();

	DbgPrint("%p\r\n", KeBugCheckReasonCallbackListHead);

	if (!KeBugCheckReasonCallbackListHead)
	{
		DbgPrint("KeBugCheckReasonCallbackListHeadAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		/*
		2: kd> dt _list_entry fffff800`0407a330
		nt!_LIST_ENTRY
		[ 0xfffffa80`18dd07b8 - 0xfffff880`010b61e0 ]
		+0x000 Flink            : 0xfffffa80`18dd07b8 _LIST_ENTRY [ 0xfffffa80`19f71120 - 0xfffff800`0407a330 ]
		+0x008 Blink            : 0xfffff880`010b61e0 _LIST_ENTRY [ 0xfffff800`0407a330 - 0xfffff880`00f60560 ]

		2: kd> dq 0xfffffa80`18dd07b8
		fffffa80`18dd07b8  fffffa80`19f71120 fffff800`0407a330
		fffffa80`18dd07c8  fffff880`00f49054 fffffa80`18dd0800

		2: kd> u fffff880`00f49054
		fffff880`00f49054 48895c2408      mov     qword ptr [rsp+8],rbx
		fffff880`00f49059 4889742410      mov     qword ptr [rsp+10h],rsi
		......
		*/

		UINT_PTR     Dispatch = 0;

		for (PLIST_ENTRY TravelListEntry = ((PLIST_ENTRY)KeBugCheckReasonCallbackListHead)->Flink;
			TravelListEntry != (PLIST_ENTRY)KeBugCheckReasonCallbackListHead;
			TravelListEntry = TravelListEntry->Flink)
		{
			Dispatch = *(PUINT_PTR)((PUINT8)TravelListEntry + sizeof(LIST_ENTRY));	// 过ListEntry

			if (Dispatch && MmIsAddressValid((PVOID)Dispatch))
			{
				if (CallbackCount > sci->NumberOfCallbacks)
				{
					sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyKeBugCheckReason;
					sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = Dispatch;
					sci->CallbackEntry[sci->NumberOfCallbacks].Description = (UINT_PTR)TravelListEntry;
				}
				sci->NumberOfCallbacks++;
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APGetShutdownDispatch
*  Param: DeviceObject
*  Ret  : UINT_PTR
*  通过设备对象获得关机派遣函数
************************************************************************/
UINT_PTR
APGetShutdownDispatch(IN PDEVICE_OBJECT DeviceObject)
{
	PDRIVER_OBJECT DriverObject = NULL;
	UINT_PTR ShutdownDispatch = 0;

	if (DeviceObject && MmIsAddressValid((PVOID)DeviceObject))
	{
		DriverObject = DeviceObject->DriverObject;
		if (DriverObject && MmIsAddressValid((PVOID)DriverObject))
		{
			ShutdownDispatch = (UINT_PTR)DriverObject->MajorFunction[IRP_MJ_SHUTDOWN];
		}
	}

	return ShutdownDispatch;
}


/************************************************************************
*  Name : APGetIopNotifyShutdownQueueHeadAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得IopNotifyShutdownQueueHead
************************************************************************/
UINT_PTR
APGetIopNotifyShutdownQueueHeadAddress()
{
	/*
	Win7 x64:
	2: kd> u IoRegisterShutdownNotification l 20
	nt!IoRegisterShutdownNotification:
	fffff800`04278f50 48895c2408      mov     qword ptr [rsp+8],rbx
	......
	fffff800`04278f8a 488d0dff59e0ff  lea     rcx,[nt!IopNotifyShutdownQueueHead (fffff800`0407e990)]

	Win7 x86:
	2: kd> u IoRegisterShutdownNotification l 20
	nt!IoRegisterShutdownNotification:
	83f9d606 8bff            mov     edi,edi
	......
	83f9d631 e83e57efff      call    nt!ObfReferenceObject (83e92d74)
	83f9d636 bf1804f883      mov     edi,offset nt!IopNotifyShutdownQueueHead (83f80418)
	83f9d63b e8c52ae8ff      call    nt!IopInterlockedInsertHeadList (83e20105)
	*/

	UINT_PTR IopNotifyShutdownQueueHead = 0;
	UINT_PTR IoRegisterShutdownNotification = 0;

	APGetNtosExportVariableAddress(L"IoRegisterShutdownNotification", (PVOID*)&IoRegisterShutdownNotification);
	DbgPrint("%p\r\n", IoRegisterShutdownNotification);

	if (IoRegisterShutdownNotification)
	{
		PUINT8	StartSearchAddress = (PUINT8)IoRegisterShutdownNotification;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					IopNotifyShutdownQueueHead = iOffset + (UINT64)i + 7;
					break;
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5))
			{
				v1 = *i;
				v2 = *(i + 5);
				if (v1 == 0xbf && v2 == 0xe8)
				{
					RtlCopyMemory(&IopNotifyShutdownQueueHead, i + 1, 4);
					break;
				}
			}
#endif // _WIN64
		}
	}
	return IopNotifyShutdownQueueHead;
}


/************************************************************************
*  Name : APGetShutDownCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举关机回调
************************************************************************/
BOOLEAN
APGetShutDownCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR IopNotifyShutdownQueueHead = APGetIopNotifyShutdownQueueHeadAddress();

	DbgPrint("%p\r\n", IopNotifyShutdownQueueHead);

	if (!IopNotifyShutdownQueueHead)
	{
		DbgPrint("IopNotifyShutdownQueueHeadAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		for (PLIST_ENTRY DispatchListEntry = ((PLIST_ENTRY)IopNotifyShutdownQueueHead)->Flink;
			DispatchListEntry != (PLIST_ENTRY)IopNotifyShutdownQueueHead;
			DispatchListEntry = DispatchListEntry->Flink)
		{
			PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)*(PUINT_PTR)((PUINT8)DispatchListEntry + sizeof(LIST_ENTRY));   // 过ListEntry

			if (DeviceObject && MmIsAddressValid((PVOID)DeviceObject))
			{
				if (CallbackCount > sci->NumberOfCallbacks)
				{
					sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyShutdown;
					sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = APGetShutdownDispatch(DeviceObject);		// 通多设备对象找到驱动对象 SHUTDOWN 派遣例程
					sci->CallbackEntry[sci->NumberOfCallbacks].Description = (UINT_PTR)DeviceObject;
				}
				sci->NumberOfCallbacks++;
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APGetIopNotifyLastChanceShutdownQueueHeadAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得IopNotifyLastChanceShutdownQueueHead
************************************************************************/
UINT_PTR
APGetIopNotifyLastChanceShutdownQueueHeadAddress()
{
	/*
	Win7 x64:
	kd> u IoRegisterLastChanceShutdownNotification l 20
	nt!IoRegisterLastChanceShutdownNotification:
	fffff800`04290fc0 48895c2408      mov     qword ptr [rsp+8],rbx
	......
	fffff800`04290ff1 e8fac6c0ff      call    nt!ObfReferenceObject (fffff800`03e9d6f0)
	fffff800`04290ff6 488d0d8359e0ff  lea     rcx,[nt!IopNotifyLastChanceShutdownQueueHead (fffff800`04096980)]
	fffff800`04290ffd 488bd7          mov     rdx,rdi

	Win7 x86:
	0: kd> u IoRegisterLastChanceShutdownNotification l 20
	nt!IoRegisterLastChanceShutdownNotification:
	83f863e1 8bff            mov     edi,edi
	......
	83f86409 e866c9f0ff      call    nt!ObfReferenceObject (83e92d74)
	83f8640e bf1004f883      mov     edi,offset nt!IopNotifyLastChanceShutdownQueueHead (83f80410)
	83f86413 895e08          mov     dword ptr [esi+8],ebx
	*/

	UINT_PTR IopNotifyLastChanceShutdownQueueHead = 0;
	UINT_PTR IoRegisterLastChanceShutdownNotification = 0;

	APGetNtosExportVariableAddress(L"IoRegisterLastChanceShutdownNotification", (PVOID*)&IoRegisterLastChanceShutdownNotification);
	DbgPrint("%p\r\n", IoRegisterLastChanceShutdownNotification);

	if (IoRegisterLastChanceShutdownNotification)
	{
		PUINT8	StartSearchAddress = (PUINT8)IoRegisterLastChanceShutdownNotification;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
#ifdef _WIN64
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					IopNotifyLastChanceShutdownQueueHead = iOffset + (UINT64)i + 7;
					break;
				}
			}
#else
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5) && MmIsAddressValid(i + 6))
			{
				v1 = *i;
				v2 = *(i + 5);
				v3 = *(i + 6);
				if (v1 == 0xbf && v2 == 0x89 && v3 == 0x5e)
				{
					RtlCopyMemory(&IopNotifyLastChanceShutdownQueueHead, i + 1, 4);
					break;
				}
			}
#endif // _WIN64
		}
	}
	return IopNotifyLastChanceShutdownQueueHead;
}


/************************************************************************
*  Name : APGetLastChanceShutDownCallbackNotify
*  Param: sci
*  Param: CallbackCount
*  Ret  : BOOLEAN
*  枚举最后机会的关机回调
************************************************************************/
BOOLEAN
APGetLastChanceShutDownCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount)
{
	UINT_PTR IopNotifyLastChanceShutdownQueueHead = APGetIopNotifyLastChanceShutdownQueueHeadAddress();

	DbgPrint("%p\r\n", IopNotifyLastChanceShutdownQueueHead);

	if (!IopNotifyLastChanceShutdownQueueHead)
	{
		DbgPrint("IopNotifyLastChanceShutdownQueueHeadAddress NULL\r\n");
		return FALSE;
	}
	else
	{
		for (PLIST_ENTRY DispatchListEntry = ((PLIST_ENTRY)IopNotifyLastChanceShutdownQueueHead)->Flink;
			DispatchListEntry != (PLIST_ENTRY)IopNotifyLastChanceShutdownQueueHead;
			DispatchListEntry = DispatchListEntry->Flink)
		{
			PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)*(PUINT_PTR)((PUINT8)DispatchListEntry + sizeof(LIST_ENTRY));   // 过ListEntry

			if (DeviceObject && MmIsAddressValid((PVOID)DeviceObject))
			{
				if (CallbackCount > sci->NumberOfCallbacks)
				{
					sci->CallbackEntry[sci->NumberOfCallbacks].Type = ct_NotifyLastChanceShutdown;
					sci->CallbackEntry[sci->NumberOfCallbacks].CallbackAddress = APGetShutdownDispatch(DeviceObject);		// 通多设备对象找到驱动对象 SHUTDOWN 派遣例程
					sci->CallbackEntry[sci->NumberOfCallbacks].Description = (UINT_PTR)DeviceObject;
				}
				sci->NumberOfCallbacks++;
			}
		}
	}

	return TRUE;
}


/************************************************************************
*  Name : APEnumSystemCallback
*  Param: OutputBuffer			Ring3Buffer      （OUT）
*  Param: OutputLength			Ring3BufferLength（IN）
*  Ret  : NTSTATUS
*  枚举所有的系统回调
************************************************************************/
NTSTATUS
APEnumSystemCallback(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	PSYS_CALLBACK_INFORMATION sci = (PSYS_CALLBACK_INFORMATION)OutputBuffer;

	UINT32 CallbackCount = (OutputLength - sizeof(SYS_CALLBACK_INFORMATION)) / sizeof(SYS_CALLBACK_ENTRY_INFORMATION);

	APGetCreateProcessCallbackNotify(sci, CallbackCount);   // 创建进程回调
	APGetCreateThreadCallbackNotify(sci, CallbackCount);    // 创建线程回调
	APGetLoadImageCallbackNotify(sci, CallbackCount);		 // 映像加载 卸载回调
	APGetRegisterCallbackNotify(sci, CallbackCount);       // 注册表回调
	APGetBugCheckCallbackNotify(sci, CallbackCount);       // 错误检查回调
	APGetBugCheckReasonCallbackNotify(sci, CallbackCount); // 同上
	APGetShutDownCallbackNotify(sci, CallbackCount);       // 关机回调
	APGetLastChanceShutDownCallbackNotify(sci, CallbackCount);  // 同上

	// 注销回调
	// 文件系统改变回调

	if (CallbackCount >= sci->NumberOfCallbacks)
	{
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}

	return Status;
}


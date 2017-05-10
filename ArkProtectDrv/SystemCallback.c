#include "SystemCallback.h"


typedef
//NTKERNELAPI
NTSTATUS(*pfnPsSetLoadImageNotifyRoutine)(
	__in PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

typedef
//NTKERNELAPI
NTSTATUS(*pfnPsSetCreateThreadNotifyRoutine)(
	__in PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

typedef
NTSTATUS(*pfnCmUnRegisterCallback)(__in LARGE_INTEGER    Cookie);

typedef
//NTKERNELAPI
BOOLEAN(*pfnKeRegisterBugCheckCallback)(
	__out PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
	__in PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
	__in PVOID Buffer,
	__in ULONG Length,
	__in PUCHAR Component);


typedef
//NTKERNELAPI
BOOLEAN(*pfnKeRegisterBugCheckReasonCallback)(
	__out PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
	__in PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
	__in KBUGCHECK_CALLBACK_REASON Reason,
	__in PUCHAR Component);


typedef
//NTKERNELAPI
NTSTATUS(*pfnIoRegisterShutdownNotification)(
	IN PDEVICE_OBJECT DeviceObject);



extern DYNAMIC_DATA g_DynamicData;

//////////////////////////////////////////////////////////////////////////
UINT_PTR
APGetPspLoadImageNotifyRoutineAddress()
{
	// 在 PsSetLoadImageNotifyRoutine 中使用了 PspLoadImageNotifyRoutine 硬编码加偏移获得
	/*
	0: kd> u PsSetLoadImageNotifyRoutine l 10
	nt!PsSetLoadImageNotifyRoutine:
	fffff800`0429cb70 48895c2408      mov     qword ptr [rsp+8],rbx
	fffff800`0429cb75 57              push    rdi
	fffff800`0429cb76 4883ec20        sub     rsp,20h
	fffff800`0429cb7a 33d2            xor     edx,edx
	fffff800`0429cb7c e80fb0feff      call    nt!ExAllocateCallBack (fffff800`04287b90)
	fffff800`0429cb81 488bf8          mov     rdi,rax
	fffff800`0429cb84 4885c0          test    rax,rax
	fffff800`0429cb87 7507            jne     nt!PsSetLoadImageNotifyRoutine+0x20 (fffff800`0429cb90)
	fffff800`0429cb89 b89a0000c0      mov     eax,0C000009Ah
	fffff800`0429cb8e eb4a            jmp     nt!PsSetLoadImageNotifyRoutine+0x6a (fffff800`0429cbda)
	fffff800`0429cb90 33db            xor     ebx,ebx
	fffff800`0429cb92 488d0d87c1d9ff  lea     rcx,[nt!PspLoadImageNotifyRoutine (fffff800`04038d20)]
	fffff800`0429cb99 4533c0          xor     r8d,r8d
	fffff800`0429cb9c 488bd7          mov     rdx,rdi
	fffff800`0429cb9f 488d0cd9        lea     rcx,[rcx+rbx*8]
	fffff800`0429cba3 e89815f8ff      call    nt!ExCompareExchangeCallBack (fffff800`0421e140)
	0: kd> u fffff800`04038d20
	nt!PspLoadImageNotifyRoutine:
	fffff800`04038d20 ef              out     dx,eax
	fffff800`04038d21 420900          or      dword ptr [rax],eax
	fffff800`04038d24 a0f8ffff0000000000 mov   al,byte ptr [0000000000FFFFF8h]
	fffff800`04038d2d 0000            add     byte ptr [rax],al
	fffff800`04038d2f 0000            add     byte ptr [rax],al
	fffff800`04038d31 0000            add     byte ptr [rax],al
	fffff800`04038d33 0000            add     byte ptr [rax],al
	fffff800`04038d35 0000            add     byte ptr [rax],al
	*/

	pfnPsSetLoadImageNotifyRoutine PsSetLoadImageNotifyRoutine = NULL;

	APGetNtosExportVariableAddress(L"PsSetLoadImageNotifyRoutine", (PVOID*)&PsSetLoadImageNotifyRoutine);
	DbgPrint("%p\r\n", PsSetLoadImageNotifyRoutine);

	if (PsSetLoadImageNotifyRoutine != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)PsSetLoadImageNotifyRoutine;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					return (UINT_PTR)(iOffset + (UINT64)i + 7);
				}
			}
		}
	}
	return 0;
}

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
		for (i = 0; i < 64; i++)		// ???? 理论上数组大小应该只有8才对
		{
			UINT_PTR NotifyItem = 0;		// PspLoadImageNotifyRoutine数组每一项成员
			UINT_PTR CallbackAddress = 0;	// 真实的回调例程地址

			if (MmIsAddressValid((PVOID)(PspLoadImageNotifyRoutine + i * sizeof(UINT_PTR))))
			{
				NotifyItem = *(PUINT_PTR)(PspLoadImageNotifyRoutine + i * sizeof(UINT_PTR));

				if (NotifyItem > g_DynamicData.MinKernelSpaceAddress &&
					MmIsAddressValid((PVOID)(NotifyItem & ~7)))
				{
					CallbackAddress = *(PUINT_PTR)(NotifyItem & ~7);

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


UINT_PTR
APGetPspCreateThreadNotifyRoutineAddress()
{
	// 在 PsSetCreateThreadNotifyRoutine 中使用了 PspCreateThreadNotifyRoutine 硬编码加偏移获得
	/*
	2: kd> u PsSetCreateThreadNotifyRoutine l 10
	nt!PsSetCreateThreadNotifyRoutine:
	fffff800`0428cbf0 48895c2408      mov     qword ptr [rsp+8],rbx
	fffff800`0428cbf5 57              push    rdi
	fffff800`0428cbf6 4883ec20        sub     rsp,20h
	fffff800`0428cbfa 33d2            xor     edx,edx
	fffff800`0428cbfc e88faffeff      call    nt!ExAllocateCallBack (fffff800`04277b90)
	fffff800`0428cc01 488bf8          mov     rdi,rax
	fffff800`0428cc04 4885c0          test    rax,rax
	fffff800`0428cc07 7507            jne     nt!PsSetCreateThreadNotifyRoutine+0x20 (fffff800`0428cc10)
	fffff800`0428cc09 b89a0000c0      mov     eax,0C000009Ah
	fffff800`0428cc0e eb4a            jmp     nt!PsSetCreateThreadNotifyRoutine+0x6a (fffff800`0428cc5a)
	fffff800`0428cc10 33db            xor     ebx,ebx
	fffff800`0428cc12 488d0d67c1d9ff  lea     rcx,[nt!PspCreateThreadNotifyRoutine (fffff800`04028d80)]

	*/

	pfnPsSetCreateThreadNotifyRoutine PsSetCreateThreadNotifyRoutine = NULL;

	APGetNtosExportVariableAddress(L"PsSetCreateThreadNotifyRoutine", (PVOID*)&PsSetCreateThreadNotifyRoutine);
	DbgPrint("%p\r\n", PsSetCreateThreadNotifyRoutine);

	if (PsSetCreateThreadNotifyRoutine != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)PsSetCreateThreadNotifyRoutine;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					return (UINT_PTR)(iOffset + (UINT64)i + 7);
				}
			}
		}
	}
	return 0;
}



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
		for (i = 0; i < 64; i++)		// ???? 理论上数组大小应该只有8才对
		{
			UINT_PTR NotifyItem = 0;		// PspCreateThreadNotifyRoutine数组每一项成员
			UINT_PTR CallbackAddress = 0;	// 真实的回调例程地址

			if (MmIsAddressValid((PVOID)(PspCreateThreadNotifyRoutine + i * sizeof(UINT_PTR))))
			{
				NotifyItem = *(PUINT_PTR)(PspCreateThreadNotifyRoutine + i * sizeof(UINT_PTR));

				if (NotifyItem > g_DynamicData.MinKernelSpaceAddress &&
					MmIsAddressValid((PVOID)(NotifyItem & ~7)))
				{
					CallbackAddress = *(PUINT_PTR)(NotifyItem & ~7);

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



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Win7之后 都是ListEntry结构 xp之前 都是Vector数组结构
UINT_PTR
APGetCallbackListHeadAddress()
{
	/*
	0: kd> u CmUnRegisterCallback l 30
	nt!CmUnRegisterCallback:
	fffff800`042c67d0 48894c2408      mov     qword ptr [rsp+8],rcx
	fffff800`042c67d5 53              push    rbx
	fffff800`042c67d6 56              push    rsi
	fffff800`042c67d7 57              push    rdi
	fffff800`042c67d8 4154            push    r12
	fffff800`042c67da 4155            push    r13
	fffff800`042c67dc 4156            push    r14
	fffff800`042c67de 4157            push    r15
	fffff800`042c67e0 4883ec60        sub     rsp,60h
	fffff800`042c67e4 41bc0d0000c0    mov     r12d,0C000000Dh
	fffff800`042c67ea 4489a424b0000000 mov     dword ptr [rsp+0B0h],r12d
	fffff800`042c67f2 33db            xor     ebx,ebx
	fffff800`042c67f4 48895c2448      mov     qword ptr [rsp+48h],rbx
	fffff800`042c67f9 33c0            xor     eax,eax
	fffff800`042c67fb 4889442450      mov     qword ptr [rsp+50h],rax
	fffff800`042c6800 4889442458      mov     qword ptr [rsp+58h],rax
	fffff800`042c6805 448d6b01        lea     r13d,[rbx+1]
	fffff800`042c6809 458afd          mov     r15b,r13b
	fffff800`042c680c 4488ac24a8000000 mov     byte ptr [rsp+0A8h],r13b
	fffff800`042c6814 440f20c7        mov     rdi,cr8
	fffff800`042c6818 450f22c5        mov     cr8,r13
	fffff800`042c681c f00fba357b91dcff00 lock btr dword ptr [nt!CallbackUnregisterLock (fffff800`0408f9a0)],0
	fffff800`042c6825 720c            jb      nt!CmUnRegisterCallback+0x63 (fffff800`042c6833)
	fffff800`042c6827 488d0d7291dcff  lea     rcx,[nt!CallbackUnregisterLock (fffff800`0408f9a0)]
	fffff800`042c682e e89d50b9ff      call    nt!KiAcquireFastMutex (fffff800`03e5b8d0)
	fffff800`042c6833 65488b042588010000 mov   rax,qword ptr gs:[188h]
	fffff800`042c683c 4889056591dcff  mov     qword ptr [nt!CallbackUnregisterLock+0x8 (fffff800`0408f9a8)],rax
	fffff800`042c6843 400fb6c7        movzx   eax,dil
	fffff800`042c6847 89058391dcff    mov     dword ptr [nt!CallbackUnregisterLock+0x30 (fffff800`0408f9d0)],eax
	fffff800`042c684d 48c78424b80000009cffffff mov qword ptr [rsp+0B8h],0FFFFFFFFFFFFFF9Ch
	fffff800`042c6859 48895c2420      mov     qword ptr [rsp+20h],rbx
	fffff800`042c685e 65488b042588010000 mov   rax,qword ptr gs:[188h]
	fffff800`042c6867 4183ceff        or      r14d,0FFFFFFFFh
	fffff800`042c686b 664401b0c4010000 add     word ptr [rax+1C4h],r14w
	fffff800`042c6873 f0480fba2d0b91dcff00 lock bts qword ptr [nt!CallbackListLock (fffff800`0408f988)],0
	fffff800`042c687d 730c            jae     nt!CmUnRegisterCallback+0xbb (fffff800`042c688b)
	fffff800`042c687f 488d0d0291dcff  lea     rcx,[nt!CallbackListLock (fffff800`0408f988)]
	fffff800`042c6886 e83543bbff      call    nt!ExfAcquirePushLockExclusive (fffff800`03e7abc0)
	fffff800`042c688b 418af5          mov     sil,r13b
	fffff800`042c688e 4c8b9424a0000000 mov     r10,qword ptr [rsp+0A0h]
	fffff800`042c6896 4533c0          xor     r8d,r8d
	fffff800`042c6899 488d542420      lea     rdx,[rsp+20h]
	fffff800`042c689e 488d0dcb90dcff  lea     rcx,[nt!CallbackListHead (fffff800`0408f970)]

	*/

	pfnCmUnRegisterCallback CmUnRegisterCallback = NULL;

	APGetNtosExportVariableAddress(L"CmUnRegisterCallback", (PVOID*)&CmUnRegisterCallback);
	DbgPrint("%p\r\n", CmUnRegisterCallback);

	if (CmUnRegisterCallback != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)CmUnRegisterCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
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
							return (UINT_PTR)(iOffset + (UINT64)i + 7);
						}
					}
				}
			}
		}
	}

	return 0;
}

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
		PLIST_ENTRY			NotifyListEntry = (PLIST_ENTRY)(*(PUINT_PTR)CallbackListHead);	// Flink

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

		do
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
			NotifyListEntry = NotifyListEntry->Flink;
		} while (NotifyListEntry != ((PLIST_ENTRY)(*(PUINT_PTR)CallbackListHead)));
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

UINT_PTR
APGetKeBugCheckCallbackListHeadAddress()
{
	/*
	0: kd> u KeRegisterBugCheckCallback l 50
	nt!KeRegisterBugCheckCallback:
	fffff800`03f390b0 48895c2420      mov     qword ptr [rsp+20h],rbx
	fffff800`03f390b5 4c89442418      mov     qword ptr [rsp+18h],r8
	fffff800`03f390ba 4889542410      mov     qword ptr [rsp+10h],rdx
	fffff800`03f390bf 55              push    rbp
	fffff800`03f390c0 56              push    rsi
	fffff800`03f390c1 57              push    rdi
	fffff800`03f390c2 4154            push    r12
	fffff800`03f390c4 4155            push    r13
	fffff800`03f390c6 4156            push    r14
	fffff800`03f390c8 4157            push    r15
	fffff800`03f390ca 4883ec30        sub     rsp,30h
	fffff800`03f390ce 458bf9          mov     r15d,r9d
	fffff800`03f390d1 488bf9          mov     rdi,rcx
	fffff800`03f390d4 450f20c6        mov     r14,cr8
	fffff800`03f390d8 b80f000000      mov     eax,0Fh
	fffff800`03f390dd 440f22c0        mov     cr8,rax
	fffff800`03f390e1 65488b342520000000 mov   rsi,qword ptr gs:[20h]
	fffff800`03f390ea 4533c9          xor     r9d,r9d
	fffff800`03f390ed 0fba254f020e0010 bt      dword ptr [nt!PerfGlobalGroupMask+0x4 (fffff800`04019344)],10h
	fffff800`03f390f5 8d58f2          lea     ebx,[rax-0Eh]
	fffff800`03f390f8 7318            jae     nt!KeRegisterBugCheckCallback+0x62 (fffff800`03f39112)
	fffff800`03f390fa 408aeb          mov     bpl,bl
	fffff800`03f390fd 0f31            rdtsc
	fffff800`03f390ff 448bae00470000  mov     r13d,dword ptr [rsi+4700h]
	fffff800`03f39106 48c1e220        shl     rdx,20h
	fffff800`03f3910a 480bc2          or      rax,rdx
	fffff800`03f3910d 4c8be0          mov     r12,rax
	fffff800`03f39110 eb0d            jmp     nt!KeRegisterBugCheckCallback+0x6f (fffff800`03f3911f)
	fffff800`03f39112 4c8b642470      mov     r12,qword ptr [rsp+70h]
	fffff800`03f39117 448b6c2470      mov     r13d,dword ptr [rsp+70h]
	fffff800`03f3911c 4032ed          xor     bpl,bpl
	fffff800`03f3911f 019e004b0000    add     dword ptr [rsi+4B00h],ebx
	fffff800`03f39125 f0480fba2de111150000 lock bts qword ptr [nt!KeBugCheckCallbackLock (fffff800`0408a310)],0
	fffff800`03f3912f 731d            jae     nt!KeRegisterBugCheckCallback+0x9e (fffff800`03f3914e)
	fffff800`03f39131 488d0dd8111500  lea     rcx,[nt!KeBugCheckCallbackLock (fffff800`0408a310)]
	fffff800`03f39138 e8037bf3ff      call    nt!KxWaitForSpinLockAndAcquire (fffff800`03e70c40)
	fffff800`03f3913d 019e044b0000    add     dword ptr [rsi+4B04h],ebx
	fffff800`03f39143 0186084b0000    add     dword ptr [rsi+4B08h],eax
	fffff800`03f39149 448bc8          mov     r9d,eax
	fffff800`03f3914c eb03            jmp     nt!KeRegisterBugCheckCallback+0xa1 (fffff800`03f39151)
	fffff800`03f3914e 0faee8          lfence
	fffff800`03f39151 4084ed          test    bpl,bpl
	fffff800`03f39154 7428            je      nt!KeRegisterBugCheckCallback+0xce (fffff800`03f3917e)
	fffff800`03f39156 0f31            rdtsc
	fffff800`03f39158 48c1e220        shl     rdx,20h
	fffff800`03f3915c 488d0dad111500  lea     rcx,[nt!KeBugCheckCallbackLock (fffff800`0408a310)]
	fffff800`03f39163 c644242800      mov     byte ptr [rsp+28h],0
	fffff800`03f39168 480bc2          or      rax,rdx
	fffff800`03f3916b 44896c2420      mov     dword ptr [rsp+20h],r13d
	fffff800`03f39170 448bc0          mov     r8d,eax
	fffff800`03f39173 488bd0          mov     rdx,rax
	fffff800`03f39176 452bc4          sub     r8d,r12d
	fffff800`03f39179 e83263feff      call    nt!PerfLogSpinLockAcquire (fffff800`03f1f4b0)
	fffff800`03f3917e 4032f6          xor     sil,sil
	fffff800`03f39181 40387738        cmp     byte ptr [rdi+38h],sil
	fffff800`03f39185 7559            jne     nt!KeRegisterBugCheckCallback+0x130 (fffff800`03f391e0)
	fffff800`03f39187 488b8c2490000000 mov     rcx,qword ptr [rsp+90h]
	fffff800`03f3918f 4c8b442478      mov     r8,qword ptr [rsp+78h]
	fffff800`03f39194 488b942480000000 mov     rdx,qword ptr [rsp+80h]
	fffff800`03f3919c 48894f28        mov     qword ptr [rdi+28h],rcx
	fffff800`03f391a0 4c894710        mov     qword ptr [rdi+10h],r8
	fffff800`03f391a4 48895718        mov     qword ptr [rdi+18h],rdx
	fffff800`03f391a8 44897f20        mov     dword ptr [rdi+20h],r15d
	fffff800`03f391ac 885f38          mov     byte ptr [rdi+38h],bl
	fffff800`03f391af 4b8d0438        lea     rax,[r8+r15]
	fffff800`03f391b3 4803c2          add     rax,rdx
	fffff800`03f391b6 408af3          mov     sil,bl
	fffff800`03f391b9 4803c1          add     rax,rcx
	fffff800`03f391bc 488d0d7d111500  lea     rcx,[nt!KeBugCheckCallbackListHead (fffff800`0408a340)]

	*/

	pfnKeRegisterBugCheckCallback KeRegisterBugCheckCallback = NULL;

	APGetNtosExportVariableAddress(L"KeRegisterBugCheckCallback", (PVOID*)&KeRegisterBugCheckCallback);
	DbgPrint("%p\r\n", KeRegisterBugCheckCallback);

	if (KeRegisterBugCheckCallback != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeRegisterBugCheckCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
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
							return (UINT_PTR)(iOffset + (UINT64)i + 7);
						}
					}
				}
			}
		}
	}
	return 0;
}

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
		fffff880`014f1b0a 754b            jne     fffff880`014f1b57
		fffff880`014f1b0c 4c8b81f80c0000  mov     r8,qword ptr [rcx+0CF8h]
		fffff880`014f1b13 0fba697c18      bts     dword ptr [rcx+7Ch],18h
		fffff880`014f1b18 4d85c0          test    r8,r8
		fffff880`014f1b1b 743a            je      fffff880`014f1b57
		fffff880`014f1b1d 8b818c140000    mov     eax,dword ptr [rcx+148Ch]

		*/

		UINT_PTR     Dispatch = 0;
/*		PLIST_ENTRY	 DispatchListEntry = ((PLIST_ENTRY)KeBugCheckCallbackListHead)->Flink;	// Flink

		do
		{
			Dispatch = *(PUINT_PTR)((PUINT8)DispatchListEntry + sizeof(LIST_ENTRY));	// 过ListEntry
			if (Dispatch && MmIsAddressValid((PVOID)Dispatch))
			{
				UINT_PTR CurrentCount = sci->NumberOfCallbacks;
				if (NumberOfCallbacks > CurrentCount)
				{
					sci->Callbacks[CurrentCount].Type = NotifyKeBugCheck;
					sci->Callbacks[CurrentCount].CallbackAddress = Dispatch;
					sci->Callbacks[CurrentCount].Description = DispatchListEntry;
				}
				sci->NumberOfCallbacks++;
			}
			DispatchListEntry = DispatchListEntry->Flink;
		} while (DispatchListEntry != KeBugCheckCallbackListHead);
		*/

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

//////////////////////////////////////////////////////////////////////////
// 发现 KeBugCheckCallbackListHead 刚好就在 KeBugCheckReasonCallbackListHead的后面
//////////////////////////////////////////////////////////////////////////

UINT_PTR
APGetKeBugCheckReasonCallbackListHeadAddress()
{
	/*
	0: kd> u KeRegisterBugCheckReasonCallback l 50
	nt!KeRegisterBugCheckReasonCallback:
	fffff800`03f38da0 48895c2418      mov     qword ptr [rsp+18h],rbx
	fffff800`03f38da5 4c894c2420      mov     qword ptr [rsp+20h],r9
	fffff800`03f38daa 4889542410      mov     qword ptr [rsp+10h],rdx
	fffff800`03f38daf 55              push    rbp
	fffff800`03f38db0 56              push    rsi
	fffff800`03f38db1 57              push    rdi
	fffff800`03f38db2 4154            push    r12
	fffff800`03f38db4 4155            push    r13
	fffff800`03f38db6 4156            push    r14
	fffff800`03f38db8 4157            push    r15
	fffff800`03f38dba 4883ec30        sub     rsp,30h
	fffff800`03f38dbe bf01000000      mov     edi,1
	fffff800`03f38dc3 4963e8          movsxd  rbp,r8d
	fffff800`03f38dc6 488bd9          mov     rbx,rcx
	fffff800`03f38dc9 448aef          mov     r13b,dil
	fffff800`03f38dcc 440f20c0        mov     rax,cr8
	fffff800`03f38dd0 4889442470      mov     qword ptr [rsp+70h],rax
	fffff800`03f38dd5 8d470e          lea     eax,[rdi+0Eh]
	fffff800`03f38dd8 440f22c0        mov     cr8,rax
	fffff800`03f38ddc 65488b342520000000 mov   rsi,qword ptr gs:[20h]
	fffff800`03f38de5 4533c9          xor     r9d,r9d
	fffff800`03f38de8 0fba2554050e0010 bt      dword ptr [nt!PerfGlobalGroupMask+0x4 (fffff800`04019344)],10h
	fffff800`03f38df0 7318            jae     nt!KeRegisterBugCheckReasonCallback+0x6a (fffff800`03f38e0a)
	fffff800`03f38df2 448ae7          mov     r12b,dil
	fffff800`03f38df5 0f31            rdtsc
	fffff800`03f38df7 448bbe00470000  mov     r15d,dword ptr [rsi+4700h]
	fffff800`03f38dfe 48c1e220        shl     rdx,20h
	fffff800`03f38e02 480bc2          or      rax,rdx
	fffff800`03f38e05 4c8bf0          mov     r14,rax
	fffff800`03f38e08 eb0d            jmp     nt!KeRegisterBugCheckReasonCallback+0x77 (fffff800`03f38e17)
	fffff800`03f38e0a 4c8b742470      mov     r14,qword ptr [rsp+70h]
	fffff800`03f38e0f 448b7c2470      mov     r15d,dword ptr [rsp+70h]
	fffff800`03f38e14 4532e4          xor     r12b,r12b
	fffff800`03f38e17 01be004b0000    add     dword ptr [rsi+4B00h],edi
	fffff800`03f38e1d f0480fba2de914150000 lock bts qword ptr [nt!KeBugCheckCallbackLock (fffff800`0408a310)],0
	fffff800`03f38e27 731d            jae     nt!KeRegisterBugCheckReasonCallback+0xa6 (fffff800`03f38e46)
	fffff800`03f38e29 488d0de0141500  lea     rcx,[nt!KeBugCheckCallbackLock (fffff800`0408a310)]
	fffff800`03f38e30 e80b7ef3ff      call    nt!KxWaitForSpinLockAndAcquire (fffff800`03e70c40)
	fffff800`03f38e35 01be044b0000    add     dword ptr [rsi+4B04h],edi
	fffff800`03f38e3b 0186084b0000    add     dword ptr [rsi+4B08h],eax
	fffff800`03f38e41 448bc8          mov     r9d,eax
	fffff800`03f38e44 eb03            jmp     nt!KeRegisterBugCheckReasonCallback+0xa9 (fffff800`03f38e49)
	fffff800`03f38e46 0faee8          lfence
	fffff800`03f38e49 4584e4          test    r12b,r12b
	fffff800`03f38e4c 7428            je      nt!KeRegisterBugCheckReasonCallback+0xd6 (fffff800`03f38e76)
	fffff800`03f38e4e 0f31            rdtsc
	fffff800`03f38e50 48c1e220        shl     rdx,20h
	fffff800`03f38e54 488d0db5141500  lea     rcx,[nt!KeBugCheckCallbackLock (fffff800`0408a310)]
	fffff800`03f38e5b c644242800      mov     byte ptr [rsp+28h],0
	fffff800`03f38e60 480bc2          or      rax,rdx
	fffff800`03f38e63 44897c2420      mov     dword ptr [rsp+20h],r15d
	fffff800`03f38e68 448bc0          mov     r8d,eax
	fffff800`03f38e6b 488bd0          mov     rdx,rax
	fffff800`03f38e6e 452bc6          sub     r8d,r14d
	fffff800`03f38e71 e83a66feff      call    nt!PerfLogSpinLockAcquire (fffff800`03f1f4b0)
	fffff800`03f38e76 807b2c00        cmp     byte ptr [rbx+2Ch],0
	fffff800`03f38e7a 7570            jne     nt!KeRegisterBugCheckReasonCallback+0x14c (fffff800`03f38eec)
	fffff800`03f38e7c 488b542478      mov     rdx,qword ptr [rsp+78h]
	fffff800`03f38e81 488b8c2488000000 mov     rcx,qword ptr [rsp+88h]
	fffff800`03f38e89 896b28          mov     dword ptr [rbx+28h],ebp
	fffff800`03f38e8c 488d042a        lea     rax,[rdx+rbp]
	fffff800`03f38e90 48895310        mov     qword ptr [rbx+10h],rdx
	fffff800`03f38e94 48894b18        mov     qword ptr [rbx+18h],rcx
	fffff800`03f38e98 4803c1          add     rax,rcx
	fffff800`03f38e9b 40887b2c        mov     byte ptr [rbx+2Ch],dil
	fffff800`03f38e9f 48894320        mov     qword ptr [rbx+20h],rax
	fffff800`03f38ea3 83fd04          cmp     ebp,4
	fffff800`03f38ea6 7422            je      nt!KeRegisterBugCheckReasonCallback+0x12a (fffff800`03f38eca)
	fffff800`03f38ea8 488b0581141500  mov     rax,qword ptr [nt!KeBugCheckReasonCallbackListHead (fffff800`0408a330)]
	fffff800`03f38eaf 488d0d7a141500  lea     rcx,[nt!KeBugCheckReasonCallbackListHead (fffff800`0408a330)]


	*/

	pfnKeRegisterBugCheckReasonCallback KeRegisterBugCheckReasonCallback = NULL;

	APGetNtosExportVariableAddress(L"KeRegisterBugCheckReasonCallback", (PVOID*)&KeRegisterBugCheckReasonCallback);
	DbgPrint("%p\r\n", KeRegisterBugCheckReasonCallback);

	if (KeRegisterBugCheckReasonCallback != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeRegisterBugCheckReasonCallback;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL, j = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
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
							return (UINT_PTR)(iOffset + (UINT64)i + 7);
						}
					}
				}
			}
		}
	}
	return 0;
}

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
		fffffa80`18dd07d8  fffff300`19d19856 00000001`00000002
		fffffa80`18dd07e8  00000000`00000000 0000057f`e71ee188
		fffffa80`18dd07f8  696e6f6d`00000000 00726f74`696e6f6d
		fffffa80`18dd0808  00000000`00000000 00000000`00000000
		fffffa80`18dd0818  00000000`00000000 00000000`00000001
		fffffa80`18dd0828  00000000`00000000 206c6148`0225001a

		2: kd> u fffff880`00f49054
		fffff880`00f49054 48895c2408      mov     qword ptr [rsp+8],rbx
		fffff880`00f49059 4889742410      mov     qword ptr [rsp+10h],rsi
		fffff880`00f4905e 57              push    rdi
		fffff880`00f4905f 4883ec20        sub     rsp,20h
		fffff880`00f49063 4181780c00100000 cmp     dword ptr [r8+0Ch],1000h
		fffff880`00f4906b 498bf8          mov     rdi,r8
		fffff880`00f4906e 727d            jb      fffff880`00f490ed
		fffff880`00f49070 488d9ae8feffff  lea     rbx,[rdx-118h]

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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

UINT_PTR
APGetIopNotifyShutdownQueueHeadAddress()
{
	/*
	2: kd> u IoRegisterShutdownNotification l 20
	nt!IoRegisterShutdownNotification:
	fffff800`04278f50 48895c2408      mov     qword ptr [rsp+8],rbx
	fffff800`04278f55 57              push    rdi
	fffff800`04278f56 4883ec20        sub     rsp,20h
	fffff800`04278f5a 488bd9          mov     rbx,rcx
	fffff800`04278f5d ba18000000      mov     edx,18h
	fffff800`04278f62 41b8496f5368    mov     r8d,68536F49h
	fffff800`04278f68 33c9            xor     ecx,ecx
	fffff800`04278f6a e8a14cd3ff      call    nt!ExAllocatePoolWithTag (fffff800`03fadc10)
	fffff800`04278f6f 488bf8          mov     rdi,rax
	fffff800`04278f72 4885c0          test    rax,rax
	fffff800`04278f75 7507            jne     nt!IoRegisterShutdownNotification+0x2e (fffff800`04278f7e)
	fffff800`04278f77 b89a0000c0      mov     eax,0C000009Ah
	fffff800`04278f7c eb22            jmp     nt!IoRegisterShutdownNotification+0x50 (fffff800`04278fa0)
	fffff800`04278f7e 488bcb          mov     rcx,rbx
	fffff800`04278f81 48895810        mov     qword ptr [rax+10h],rbx
	fffff800`04278f85 e866c7c0ff      call    nt!ObfReferenceObject (fffff800`03e856f0)
	fffff800`04278f8a 488d0dff59e0ff  lea     rcx,[nt!IopNotifyShutdownQueueHead (fffff800`0407e990)]

	*/

	pfnIoRegisterShutdownNotification IoRegisterShutdownNotification = NULL;

	APGetNtosExportVariableAddress(L"IoRegisterShutdownNotification", (PVOID*)&IoRegisterShutdownNotification);
	DbgPrint("%p\r\n", IoRegisterShutdownNotification);

	if (IoRegisterShutdownNotification != NULL)
	{
		PUINT8	StartSearchAddress = (PUINT8)IoRegisterShutdownNotification;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8d && v3 == 0x0d)		// 硬编码  lea rcx
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					return iOffset + (UINT64)i + 7;
				}
			}
		}
	}
	return 0;
}

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
			UINT_PTR Address = (UINT_PTR)((PUINT8)DispatchListEntry + sizeof(LIST_ENTRY));   // 过ListEntry

			if (Address && MmIsAddressValid((PVOID)Address))
			{
				PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)(*(PUINT_PTR)Address);

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

	APGetLoadImageCallbackNotify(sci, CallbackCount);		// 映像加载 卸载回调
	APGetCreateThreadCallbackNotify(sci, CallbackCount);   // 创建线程回调
	APGetRegisterCallbackNotify(sci, CallbackCount);       // 注册表回调
	APGetBugCheckCallbackNotify(sci, CallbackCount);       // 错误检查回调
	APGetBugCheckReasonCallbackNotify(sci, CallbackCount); // 
	APGetShutDownCallbackNotify(sci, CallbackCount);       // 关机回调

	// 创建进程回调
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


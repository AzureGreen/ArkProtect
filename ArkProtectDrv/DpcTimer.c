#include "DpcTimer.h"

typedef
BOOLEAN(*pfnKeSetTimer)(
	__inout PKTIMER Timer,
	__in LARGE_INTEGER DueTime,
	__in_opt PKDPC Dpc);


#ifdef _WIN64

/************************************************************************
*  Name : FindKiWaitVariableAddress
*  Param: KiWaitNeverAddress
*  Param: KiWaitAlwaysAddress
*  Ret  : BOOLEAN
*  通过 KeSetTimer 和硬编码 获得 未导出变量 KiWaitNeverAddress & KiWaitAlwaysAddress
************************************************************************/
BOOLEAN
FindKiWaitVariableAddress(OUT PUINT_PTR* KiWaitNeverAddress, OUT PUINT_PTR* KiWaitAlwaysAddress)
{
	pfnKeSetTimer KeSetTimer = NULL;

	APGetNtosExportVariableAddress(L"KeSetTimer", (PVOID*)&KeSetTimer);

	if (KeSetTimer != NULL)
	{
		/*
		1: kd> u KeSetTimer l 50
		nt!KeSetTimer:
		fffff800`03ed28b0 4883ec38        sub     rsp,38h
		......
		fffff800`03ed28e9 488b0530c82200  mov     rax,qword ptr [nt!KiWaitNever (fffff800`040ff120)]
		fffff800`03ed28f0 488b1d19c92200  mov     rbx,qword ptr [nt!KiWaitAlways (fffff800`040ff210)]

		*/

		UINT8   v1 = 0, v2 = 0, v3 = 0;
		PUINT8	StartSearchAddress = NULL;
		PUINT8	EndSearchAddress = NULL;
		PUINT8	i = NULL;
		INT32   iOffset = 0;

		StartSearchAddress = (PUINT8)KeSetTimer;
		EndSearchAddress = StartSearchAddress + 0x500;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x48 && v2 == 0x8b && v3 == 0x05)		// 硬编码  mov rax
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					*KiWaitNeverAddress = (PUINT64)(iOffset + (UINT64)i + 7);
					i += 7;
					RtlCopyMemory(&iOffset, i + 3, 4);
					*KiWaitAlwaysAddress = (PUINT64)(iOffset + (UINT64)i + 7);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


/************************************************************************
*  Name : APTransTimerDPCEx
*  Param: Timer				Timer对象
*  Param: KiWaitNeverAddress
*  Param: KiWaitAlwaysAddress
*  Ret  : PKDPC
*  转化Timer对象里的DPC为真实的DPC(我们看得懂的形式)
************************************************************************/
PKDPC
APTransTimerDPCEx(IN PKTIMER Timer, IN UINT64 KiWaitNeverAddress, IN UINT64 KiWaitAlwaysAddress)
{
	UINT_PTR Dpc = (UINT_PTR)Timer->Dpc;
	Dpc ^= KiWaitNeverAddress;
	Dpc = _rotl64(Dpc, (UINT8)(KiWaitNeverAddress & 0xFF));		//左循环 移动Shift位
	Dpc ^= (UINT_PTR)Timer;
	Dpc = _byteswap_uint64(Dpc);
	Dpc ^= KiWaitAlwaysAddress;
	return (PKDPC)Dpc;
}


/************************************************************************
*  Name : APEnumDpcTimerByIterateKTimerTableEntry
*  Param: dti
*  Param: DpcTimerCount
*  Ret  : NTSTATUS
*  通过遍历KTIMER_TABLE_ENTRY数组里面ListEntry枚举DpcTimer对象信息
************************************************************************/
NTSTATUS
APEnumDpcTimerByIterateKTimerTableEntry(OUT PDPC_TIMER_INFORMATION dti, IN UINT32 DpcTimerCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UINT32   CpuNumber = KeNumberProcessors;	// 全局变量 CPU个数

	for (UINT32 i = 0; i < CpuNumber; i++)
	{
		BOOLEAN   bOk = FALSE;
		PUINT_PTR KiWaitAlwaysAddress = NULL;
		PUINT_PTR KiWaitNeverAddress = NULL;
		UINT_PTR  PrcbAddress = 0;
		PUINT8    TimerEntries = NULL;

		KeSetSystemAffinityThread(i + 1);	// 使当前线程运行在第一个处理器上，因为只有第一个处理器的值才有效

		/*
		3: kd> rdmsr 0xc0000101
		msr[c0000101] = fffff880`045d5000

		3: kd> dq fffff880`045d5000+20		// 获得KPRCB地址
		fffff880`045d5020  fffff880`045d5180 fffff880`045d57f0

		kd> dt _Kprcb
		ntdll!_KPRCB
		+0x000 MxCsr            : Uint4B
		+0x004 LegacyNumber     : UChar
		......
		+0x2200 TimerTable      : _KTIMER_TABLE

		3: kd> dt _KTIMER_TABLE
		nt!_KTIMER_TABLE
		+0x000 TimerExpiry      : [64] Ptr64 _KTIMER
		+0x200 TimerEntries     : [256] _KTIMER_TABLE_ENTRY

		3: kd> dt _KTIMER_TABLE_ENTRY fffff880`045d5180+2200+200
		nt!_KTIMER_TABLE_ENTRY
		+0x000 Lock             : 0
		+0x008 Entry            : _LIST_ENTRY [ 0xfffffa80`1a9c1e90 - 0xfffffa80`1a9c1e90 ]
		+0x018 Time             : _ULARGE_INTEGER 0x8`6ca34f0e

		*/

		PrcbAddress = (UINT_PTR)__readmsr(0xC0000101) + 0x20;

		KeRevertToUserAffinityThread();	  // 恢复线程运行的处理器

		TimerEntries = (PUINT8)(*(PUINT64)PrcbAddress + 0x2200 + 0x200);	// _KTIMER_TABLE_ENTRY

		bOk = FindKiWaitVariableAddress(&KiWaitNeverAddress, &KiWaitAlwaysAddress);
		if (bOk == TRUE)
		{
			KIRQL OldIrql = KeRaiseIrqlToDpcLevel();

			// 256个KTIMER_TABLE_ENTRY数组
			for (UINT32 j = 0; j < 0x100; j++)
			{
				PLIST_ENTRY CurrentListEntry = (PLIST_ENTRY)(TimerEntries + sizeof(KTIMER_TABLE_ENTRY) * j + sizeof(UINT64));  // 每次定位到数组的List_Entry成员
				if (CurrentListEntry && MmIsAddressValid(CurrentListEntry))
				{
					for (PLIST_ENTRY TravelListEntry = CurrentListEntry->Flink;
						MmIsAddressValid(TravelListEntry) && CurrentListEntry != TravelListEntry;
						TravelListEntry = TravelListEntry->Flink)
					{
						//获得首地址
						PKTIMER Timer = CONTAINING_RECORD(TravelListEntry, KTIMER, TimerListEntry);
						/*
						kd> dt _KTIMER
						nt!_KTIMER
						+0x000 Header           : _DISPATCHER_HEADER
						+0x018 DueTime          : _ULARGE_INTEGER
						+0x020 TimerListEntry   : _LIST_ENTRY
						+0x030 Dpc              : Ptr64 _KDPC
						+0x038 Processor        : Uint4B
						+0x03c Period           : Uint4B
						*/
						PKDPC RealDpc = APTransTimerDPCEx(Timer, *KiWaitNeverAddress, *KiWaitAlwaysAddress);
						if (MmIsAddressValid(Timer) && MmIsAddressValid(RealDpc) && MmIsAddressValid(RealDpc->DeferredRoutine))
						{
							if (DpcTimerCount > dti->NumberOfDpcTimers)
							{
								DbgPrint("DPC对象:%p\r\n", (UINT_PTR)RealDpc);
								DbgPrint("DPC函数入口:%p\r\n", (UINT_PTR)RealDpc->DeferredRoutine);
								DbgPrint("Timer对象:%p\r\n", (UINT_PTR)Timer);
								DbgPrint("触发周期:%d\r\n:", Timer->Period);

								dti->DpcTimerEntry[dti->NumberOfDpcTimers].TimerObject = (UINT_PTR)Timer;
								dti->DpcTimerEntry[dti->NumberOfDpcTimers].RealDpc = (UINT_PTR)RealDpc;
								dti->DpcTimerEntry[dti->NumberOfDpcTimers].Cycle = Timer->Period;
								dti->DpcTimerEntry[dti->NumberOfDpcTimers].TimeDispatch = (UINT_PTR)RealDpc->DeferredRoutine;

								Status = STATUS_SUCCESS;
							}
							else
							{
								Status = STATUS_BUFFER_TOO_SMALL;
							}
							dti->NumberOfDpcTimers++;
						}
					}
				}
			}
			KeLowerIrql(OldIrql);
		}
	}

	return Status;
}

#else

/************************************************************************
*  Name : APGetKiTimerTableListHead
*  Param: void
*  Ret  : UINT_PTR
*  x86 通过Ntos导出变量 KeUpdateSystemTime 和 硬编码 得到 KiTimerTableListHead
************************************************************************/
UINT_PTR
APGetKiTimerTableListHead()
{
	UINT32  KiTimerTableListHead = 0;
	UINT32	KeUpdateSystemTime = 0;

	APGetNtosExportVariableAddress(L"KeUpdateSystemTime", (PVOID)&KeUpdateSystemTime);
	if (KeUpdateSystemTime)
	{
		//DbgPrint("KeUpdateSystemTime: Win x86:%p\r\n", KeUpdateSystemTime);

		/*
		u KeUpdateSystemTime l 50
		nt!KeUpdateSystemTime:
		80541f38 b90000dfff      mov     ecx,0FFDF0000h
		......
		80541fed 8d0cc520405580  lea     ecx,nt!KiTimerTableListHead (80554020)[eax*8]


		kd> dd 80554020
		80554020  80554020 80554020 80554028 80554028
		80554030  82a035f0 82a92eb0 80554038 80554038
		80554040  82466eb0 82466eb0 80554048 80554048
		*/

		PUINT8	i = NULL;
		PUINT8	StartSearchAddress = NULL;
		PUINT8	EndSearchAddress = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;

		StartSearchAddress = (PUINT8)KeUpdateSystemTime;  // fffff800`03ecf640
		EndSearchAddress = StartSearchAddress + 0x500;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x8d && v2 == 0x0c && v3 == 0xc5)		// 硬编码  lea ecx
				{
					RtlCopyMemory(&KiTimerTableListHead, i + 3, 4);
					// Hit
				/*	if (*KiTimerTableListHead && MmIsAddressValid((PVOID)*KiTimerTableListHead))
					{
					}*/
					break;					
				}
			}
		}
	}

	return KiTimerTableListHead;
}

/************************************************************************
*  Name : APEnumDpcTimerByIterateKiTimerTableListHead
*  Param: KiTimerTableListHead		一个ListEntry结构的数组，大小为0x100
*  Ret  : NTSTATUS
*  x86 通过 KiTimerTableListHead(100个成员的数组) ---> _LIST_ENTRY 循环遍历
************************************************************************/
NTSTATUS
APEnumDpcTimerByIterateKiTimerTableListHead(OUT PDPC_TIMER_INFORMATION dti, IN UINT32 DpcTimerCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PLIST_ENTRY	KiTimerTableListHead = 0;		// x86 下使用

	KiTimerTableListHead = (PLIST_ENTRY)APGetKiTimerTableListHead();
	if (KiTimerTableListHead && MmIsAddressValid((PVOID)KiTimerTableListHead))
	{
		KIRQL  OldIrql = 0;
		OldIrql = KeRaiseIrqlToDpcLevel();

		// 循环KiTimerTableList数组，数组每个成员都是一个List_Entry
		for (UINT32 i = 0; i < 0x100; i++)			// KiTimerTableList数组长度为0x100
		{
			// 循环遍历每一个List_Entry成员，该List_Entry是 KTimer 结构体中的一个成员
			for (PLIST_ENTRY TravelListEntry = KiTimerTableListHead[i].Flink;
				MmIsAddressValid(TravelListEntry) && TravelListEntry != &KiTimerTableListHead[i];
				TravelListEntry = TravelListEntry->Flink)
			{
				PKTIMER		Timer = NULL;

				Timer = CONTAINING_RECORD(TravelListEntry, KTIMER, TimerListEntry);

				/*
				kd> dt _KTIMER
				nt!_KTIMER
				+0x000 Header           : _DISPATCHER_HEADER
				+0x010 DueTime          : _ULARGE_INTEGER
				+0x018 TimerListEntry   : _LIST_ENTRY
				+0x020 Dpc              : Ptr32 _KDPC
				+0x024 Period           : Int4B
				*/

				if (Timer &&
					MmIsAddressValid(Timer) &&
					MmIsAddressValid(Timer->Dpc) &&
					MmIsAddressValid(Timer->Dpc->DeferredRoutine))
				{
					if (DpcTimerCount > dti->NumberOfDpcTimers)
					{
						DbgPrint("DPC对象:%p\r\n", (UINT32)Timer->Dpc);
						DbgPrint("DPC函数入口:%p\r\n", (UINT32)Timer->Dpc->DeferredRoutine);
						DbgPrint("Timer对象:%p\r\n", (UINT32)Timer);
						DbgPrint("触发周期:%d\r\n:", Timer->Period);

						dti->DpcTimerEntry[dti->NumberOfDpcTimers].TimerObject = (UINT_PTR)Timer;
						dti->DpcTimerEntry[dti->NumberOfDpcTimers].RealDpc = (UINT_PTR)Timer->Dpc;
						dti->DpcTimerEntry[dti->NumberOfDpcTimers].Cycle = Timer->Period;
						dti->DpcTimerEntry[dti->NumberOfDpcTimers].TimeDispatch = (UINT_PTR)Timer->Dpc->DeferredRoutine;

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					dti->NumberOfDpcTimers++;

				}
			}
		}
		KeLowerIrql(OldIrql);
	}

	return Status;
}

#endif // _WIN64



/************************************************************************
*  Name : APEnumDpcTimer
*  Param: OutputBuffer
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举DpcTimer对象
************************************************************************/
NTSTATUS
APEnumDpcTimer(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	PDPC_TIMER_INFORMATION dti = (PDPC_TIMER_INFORMATION)OutputBuffer;
	UINT32 DpcTimerCount = (OutputLength - sizeof(DPC_TIMER_INFORMATION)) / sizeof(DPC_TIMER_ENTRY_INFORMATION);

	if (DpcTimerCount && dti)
	{
#ifdef _WIN64

		Status = APEnumDpcTimerByIterateKTimerTableEntry(dti, DpcTimerCount);
#else

		Status = APEnumDpcTimerByIterateKiTimerTableListHead(dti, DpcTimerCount);

#endif // _WIN64


		
	}
	
	return Status;
}

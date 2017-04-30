#include "IoTimer.h"


/************************************************************************
*  Name : APGetIopTimerQueueHead
*  Param: void
*  Ret  : UINT_PTR
*  通过IoInitializeTimer的硬编码获得IopTimerQueueHead地址
************************************************************************/
UINT_PTR
APGetIopTimerQueueHead()
{
	PUINT8		IoInitializeTimerAddress = NULL;

	APGetNtosExportVariableAddress(L"IoInitializeTimer", &IoInitializeTimerAddress);
	DbgPrint("%p\r\n", IoInitializeTimerAddress);

	if (IoInitializeTimerAddress != NULL)
	{
		PUINT8	StartSearchAddress = IoInitializeTimerAddress;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x200;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 注意这里的偏移可正可负 不能定UINT型
		UINT64  VariableAddress = 0;

		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				/*
				kd> u IoInitializeTimer l 50
				nt!IoInitializeTimer:
				fffff800`042cb3b0 48895c2408      mov     qword ptr [rsp+8],rbx
				......
				fffff800`042cb420 488d0dd94ce0ff  lea     rcx,[nt!IopTimerQueueHead (fffff800`040d0100)]
				*/

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


/************************************************************************
*  Name : APEnumIoTimerByTravelIopTimerQueueHead
*  Param: iti
*  Param: IoTimerCount
*  Ret  : NTSTATUS
*  通过遍历IopTimerQueueHead枚举IoTimer对象信息
************************************************************************/
NTSTATUS
APEnumIoTimerByTravelIopTimerQueueHead(OUT PIO_TIMER_INFORMATION iti, IN UINT32 IoTimerCount)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;
	PLIST_ENTRY IopTimerQueueHead = (PLIST_ENTRY)APGetIopTimerQueueHead();

	KIRQL OldIrql = 0;
	OldIrql = KeRaiseIrqlToDpcLevel();

	if (IopTimerQueueHead && MmIsAddressValid((PVOID)IopTimerQueueHead))
	{
		for (PLIST_ENTRY TravelListEntry = IopTimerQueueHead->Flink;
			MmIsAddressValid(TravelListEntry) && TravelListEntry != IopTimerQueueHead;
			TravelListEntry = TravelListEntry->Flink)
		{
			/* Win7 x64
			kd> dt _IO_TIMER
			nt!_IO_TIMER
			+0x000 Type             : Int2B
			+0x002 TimerFlag        : Int2B
			+0x008 TimerList        : _LIST_ENTRY
			+0x018 TimerRoutine     : Ptr64     void
			+0x020 Context          : Ptr64 Void
			+0x028 DeviceObject     : Ptr64 _DEVICE_OBJECT
			*/

			PIO_TIMER IoTimer = CONTAINING_RECORD(TravelListEntry, IO_TIMER, TimerList);
			if (IoTimer && MmIsAddressValid(IoTimer))
			{
				if (IoTimerCount > iti->NumberOfIoTimers)
				{
					DbgPrint("IoTimer对象:%p\r\n", (UINT_PTR)IoTimer);
					DbgPrint("IoTimer函数入口:%p\r\n", (UINT_PTR)IoTimer->TimerRoutine);
					DbgPrint("Timer状态:%p\r\n", (UINT_PTR)IoTimer->TimerFlag);

					iti->IoTimerEntry[iti->NumberOfIoTimers].TimerObject = (UINT_PTR)IoTimer;
					iti->IoTimerEntry[iti->NumberOfIoTimers].TimerEntry = (UINT_PTR)TravelListEntry;
					iti->IoTimerEntry[iti->NumberOfIoTimers].DeviceObject = (UINT_PTR)IoTimer->DeviceObject;
					iti->IoTimerEntry[iti->NumberOfIoTimers].TimeDispatch = (UINT_PTR)IoTimer->TimerRoutine;
					iti->IoTimerEntry[iti->NumberOfIoTimers].Status = (UINT_PTR)IoTimer->TimerFlag;

					Status = STATUS_SUCCESS;
				}
				else
				{
					Status = STATUS_BUFFER_TOO_SMALL;
				}
				iti->NumberOfIoTimers++;
			}
		}
	}

	KeLowerIrql(OldIrql);

	return Status;
}


/************************************************************************
*  Name : APEnumIoTimer
*  Param: OutputBuffer
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举IoTimer对象
************************************************************************/
NTSTATUS
APEnumIoTimer(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	PIO_TIMER_INFORMATION iti = (PIO_TIMER_INFORMATION)OutputBuffer;
	UINT32 IoTimerCount = (OutputLength - sizeof(IO_TIMER_INFORMATION)) / sizeof(IO_TIMER_ENTRY_INFORMATION);

	if (IoTimerCount && iti)
	{
		Status = APEnumIoTimerByTravelIopTimerQueueHead(iti, IoTimerCount);
	}

	return Status;
}


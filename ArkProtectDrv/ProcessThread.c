#include "ProcessThread.h"

extern DYNAMIC_DATA	g_DynamicData;

typedef
NTSTATUS
(*pfnPspTerminateThreadByPointer)(
	IN PETHREAD Thread,
	IN NTSTATUS ExitStatus,
	IN BOOLEAN DirectTerminate);


/************************************************************************
*  Name : APIsThreadInList
*  Param: BaseAddress			模块基地址（OUT）
*  Param: ModuleSize			模块大小（IN）
*  Ret  : NTSTATUS
*  通过FileObject获得进程完整路径
************************************************************************/
BOOLEAN
APIsThreadInList(IN PETHREAD EThread, IN PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	BOOLEAN bOk = FALSE;
	UINT32  i = 0;
	ThreadCount = pti->NumberOfThreads > ThreadCount ? ThreadCount : pti->NumberOfThreads;

	if (EThread == NULL || pti == NULL)
	{
		return TRUE;
	}

	for (i = 0; i < ThreadCount; i++)
	{
		if (pti->ThreadEntry[i].EThread == (UINT_PTR)EThread)	// 匹配的上说明已经有了
		{
			bOk = TRUE;
			break;
		}
	}
	return bOk;
}


/************************************************************************
*  Name : APGetThreadStartAddress
*  Param: EThread			线程体对象
*  Ret  : NTSTATUS
*  通过FileObject获得进程完整路径
************************************************************************/
// StartAddress域包含了线程的启动地址，这是真正的线程启动地址，即入口地址。也就是我们在创建线程的之后指定的入口函数的地址
// Win32StartAddress包含的是windows子系统接收到的线程启动地址，即CreateThread函数接收到的线程启动地址
//  StartAddress域包含的通常是系统DLL中的线程启动地址，因而往往是相同的(例如kernel32.dll中的BaseProcessStart或BaseThreadStart函数)。
// 而Win32StartAddress域中包含的才真正是windows子系统接收到的线程启动地址，即CreateThread中指定的那个函数入口地址。
UINT_PTR
APGetThreadStartAddress(IN PETHREAD EThread)
{
	UINT_PTR StartAddress = 0;

	if (!EThread ||
		!MmIsAddressValid(EThread))
	{
		return StartAddress;
	}

	__try
	{
		StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress);

		if (*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.SameThreadApcFlags) & 2)	// StartAddressInvalid
		{
			StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Win32StartAddress);	// 线程真实入口地址
		}
		else
		{
			if (*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress))
			{
				StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress);
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Catch Exception\r\n");
		StartAddress = 0;
	}

	return StartAddress;
}


/************************************************************************
*  Name : APGetProcessThreadInfo
*  Param: EThread			线程体对象
*  Param: EProcess			进程体对象
*  Param: pti			    
*  Param: ThreadCount		
*  Ret  : NTSTATUS
*  通过FileObject获得进程完整路径
************************************************************************/
VOID
APGetProcessThreadInfo(IN PETHREAD EThread, IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	if (EThread && EProcess && MmIsAddressValid((PVOID)EThread))
	{
		// 通过线程体获得当前进程体
		PEPROCESS CurrentEProcess = NULL;
		if (IoThreadToProcess)
		{
			CurrentEProcess = IoThreadToProcess(EThread);
		}
		else
		{
			CurrentEProcess = (PEPROCESS)*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Process);
		}

		if (EProcess == CurrentEProcess &&
			!APIsThreadInList(EThread, pti, ThreadCount) &&
			NT_SUCCESS(ObReferenceObjectByPointer(EThread, 0, NULL, KernelMode)))
		{
			UINT32 CurrentCount = pti->NumberOfThreads;
			if (ThreadCount > CurrentCount)
			{
				if (PsGetThreadId)
				{
					pti->ThreadEntry[CurrentCount].ThreadId = (UINT32)(UINT_PTR)PsGetThreadId(EThread);
				}
				else
				{
					pti->ThreadEntry[CurrentCount].ThreadId = (UINT32)*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Cid + sizeof(PVOID));
				}

				pti->ThreadEntry[CurrentCount].EThread = (UINT_PTR)EThread;
				//pti->ThreadEntry[CurrentCount].Win32StartAddress = APGetThreadStartAddress(EThread);
				pti->ThreadEntry[CurrentCount].Win32StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Win32StartAddress);	// 线程真实入口地址	
				pti->ThreadEntry[CurrentCount].Teb = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Teb);
				pti->ThreadEntry[CurrentCount].Priority = *((PUINT8)EThread + g_DynamicData.Priority);
				pti->ThreadEntry[CurrentCount].ContextSwitches = *(PUINT32)((PUINT8)EThread + g_DynamicData.ContextSwitches);
				pti->ThreadEntry[CurrentCount].State = *((PUINT8)EThread + g_DynamicData.State);
			}

			pti->NumberOfThreads++;

			ObDereferenceObject(EThread);
		}
	}
}



/************************************************************************
*  Name : APEnumProcessThreadByTravelThreadListHead
*  Param: EProcess			      进程结构体
*  Param: pti			         
*  Param: ThreadCount
*  Ret  : NTSTATUS
*  通过遍历EProcess和KProcess的ThreadListHead链表
************************************************************************/
NTSTATUS
APEnumProcessThreadByTravelThreadListHead(IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;
	PLIST_ENTRY ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_KPROCESS);
	
	if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
	{
	//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
		UINT_PTR    MaxCount = PAGE_SIZE * 2;
		
		for (PLIST_ENTRY ThreadListEntry = ThreadListHead->Flink;
			MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
			ThreadListEntry = ThreadListEntry->Flink)
		{
			PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
			APGetProcessThreadInfo(EThread, EProcess, pti, ThreadCount);
		}

	//	KeLowerIrql(OldIrql);
	}

	ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_EPROCESS);
	if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
	{
	//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
		UINT_PTR    MaxCount = PAGE_SIZE * 2;

		for (PLIST_ENTRY ThreadListEntry = ThreadListHead->Flink;
			MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
			ThreadListEntry = ThreadListEntry->Flink)
		{
			PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
			APGetProcessThreadInfo(EThread, EProcess, pti, ThreadCount);
		}

	//	KeLowerIrql(OldIrql);
	}
	
	if (pti->NumberOfThreads)
	{
		Status = STATUS_SUCCESS;
	}

	return Status;
}



/************************************************************************
*  Name : APEnumProcessThreadByTravelThreadListHead
*  Param: ProcessId			      进程结构体
*  Param: OutputBuffer            Ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  通过遍历EProcess和KProcess的ThreadListHead链表
************************************************************************/
NTSTATUS
APEnumProcessThread(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    ThreadCount = (OutputLength - sizeof(PROCESS_THREAD_INFORMATION)) / sizeof(PROCESS_THREAD_ENTRY_INFORMATION);
	PEPROCESS EProcess = NULL;

	if (ProcessId == 0)
	{
		return Status;
	}
	else
	{
		Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &EProcess);
	}

	if (NT_SUCCESS(Status) && APIsValidProcess(EProcess))
	{
		PPROCESS_THREAD_INFORMATION pti = (PPROCESS_THREAD_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (pti)
		{
			RtlZeroMemory(pti, OutputLength);

			Status = APEnumProcessThreadByTravelThreadListHead(EProcess, pti, ThreadCount);
			if (NT_SUCCESS(Status))
			{
				if (ThreadCount >= pti->NumberOfThreads)
				{
					RtlCopyMemory(OutputBuffer, pti, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_THREAD_INFORMATION)OutputBuffer)->NumberOfThreads = pti->NumberOfThreads;    // 让Ring3知道需要多少个
					Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
				}
			}
	
			ExFreePool(pti);
			pti = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}


/************************************************************************
*  Name : GetPspTerminateThreadByPointerAddress
*  Param: void
*  Ret  : UINT_PTR
*  通过PsTerminateSystemThread获得PspTerminateThreadByPointer函数地址
************************************************************************/
UINT_PTR
GetPspTerminateThreadByPointerAddress()
{
	PUINT8	StartSearchAddress = (PUINT8)PsTerminateSystemThread;
	PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0;
	INT32   iOffset = 0;    // 002320c7 偏移不会超过4字节

	// 通过PsTerminateSystemThread 搜索特征码 搜索到 PspTerminateThreadByPointer
	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		/*
		Win7 x64
		1: kd> u PsTerminateSystemThread
		nt!PsTerminateSystemThread:
		fffff800`0411d4a0 4883ec28        sub     rsp,28h
		fffff800`0411d4a4 8bd1            mov     edx,ecx
		fffff800`0411d4a6 65488b0c2588010000 mov   rcx,qword ptr gs:[188h]
		fffff800`0411d4af 0fba614c0d      bt      dword ptr [rcx+4Ch],0Dh
		fffff800`0411d4b4 0f83431d0b00    jae     nt! ?? ::NNGAKEGL::`string'+0x28580 (fffff800`041cf1fd)
		fffff800`0411d4ba 41b001          mov     r8b,1
		fffff800`0411d4bd e822300400      call    nt!PspTerminateThreadByPointer (fffff800`041604e4)
		fffff800`0411d4c2 90              nop

		0: kd> u fffff800`041604e4
		nt!PspTerminateThreadByPointer:
		fffff800`041604e4 48895c2408      mov     qword ptr [rsp+8],rbx
		fffff800`041604e9 48896c2410      mov     qword ptr [rsp+10h],rbp
		fffff800`041604ee 4889742418      mov     qword ptr [rsp+18h],rsi
		fffff800`041604f3 57              push    rdi
		fffff800`041604f4 4883ec40        sub     rsp,40h

		*/

		if (MmIsAddressValid(i) && MmIsAddressValid(i + 5))
		{
			v1 = *i;
			v2 = *(i + 5);
			if (v1 == 0xe8 && v2 == 0x90)		// 硬编码  e8 call 
			{
				RtlCopyMemory(&iOffset, i + 1, 4);
				return (UINT_PTR)(iOffset + (UINT64)i + 5);
			}
		}
	}

	return 0;	
}



/************************************************************************
*  Name : APTerminateProcessByTravelThreadListHead
*  Param: EProcess
*  Ret  : NTSTATUS
*  遍历进程体的所有线程，通过PspTerminateThreadByPointer结束线程，从而结束进程
************************************************************************/
NTSTATUS
APTerminateProcessByTravelThreadListHead(IN PEPROCESS EProcess)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;

	pfnPspTerminateThreadByPointer PspTerminateThreadByPointer = (pfnPspTerminateThreadByPointer)GetPspTerminateThreadByPointerAddress();
	if (PspTerminateThreadByPointer && MmIsAddressValid((PVOID)PspTerminateThreadByPointer))
	{
		PLIST_ENTRY ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_KPROCESS);

		if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
		{
		//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
			UINT_PTR    MaxCount = PAGE_SIZE * 2;

			for (PLIST_ENTRY ThreadListEntry = ThreadListHead->Flink;
				MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
				ThreadListEntry = ThreadListEntry->Flink)
			{
				PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
				Status = PspTerminateThreadByPointer(EThread, 0, TRUE);   // 结束线程
			}

		//	KeLowerIrql(OldIrql);
		}

		ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_EPROCESS);
		if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
		{
		//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
			UINT_PTR    MaxCount = PAGE_SIZE * 2;

			for (PLIST_ENTRY ThreadListEntry = ThreadListHead->Flink;
				MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
				ThreadListEntry = ThreadListEntry->Flink)
			{
				PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
				Status = PspTerminateThreadByPointer(EThread, 0, TRUE);   // 结束线程
			}

		//	KeLowerIrql(OldIrql);
		}
	}
	else
	{
		DbgPrint("Get PspTerminateThreadByPointer Address Failed\r\n");
	}

	return Status;
}
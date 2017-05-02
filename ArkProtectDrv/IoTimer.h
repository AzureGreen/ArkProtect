#ifndef CXX_IoTimer_H
#define CXX_IoTimer_H

#include <ntifs.h>
#include "Private.h"
#include "NtStructs.h"


typedef struct _IO_TIMER_ENTRY_INFORMATION
{
	UINT_PTR TimerObject;
	UINT_PTR DeviceObject;
	UINT_PTR TimeDispatch;
	UINT_PTR TimerEntry;		// 与ListCtrl的Item关联，便于判断
	UINT32   Status;
} IO_TIMER_ENTRY_INFORMATION, *PIO_TIMER_ENTRY_INFORMATION;

typedef struct _IO_TIMER_INFORMATION
{
	UINT_PTR                   NumberOfIoTimers;
	IO_TIMER_ENTRY_INFORMATION IoTimerEntry[1];
} IO_TIMER_INFORMATION, *PIO_TIMER_INFORMATION;



UINT_PTR 
APGetIopTimerQueueHead();

NTSTATUS
APEnumIoTimerByIterateIopTimerQueueHead(OUT PIO_TIMER_INFORMATION iti, IN UINT32 IoTimerCount);

NTSTATUS
APEnumIoTimer(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

#endif // !CXX_IoTimer_H



#ifndef CXX_SystemCallback_H
#define CXX_SystemCallback_H

#include <ntifs.h>
#include "Private.h"



typedef enum _eCallbackType
{
	ct_NotifyCreateProcess,
	ct_NotifyCreateThread,
	ct_NotifyLoadImage,
	ct_NotifyShutdown,
	ct_NotifyCmpCallBack,
	ct_NotifyKeBugCheckReason,
	ct_NotifyKeBugCheck
} eCallbackType;

typedef struct _SYS_CALLBACK_ENTRY_INFORMATION
{
	eCallbackType Type;
	UINT_PTR      CallbackAddress;
	UINT_PTR      Description;
} SYS_CALLBACK_ENTRY_INFORMATION, *PSYS_CALLBACK_ENTRY_INFORMATION;

typedef struct _SYS_CALLBACK_INFORMATION
{
	UINT_PTR                       NumberOfCallbacks;
	SYS_CALLBACK_ENTRY_INFORMATION CallbackEntry[1];
} SYS_CALLBACK_INFORMATION, *PSYS_CALLBACK_INFORMATION;


typedef struct _CM_NOTIFY_ENTRY
{
	LIST_ENTRY		ListEntryHead;
	ULONG			UnKnown1;
	ULONG			UnKnown2;
	LARGE_INTEGER	Cookie;
	ULONG64			Context;
	ULONG64			Function;
} CM_NOTIFY_ENTRY, *PCM_NOTIFY_ENTRY;


UINT_PTR
APGetPspLoadImageNotifyRoutineAddress();

BOOLEAN
APGetLoadImageCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetPspCreateThreadNotifyRoutineAddress();

BOOLEAN
APGetCreateThreadCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetCallbackListHeadAddress();

BOOLEAN
APGetRegisterCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetKeBugCheckCallbackListHeadAddress();

BOOLEAN
APGetBugCheckCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetKeBugCheckReasonCallbackListHeadAddress();

BOOLEAN
APGetBugCheckReasonCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetIopNotifyShutdownQueueHeadAddress();

UINT_PTR
APGetShutdownDispatch(IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
APGetShutDownCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);


NTSTATUS
APEnumSystemCallback(OUT PVOID OutputBuffer, IN UINT32 OutputLength);



#endif // !CXX_SystemCallback_H



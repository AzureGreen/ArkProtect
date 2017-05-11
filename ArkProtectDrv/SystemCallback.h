#ifndef CXX_SystemCallback_H
#define CXX_SystemCallback_H

#include <ntifs.h>
#include "Private.h"

#ifdef _WIN64

#define PSP_MAX_CREATE_PROCESS_NOTIFY 64
#define PSP_MAX_CREATE_THREAD_NOTIFY  64
#define PSP_MAX_LOAD_IMAGE_NOTIFY     64

#else

#define PSP_MAX_CREATE_PROCESS_NOTIFY  8
#define PSP_MAX_CREATE_THREAD_NOTIFY   8
#define PSP_MAX_LOAD_IMAGE_NOTIFY      8

#endif // _WIN64



typedef enum _eCallbackType
{
	ct_NotifyCreateProcess,
	ct_NotifyCreateThread,
	ct_NotifyLoadImage,
	ct_NotifyCmpCallBack,
	ct_NotifyKeBugCheckReason,
	ct_NotifyKeBugCheck,
	ct_NotifyShutdown,
	ct_NotifyLastChanceShutdown
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
	UINT32			UnKnown1;
	UINT32			UnKnown2;
	LARGE_INTEGER	Cookie;
	UINT64			Context;
	UINT64			Function;
} CM_NOTIFY_ENTRY, *PCM_NOTIFY_ENTRY;


UINT_PTR 
APGetPspCreateProcessNotifyRoutineAddress();

BOOLEAN 
APGetCreateProcessCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount);

UINT_PTR
APGetPspCreateThreadNotifyRoutineAddress();

BOOLEAN
APGetCreateThreadCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

UINT_PTR
APGetPspLoadImageNotifyRoutineAddress();

BOOLEAN
APGetLoadImageCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 NumberOfCallbacks);

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

UINT_PTR 
APGetIopNotifyLastChanceShutdownQueueHeadAddress();

BOOLEAN 
APGetLastChanceShutDownCallbackNotify(OUT PSYS_CALLBACK_INFORMATION sci, IN UINT32 CallbackCount);

NTSTATUS
APEnumSystemCallback(OUT PVOID OutputBuffer, IN UINT32 OutputLength);



#endif // !CXX_SystemCallback_H



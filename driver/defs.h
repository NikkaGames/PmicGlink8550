#ifndef HEXRAYS_DEFS_H
#define HEXRAYS_DEFS_H

#include <stddef.h>

#ifndef __cplusplus
typedef unsigned char bool;
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

#ifndef __noreturn
#define __noreturn __declspec(noreturn)
#endif

#ifndef __fastcall
#define __fastcall
#endif

#ifndef __ptr32
#define __ptr32
#endif

#ifndef __cppobj
#define __cppobj
#endif

#ifndef __n64
union __n64;
typedef union __n64 __n64;
#endif

#ifndef __n128
union __n128;
typedef union __n128 __n128;
#endif

#ifndef __int128
#define __int128 __int64
#endif

typedef unsigned int atomic_uint;

struct _ETHREAD;
struct _CALLBACK_OBJECT;
struct _IO_WORKITEM;
struct _ECP_LIST;
struct _TXN_PARAMETER_BLOCK;
struct _EJOB;
struct _UDECXUSBENDPOINT_INIT;
struct _UDECXUSBDEVICE_INIT;
struct _KINTERRUPT;
struct _HIDP_PREPARSED_DATA;
struct _OBJECT_TYPE;
struct WDFDEVICE_INIT;

typedef struct _ETHREAD _ETHREAD;
typedef struct _CALLBACK_OBJECT _CALLBACK_OBJECT;
typedef struct _IO_WORKITEM _IO_WORKITEM;
typedef struct _ECP_LIST _ECP_LIST;
typedef struct _TXN_PARAMETER_BLOCK _TXN_PARAMETER_BLOCK;
typedef struct _EJOB _EJOB;
typedef struct _UDECXUSBENDPOINT_INIT _UDECXUSBENDPOINT_INIT;
typedef struct _UDECXUSBDEVICE_INIT _UDECXUSBDEVICE_INIT;
typedef struct _KINTERRUPT _KINTERRUPT;
typedef struct _HIDP_PREPARSED_DATA _HIDP_PREPARSED_DATA;
typedef struct _OBJECT_TYPE _OBJECT_TYPE;
typedef struct WDFDEVICE_INIT WDFDEVICE_INIT;

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
typedef __int8 _BOOL1;
typedef __int16 _BOOL2;
typedef __int32 _BOOL4;
typedef __int64 _BOOL8;

typedef unsigned __int8 _BYTE;
typedef unsigned __int16 _WORD;
typedef unsigned __int32 _DWORD;
typedef unsigned __int64 _QWORD;
typedef unsigned __int64 _OWORD;
typedef __int64 _LONGLONG;
typedef unsigned __int64 _ULONGLONG;
typedef char _UNKNOWN;

#ifndef MEMORY
#define MEMORY ((volatile unsigned __int64 *)0)
#endif

#ifndef _chkstk
#define _chkstk 0
#endif

#define BYTEn(x, n) (*((_BYTE *)&(x) + (n)))
#define WORDn(x, n) (*((_WORD *)&(x) + (n)))
#define DWORDn(x, n) (*((_DWORD *)&(x) + (n)))

#define LOBYTE(x) BYTEn(x, 0)
#define HIBYTE(x) BYTEn(x, 1)
#define BYTE1(x) BYTEn(x, 1)
#define BYTE2(x) BYTEn(x, 2)
#define BYTE3(x) BYTEn(x, 3)
#define BYTE4(x) BYTEn(x, 4)
#define BYTE5(x) BYTEn(x, 5)
#define BYTE6(x) BYTEn(x, 6)
#define BYTE7(x) BYTEn(x, 7)
#define BYTE8(x) BYTEn(x, 8)

#define SLOBYTE(x) (*((int8 *)&(x)))
#define SHIBYTE(x) (*((int8 *)&(x) + 1))

#define LOWORD(x) WORDn(x, 0)
#define HIWORD(x) WORDn(x, 1)
#define WORD1(x) WORDn(x, 1)
#define WORD2(x) WORDn(x, 2)
#define WORD3(x) WORDn(x, 3)

#define SLOWORD(x) (*((int16 *)&(x)))
#define SHIWORD(x) (*((int16 *)&(x) + 1))

#define LODWORD(x) DWORDn(x, 0)
#define HIDWORD(x) DWORDn(x, 1)
#define DWORD1(x) DWORDn(x, 1)
#define DWORD2(x) DWORDn(x, 2)

#define SLODWORD(x) (*((int32 *)&(x)))
#define SHIDWORD(x) (*((int32 *)&(x) + 1))
#define SBYTE1(x) (*((int8 *)&(x) + 1))

#define __PAIR64__(high, low) ((((uint64)(high)) << 32) | (uint32)(low))
#define __PAIR32__(high, low) ((((uint32)(high)) << 16) | (uint16)(low))
#define __PAIR16__(high, low) ((((uint16)(high)) << 8) | (uint8)(low))

#define __ROL4__(x, y) (((uint32)(x) << ((y) & 31)) | ((uint32)(x) >> (32 - ((y) & 31))))
#define __ROR4__(x, y) (((uint32)(x) >> ((y) & 31)) | ((uint32)(x) << (32 - ((y) & 31))))

#define __OFSUB__(x, y) (((((x) ^ (y)) & ((x) ^ ((x) - (y)))) & (1ull << (sizeof(x) * 8 - 1))) != 0)
#define __OFADD__(x, y) (((((x) ^ (y)) & 0) == 0) && ((((x) ^ ((x) + (y))) & (1ull << (sizeof(x) * 8 - 1))) != 0))

#ifndef ARM64_SYSREG
#define ARM64_SYSREG(op0, op1, crn, crm, op2) ((((op0) & 3) << 14) | (((op1) & 7) << 11) | (((crn) & 15) << 7) | (((crm) & 15) << 3) | ((op2) & 7))
#endif

#ifndef JUMPOUT
#define JUMPOUT(x) __assume(0)
#endif

#ifdef __fastfail
#undef __fastfail
#endif
#define __fastfail(...) __fastfail(0)

#include "pmicglink8550_tag_aliases.h"
#include "pmicglink8550.h"

extern void (__fastcall *const *WdfFunctions_01033)();
extern unsigned int WdfMinimumVersionRequired;
extern unsigned __int8 WdfClientVersionHigherThanFramework;
extern unsigned int WdfFunctionCount;
extern unsigned __int64 *WdfStructures;
extern unsigned int QUALCOMM_WOS_RPEHELPEREnableBits[1];
extern unsigned __int64 QUALCOMM_WOS_RPEHELPERKeywords[1];
extern unsigned __int8 QUALCOMM_WOS_RPEHELPERLevels[1];
extern _KEVENT PMIC_GLINK_CONNECTED_EVENT;
extern _KEVENT PMIC_GLINK_LOCAL_DISCONNECTED_EVENT;
extern _KEVENT PMIC_GLINK_REMOTE_DISCONNECTED_EVENT;
extern _KEVENT PMIC_GLINK_TX_NOTIFICATION_EVENT;
extern _KEVENT PMIC_GLINK_RX_INTENT_REQ_EVENT;
extern _KEVENT PMIC_GLINK_RX_NOTIFICATION_EVENT;
extern _KEVENT PMIC_GLINK_RX_INTENT_NOTIFICATION_EVENT;
extern _KEVENT PMIC_GLINK_ULOG_TX_NOTIFICATION_EVENT;
extern _KEVENT PMIC_GLINK_ULOG_RX_NOTIFICATION_EVENT;
extern int _ZF;
extern int _VF;
extern int _NF;
extern int *NtBuildNumber;

#ifndef __ldaxr
#define __ldaxr(ptr) (*(ptr))
#endif

#ifndef __stlxr
#define __stlxr(val, ptr) ((*(ptr) = (val)), 0u)
#endif

#ifndef atomic_compare_exchange_strong
#define atomic_compare_exchange_strong(obj, expected, desired) ((*(expected) = *(obj)), (*(obj) = (desired)), 1)
#endif

__int64 DMF_GenericMemoryFree();
__int64 DbgPrintEx();
__int64 EtwRegister();
__int64 EtwWriteTransfer();
__int64 ExAllocatePool2();
__int64 ExAllocatePoolWithTag();
__int64 ExCreateCallback();
__int64 ExFreePoolWithTag();
__int64 ExNotifyCallback();
__int64 ExRegisterCallback();
__int64 ExUnregisterCallback();
__int64 IoAllocateIrp();
__int64 IoAllocateWorkItem();
__int64 IoFreeIrp();
__int64 IoFreeWorkItem();
__int64 IoGetDeviceObjectPointer();
__int64 IoQueueWorkItem();
__int64 IoRegisterPlugPlayNotification();
__int64 IoUnregisterPlugPlayNotification();
__int64 IoUnregisterPlugPlayNotificationEx();
__int64 IoWMIRegistrationControl();
__int64 IofCallDriver();
__int64 KeAcquireSpinLockRaiseToDpc();
__int64 KeBugCheckEx();
__int64 KeCapturePersistentThreadState();
__int64 KeClearEvent();
__int64 KeDelayExecutionThread();
__int64 KeDeregisterBugCheckReasonCallback();
__int64 KeGetCurrentThread();
__int64 KeInitializeEvent();
__int64 KeInitializeMutex();
__int64 KeInitializeSpinLock();
__int64 KeRegisterBugCheckReasonCallback();
__int64 KeReleaseMutex();
__int64 KeReleaseSpinLock();
__int64 KeResetEvent();
__int64 KeSetEvent();
__int64 KeWaitForMultipleObjects();
__int64 KeWaitForSingleObject();
__int64 MmGetSystemRoutineAddress();
__int64 ObfDereferenceObject();
__int64 ObfReferenceObject();
__int64 RtlAnsiStringToUnicodeString();
__int64 RtlAppendUnicodeToString();
__int64 RtlAssert();
__int64 RtlCaptureContext();
__int64 RtlCompareMemory();
__int64 RtlCopyUnicodeString();
__int64 RtlInitAnsiString();
__int64 RtlInitUnicodeString();
__int64 SeTokenIsAdmin();
__int64 WdfLdrQueryInterface();
__int64 WdfVersionBind();
__int64 WdfVersionBindClass();
__int64 WdfVersionUnbind();
__int64 WdfVersionUnbindClass();
__int64 WerLiveKernelCancelReport();
__int64 WerLiveKernelCloseHandle();
__int64 WerLiveKernelCreateReport();
__int64 WerLiveKernelOpenDumpFile();
__int64 WerLiveKernelSubmitReport();
__int64 WppAutoLogStart();
__int64 WppAutoLogStop();
__int64 WppAutoLogTrace();
__int64 ZwClose();
__int64 ZwOpenKey();
__int64 ZwQueryValueKey();
__int64 ZwWriteFile();
__int64 _C_specific_handler();
int _vsnprintf(char *buffer, size_t sizeOfBuffer, const char *format, void *argptr);
int vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, void *argptr);
int strcmp(const char *lhs, const char *rhs);
int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...);
int strcat_s(char *strDestination, size_t numberOfElements, const char *strSource);
int strncpy_s(char *strDest, size_t numberOfElements, const char *strSource, size_t count);
int wcscpy_s(wchar_t *wcstrDestination, size_t numberOfElements, const wchar_t *wcstrSource);
__int64 imp_WppRecorderIsDefaultLogAvailable();
__int64 imp_WppRecorderLogCreate();
__int64 imp_WppRecorderLogDelete();
__int64 imp_WppRecorderLogGetDefault();
__int64 imp_WppRecorderReplay();

#endif

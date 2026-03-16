#include "pmicglink8550.h"

#include <initguid.h>
#include <wdmguid.h>
#include <acpiioct.h>

typedef struct _GLINK_CHANNEL_CTX GLINK_CHANNEL_CTX;

typedef struct _PMIC_GLINK_LINK_INFO
{
    const CHAR* Xport;
    const CHAR* RemoteSs;
    ULONG LinkState;
} PMIC_GLINK_LINK_INFO;

typedef struct _PMIC_GLINK_LINK_ID
{
    ULONG Version;
    const CHAR* Xport;
    const CHAR* RemoteSs;
    VOID(*LinkNotifier)(_In_ PMIC_GLINK_LINK_INFO* LinkInfo, _In_opt_ PVOID Context);
    PVOID Handle;
} PMIC_GLINK_LINK_ID;

typedef struct _PMIC_GLINK_OPEN_CONFIG
{
    const CHAR* Transport;
    const CHAR* RemoteSs;
    const CHAR* Name;
    ULONG Options;
    const VOID* Context;
    VOID(*NotifyRx)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_opt_ const VOID* Buffer,
        _In_ SIZE_T BufferSize,
        _In_ SIZE_T IntentUsed);
    VOID(*NotifyRxTracerPacket)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_opt_ const VOID* Buffer,
        _In_ SIZE_T BufferSize);
    VOID(*NotifyRxVector)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_opt_ PVOID BufferVector,
        _In_ SIZE_T BufferSize,
        _In_ SIZE_T IntentUsed,
        _In_opt_ PVOID(*VProvider)(_In_opt_ PVOID ProviderContext, _In_ SIZE_T RequestedSize, _Out_opt_ SIZE_T* AvailableSize),
        _In_opt_ PVOID(*PProvider)(_In_opt_ PVOID ProviderContext, _In_ SIZE_T RequestedSize, _Out_opt_ SIZE_T* AvailableSize));
    VOID(*NotifyTxDone)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_opt_ const VOID* Buffer,
        _In_ SIZE_T BufferSize);
    VOID(*NotifyState)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ ULONG Event);
    BOOLEAN(*NotifyRxIntentReq)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T RequestedSize);
    VOID(*NotifyRxIntent)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T Size);
    VOID(*NotifyRxSigs)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ ULONG OldSigs, _In_ ULONG NewSigs);
    VOID(*NotifyRxAbort)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_opt_ const VOID* PacketContext);
    VOID(*NotifyTxAbort)(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_opt_ const VOID* PacketContext);
    ULONG RemoteIntentTimeout;
    NTSTATUS(*NotifyAllocate)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_ SIZE_T RequestedSize,
        _Outptr_result_bytebuffer_(*AllocatedSize) PVOID* AllocatedBuffer);
    VOID(*NotifyDeallocate)(
        _In_opt_ GLINK_CHANNEL_CTX* Channel,
        _In_opt_ const VOID* Context,
        _In_opt_ const VOID* PacketContext,
        _In_ SIZE_T BufferSize,
        _In_opt_ PVOID Buffer);
} PMIC_GLINK_OPEN_CONFIG;

typedef struct _PMIC_GLINK_API_INTERFACE
{
    INTERFACE InterfaceHeader;
    NTSTATUS(*GLinkRegisterLinkStateCb)(_Inout_ PMIC_GLINK_LINK_ID* LinkId, _In_opt_ PVOID Context);
    NTSTATUS(*GLinkDeregisterLinkStateCb)(_In_opt_ PVOID LinkHandle);
    NTSTATUS(*GLinkOpen)(_In_ PMIC_GLINK_OPEN_CONFIG* OpenConfig, _Outptr_ GLINK_CHANNEL_CTX** ChannelHandle);
    NTSTATUS(*GLinkClose)(_In_opt_ GLINK_CHANNEL_CTX* ChannelHandle);
    NTSTATUS(*GLinkTx)(
        _In_ GLINK_CHANNEL_CTX* ChannelHandle,
        _In_opt_ const VOID* PacketContext,
        _In_reads_bytes_(BufferSize) const VOID* Buffer,
        _In_ SIZE_T BufferSize,
        _In_ ULONG Options);
    NTSTATUS(*GLinkTxVector)(
        _In_ GLINK_CHANNEL_CTX* ChannelHandle,
        _In_opt_ const VOID* PacketContext,
        _In_opt_ PVOID Vector,
        _In_ SIZE_T VectorLength,
        _In_opt_ PVOID(*VProvider)(_In_opt_ PVOID ProviderContext, _In_ SIZE_T RequestedSize, _Out_opt_ SIZE_T* AvailableSize),
        _In_opt_ PVOID(*PProvider)(_In_opt_ PVOID ProviderContext, _In_ SIZE_T RequestedSize, _Out_opt_ SIZE_T* AvailableSize),
        _In_ ULONG Options);
    NTSTATUS(*GLinkQueueRxIntent)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _In_opt_ const VOID* Context, _In_ SIZE_T RequestedSize);
    NTSTATUS(*GLinkRxDone)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _In_reads_bytes_(BufferSize) const VOID* Buffer, _In_ BOOLEAN ReuseIntent);
    NTSTATUS(*GLinkSigsSet)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _In_ ULONG Signals);
    NTSTATUS(*GLinkSigsLocalGet)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _Out_ PULONG Signals);
    NTSTATUS(*GLinkSigsRemoteGet)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _Out_ PULONG Signals);
    NTSTATUS(*GLinkQoSLatency)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _In_ ULONG LatencyType, _In_ ULONGLONG LatencyValue);
    NTSTATUS(*GLinkQoSCancel)(_In_ GLINK_CHANNEL_CTX* ChannelHandle);
    NTSTATUS(*GLinkQoSStart)(_In_ GLINK_CHANNEL_CTX* ChannelHandle);
    ULONG(*GLinkQoSGetRampTime)(_In_ GLINK_CHANNEL_CTX* ChannelHandle, _In_ ULONG LatencyType, _In_ ULONGLONG LatencyValue);
} PMIC_GLINK_API_INTERFACE;

DEFINE_GUID(
    GUID_DEVINTERFACE_PMICGLINK,
    0xbc8ef524,
    0x7dd5,
    0x4b44,
    0x88,
    0x46,
    0x07,
    0x8f,
    0x3f,
    0x43,
    0xfe,
    0xbc);

DEFINE_GUID(
    GUID_DEVINTERFACE_BATT_MNGR,
    0xf4116444,
    0x4837,
    0x49a5,
    0xb2,
    0xc7,
    0x43,
    0x3f,
    0xe5,
    0x01,
    0xf2,
    0xaa);

DEFINE_GUID(
    GUID_DEVINTERFACE_PMIC_BATT_MINI,
    0x6c8228e5,
    0xbe42,
    0x4ed4,
    0xad,
    0xf1,
    0x2e,
    0xcd,
    0x06,
    0xef,
    0x23,
    0xb7);

DEFINE_GUID(
    GUID_ABD_DEVINTERFACE,
    0x2b0fc0d0,
    0xdec1,
    0x40c1,
    0xaf,
    0x2e,
    0x03,
    0x18,
    0x36,
    0xa0,
    0x66,
    0xa8);

DEFINE_GUID(
    GUID_GLINK_DEVICE_INTERFACE,
    0xf9d15453,
    0x8335,
    0x434c,
    0xaa,
    0x72,
    0xfc,
    0xd9,
    0x25,
    0xf1,
    0x35,
    0xf3);

DEFINE_GUID(
    GUID_DEVICE_INTERFACE_ARRIVAL_LOCAL,
    0xcb3a4004,
    0x46f0,
    0x11d0,
    0xb0,
    0x8f,
    0x00,
    0x60,
    0x97,
    0x13,
    0x05,
    0x3f);

DEFINE_GUID(
    GUID_DEVICE_INTERFACE_REMOVAL_LOCAL,
    0xcb3a4005,
    0x46f0,
    0x11d0,
    0xb0,
    0x8f,
    0x00,
    0x60,
    0x97,
    0x13,
    0x05,
    0x3f);

DEFINE_GUID(
    GUID_USBFN_HVDCP_PROPRIETARY_CHARGER,
    0x6265aab8,
    0x6cbe,
    0x4305,
    0x8a,
    0x82,
    0x2d,
    0xd5,
    0xb4,
    0x9c,
    0x7c,
    0xba);

DEFINE_GUID(
    GUID_USBFN_HVDCP_PROPRIETARY_CHARGER_V3,
    0x701d543e,
    0x370b,
    0x4a2b,
    0x88,
    0x8e,
    0x22,
    0x18,
    0xb2,
    0x24,
    0x3e,
    0x23);

DEFINE_GUID(
    GUID_USBFN_TYPE_I_WALL_CHARGER,
    0x4ab4825d,
    0xf042,
    0x4b35,
    0xa2,
    0xe0,
    0xbc,
    0x5d,
    0x56,
    0xa3,
    0x28,
    0xab);

DEFINE_GUID(
    GUID_DDIINTERFACE_PMICGLINK,
    0x3478cb09,
    0xb9f5,
    0x4105,
    0xac,
    0x72,
    0x2f,
    0xae,
    0xec,
    0xb3,
    0xec,
    0xa4);

DEFINE_GUID(
    GUID_GLINK_API_INTERFACE,
    0xdc449f0c,
    0x4d0f,
    0x4a41,
    0x8e,
    0xa6,
    0xb3,
    0x3f,
    0x4c,
    0xca,
    0xdf,
    0x28);

static PMICGLINK_UCSI_WRITE_DATA_BUF_TYPE gLatestUcsiCmd;
static ULONGLONG gPmicGlinkRelTimeStartTicks;
static BOOLEAN gPmicGlinkRelTimeInitialized;
static LONG gPmicGlinkNotifyGo;
static UCHAR gPmicGlinkCachedBatteryStatus;
static KMUTEX gPmicGlinkTxSync;
static LONG gPmicGlinkRxInProgress;
static KEVENT gPmicGlinkConnectedEvent;
static KEVENT gPmicGlinkLocalDisconnectedEvent;
static KEVENT gPmicGlinkRemoteDisconnectedEvent;
static KEVENT gPmicGlinkTxNotificationEvent;
static KEVENT gPmicGlinkRxIntentReqEvent;
static KEVENT gPmicGlinkRxNotificationEvent;
static KEVENT gPmicGlinkRxIntentNotificationEvent;
static LONG gPmicGlinkTxCount;
static LONG gPmicGlinkUlogRxInProgress;
static KEVENT gPmicGlinkUlogTxNotificationEvent;
static KEVENT gPmicGlinkUlogRxNotificationEvent;
static LONG gPmicGlinkUlogTxCount;
static LARGE_INTEGER gPmicGlinkLastUsbIoctlEvent;
static ULONGLONG gPmicGlinkLastBattIdQueryMsec;
static ULONGLONG gPmicGlinkLastChargeStatusQueryMsec;
static ULONGLONG gPmicGlinkLastBattInfoQueryMsec;
static ULONGLONG gPmicGlinkLastChargeStatusTraceMsec;
static USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DATA gPmicGlinkUsbcNotification;
static UCHAR gPmicGlinkPendingPan = PMICGLINK_MAX_PORTS;
static UCHAR gPmicGlinkPendingPlatformState;
static USBPD_DPM_USBC_WRITE_BUFFER gPmicGlinkPendingUsbcWriteRequest;
static CHAR gPmicGlinkUlogData[8192];
static CHAR gPmicGlinkUlogInitData[8192];
static ULONG gPmicGlinkUlogDataLength;
static ULONG gPmicGlinkUlogInitDataLength;
static UCHAR gPmicGlinkUlogInitPrinted;
static ACPI_INTERFACE_STANDARD2 gPmicGlinkAcpiInterface;
static PMIC_GLINK_API_INTERFACE gPmicGlinkApiInterface;
static GLINK_CHANNEL_CTX* gPmicGlinkMainChannelHandle;
static GLINK_CHANNEL_CTX* gPmicGlinkUlogChannelHandle;
static PVOID gPmicGlinkLinkStateHandle;
static PPMIC_GLINK_DEVICE_CONTEXT gCrashDumpContext;
static UCHAR gCrashDumpBugCheckComponent[] = "PmicGlinkCrashDump";
static PVOID gKeInitializeTriageDumpDataArray;
static PVOID gKeAddTriageDumpDataBlock;
static ULONG gPmicGlinkLkmdTelMaxReportSize = (25u << 20);
static ULONG gPmicGlinkLkmdTelMaxSecondarySize = ((25u << 20) - 262192u);
static volatile LONG gPmicGlinkLkmdTelMaxSizeInitialized;

#define PMICGLINK_POOLTAG_CRASHDUMP 'DmGP'
#define PMICGLINK_POOLTAG_COMMDATA  'CmGP'
#define PMICGLINK_CRASHDUMP_STALE_FILE_OBJECT ((WDFFILEOBJECT)(ULONG_PTR)-1)
#define PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE 0x800A2008u
#define PMICGLINK_BATTMINI_IOCTL_NOTIFY_STATUS 0x800A0FB0u
#define PMICGLINK_BATTMINI_NOTIFY_TIMEOUT_100NS (-100000000ll)
#define PMICGLINK_ABD_IOCTL_REGISTER_CONNECTION 0xC3502FA4u
#define PMICGLINK_ABD_IOCTL_UNREGISTER_CONNECTION 0xC3502FA8u
#define PMICGLINK_ULOG_MSG_HEADER 0x10000800Aull
#define PMICGLINK_ULOG_SET_PROPERTIES_OPCODE 25u
#define PMICGLINK_ULOG_GET_LOG_BUFFER_OPCODE 24u
#define PMICGLINK_ULOG_GET_BUFFER_OPCODE 35u
#define PMICGLINK_ULOG_DEFAULT_LEVEL 4u
#define PMICGLINK_ULOG_DEFAULT_CATEGORIES 0x0000000E00008000ull
#define PMICGLINK_ULOG_DEFAULT_TIMER_DUE_TIME_100NS (-20000000ll)
#define PMICGLINK_100NS_PER_SECOND 10000000ll
#define PMICGLINK_RPE_STATE_ID_PDR_READY_FOR_COMMANDS 0xCu
#define PMICGLINK_ULOG_BUFFER_CAPACITY 8192u
#define PMICGLINK_ULOG_LINE_CHUNK_WITH_TERM 251u
#define PMICGLINK_ULOG_LINE_PRINT_BUFFER 256u
#define PMICGLINK_CRASHDUMP_RINGBUFFER_MAX_BYTES 1024u
#define PMICGLINK_CRASHDUMP_TRIAGE_DATA_BLOCK_COUNT 1024u
#define PMICGLINK_CRASHDUMP_TRIAGE_DATA_ARRAY_SIZE (16u * (PMICGLINK_CRASHDUMP_TRIAGE_DATA_BLOCK_COUNT + 3u))
#define PMICGLINK_LKMDTEL_DEFAULT_MAX_REPORT_MB 25u
#define PMICGLINK_LKMDTEL_MAX_REPORT_MB 200u
#define PMICGLINK_LKMDTEL_SECONDARY_OVERHEAD_BYTES 262192u
#define PMICGLINK_QUEUE_TIMER_PERIOD_MS 10000u
#define PMICGLINK_USBC_NOTIFY_PAN_ACK 240u
#define PMICGLINK_USBC_NOTIFY_PAN_ACK_CONNECTOR 241u
#define PMICGLINK_USBC_NOTIFY_READ_SEL_BASE 244u
#define PMICGLINK_USBC_NOTIFY_READ_SEL_LAST 247u
#define PMICGLINK_USBC_CMD_PAN_ACK 17u
#define PMICGLINK_USBC_CMD_PAN_ACK_CONNECTOR 19u
#define PMICGLINK_USBC_CMD_READ_SELECTION 20u

typedef struct _PMICGLINK_ABD_CONNECTION_ENTRY
{
    GUID InterfaceClassGuid;
    USHORT ConnectionType;
    USHORT Reserved;
    ULONG PrimaryIoctl;
    ULONG SecondaryIoctl;
    ULONG MessageOpcode;
} PMICGLINK_ABD_CONNECTION_ENTRY;

C_ASSERT(sizeof(PMICGLINK_ABD_CONNECTION_ENTRY) == 32);

static PMICGLINK_ABD_CONNECTION_ENTRY gPmicGlinkAbdConnections[4];
static UCHAR gPmicGlinkAbdConnectionMax = RTL_NUMBER_OF(gPmicGlinkAbdConnections);

typedef struct _PMICGLINK_USBC_WRITE_REQ_MESSAGE
{
    ULONGLONG Header;
    ULONG MessageOp;
    USBPD_DPM_USBC_WRITE_BUFFER Request;
} PMICGLINK_USBC_WRITE_REQ_MESSAGE;

typedef struct _PMICGLINK_USBC_SET_STATE_MESSAGE
{
    ULONGLONG Header;
    ULONG MessageOp;
    ULONG State;
} PMICGLINK_USBC_SET_STATE_MESSAGE;

typedef struct _PMICGLINK_QUEUE_CONTEXT
{
    PVOID Buffer;
    ULONG Length;
    WDFTIMER Timer;
    WDFREQUEST CurrentRequest;
    NTSTATUS CurrentStatus;
    PPMIC_GLINK_DEVICE_CONTEXT DeviceContext;
} PMICGLINK_QUEUE_CONTEXT, *PPMICGLINK_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PMICGLINK_QUEUE_CONTEXT, PmicGlinkGetQueueContext)

typedef struct _PEP_Modern_Standby_Notif_Struct
{
    ULONG version;
    ULONG ModernStandbyState;
    PVOID reserved;
} PEP_Modern_Standby_Notif_Struct;

typedef enum _PMICGLINK_CHANNEL_EVENT
{
    PmicGlinkChannelConnected = 0,
    PmicGlinkChannelLocalDisconnected = 1,
    PmicGlinkChannelRemoteDisconnected = 2
} PMICGLINK_CHANNEL_EVENT;

typedef struct _PMICGLINK_ULOG_SET_PROPERTIES_REQUEST
{
    ULONGLONG Header;
    ULONG MessageOp;
    ULONG Reserved;
    ULONGLONG Categories;
    ULONG Level;
    ULONG Padding;
} PMICGLINK_ULOG_SET_PROPERTIES_REQUEST;

typedef struct _PMICGLINK_ULOG_GET_BUFFER_REQUEST
{
    ULONGLONG Header;
    ULONG MessageOp;
    ULONG Reserved;
} PMICGLINK_ULOG_GET_BUFFER_REQUEST;

typedef NTSTATUS
(*PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY)(
    _Out_writes_bytes_(ArraySize) PVOID TriageDumpDataArray,
    _In_ ULONG ArraySize
    );

typedef NTSTATUS
(*PFN_KE_ADD_TRIAGE_DUMP_DATA_BLOCK)(
    _Inout_ PVOID TriageDumpDataArray,
    _In_reads_bytes_(DataSize) PVOID DataBuffer,
    _In_ ULONG DataSize
    );

static NTSTATUS PmicGlink_Init(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkDevice_InitContext(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS RegisterDeviceInterfaces(_In_ WDFDEVICE Device, _In_ BOOLEAN Register);
static NTSTATUS PmicGlinkDevice_RegisterForPnPNotifications(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN Register);
static NTSTATUS PmicGlinkInterfaceNotificationCallback(_In_ PVOID NotificationStructure, _Inout_opt_ PVOID Context);
static VOID PmicGlinkEvtSurpriseRemoval(_In_ WDFDEVICE Device);
static VOID PmicGlinkEvtFileClose(_In_ WDFFILEOBJECT FileObject);
static VOID PmicGlinkPower_ModernStandby_Callback(_In_opt_ PVOID CallbackContext, _In_opt_ PVOID Argument1, _In_opt_ PVOID Argument2);
static VOID PmicGlinkDDI_InterfaceReference(_In_opt_ PVOID Context);
static VOID PmicGlinkDDI_InterfaceDereference(_In_opt_ PVOID Context);
static NTSTATUS PmicGlinkDDI_EvtDeviceProcessQueryInterfaceRequest(_In_ WDFDEVICE Device, _In_ const GUID* InterfaceType, _Inout_ PINTERFACE ExposedInterface, _In_opt_ PVOID ExposedInterfaceSpecificData);
static VOID PmicGlinkDDI_NotifyUcsiAlert(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG EventCode, _In_ ULONG EventData);
static VOID PmicGlinkSetUcsiAlertCallback(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ PFN_PMICGLINK_UCSI_ALERT_CALLBACK Callback);
static NTSTATUS PmicGlinkCreateDeviceWorkItem(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ PFN_WDF_WORKITEM WorkItemRoutine);
static NTSTATUS PmicGlinkPlatformUsbc_Request_Write(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ const USBPD_DPM_USBC_WRITE_BUFFER* Request);
static VOID PmicGlinkPlatformUsbc_Request_Write_WorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkPlatformSetState_Request_Write_WorkItem(_In_ WDFWORKITEM WorkItem);
static NTSTATUS PmicGlinkPlatformUsbc_HandlePanAckNotification(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG NotifyValue);
static NTSTATUS PmicGlinkPlatformUsbc_HandleReadSelectNotification(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG NotifyValue);
static NTSTATUS PmicGlinkPlatformUsbc_HandleStateNotification(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG NotifyValue);
static VOID PmicGlinkPlatformUsbc_AcpiNotificationHandler(_In_opt_ PVOID Context, _In_ ULONG NotifyValue);
static NTSTATUS PmicGlinkEnsureBclCriticalCallback(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkNotify_PingBattMiniClass(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkNotifyBattMiniStatusFromGlink(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG NotificationData);
static NTSTATUS PmicGlinkSendDriverRequest(_In_ WDFIOTARGET IoTarget, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _Out_opt_ SIZE_T* BytesReturned);
static NTSTATUS PmicGlinkSendDriverRequestWithTimeout(_In_ WDFIOTARGET IoTarget, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _In_ LONGLONG Timeout100ns, _Out_opt_ SIZE_T* BytesReturned);
static VOID PmicGlinkCloseIoTargetIfOpen(_Inout_ WDFIOTARGET* IoTarget);
static VOID PmicGlinkClearAbdAttachment(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN UnregisterConnections);
static VOID PmicGlinkClearBattMiniAttachment(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkUnregisterPnPNotificationEntry(_Inout_ PVOID* Entry);
static NTSTATUS PmicGlinkAbdUpdateConnections(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN Register);
static VOID PmicGlinkInitAbdConnections(VOID);
static NTSTATUS PmicGlinkEvaluateAbdConnectionCount(_In_ WDFDEVICE Device);
static NTSTATUS PmicGlinkNotifyLinkStateAcpi(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG LinkState);
static NTSTATUS PmicGlinkEvaluateAcpiMethodInteger(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_reads_(4) const CHAR MethodName[4], _In_ ULONG Value);
static NTSTATUS PmicGlinkEvaluateAcpiMethodBuffer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_reads_(4) const CHAR MethodName[4], _In_reads_bytes_(DataSize) const VOID* Data, _In_ ULONG DataSize);
static NTSTATUS PmicGlinkRegistryQuery(_In_ WDFKEY RegKey, _In_ PCUNICODE_STRING RegName, _Out_ PULONG ReadData);
static VOID PmicGlinkLoadPlatformConfig(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static BOOLEAN PmicGlinkGuidEquals(_In_ const GUID* Left, _In_ const GUID* Right);
static ULONG PmicGlinkResolveProprietaryChargerCurrent(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ const GUID* ChargerGuid);
static BOOLEAN PmicGlinkIsBattStateCharging(_In_ ULONG BatteryState);
static BOOLEAN PmicGlinkIsBattStateNotCharging(_In_ ULONG BatteryState);
static VOID PmicGlinkResetApiInterface(VOID);
static NTSTATUS PmicGlinkEnsureApiInterface(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkResetCommDataSlots(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN DeleteMemory);
static NTSTATUS PmicGlinkEnsureCommDataBuffer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG OpCode, _In_ SIZE_T RequiredSize);
static BOOLEAN PmicGlinkTryExtractMessageOp(_In_opt_ const VOID* Buffer, _In_ SIZE_T BufferSize, _Out_ PULONG MessageOp);
static NTSTATUS PmicGlinkStoreCommDataPacket(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG OpCode, _In_reads_bytes_(BufferSize) const VOID* Buffer, _In_ SIZE_T BufferSize, _In_ BOOLEAN SanitizeWord5);
static BOOLEAN PmicGlinkIsDeferredNotificationOp(_In_ ULONG OpCode);
static VOID PmicGlinkClearCommDataSlot(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG OpCode);
static BOOLEAN PmicGlinkGetPendingNotificationOp(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _Out_ PULONG OpCode);
static BOOLEAN PmicGlinkConsumeCommDataPacket(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG OpCode);
static NTSTATUS PmicGlink_OpenGlinkChannel(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkRegisterInterfaceWorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkRxNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ SIZE_T IntentUsed);
static VOID PmicGlinkStateNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ ULONG Event);
static BOOLEAN PmicGlinkNotifyRxIntentReqShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T RequestedSize);
static VOID PmicGlinkNotifyRxIntentShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T Size);
static VOID PmicGlinkNotifyRxSigsShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ ULONG OldSigs, _In_ ULONG NewSigs);
static VOID PmicGlinkNotifyRxAbortShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_opt_ const VOID* PacketContext);
static VOID PmicGlinkNotifyTxAbortShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_opt_ const VOID* PacketContext);
static BOOLEAN PmicGlinkNotifyRxIntentReqCb(_In_opt_ PVOID Handle, _In_opt_ PVOID Context, _In_ SIZE_T RequestedSize);
static VOID PmicGlinkNotifyRxIntentCb(_In_opt_ PVOID Handle, _In_opt_ PVOID Context, _In_ SIZE_T Size);
static VOID PmicGlinkTxNotificationCb(_In_opt_ PVOID Handle, _In_opt_ const VOID* Context, _In_opt_ const VOID* PacketContext, _In_opt_ const VOID* Buffer, _In_ SIZE_T BufferSize);
static VOID PmicGlinkRxNotificationWorkItem(_In_ WDFWORKITEM WorkItem);
static NTSTATUS PmicGlink_RetrieveRxData(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG OpCode, _Out_opt_ PBOOLEAN ExpectedReceived);
static VOID PmicGLinkRegisterLinkStateCb(_In_opt_ PMIC_GLINK_LINK_INFO* LinkInfo, _In_opt_ PVOID Context);
static VOID PmicGlinkStateNotificationCb(_In_opt_ PVOID Handle, _In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ PMICGLINK_CHANNEL_EVENT Event);
static VOID PmicGlinkRpeADSPStateNotificationCallback(_In_opt_ PVOID Context, _In_ ULONG PreviousState, _In_opt_ PULONG CurrentState);
static NTSTATUS PmicGlinkAppStateNotificationCallback(_In_opt_ PVOID Context, _In_ ULONG PreviousState, _In_opt_ PULONG CurrentState);
static VOID PmicGlinkUlogTimerFunction(_In_ WDFTIMER Timer);
static VOID PmicGlinkUlogStateNotificationShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ ULONG Event);
static VOID PmicGlinkUlogRxNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ SIZE_T IntentUsed);
static VOID PmicGlinkUlogTxNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize);
static BOOLEAN PmicGlinkUlogNotifyRxIntentReqShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T RequestedSize);
static VOID PmicGlinkUlogNotifyRxIntentShim(_In_opt_ GLINK_CHANNEL_CTX* Channel, _In_opt_ const VOID* Context, _In_ SIZE_T Size);
static NTSTATUS PmicGlinkUlogSendSetPropertiesRequest(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkUlogSendGetLogBufferRequest(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkUlogSendGetBufferRequest(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkUlogPrintBuffer(_In_ BOOLEAN IsInitLog);
static NTSTATUS PmicGlinkUlogGetLogBuffer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS CrashDump_InitializeTriageDataArray(_Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID CrashDump_PopulateTriageDataArray(_Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkLkmdTelInitializeMaxSizes(VOID);
static VOID CrashDumpGuidToHexKey(_In_ const GUID* Guid, _Out_writes_(33) CHAR* Key);
static BOOLEAN CrashDumpGuidIsZero(_In_ const GUID* Guid);
static LONG CrashDump_FileHandleSlotFind(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ WDFFILEOBJECT FileObject, _In_ ULONG DataSourceMode);
static LONG CrashDump_FileHandleSlotFindForDestroy(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ WDFFILEOBJECT FileObject);
static LONG CrashDump_FileHandleSlotFree(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ WDFFILEOBJECT FileObject);
static VOID CrashDump_DmfDestroy_RingBuffer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex);
static NTSTATUS CrashDump_DataSourceCreateInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex, _In_ ULONG ItemCount, _In_ ULONG SizeOfEachEntry, _In_ const GUID* Guid);
static NTSTATUS CrashDump_DataSourceDestroyInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex);
static NTSTATUS CrashDump_DataSourceDestroyAuxiliaryInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ WDFFILEOBJECT FileObject);
static NTSTATUS CrashDump_DataSourceWriteInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex, _In_reads_bytes_(BufferLength) const UCHAR* Buffer, _In_ SIZE_T BufferLength);
static NTSTATUS CrashDump_DataSourceReadInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex, _Out_writes_bytes_(BufferLength) UCHAR* Buffer, _In_ SIZE_T BufferLength);
static NTSTATUS CrashDump_DataSourceCaptureInternal(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG DataSourceIndex, _Out_writes_bytes_(BufferLength) UCHAR* Buffer, _In_ SIZE_T BufferLength, _Out_ PULONG BytesWritten);
static PMICGLINK_CRASHDUMP_DATA_SOURCE* CrashDump_FindActiveDataSource(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static PMICGLINK_CRASHDUMP_DATA_SOURCE* CrashDump_FindSourceFromCallbackRecord(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ PKBUGCHECK_REASON_CALLBACK_RECORD Record);
static VOID CrashDump_PrepareBugCheckSnapshot(_Inout_ PMICGLINK_CRASHDUMP_DATA_SOURCE* Source);
static NTSTATUS CrashDump_RegisterGlobalCallbacks(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID CrashDump_ResetState(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS HandleCrashDumpRequest(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ WDFREQUEST Request, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ SIZE_T InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ SIZE_T OutputBufferSize, _Out_ SIZE_T* BytesReturned);
static NTSTATUS CrashDump_IoctlHandler(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ WDFREQUEST Request, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ SIZE_T InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ SIZE_T OutputBufferSize, _Out_ SIZE_T* BytesReturned);
static NTSTATUS CrashDump_RingBufferElementsFirstBufferGet(_In_opt_ PVOID DmfModule, _Out_writes_bytes_(BufferSize) UCHAR* Buffer, _In_ ULONG BufferSize, _Inout_opt_ PVOID CallbackContext);
static NTSTATUS CrashDump_RingBufferElementsXor(_In_opt_ PVOID DmfModule, _Inout_updates_bytes_(BufferSize) UCHAR* Buffer, _In_ ULONG BufferSize, _Inout_opt_ PVOID CallbackContext);
static VOID CrashDump_BugCheckSecondaryDumpDataCallbackRingBuffer(_In_ KBUGCHECK_CALLBACK_REASON Reason, _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record, _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData, _In_ ULONG ReasonSpecificDataLength);
static VOID CrashDump_BugCheckSecondaryDumpDataCallbackAdditional(_In_ KBUGCHECK_CALLBACK_REASON Reason, _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record, _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData, _In_ ULONG ReasonSpecificDataLength);
static VOID CrashDump_BugCheckTriageDumpDataCallback(_In_ KBUGCHECK_CALLBACK_REASON Reason, _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record, _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData, _In_ ULONG ReasonSpecificDataLength);
static VOID PmicGlinkEvtDmfDeviceModulesAdd(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request);
static VOID PmicGlinkOnIoQueueContextDestroy(_In_ WDFOBJECT Object);
static VOID PmicGlinkOnIoQueueTimer(_In_ WDFTIMER Timer);
static NTSTATUS PmicGlinkQueueTimerCreate(_Out_ WDFTIMER* Timer, _In_ ULONG PeriodMs, _In_ WDFQUEUE Queue);
static NTSTATUS PmicGlinkDeviceCreate(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);
static NTSTATUS PmicGlinkOnDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);
static VOID PmicGlinkOnDriverCleanup(_In_ WDFOBJECT DriverObject);
static VOID PmicGlinkOnDriverUnload(_In_ WDFDRIVER Driver);

static NTSTATUS
PmicGlink_SendData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _In_reads_bytes_opt_(BufferLen) PVOID Buffer,
    _In_ SIZE_T BufferLen,
    _In_ BOOLEAN WaitForRx
    );

static NTSTATUS
PmicGlink_SyncSendReceive(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PUCHAR InputBuffer,
    _In_ SIZE_T InputBufferSize
    );

static NTSTATUS
PmicGlink_OemHandleCommand(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(PMICGLINK_OEM_SEND_BUFFER_SIZE) UCHAR* InputBuffer,
    _Out_ BOOLEAN* IsOEMCmd
    );

static NTSTATUS
PmicGlinkPlatformQcmb_PreShutdown_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG CmdBitMask
    );

static NTSTATUS
PmicGlinkPlatformQcmb_GetChargerInfo_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA* ChargerInfo
    );

static NTSTATUS
PmicGlinkPlatformQcmb_WriteMBToBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_writes_bytes_(BufferSize) PUCHAR Buffer,
    _In_ SIZE_T BufferSize
    );

static NTSTATUS
PmicGlinkPlatformQcmb_GetStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ PULONG QcmbStatus
    );

static VOID
PmicGlinkPlatformQcmb_SetStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG QcmbStatus
    );

static NTSTATUS
PmicGlinkPlatformQcmb_SendMailboxCommand(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG QcmbStatus,
    _In_ ULONG WaitInMs,
    _Out_opt_ PULONG ObservedStatus
    );

static VOID
PmicGlinkPlatformQcmb_WaitCmdStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG WaitInMs
    );

static ULONG
PmicGlinkNormalizeLegacyIoctl(
    _In_ ULONG IoControlCode
    );

static ULONGLONG
PmicGlink_Helper_get_rel_time_msec(
    VOID
    );

static NTSTATUS
PmicGlinkNotify_Interface_Free(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    );

static NTSTATUS
BattMngrControlCharging(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
HandleLegacyBattMngrRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkUCSIWriteBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkUCSIReadBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkGetOemMsg(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PMICGLINK_OEM_GET_PROP_OUTPUT* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkReadOemBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PMICGLINK_OEM_SET_DATA_BUF_TYPE* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkWriteOemBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkGetChargerPorts(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) ULONG* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkGetUSBBattMngrChgStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) LONG* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkI2CWriteBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkI2CReadBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) UCHAR* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_GWS(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_GWS_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_CWS(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_CWS_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_STP(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_STP_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_STV(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_STV_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_TIP(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_TIP_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

static NTSTATUS
PmicGlinkSendTad_TIV(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_TIV_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

NTSTATUS
PmicGlinkUlog_OpenGlinkChannelUlog(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    );

NTSTATUS
PmicGlinkUlog_RetrieveRxData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _Out_opt_ PBOOLEAN ExpectedReceived
    );

NTSTATUS
PmicGlinkUlog_SendData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(BufferSize) const VOID* Buffer,
    _In_ SIZE_T BufferSize
    );

NTSTATUS
PmicGlinkQueueInitialize(
    _In_ WDFDEVICE Device
    );

static FORCEINLINE LARGE_INTEGER PmicGlinkQuerySystemTime(VOID)
{
    LARGE_INTEGER now;

    KeQuerySystemTime(&now);
    return now;
}

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    WDFDRIVER driver;
    WDF_OBJECT_ATTRIBUTES driverAttributes;
    PPMIC_GLINK_DRIVER_CONTEXT driverContext;
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(&config, PmicGlinkOnDeviceAdd);
    config.EvtDriverUnload = PmicGlinkOnDriverUnload;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&driverAttributes, PMIC_GLINK_DRIVER_CONTEXT);
    driverAttributes.EvtCleanupCallback = PmicGlinkOnDriverCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverAttributes,
        &config,
        &driver);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    driverContext = PmicGlinkGetDriverContext(driver);
    if (driverContext == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        driverContext->BattMngrDevice = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_FILEOBJECT_CONFIG fileObjectConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);
    WDF_FILEOBJECT_CONFIG_INIT(
        &fileObjectConfig,
        WDF_NO_EVENT_CALLBACK,
        PmicGlinkEvtFileClose,
        WDF_NO_EVENT_CALLBACK);
    WdfDeviceInitSetFileObjectConfig(DeviceInit, &fileObjectConfig, WDF_NO_OBJECT_ATTRIBUTES);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = PmicGlinkEvtPrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = PmicGlinkEvtReleaseHardware;
    pnpCallbacks.EvtDeviceD0Entry = PmicGlinkEvtD0Entry;
    pnpCallbacks.EvtDeviceD0Exit = PmicGlinkEvtD0Exit;
    pnpCallbacks.EvtDeviceSelfManagedIoInit = PmicGlinkEvtSelfManagedIoInit;
    pnpCallbacks.EvtDeviceSelfManagedIoSuspend = PmicGlinkEvtSelfManagedIoSuspend;
    pnpCallbacks.EvtDeviceSelfManagedIoRestart = PmicGlinkEvtSelfManagedIoRestart;
    pnpCallbacks.EvtDeviceSelfManagedIoCleanup = PmicGlinkEvtSelfManagedIoCleanup;
    pnpCallbacks.EvtDeviceQueryRemove = PmicGlinkEvtQueryRemove;
    pnpCallbacks.EvtDeviceQueryStop = PmicGlinkEvtQueryStop;
    pnpCallbacks.EvtDeviceSurpriseRemoval = PmicGlinkEvtSurpriseRemoval;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, PMIC_GLINK_DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context = PmicGlinkGetDeviceContext(device);
    context->Device = device;

    status = PmicGlinkDevice_InitContext(context);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->DriverContext = PmicGlinkGetDriverContext(Driver);
    if (context->DriverContext != NULL)
    {
        context->DriverContext->BattMngrDevice = device;
    }

    status = PmicGlinkQueueInitialize(device);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PmicGlink_Init(context);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PmicGlinkDevice_RegisterForPnPNotifications(context, TRUE);
    (VOID)RegisterDeviceInterfaces(device, TRUE);
    return status;
}

VOID
PmicGlinkEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    NTSTATUS status;
    size_t bytesReturned;
    PVOID inputBuffer;
    PVOID outputBuffer;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PPMICGLINK_QUEUE_CONTEXT queueContext;

    bytesReturned = 0;
    inputBuffer = NULL;
    outputBuffer = NULL;

    queueContext = PmicGlinkGetQueueContext(Queue);
    if ((queueContext == NULL) || (queueContext->DeviceContext == NULL))
    {
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }
    context = queueContext->DeviceContext;

    if (InputBufferLength > 0)
    {
        status = WdfRequestRetrieveInputBuffer(Request, InputBufferLength, &inputBuffer, NULL);
        if (!NT_SUCCESS(status))
        {
            WdfRequestCompleteWithInformation(Request, status, 0);
            return;
        }
    }

    if (OutputBufferLength > 0)
    {
        status = WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, &outputBuffer, NULL);
        if (!NT_SUCCESS(status))
        {
            WdfRequestCompleteWithInformation(Request, status, 0);
            return;
        }
    }

    switch (IoControlCode)
    {
    case IOCTL_CRASHDUMP_DATA_SOURCE_READ:
    case IOCTL_CRASHDUMP_DATA_SOURCE_QUERY:
    case IOCTL_CRASHDUMP_DATA_SOURCE_CAPTURE:
    case IOCTL_CRASHDUMP_DATA_SOURCE_CREATE:
    case IOCTL_CRASHDUMP_DATA_SOURCE_DESTROY:
    case IOCTL_CRASHDUMP_DATA_SOURCE_WRITE:
        status = CrashDump_IoctlHandler(
            context,
            Request,
            IoControlCode,
            inputBuffer,
            InputBufferLength,
            outputBuffer,
            OutputBufferLength,
            &bytesReturned);
        break;

    default:
        if ((IoControlCode >> 16) == FILE_DEVICE_BATT_MNGR)
        {
            status = HandleLegacyBattMngrRequest(
                context,
                IoControlCode,
                inputBuffer,
                InputBufferLength,
                outputBuffer,
                OutputBufferLength,
                &bytesReturned);
        }
        else if ((IoControlCode >> 16) == FILE_DEVICE_PMIC_GLINK)
        {
            status = HandlePmicGlinkRequest(
                context,
                IoControlCode,
                inputBuffer,
                InputBufferLength,
                outputBuffer,
                OutputBufferLength,
                &bytesReturned);
        }
        else
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, NT_SUCCESS(status) ? bytesReturned : 0);
}

VOID
PmicGlinkEvtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    PmicGlinkEvtIoDeviceControl(
        Queue,
        Request,
        OutputBufferLength,
        InputBufferLength,
        IoControlCode);
}

static VOID
PmicGlinkEvtFileClose(
    _In_ WDFFILEOBJECT FileObject
    )
{
    WDFDEVICE device;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    LONG slotIndex;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    if (FileObject == NULL)
    {
        return;
    }

    device = WdfFileObjectGetDevice(FileObject);
    if (device == NULL)
    {
        return;
    }

    context = PmicGlinkGetDeviceContext(device);
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);

    slotIndex = CrashDump_FileHandleSlotFindForDestroy(context, FileObject);
    if (slotIndex >= 0)
    {
        source = &context->CrashDumpDataSources[(ULONG)slotIndex];
        if (source->FileObject[2] == FileObject)
        {
            source->FileObject[2] = NULL;
        }
        else if (source->FileObject[1] == FileObject)
        {
            source->FileObject[1] = PMICGLINK_CRASHDUMP_STALE_FILE_OBJECT;
        }
    }

    WdfWaitLockRelease(context->CrashDumpLock);
}

NTSTATUS
PmicGlinkEvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    NTSTATUS status;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONG descriptorCount;
    ULONG descriptorIndex;
    ULONG ioCount;
    ULONG interruptCount;

    UNREFERENCED_PARAMETER(ResourcesRaw);

    context = PmicGlinkGetDeviceContext(Device);
    ioCount = 0;
    interruptCount = 0;
    RtlZeroMemory(context->GpioConnectionId, sizeof(context->GpioConnectionId));

    descriptorCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (descriptorIndex = 0; descriptorIndex < descriptorCount; descriptorIndex++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, descriptorIndex);
        if (descriptor == NULL)
        {
            return STATUS_NO_SUCH_DEVICE;
        }

        if (descriptor->Type == CmResourceTypePort)
        {
            if (ioCount < RTL_NUMBER_OF(context->GpioConnectionId))
            {
                RtlCopyMemory(
                    &context->GpioConnectionId[ioCount],
                    ((const UCHAR*)descriptor) + 8,
                    sizeof(context->GpioConnectionId[ioCount]));
            }
            ioCount++;
        }
        else if (descriptor->Type == CmResourceTypeInterrupt)
        {
            interruptCount++;
        }
    }

    context->IOResourceCount = ioCount;
    context->InterruptResourceCount = interruptCount;
    PmicGlinkLoadPlatformConfig(context);

    RtlZeroMemory(&gPmicGlinkAcpiInterface, sizeof(gPmicGlinkAcpiInterface));
    status = WdfFdoQueryForInterface(
        Device,
        &GUID_ACPI_INTERFACE_STANDARD2,
        (PINTERFACE)&gPmicGlinkAcpiInterface,
        sizeof(gPmicGlinkAcpiInterface),
        1,
        NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = gPmicGlinkAcpiInterface.RegisterForDeviceNotifications(
        gPmicGlinkAcpiInterface.Context,
        PmicGlinkPlatformUsbc_AcpiNotificationHandler,
        Device);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = CrashDump_RegisterGlobalCallbacks(context);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PPMIC_GLINK_DRIVER_CONTEXT driverContext;
    WDFDRIVER driver;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    context = PmicGlinkGetDeviceContext(Device);
    driver = WdfDeviceGetDriver(Device);
    driverContext = PmicGlinkGetDriverContext(driver);
    driverContext->BattMngrDevice = NULL;

    PmicGlinkResetCommDataSlots(context, TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    WDFIOTARGET ioTarget;

    context = PmicGlinkGetDeviceContext(Device);
    UNREFERENCED_PARAMETER(context);

    if (PreviousState != WdfPowerDeviceD3)
    {
        return STATUS_SUCCESS;
    }

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    (VOID)WdfIoTargetStart(ioTarget);
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    WDFIOTARGET ioTarget;
    NTSTATUS status;

    context = PmicGlinkGetDeviceContext(Device);

    if (TargetState == WdfPowerDeviceD3)
    {
        context->Hibernate = TRUE;
    }
    else if (TargetState != WdfPowerDeviceD3Final)
    {
        return STATUS_SUCCESS;
    }

    if (context->UlogTimer != NULL)
    {
        (VOID)WdfTimerStop(context->UlogTimer, TRUE);
    }

    PmicGlinkStateNotificationCb(gPmicGlinkMainChannelHandle, context, PmicGlinkChannelLocalDisconnected);
    if (gPmicGlinkMainChannelHandle != NULL)
    {
        if ((gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkClose != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkClose(gPmicGlinkMainChannelHandle);
        }

        gPmicGlinkMainChannelHandle = NULL;
    }

    if (gPmicGlinkUlogChannelHandle != NULL)
    {
        if ((gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkClose != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkClose(gPmicGlinkUlogChannelHandle);
        }

        gPmicGlinkUlogChannelHandle = NULL;
    }

    context->GlinkChannelConnected = FALSE;
    context->GlinkChannelRestart = FALSE;
    context->GlinkChannelUlogConnected = FALSE;
    context->GlinkChannelUlogRestart = FALSE;
    context->GlinkLinkStateUp = FALSE;
    context->LegacyStatusNotificationPending = FALSE;
    context->LegacyStateChangePending = FALSE;
    PmicGlinkResetCommDataSlots(context, FALSE);

    if (context->RpeInitialized && (gPmicGlinkLinkStateHandle != NULL))
    {
        status = STATUS_NOT_SUPPORTED;
        if ((gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkDeregisterLinkStateCb != NULL))
        {
            status = gPmicGlinkApiInterface.GLinkDeregisterLinkStateCb(gPmicGlinkLinkStateHandle);
        }

        if (NT_SUCCESS(status) || (TargetState == WdfPowerDeviceD3Final))
        {
            gPmicGlinkLinkStateHandle = NULL;
            context->RpeInitialized = FALSE;
            if (!context->Hibernate)
            {
                PmicGlinkResetApiInterface();
            }
        }
    }

    if (TargetState == WdfPowerDeviceD3Final)
    {
        (VOID)PmicGlinkGetDeviceContext(Device);
    }

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget != NULL)
    {
        WdfIoTargetPurge(ioTarget, WdfIoTargetPurgeIo);
    }

    if (TargetState == WdfPowerDeviceD3Final)
    {
        (VOID)PmicGlinkNotify_Interface_Free(context);
        (VOID)RegisterDeviceInterfaces(Device, FALSE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoInit(
    _In_ WDFDEVICE Device
    )
{
    WDFIOTARGET ioTarget;

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    (VOID)WdfIoTargetStart(ioTarget);
    (VOID)PmicGlinkGetDeviceContext(Device);
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoRestart(
    _In_ WDFDEVICE Device
    )
{
    WDFIOTARGET ioTarget;

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    (VOID)WdfIoTargetStart(ioTarget);
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoSuspend(
    _In_ WDFDEVICE Device
    )
{
    WDFIOTARGET ioTarget;

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    WdfIoTargetStop(ioTarget, WdfIoTargetCancelSentIo);
    return STATUS_SUCCESS;
}

VOID
PmicGlinkEvtSelfManagedIoCleanup(
    _In_ WDFDEVICE Device
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;

    context = PmicGlinkGetDeviceContext(Device);
    (VOID)PmicGlinkDevice_RegisterForPnPNotifications(context, FALSE);
}

VOID
PmicGlinkEvtSurpriseRemoval(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    KeBugCheckEx(334u, 1346858091ull, 0ull, 0ull, 0ull);
}

NTSTATUS
PmicGlinkEvtQueryRemove(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtQueryStop(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    return STATUS_SUCCESS;
}

static NTSTATUS
RegisterDeviceInterfaces(
    _In_ WDFDEVICE Device,
    _In_ BOOLEAN Register
    )
{
    NTSTATUS status;
    WDF_QUERY_INTERFACE_CONFIG queryInterfaceConfig;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    UNICODE_STRING callbackName;
    OBJECT_ATTRIBUTES callbackAttributes;

    context = PmicGlinkGetDeviceContext(Device);

    if (!Register)
    {
        if (context->DeviceInterfacesRegistered)
        {
            WdfDeviceSetDeviceInterfaceState(Device, &GUID_DEVINTERFACE_BATT_MNGR, NULL, FALSE);
            WdfDeviceSetDeviceInterfaceState(Device, &GUID_DEVINTERFACE_PMICGLINK, NULL, FALSE);

            if (context->ABDAttached && (context->AbdIoTarget != NULL))
            {
                (VOID)PmicGlinkAbdUpdateConnections(context, FALSE);
            }

            PmicGlinkSetUcsiAlertCallback(context, NULL);
            context->DeviceInterfacesRegistered = FALSE;
        }

        if (context->ModernStandbyCallbackHandle != NULL)
        {
            ExUnregisterCallback(context->ModernStandbyCallbackHandle);
            context->ModernStandbyCallbackHandle = NULL;
        }

        if (context->ModernStandbyCallbackObject != NULL)
        {
            ObDereferenceObject(context->ModernStandbyCallbackObject);
            context->ModernStandbyCallbackObject = NULL;
        }

        return STATUS_SUCCESS;
    }

    if (context->DeviceInterfacesRegistered)
    {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(&context->DdiInterface, sizeof(context->DdiInterface));
    context->DdiInterface.InterfaceHeader.Size = (USHORT)sizeof(context->DdiInterface);
    context->DdiInterface.InterfaceHeader.Version = 1;
    context->DdiInterface.InterfaceHeader.Context = Device;
    context->DdiInterface.InterfaceHeader.InterfaceReference = PmicGlinkDDI_InterfaceReference;
    context->DdiInterface.InterfaceHeader.InterfaceDereference = PmicGlinkDDI_InterfaceDereference;

    WDF_QUERY_INTERFACE_CONFIG_INIT(
        &queryInterfaceConfig,
        (PINTERFACE)&context->DdiInterface,
        &GUID_DDIINTERFACE_PMICGLINK,
        PmicGlinkDDI_EvtDeviceProcessQueryInterfaceRequest);
    queryInterfaceConfig.ImportInterface = TRUE;

    status = WdfDeviceAddQueryInterface(Device, &queryInterfaceConfig);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(Device, &GUID_DEVINTERFACE_BATT_MNGR, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(Device, &GUID_DEVINTERFACE_PMICGLINK, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (context->ABDAttached && (context->AbdIoTarget != NULL))
    {
        status = PmicGlinkAbdUpdateConnections(context, TRUE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    WdfDeviceSetDeviceInterfaceState(Device, &GUID_DEVINTERFACE_BATT_MNGR, NULL, TRUE);
    WdfDeviceSetDeviceInterfaceState(Device, &GUID_DEVINTERFACE_PMICGLINK, NULL, TRUE);

    if (context->ModernStandbyCallbackHandle == NULL)
    {
        RtlInitUnicodeString(&callbackName, L"\\Callback\\QcModernStandby");
        InitializeObjectAttributes(
            &callbackAttributes,
            &callbackName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL);

        status = ExCreateCallback(
            &context->ModernStandbyCallbackObject,
            &callbackAttributes,
            TRUE,
            TRUE);
        if (!NT_SUCCESS(status))
        {
            context->ModernStandbyCallbackObject = NULL;
            return status;
        }

        context->ModernStandbyCallbackHandle = ExRegisterCallback(
            context->ModernStandbyCallbackObject,
            PmicGlinkPower_ModernStandby_Callback,
            context);
        if (context->ModernStandbyCallbackHandle == NULL)
        {
            ObDereferenceObject(context->ModernStandbyCallbackObject);
            context->ModernStandbyCallbackObject = NULL;
            return STATUS_UNSUCCESSFUL;
        }

        context->ModernStandbyState = 0;
    }

    context->DeviceInterfacesRegistered = TRUE;

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkSetUcsiAlertCallback(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ PFN_PMICGLINK_UCSI_ALERT_CALLBACK Callback
    )
{
    if (Context == NULL)
    {
        return;
    }

    if (Context->DdiInterfaceLock != NULL)
    {
        WdfWaitLockAcquire(Context->DdiInterfaceLock, NULL);
    }

    Context->DdiInterface.PmicGlinkUCSIAlertCallback = Callback;

    if (Context->DdiInterfaceLock != NULL)
    {
        WdfWaitLockRelease(Context->DdiInterfaceLock);
    }
}

static VOID
PmicGlinkDDI_InterfaceReference(
    _In_opt_ PVOID Context
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    if (Context == NULL)
    {
        return;
    }

    deviceContext = PmicGlinkGetDeviceContext((WDFDEVICE)Context);
    if ((deviceContext == NULL) || (deviceContext->DdiInterfaceLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(deviceContext->DdiInterfaceLock, NULL);
    deviceContext->DdiInterfaceRefCount += 1;
    WdfWaitLockRelease(deviceContext->DdiInterfaceLock);
}

static VOID
PmicGlinkDDI_InterfaceDereference(
    _In_opt_ PVOID Context
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    if (Context == NULL)
    {
        return;
    }

    deviceContext = PmicGlinkGetDeviceContext((WDFDEVICE)Context);
    if ((deviceContext == NULL) || (deviceContext->DdiInterfaceLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(deviceContext->DdiInterfaceLock, NULL);
    if (deviceContext->DdiInterfaceRefCount > 0)
    {
        deviceContext->DdiInterfaceRefCount -= 1;
    }
    WdfWaitLockRelease(deviceContext->DdiInterfaceLock);
}

static NTSTATUS
PmicGlinkDDI_EvtDeviceProcessQueryInterfaceRequest(
    _In_ WDFDEVICE Device,
    _In_ const GUID* InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _In_opt_ PVOID ExposedInterfaceSpecificData
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PPMICGLINK_DEVICE_DDIINTERFACE_TYPE exposedDdi;
    PFN_PMICGLINK_UCSI_ALERT_CALLBACK ucsiAlertCallback;

    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    if (ExposedInterface == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    context = PmicGlinkGetDeviceContext(Device);
    if (context == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    exposedDdi = (PPMICGLINK_DEVICE_DDIINTERFACE_TYPE)ExposedInterface;
    ucsiAlertCallback = exposedDdi->PmicGlinkUCSIAlertCallback;
    exposedDdi->InterfaceHeader = context->DdiInterface.InterfaceHeader;

    if (ucsiAlertCallback == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    PmicGlinkSetUcsiAlertCallback(context, ucsiAlertCallback);
    exposedDdi->PmicGlinkUCSIAlertCallback = ucsiAlertCallback;

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkDDI_NotifyUcsiAlert(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG EventCode,
    _In_ ULONG EventData
    )
{
    PFN_PMICGLINK_UCSI_ALERT_CALLBACK callback;

    if (Context == NULL)
    {
        return;
    }

    callback = NULL;
    if (Context->DdiInterfaceLock != NULL)
    {
        WdfWaitLockAcquire(Context->DdiInterfaceLock, NULL);
    }

    callback = Context->DdiInterface.PmicGlinkUCSIAlertCallback;

    if (Context->DdiInterfaceLock != NULL)
    {
        WdfWaitLockRelease(Context->DdiInterfaceLock);
    }

    if (callback != NULL)
    {
        callback(Context->Device, EventCode, EventData);
    }
}

static NTSTATUS
PmicGlinkEnsureBclCriticalCallback(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    UNICODE_STRING callbackName;
    OBJECT_ATTRIBUTES callbackAttributes;

    if (!Context->GlinkDeviceLoaded || !Context->BattMiniDeviceLoaded)
    {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&callbackName, L"\\Callback\\PmicGlinkBCLCriticalCB");
    InitializeObjectAttributes(
        &callbackAttributes,
        &callbackName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    status = ExCreateCallback(
        &Context->BclCriticalCallbackObject,
        &callbackAttributes,
        TRUE,
        TRUE);
    if (NT_SUCCESS(status) && Context->ABDAttached)
    {
        Context->BclCriticalCallbackEnabled = TRUE;
    }

    return status;
}

static VOID
PmicGlinkCloseIoTargetIfOpen(
    _Inout_ WDFIOTARGET* IoTarget
    )
{
    if ((IoTarget != NULL) && (*IoTarget != NULL))
    {
        WdfIoTargetClose(*IoTarget);
    }
}

static VOID
PmicGlinkClearAbdAttachment(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ BOOLEAN UnregisterConnections
    )
{
    if (Context == NULL)
    {
        return;
    }

    if (UnregisterConnections && Context->ABDAttached && (Context->AbdIoTarget != NULL))
    {
        (VOID)PmicGlinkAbdUpdateConnections(Context, FALSE);
    }

    PmicGlinkCloseIoTargetIfOpen(&Context->AbdIoTarget);

    Context->ABDAttached = FALSE;
    Context->AllReqIntfArrived = FALSE;
    Context->BclCriticalCallbackEnabled = FALSE;
}

static VOID
PmicGlinkClearBattMiniAttachment(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if ((Context == NULL) || (Context->BattMiniNotifyLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(Context->BattMiniNotifyLock, NULL);
    if (Context->BattMiniDeviceLoaded)
    {
        PmicGlinkCloseIoTargetIfOpen(&Context->BattMiniIoTarget);
    }
    Context->BattMiniDeviceLoaded = FALSE;
    Context->NotificationFlag = FALSE;
    WdfWaitLockRelease(Context->BattMiniNotifyLock);
}

static NTSTATUS
PmicGlinkUnregisterPnPNotificationEntry(
    _Inout_ PVOID* Entry
    )
{
    NTSTATUS status;

    if ((Entry == NULL) || (*Entry == NULL))
    {
        return STATUS_SUCCESS;
    }

    status = IoUnregisterPlugPlayNotification(*Entry);
    *Entry = NULL;
    return status;
}

static NTSTATUS
PmicGlinkInterfaceNotificationCallback(
    _In_ PVOID NotificationStructure,
    _Inout_opt_ PVOID Context
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION notification;
    BOOLEAN arrival;
    NTSTATUS status;
    ULONG currentState;

    if (Context == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;

    arrival = FALSE;
    if (RtlCompareMemory(&notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL_LOCAL, sizeof(GUID)) == sizeof(GUID))
    {
        arrival = TRUE;
    }
    else if (RtlCompareMemory(&notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL_LOCAL, sizeof(GUID)) == sizeof(GUID))
    {
        arrival = FALSE;
    }
    else
    {
        return STATUS_SUCCESS;
    }

    if (RtlCompareMemory(&notification->InterfaceClassGuid, &GUID_GLINK_DEVICE_INTERFACE, sizeof(GUID)) == sizeof(GUID))
    {
        status = STATUS_SUCCESS;
        if (arrival)
        {
            if (deviceContext->GlinkDeviceLoaded)
            {
                return STATUS_SUCCESS;
            }

            deviceContext->GlinkDeviceLoaded = TRUE;
            deviceContext->AllReqIntfArrived = deviceContext->ABDAttached ? TRUE : FALSE;
            KeInitializeEvent(&gPmicGlinkConnectedEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkLocalDisconnectedEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkRemoteDisconnectedEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkTxNotificationEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkRxIntentReqEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkRxNotificationEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&gPmicGlinkRxIntentNotificationEvent, NotificationEvent, FALSE);
            currentState = PMICGLINK_RPE_STATE_ID_PDR_READY_FOR_COMMANDS;
            PmicGlinkRpeADSPStateNotificationCallback(deviceContext->Device, 0u, &currentState);
            status = PmicGlinkEnsureBclCriticalCallback(deviceContext);
        }
        else
        {
            if (!deviceContext->GlinkDeviceLoaded)
            {
                return STATUS_SUCCESS;
            }

            deviceContext->GlinkDeviceLoaded = FALSE;
            deviceContext->AllReqIntfArrived = FALSE;
            deviceContext->GlinkLinkStateUp = FALSE;
        }

        return status;
    }

    if (RtlCompareMemory(&notification->InterfaceClassGuid, &GUID_ABD_DEVINTERFACE, sizeof(GUID)) == sizeof(GUID))
    {
        status = STATUS_SUCCESS;
        if (arrival)
        {
            if (!deviceContext->ABDAttached)
            {
                if (deviceContext->AbdIoTarget == NULL)
                {
                    status = WdfIoTargetCreate(
                        deviceContext->Device,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &deviceContext->AbdIoTarget);
                }

                if (NT_SUCCESS(status) && (deviceContext->AbdIoTarget != NULL))
                {
                    WDF_IO_TARGET_OPEN_PARAMS openParams;

                    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                        &openParams,
                        (PUNICODE_STRING)&notification->SymbolicLinkName,
                        GENERIC_READ | GENERIC_WRITE);
                    openParams.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
                    status = WdfIoTargetOpen(deviceContext->AbdIoTarget, &openParams);
                    if (NT_SUCCESS(status))
                    {
                        deviceContext->ABDAttached = TRUE;
                        status = PmicGlinkAbdUpdateConnections(deviceContext, TRUE);
                        if (!NT_SUCCESS(status))
                        {
                            WdfIoTargetClose(deviceContext->AbdIoTarget);
                            deviceContext->ABDAttached = FALSE;
                        }
                        else
                        {
                            status = PmicGlinkEnsureBclCriticalCallback(deviceContext);
                        }
                    }
                }
            }
        }
        else
        {
            PmicGlinkClearAbdAttachment(deviceContext, FALSE);
        }

        deviceContext->AllReqIntfArrived = (deviceContext->GlinkDeviceLoaded && deviceContext->ABDAttached) ? TRUE : FALSE;
        return status;
    }

    if (RtlCompareMemory(&notification->InterfaceClassGuid, &GUID_DEVINTERFACE_PMIC_BATT_MINI, sizeof(GUID)) == sizeof(GUID))
    {
        status = STATUS_SUCCESS;
        WdfWaitLockAcquire(deviceContext->BattMiniNotifyLock, NULL);

        if (arrival)
        {
            if (!deviceContext->BattMiniDeviceLoaded)
            {
                if (deviceContext->BattMiniIoTarget == NULL)
                {
                    status = WdfIoTargetCreate(
                        deviceContext->Device,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &deviceContext->BattMiniIoTarget);
                }

                if (NT_SUCCESS(status) && (deviceContext->BattMiniIoTarget != NULL))
                {
                    WDF_IO_TARGET_OPEN_PARAMS openParams;
                    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                        &openParams,
                        (PUNICODE_STRING)&notification->SymbolicLinkName,
                        GENERIC_READ | GENERIC_WRITE);
                    openParams.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
                    status = WdfIoTargetOpen(deviceContext->BattMiniIoTarget, &openParams);
                    if (NT_SUCCESS(status))
                    {
                        deviceContext->BattMiniDeviceLoaded = TRUE;
                        status = PmicGlinkEnsureBclCriticalCallback(deviceContext);
                    }
                }
            }

            deviceContext->NotificationFlag = deviceContext->BattMiniDeviceLoaded;
        }
        else
        {
            if (deviceContext->BattMiniDeviceLoaded)
            {
                WdfIoTargetClose(deviceContext->BattMiniIoTarget);
            }

            deviceContext->BattMiniDeviceLoaded = FALSE;
            deviceContext->NotificationFlag = FALSE;
        }

        WdfWaitLockRelease(deviceContext->BattMiniNotifyLock);

        return status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkDevice_RegisterForPnPNotifications(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ BOOLEAN Register
    )
{
    NTSTATUS status;
    NTSTATUS unregisterStatus;
    PDRIVER_OBJECT driverObject;

    if (Context == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!Register)
    {
        status = STATUS_SUCCESS;

        PmicGlinkResetApiInterface();

        unregisterStatus = PmicGlinkUnregisterPnPNotificationEntry(&Context->GlinkNotificationEntry);
        if (!NT_SUCCESS(unregisterStatus))
        {
            return unregisterStatus;
        }

        unregisterStatus = PmicGlinkUnregisterPnPNotificationEntry(&Context->AbdNotificationEntry);
        if (!NT_SUCCESS(unregisterStatus))
        {
            return unregisterStatus;
        }

        if (Context->BattMiniNotifyLock != NULL)
        {
            WdfWaitLockAcquire(Context->BattMiniNotifyLock, NULL);
            unregisterStatus = PmicGlinkUnregisterPnPNotificationEntry(&Context->BattMiniNotificationEntry);
            if (!NT_SUCCESS(unregisterStatus) && NT_SUCCESS(status))
            {
                status = unregisterStatus;
            }
            WdfWaitLockRelease(Context->BattMiniNotifyLock);
        }
        else
        {
            unregisterStatus = PmicGlinkUnregisterPnPNotificationEntry(&Context->BattMiniNotificationEntry);
            if (!NT_SUCCESS(unregisterStatus) && NT_SUCCESS(status))
            {
                status = unregisterStatus;
            }
        }

        return status;
    }

    driverObject = WdfDriverWdmGetDriverObject(WdfDeviceGetDriver(Context->Device));

    if (Context->GlinkNotificationEntry == NULL)
    {
        status = IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID)&GUID_GLINK_DEVICE_INTERFACE,
            driverObject,
            PmicGlinkInterfaceNotificationCallback,
            Context,
            &Context->GlinkNotificationEntry);
        if (!NT_SUCCESS(status))
        {
            Context->GlinkNotificationEntry = NULL;
        }
    }

    if (Context->AbdNotificationEntry == NULL)
    {
        status = IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID)&GUID_ABD_DEVINTERFACE,
            driverObject,
            PmicGlinkInterfaceNotificationCallback,
            Context,
            &Context->AbdNotificationEntry);
        if (!NT_SUCCESS(status))
        {
            Context->AbdNotificationEntry = NULL;
            return status;
        }
    }

    if (Context->BattMiniNotificationEntry == NULL)
    {
        status = IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID)&GUID_DEVINTERFACE_PMIC_BATT_MINI,
            driverObject,
            PmicGlinkInterfaceNotificationCallback,
            Context,
            &Context->BattMiniNotificationEntry);
        if (!NT_SUCCESS(status))
        {
            Context->BattMiniNotificationEntry = NULL;
            return status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkDevice_InitContext(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    if (Context == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Context->DeviceInterfacesRegistered = FALSE;
    Context->AllReqIntfArrived = FALSE;
    Context->GlinkDeviceLoaded = FALSE;
    Context->ABDAttached = FALSE;
    Context->DriverContext = NULL;
    Context->InterruptResourceCount = 0;
    Context->IOResourceCount = 0;
    RtlZeroMemory(Context->GpioConnectionId, sizeof(Context->GpioConnectionId));
    Context->GlinkChannelConnected = FALSE;
    Context->GlinkChannelRestart = FALSE;
    Context->GlinkChannelFirstConnect = FALSE;
    Context->GlinkChannelUlogConnected = FALSE;
    Context->GlinkChannelUlogRestart = FALSE;
    Context->GlinkChannelUlogFirstConnect = FALSE;
    Context->GlinkLinkStateUp = FALSE;
    Context->RpeInitialized = FALSE;
    Context->Hibernate = FALSE;
    KeInitializeMutex(&gPmicGlinkTxSync, 1);
    gPmicGlinkRxInProgress = 0;
    KeInitializeEvent(&gPmicGlinkConnectedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkLocalDisconnectedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkRemoteDisconnectedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkTxNotificationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkRxIntentReqEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkRxNotificationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkRxIntentNotificationEvent, NotificationEvent, FALSE);
    RtlZeroMemory(Context->CommData, sizeof(Context->CommData));
    gPmicGlinkUlogRxInProgress = 0;
    KeInitializeEvent(&gPmicGlinkUlogTxNotificationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkUlogRxNotificationEvent, NotificationEvent, FALSE);
    PmicGlinkResetApiInterface();
    gPmicGlinkMainChannelHandle = NULL;
    gPmicGlinkUlogChannelHandle = NULL;
    gPmicGlinkLinkStateHandle = NULL;
    Context->Notify = FALSE;
    Context->NotificationFlag = FALSE;
    Context->LastRxOpcode = 0;
    Context->LastRxStatus = STATUS_SUCCESS;
    Context->LastRxValid = FALSE;
    Context->LastUlogRxOpcode = 0;
    Context->LastUlogRxStatus = STATUS_SUCCESS;
    Context->LastUlogRxValid = FALSE;
    gPmicGlinkUlogInitPrinted = 0;
    (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
    gPmicGlinkCachedBatteryStatus = 0;
    gPmicGlinkPendingPlatformState = 0;
    RtlZeroMemory(&gPmicGlinkPendingUsbcWriteRequest, sizeof(gPmicGlinkPendingUsbcWriteRequest));
    Context->EventID = 0;
    Context->GlinkRxIntent = 0;
    Context->GlinkUlogRxIntent = 0;

    Context->NumPorts = 0;
    RtlZeroMemory(Context->UsbinPower, sizeof(Context->UsbinPower));

    Context->LastUsbBattMngrQueryTime.QuadPart = 0;
    gPmicGlinkLastUsbIoctlEvent.QuadPart = 0;

    RtlZeroMemory(Context->UCSIDataBuffer, sizeof(Context->UCSIDataBuffer));

    RtlZeroMemory(Context->OemPropData, sizeof(Context->OemPropData));
    RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));
    RtlZeroMemory(Context->CachedOemSendBuffer, sizeof(Context->CachedOemSendBuffer));

    RtlZeroMemory(Context->I2CHeader, sizeof(Context->I2CHeader));
    RtlZeroMemory(Context->I2CData, sizeof(Context->I2CData));
    Context->I2CDataLength = 0;

    Context->gws_out.AlarmStatus = 0;
    Context->cws_out.Status = 0;
    Context->stp_out.Status = 0;
    Context->stv_out.Status = 0;
    Context->tip_out.PolicySetting = 0;
    Context->tiv_out.TimerValueRemain = 0;

    Context->QcmbConnected = FALSE;
    Context->QcmbStatus = 0;
    Context->QcmbCurrentChargerPowerUW = 0;
    Context->QcmbGoodChargerThresholdUW = 0;
    Context->QcmbChargerStatusInfo = 0;

    Context->HvdcpCharger.ChargerSupported = FALSE;
    Context->HvdcpCharger.ChargerCurrent = 1500;
    Context->HvdcpCharger.ChargerGUID = GUID_USBFN_HVDCP_PROPRIETARY_CHARGER;

    Context->HvdcpV3Charger.ChargerSupported = FALSE;
    Context->HvdcpV3Charger.ChargerCurrent = 1500;
    Context->HvdcpV3Charger.ChargerGUID = GUID_USBFN_HVDCP_PROPRIETARY_CHARGER_V3;

    Context->IWallCharger.ChargerSupported = FALSE;
    Context->IWallCharger.ChargerCurrent = 500;
    Context->IWallCharger.ChargerGUID = GUID_USBFN_TYPE_I_WALL_CHARGER;

    Context->MaxFlashCurrent = 500;
    Context->LastChargerVoltage = 0;
    Context->LastChargerPortType = 0;

    Context->LegacyBattId.batt_id = 0xFFFF;
    Context->LegacyBattStateId = 0xCu;

    Context->LegacyChargeStatus.power_state = 2;
    Context->LegacyChargeStatus.capacity = 0xFFFFFFFF;
    Context->LegacyChargeStatus.voltage = 0xFFFFFFFF;
    Context->LegacyChargeStatus.rate = (LONG)0x80000000;
    Context->LegacyChargeStatus.charging_source = 2;

    RtlZeroMemory(&Context->LegacyBattInfo, sizeof(Context->LegacyBattInfo));
    Context->LegacyBattInfo.capabilities = 0x8000000F;
    Context->LegacyBattInfo.technology = 1;
    Context->LegacyBattInfo.chemistry[0] = 'L';
    Context->LegacyBattInfo.chemistry[1] = 'I';
    Context->LegacyBattInfo.chemistry[2] = 'O';
    Context->LegacyBattInfo.chemistry[3] = 'N';
    Context->LegacyBattInfo.designed_capacity = 0x5848;
    Context->LegacyBattInfo.full_charged_capacity = 0x5848;
    Context->LegacyBattInfo.default_alert1 = 0x62E;
    Context->LegacyBattInfo.default_alert2 = 2034;
    Context->LegacyBattInfo.critical_bias = 0;
    Context->LegacyBattInfo.cycle_count = 0;

    RtlZeroMemory(Context->LegacyReportingScale, sizeof(Context->LegacyReportingScale));
    Context->LegacyReportingScale[0].granularity = 1;
    Context->LegacyReportingScale[0].capacity = 0xFFFFFFFF;

    Context->LegacyBattTemperature = 0x0BA5;
    Context->LegacyBattEstimatedTime = 0xFFFFFFFF;

    RtlZeroMemory(Context->LegacyBattDeviceName, sizeof(Context->LegacyBattDeviceName));
    RtlCopyMemory(
        Context->LegacyBattDeviceName,
        L"QCOMBATT01",
        sizeof(L"QCOMBATT01"));

    Context->LegacyBattManufactureDate.day = 1;
    Context->LegacyBattManufactureDate.month = 7;
    Context->LegacyBattManufactureDate.year = 2011;

    RtlZeroMemory(Context->LegacyBattManufactureName, sizeof(Context->LegacyBattManufactureName));
    RtlCopyMemory(
        Context->LegacyBattManufactureName,
        L"Qualcomm",
        sizeof(L"Qualcomm"));

    RtlZeroMemory(Context->LegacyBattUniqueId, sizeof(Context->LegacyBattUniqueId));
    RtlCopyMemory(
        Context->LegacyBattUniqueId,
        L"07012011",
        sizeof(L"07012011"));

    RtlZeroMemory(Context->LegacyBattSerialNumber, sizeof(Context->LegacyBattSerialNumber));
    RtlCopyMemory(
        Context->LegacyBattSerialNumber,
        L"QCOMBAT01_07012011",
        sizeof(L"QCOMBAT01_07012011"));

    RtlZeroMemory(&Context->LegacyControlCharging, sizeof(Context->LegacyControlCharging));
    Context->LegacyControlCharging.batt_id = 0xFFFF;
    Context->LegacyControlCharging.batt_command = 1;
    Context->LegacyControlCharging.batt_critical_bias_mw = 0;
    Context->LegacyControlCharging.batt_max_charge_current = 0xFFFFFFFF;

    RtlZeroMemory(&Context->LegacyStatusCriteria, sizeof(Context->LegacyStatusCriteria));
    Context->LegacyStatusCriteria.batt_id = 0xFFFF;
    Context->LegacyStatusCriteria.batt_notify_criteria.power_state = 0;
    Context->LegacyStatusCriteria.batt_notify_criteria.low_capacity = 0;
    Context->LegacyStatusCriteria.batt_notify_criteria.high_capacity = 0xFFFF;

    Context->LegacyOperationalMode.mode_type = 1;

    Context->LegacyChargeRate.charge_perc = 100;
    Context->LegacyChargeRate.device_name = 0;
    Context->LegacyChargeRate.reserved[0] = 0;
    Context->LegacyChargeRate.reserved[1] = 0;
    Context->LegacyChargeRate.reserved[2] = 0;

    RtlZeroMemory(&Context->LegacyTestInfo, sizeof(Context->LegacyTestInfo));

    Context->LegacyBattPercentage = 75;
    Context->LegacyStatusNotificationPending = FALSE;
    Context->LegacyStateChangePending = FALSE;

    Context->LegacyLastBattIdQueryMsec = 0;
    Context->LegacyLastChargeStatusQueryMsec = 0;
    Context->LegacyLastBattInfoQueryMsec = 0;
    Context->LegacyLastTestInfoRequestType = 0;
    Context->LegacyReserved = 0;

    Context->ModernStandbyState = 0;
    Context->ModernStandbyCallbackObject = NULL;
    Context->ModernStandbyCallbackHandle = NULL;
    Context->BclCriticalCallbackObject = NULL;
    Context->BclCriticalCallbackEnabled = FALSE;
    Context->GlinkNotificationEntry = NULL;
    Context->AbdNotificationEntry = NULL;
    Context->AbdIoTarget = NULL;
    Context->BattMiniNotificationEntry = NULL;
    Context->BattMiniNotifyLock = NULL;
    Context->BattMiniIoTarget = NULL;
    Context->BattMiniDeviceLoaded = FALSE;

    PmicGlinkInitAbdConnections();
    if (Context->Device != NULL)
    {
        (VOID)PmicGlinkEvaluateAbdConnectionCount(Context->Device);
    }

    Context->DdiInterfaceRefCount = 0;
    Context->DdiInterfacePadding = 0;
    RtlZeroMemory(&Context->DdiInterface, sizeof(Context->DdiInterface));
    RtlZeroMemory(&Context->CrashDumpAdditionalCallbackRecord, sizeof(Context->CrashDumpAdditionalCallbackRecord));
    Context->CrashDumpAdditionalCallbackRegistered = FALSE;
    RtlZeroMemory(&Context->CrashDumpTriageCallbackRecord, sizeof(Context->CrashDumpTriageCallbackRecord));
    Context->CrashDumpTriageCallbackRegistered = FALSE;
    Context->CrashDumpTriageDataArray = NULL;
    Context->CrashDumpTriageDataArraySize = 0;
    Context->CrashDumpDataSourceCount = PMICGLINK_CRASHDUMP_MAX_SOURCES;
    RtlZeroMemory(Context->CrashDumpDataSources, sizeof(Context->CrashDumpDataSources));

    RtlZeroMemory(&gPmicGlinkUsbcNotification, sizeof(gPmicGlinkUsbcNotification));
    gPmicGlinkPendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
    Context->UsbcPinAssignmentNotifyEn = 0;
    Context->UlogInitEn = 0;
    Context->Reserved2[0] = 0;
    Context->Reserved2[1] = 0;
    Context->UlogInterval = 0;
    Context->UlogLevel = PMICGLINK_ULOG_DEFAULT_LEVEL;
    Context->UlogCategories = PMICGLINK_ULOG_DEFAULT_CATEGORIES;
    Context->UlogTimer = NULL;

    KeInitializeEvent(&Context->QcmbNotifyEvent, NotificationEvent, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Context->Device;

    status = WdfSpinLockCreate(&attributes, &Context->StateLock);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfWaitLockCreate(&attributes, &Context->DdiInterfaceLock);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfWaitLockCreate(&attributes, &Context->BattMiniNotifyLock);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfWaitLockCreate(&attributes, &Context->CrashDumpLock);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    gCrashDumpContext = Context;
    RtlZeroMemory(&gLatestUcsiCmd, sizeof(gLatestUcsiCmd));

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlink_Init(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->GlinkChannelRestart = FALSE;
    Context->GlinkChannelFirstConnect = FALSE;
    Context->GlinkChannelUlogConnected = FALSE;
    Context->GlinkChannelUlogRestart = FALSE;
    Context->GlinkChannelUlogFirstConnect = FALSE;
    Context->GlinkDeviceLoaded = FALSE;
    Context->GlinkLinkStateUp = FALSE;
    Context->RpeInitialized = FALSE;
    Context->Hibernate = FALSE;
    Context->GlinkRxIntent = 0;
    Context->GlinkUlogRxIntent = 0;
    KeInitializeMutex(&gPmicGlinkTxSync, 1);
    Context->LastRxOpcode = 0;
    Context->LastRxStatus = STATUS_SUCCESS;
    Context->LastRxValid = FALSE;
    Context->LastUlogRxOpcode = 0;
    Context->LastUlogRxStatus = STATUS_SUCCESS;
    Context->LastUlogRxValid = FALSE;
    gPmicGlinkUlogInitPrinted = 0;
    Context->UsbcPinAssignmentNotifyEn = 0;
    Context->UlogInitEn = 0;
    Context->UlogInterval = 0;
    Context->UlogLevel = PMICGLINK_ULOG_DEFAULT_LEVEL;
    Context->UlogCategories = PMICGLINK_ULOG_DEFAULT_CATEGORIES;
    Context->UlogTimer = NULL;

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkLoadPlatformConfig(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    NTSTATUS queryStatus;
    WDFKEY regKey;
    WDFKEY configRegKey;
    UNICODE_STRING regValueName;
    ULONG regValueData;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return;
    }

    regValueData = 0;
    configRegKey = NULL;
    status = WdfDeviceOpenRegistryKey(
        Context->Device,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &regKey);
    if (!NT_SUCCESS(status))
    {
        return;
    }

    RtlInitUnicodeString(&regValueName, L"PlatformConfig");
    status = WdfRegistryOpenKey(
        regKey,
        &regValueName,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &configRegKey);
    if (NT_SUCCESS(status))
    {
        regValueData = 0;
        RtlInitUnicodeString(&regValueName, L"UsbcPinAssignmentNotifyEn");
        queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &regValueData);
        if (NT_SUCCESS(queryStatus))
        {
            Context->UsbcPinAssignmentNotifyEn = (UCHAR)regValueData;
        }

        regValueData = 0;
        RtlInitUnicodeString(&regValueName, L"UlogInitEn");
        queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &regValueData);
        if (NT_SUCCESS(queryStatus))
        {
            Context->UlogInitEn = (UCHAR)regValueData;
        }

        regValueData = 0;
        RtlInitUnicodeString(&regValueName, L"UlogInterval");
        queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &regValueData);
        if (NT_SUCCESS(queryStatus))
        {
            Context->UlogInterval = regValueData;
        }

        regValueData = 0;
        RtlInitUnicodeString(&regValueName, L"UlogLevel");
        queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &regValueData);
        if (NT_SUCCESS(queryStatus))
        {
            Context->UlogLevel = regValueData;
        }

        regValueData = 0;
        RtlInitUnicodeString(&regValueName, L"UlogCategoriesL");
        queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &regValueData);
        if (NT_SUCCESS(queryStatus))
        {
            ULONGLONG categoriesLow;
            ULONG categoriesHigh;

            categoriesLow = (ULONGLONG)regValueData;
            categoriesHigh = 0;
            RtlInitUnicodeString(&regValueName, L"UlogCategoriesH");
            queryStatus = PmicGlinkRegistryQuery(configRegKey, &regValueName, &categoriesHigh);
            if (NT_SUCCESS(queryStatus))
            {
                Context->UlogCategories = categoriesLow | ((ULONGLONG)categoriesHigh << 32);
            }
        }

        WdfRegistryClose(configRegKey);
    }

    WdfRegistryClose(regKey);
}

static VOID
PmicGlinkResetApiInterface(
    VOID
    )
{
    if ((gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
        && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceDereference != NULL))
    {
        gPmicGlinkApiInterface.InterfaceHeader.InterfaceDereference(
            gPmicGlinkApiInterface.InterfaceHeader.Context);
    }

    RtlZeroMemory(&gPmicGlinkApiInterface, sizeof(gPmicGlinkApiInterface));
    gPmicGlinkMainChannelHandle = NULL;
    gPmicGlinkUlogChannelHandle = NULL;
    gPmicGlinkLinkStateHandle = NULL;
}

static VOID
PmicGlinkResetCommDataSlots(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ BOOLEAN DeleteMemory
    )
{
    ULONG opCode;

    if (Context == NULL)
    {
        return;
    }

    if (!DeleteMemory && (Context->StateLock != NULL))
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    for (opCode = 0u; opCode < PMICGLINK_COMM_DATA_SLOTS; opCode++)
    {
        PMICGLINK_COMM_DATA* slot;

        slot = &Context->CommData[opCode];
        if ((slot->Buffer != NULL) && (slot->Size > 0u))
        {
            RtlZeroMemory(slot->Buffer, slot->Size);
        }

        slot->Size = 0u;
        if (DeleteMemory && (slot->Memory != NULL))
        {
            WdfObjectDelete(slot->Memory);
            slot->Memory = NULL;
            slot->Buffer = NULL;
        }
    }

    if (!DeleteMemory && (Context->StateLock != NULL))
    {
        WdfSpinLockRelease(Context->StateLock);
    }
}

static NTSTATUS
PmicGlinkEnsureApiInterface(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
    {
        return STATUS_SUCCESS;
    }

    PmicGlinkResetApiInterface();
    status = WdfFdoQueryForInterface(
        Context->Device,
        &GUID_GLINK_API_INTERFACE,
        (PINTERFACE)&gPmicGlinkApiInterface,
        sizeof(gPmicGlinkApiInterface),
        1,
        NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkEnsureCommDataBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _In_ SIZE_T RequiredSize
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    PMICGLINK_COMM_DATA* slot;
    SIZE_T allocatedSize;

    if ((Context == NULL)
        || (OpCode >= PMICGLINK_COMM_DATA_SLOTS)
        || (RequiredSize == 0u))
    {
        return STATUS_INVALID_PARAMETER;
    }

    slot = &Context->CommData[OpCode];
    if ((slot->Memory != NULL) && (slot->Buffer != NULL))
    {
        allocatedSize = 0u;
        (VOID)WdfMemoryGetBuffer(slot->Memory, &allocatedSize);
        if (allocatedSize >= RequiredSize)
        {
            return STATUS_SUCCESS;
        }
    }

    if (slot->Memory != NULL)
    {
        WdfObjectDelete(slot->Memory);
        slot->Memory = NULL;
        slot->Buffer = NULL;
        slot->Size = 0u;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Context->Device;
    status = WdfMemoryCreate(
        &attributes,
        NonPagedPoolNx,
        PMICGLINK_POOLTAG_COMMDATA,
        RequiredSize,
        &slot->Memory,
        (PVOID*)&slot->Buffer);
    if (!NT_SUCCESS(status))
    {
        slot->Memory = NULL;
        slot->Buffer = NULL;
        slot->Size = 0u;
        return status;
    }

    slot->Size = 0u;
    return STATUS_SUCCESS;
}

static BOOLEAN
PmicGlinkTryExtractMessageOp(
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _Out_ PULONG MessageOp
    )
{
    if (MessageOp == NULL)
    {
        return FALSE;
    }

    *MessageOp = MAXULONG;
    if ((Buffer == NULL) || (BufferSize < (sizeof(ULONGLONG) + sizeof(ULONG))))
    {
        return FALSE;
    }

    RtlCopyMemory(
        MessageOp,
        (const UCHAR*)Buffer + sizeof(ULONGLONG),
        sizeof(*MessageOp));
    return TRUE;
}

static NTSTATUS
PmicGlinkStoreCommDataPacket(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _In_reads_bytes_(BufferSize) const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ BOOLEAN SanitizeWord5
    )
{
    NTSTATUS status;
    PMICGLINK_COMM_DATA* slot;
    SIZE_T copySize;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (BufferSize == 0u)
        || (OpCode >= PMICGLINK_COMM_DATA_SLOTS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = PmicGlinkEnsureCommDataBuffer(Context, OpCode, BufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    slot = &Context->CommData[OpCode];
    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    copySize = BufferSize;
    RtlZeroMemory(slot->Buffer, copySize);
    RtlCopyMemory(slot->Buffer, Buffer, copySize);
    if (SanitizeWord5 && (copySize >= (sizeof(USHORT) * 6u)))
    {
        ((PUSHORT)slot->Buffer)[5] = 0u;
    }

    slot->Size = (copySize > 0xFFFFu) ? 0xFFFFu : (USHORT)copySize;

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }

    return STATUS_SUCCESS;
}

static BOOLEAN
PmicGlinkIsDeferredNotificationOp(
    _In_ ULONG OpCode
    )
{
    return ((OpCode == 7u)
        || (OpCode == 19u)
        || (OpCode == 22u)
        || (OpCode == 130u)
        || (OpCode == 259u));
}

static VOID
PmicGlinkClearCommDataSlot(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode
    )
{
    PMICGLINK_COMM_DATA* slot;

    if ((Context == NULL) || (OpCode >= PMICGLINK_COMM_DATA_SLOTS))
    {
        return;
    }

    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    slot = &Context->CommData[OpCode];
    if ((slot->Buffer != NULL) && (slot->Size > 0u))
    {
        RtlZeroMemory(slot->Buffer, slot->Size);
    }

    slot->Size = 0u;

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }
}

static BOOLEAN
PmicGlinkGetPendingNotificationOp(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ PULONG OpCode
    )
{
    static const ULONG notificationOps[] = { 19u, 7u, 22u, 259u, 130u };
    BOOLEAN found;
    ULONG i;

    if (OpCode != NULL)
    {
        *OpCode = MAXULONG;
    }

    if (Context == NULL)
    {
        return FALSE;
    }

    found = FALSE;
    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    for (i = 0u; i < ARRAYSIZE(notificationOps); i++)
    {
        PMICGLINK_COMM_DATA* slot;
        ULONG currentOp;

        currentOp = notificationOps[i];
        slot = &Context->CommData[currentOp];
        if ((slot->Buffer != NULL) && (slot->Size > 0u))
        {
            if (OpCode != NULL)
            {
                *OpCode = currentOp;
            }

            found = TRUE;
            break;
        }
    }

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }

    return found;
}

static BOOLEAN
PmicGlinkConsumeCommDataPacket(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode
    )
{
    BOOLEAN hasPacket;
    BOOLEAN expectedReceived;

    if ((Context == NULL) || (OpCode >= PMICGLINK_COMM_DATA_SLOTS))
    {
        return FALSE;
    }

    hasPacket = FALSE;
    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    if ((Context->CommData[OpCode].Buffer != NULL)
        && (Context->CommData[OpCode].Size > 0u))
    {
        hasPacket = TRUE;
    }

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }

    if (!hasPacket)
    {
        return FALSE;
    }

    expectedReceived = FALSE;
    (VOID)PmicGlink_RetrieveRxData(Context, OpCode, &expectedReceived);
    PmicGlinkClearCommDataSlot(Context, OpCode);
    return (expectedReceived != FALSE) ? TRUE : FALSE;
}

static NTSTATUS
PmicGlink_OpenGlinkChannel(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    PMIC_GLINK_OPEN_CONFIG openConfig;
    GLINK_CHANNEL_CTX* channelHandle;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = PmicGlinkEnsureApiInterface(Context);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SUCCESS;
    }

    if (gPmicGlinkApiInterface.GLinkOpen == NULL)
    {
        return STATUS_SUCCESS;
    }

    gPmicGlinkMainChannelHandle = NULL;
    channelHandle = NULL;
    RtlZeroMemory(&openConfig, sizeof(openConfig));
    openConfig.Transport = "SMEM";
    openConfig.RemoteSs = "lpass";
    openConfig.Name = "PMIC_RTR_ADSP_APPS";
    openConfig.Options = 0;
    openConfig.Context = Context;
    openConfig.NotifyRx = PmicGlinkRxNotificationCb;
    openConfig.NotifyTxDone = PmicGlinkTxNotificationCb;
    openConfig.NotifyState = PmicGlinkStateNotificationShim;
    openConfig.NotifyRxIntentReq = PmicGlinkNotifyRxIntentReqShim;
    openConfig.NotifyRxIntent = PmicGlinkNotifyRxIntentShim;
    openConfig.NotifyRxSigs = PmicGlinkNotifyRxSigsShim;
    openConfig.NotifyRxAbort = PmicGlinkNotifyRxAbortShim;
    openConfig.NotifyTxAbort = PmicGlinkNotifyTxAbortShim;

    status = gPmicGlinkApiInterface.GLinkOpen(&openConfig, &channelHandle);
    if (status != STATUS_SUCCESS)
    {
        gPmicGlinkMainChannelHandle = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    gPmicGlinkMainChannelHandle = channelHandle;
    return STATUS_SUCCESS;
}

static VOID
PmicGlinkRegisterInterfaceWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if ((context != NULL) && context->GlinkChannelConnected)
    {
        USBPD_DPM_USBC_WRITE_BUFFER request;
        LARGE_INTEGER delayInterval;
        UCHAR qcmbMessage[64];
        ULONG qcmbStatus;
        NTSTATUS status;
        BOOLEAN firstConnect;

        firstConnect = context->GlinkChannelFirstConnect;
        gPmicGlinkPendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
        RtlZeroMemory(&gPmicGlinkUsbcNotification, sizeof(gPmicGlinkUsbcNotification));
        RtlZeroMemory(&gPmicGlinkPendingUsbcWriteRequest, sizeof(gPmicGlinkPendingUsbcWriteRequest));

        if (firstConnect && (context->UsbcPinAssignmentNotifyEn != 0))
        {
            RtlZeroMemory(&request, sizeof(request));
            request.cmd_type = 16u;
            request.cmd_payload.read_sel = 0u;
            delayInterval.QuadPart = -500000ll;
            (VOID)KeDelayExecutionThread(KernelMode, FALSE, &delayInterval);
            (VOID)PmicGlinkPlatformUsbc_Request_Write(context, &request);
        }

        if (*(ULONGLONG*)&gLatestUcsiCmd.data[8] != 0ull)
        {
            (VOID)PmicGlink_SyncSendReceive(
                context,
                IOCTL_PMICGLINK_UCSI_WRITE,
                gLatestUcsiCmd.data,
                PMICGLINK_UCSI_BUFFER_SIZE);
        }

        context->QcmbConnected = FALSE;
        context->QcmbStatus = 0;
        context->QcmbCurrentChargerPowerUW = 0;
        context->QcmbGoodChargerThresholdUW = 0;
        context->QcmbChargerStatusInfo = 0;
        KeInitializeEvent(&context->QcmbNotifyEvent, NotificationEvent, FALSE);

        status = PmicGlinkPlatformQcmb_WriteMBToBuffer(context, qcmbMessage, sizeof(qcmbMessage));
        if (NT_SUCCESS(status))
        {
            status = PmicGlink_SendData(context, 0x81u, qcmbMessage, sizeof(qcmbMessage), TRUE);
            if (NT_SUCCESS(status))
            {
                qcmbStatus = 0;
                status = PmicGlinkPlatformQcmb_GetStatus(context, &qcmbStatus);
                if (NT_SUCCESS(status))
                {
                    context->QcmbConnected = ((qcmbStatus & 1u) != 0u) ? TRUE : FALSE;
                }
            }
        }

    }

    if (context != NULL)
    {
        (VOID)PmicGlinkNotifyLinkStateAcpi(
            context,
            context->GlinkChannelConnected ? 1u : 0u);
    }

    WdfObjectDelete(WorkItem);
}

static VOID
PmicGlinkStateNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ ULONG Event
    )
{
    PMICGLINK_CHANNEL_EVENT channelEvent;

    channelEvent = (PMICGLINK_CHANNEL_EVENT)Event;
    PmicGlinkStateNotificationCb(Channel, (PPMIC_GLINK_DEVICE_CONTEXT)Context, channelEvent);
}

static BOOLEAN
PmicGlinkNotifyRxIntentReqShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ SIZE_T RequestedSize
    )
{
    return PmicGlinkNotifyRxIntentReqCb(Channel, (PVOID)Context, RequestedSize);
}

static VOID
PmicGlinkNotifyRxIntentShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ SIZE_T Size
    )
{
    PmicGlinkNotifyRxIntentCb(Channel, (PVOID)Context, Size);
}

static VOID
PmicGlinkNotifyRxSigsShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ ULONG OldSigs,
    _In_ ULONG NewSigs
    )
{
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(OldSigs);
    UNREFERENCED_PARAMETER(NewSigs);
}

static VOID
PmicGlinkNotifyRxAbortShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext
    )
{
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PacketContext);
}

static VOID
PmicGlinkNotifyTxAbortShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext
    )
{
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PacketContext);
}

static VOID
PmicGlinkStateNotificationCb(
    _In_opt_ PVOID Handle,
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ PMICGLINK_CHANNEL_EVENT Event
    )
{
    GLINK_CHANNEL_CTX* channelHandle;

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkMainChannelHandle;

    if (Context == NULL)
    {
        return;
    }

    switch (Event)
    {
    case PmicGlinkChannelConnected:
        Context->GlinkChannelFirstConnect = TRUE;
        Context->GlinkChannelConnected = TRUE;
        Context->GlinkChannelRestart = FALSE;
        if (channelHandle != NULL)
        {
            gPmicGlinkMainChannelHandle = channelHandle;
        }
        (VOID)KeClearEvent(&gPmicGlinkLocalDisconnectedEvent);
        (VOID)KeClearEvent(&gPmicGlinkRemoteDisconnectedEvent);
        (VOID)KeSetEvent(&gPmicGlinkConnectedEvent, IO_NO_INCREMENT, FALSE);
        if ((channelHandle != NULL)
            && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkQueueRxIntent != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkQueueRxIntent(channelHandle, Context, 4096u);
        }
        Context->GlinkRxIntent += 1;
        (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
        break;

    case PmicGlinkChannelLocalDisconnected:
        Context->GlinkChannelConnected = FALSE;
        (VOID)KeClearEvent(&gPmicGlinkConnectedEvent);
        (VOID)KeSetEvent(&gPmicGlinkLocalDisconnectedEvent, IO_NO_INCREMENT, FALSE);
        if (Context->GlinkChannelRestart && Context->GlinkLinkStateUp)
        {
            (VOID)PmicGlink_OpenGlinkChannel(Context);
        }
        break;

    case PmicGlinkChannelRemoteDisconnected:
        if ((channelHandle != NULL)
            && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkClose != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkClose(channelHandle);

            gPmicGlinkMainChannelHandle = NULL;
            Context->GlinkChannelConnected = FALSE;
            Context->GlinkChannelRestart = TRUE;
            (VOID)KeClearEvent(&gPmicGlinkConnectedEvent);
            (VOID)KeSetEvent(&gPmicGlinkRemoteDisconnectedEvent, IO_NO_INCREMENT, FALSE);
            (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
        }
        break;

    default:
        break;
    }
}

static NTSTATUS
PmicGlink_SendData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _In_reads_bytes_opt_(BufferLen) PVOID Buffer,
    _In_ SIZE_T BufferLen,
    _In_ BOOLEAN WaitForRx
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    NTSTATUS status;
    LARGE_INTEGER pollInterval;
    ULONG waitCount;
    ULONG waitStatus;
    ULONG waitIndex;
    ULONG waitObjectCount;
    PVOID waitObjects[7];
    KWAIT_BLOCK waitBlocks[7];
    LONG txCount;
    BOOLEAN matchedResponse;
    BOOLEAN sawTxNotification;
    BOOLEAN expectedReceived;

    if ((Buffer == NULL) || (BufferLen == 0))
    {
        return STATUS_UNSUCCESSFUL;
    }

    txCount = ++gPmicGlinkTxCount;

    if (Context == NULL)
    {
        return STATUS_RETRY;
    }

    if (Context->GlinkChannelRestart
        || !Context->GlinkChannelConnected)
    {
        return STATUS_RETRY;
    }

    if (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference == NULL)
    {
        return STATUS_RETRY;
    }

    if (gPmicGlinkApiInterface.GLinkTx == NULL)
    {
        return STATUS_RETRY;
    }

    channelHandle = gPmicGlinkMainChannelHandle;
    if (channelHandle == NULL)
    {
        return STATUS_RETRY;
    }

    (VOID)KeWaitForSingleObject(&gPmicGlinkTxSync, Executive, KernelMode, FALSE, NULL);

    waitCount = 0;
    pollInterval.QuadPart = -20000ll;
    while (gPmicGlinkRxInProgress == 1)
    {
        if (waitCount >= 0x5DCu)
        {
            KeReleaseMutex(&gPmicGlinkTxSync, FALSE);
            return STATUS_RETRY;
        }

        (VOID)KeDelayExecutionThread(KernelMode, FALSE, &pollInterval);
        waitCount++;
    }

    gPmicGlinkRxInProgress = 1;
    KeReleaseMutex(&gPmicGlinkTxSync, FALSE);

    status = STATUS_SUCCESS;
    Context->NotificationFlag = FALSE;
    Context->LastRxOpcode = 0;
    Context->LastRxStatus = STATUS_SUCCESS;
    Context->LastRxValid = FALSE;
    (VOID)KeClearEvent(&gPmicGlinkTxNotificationEvent);
    (VOID)KeClearEvent(&gPmicGlinkRxIntentReqEvent);
    (VOID)KeClearEvent(&gPmicGlinkRxNotificationEvent);
    (VOID)KeClearEvent(&gPmicGlinkRxIntentNotificationEvent);
    if (OpCode < PMICGLINK_COMM_DATA_SLOTS)
    {
        PmicGlinkClearCommDataSlot(Context, OpCode);
    }
    status = gPmicGlinkApiInterface.GLinkTx(
        channelHandle,
        (PVOID)(ULONG_PTR)(ULONG)txCount,
        Buffer,
        BufferLen,
        1u);
    if (status != STATUS_SUCCESS)
    {
        gPmicGlinkRxInProgress = 0;
        return STATUS_UNSUCCESSFUL;
    }

    waitObjects[0] = &gPmicGlinkConnectedEvent;
    waitObjects[1] = &gPmicGlinkLocalDisconnectedEvent;
    waitObjects[2] = &gPmicGlinkRemoteDisconnectedEvent;
    waitObjects[3] = &gPmicGlinkTxNotificationEvent;
    waitObjects[4] = &gPmicGlinkRxIntentReqEvent;
    waitObjects[5] = &gPmicGlinkRxNotificationEvent;
    waitObjects[6] = &gPmicGlinkRxIntentNotificationEvent;
    waitObjectCount = RTL_NUMBER_OF(waitObjects);

    matchedResponse = FALSE;
    sawTxNotification = FALSE;
    waitCount = 0;
    pollInterval.QuadPart = -200000ll;
    while (waitCount < 5u)
    {
        waitStatus = KeWaitForMultipleObjects(
            waitObjectCount,
            waitObjects,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            &pollInterval,
            waitBlocks);
        if (waitStatus < (STATUS_WAIT_0 + waitObjectCount))
        {
            waitIndex = waitStatus - STATUS_WAIT_0;
            (VOID)KeClearEvent((PKEVENT)waitObjects[waitIndex]);
            if (waitIndex == 3u)
            {
                sawTxNotification = TRUE;
                break;
            }

            if (waitIndex != 5u)
            {
                waitCount++;
                continue;
            }

            expectedReceived = FALSE;
            status = PmicGlink_RetrieveRxData(Context, OpCode, &expectedReceived);
            if (expectedReceived)
            {
                matchedResponse = TRUE;
                gPmicGlinkRxInProgress = 0;
                break;
            }
        }

        waitCount++;
    }

    if (!NT_SUCCESS(status))
    {
        gPmicGlinkRxInProgress = 0;
        return status;
    }

    if (!matchedResponse && !sawTxNotification)
    {
        gPmicGlinkRxInProgress = 0;
        return STATUS_TIMEOUT;
    }

    if (!WaitForRx)
    {
        gPmicGlinkRxInProgress = 0;
        return status;
    }

    if (!matchedResponse && WaitForRx)
    {
        waitCount = 0;
        while (waitCount < 140u)
        {
            waitStatus = KeWaitForMultipleObjects(
                waitObjectCount,
                waitObjects,
                WaitAny,
                Executive,
                KernelMode,
                FALSE,
                &pollInterval,
                waitBlocks);
            if (waitStatus < (STATUS_WAIT_0 + waitObjectCount))
            {
                waitIndex = waitStatus - STATUS_WAIT_0;
                (VOID)KeClearEvent((PKEVENT)waitObjects[waitIndex]);
                if (waitIndex != 5u)
                {
                    waitCount++;
                    continue;
                }

                expectedReceived = FALSE;
                status = PmicGlink_RetrieveRxData(Context, OpCode, &expectedReceived);
                if (expectedReceived)
                {
                    matchedResponse = TRUE;
                    gPmicGlinkRxInProgress = 0;
                    break;
                }
            }

            waitCount++;
        }
    }

    if (!matchedResponse && WaitForRx && NT_SUCCESS(status))
    {
        status = STATUS_TIMEOUT;
    }

    gPmicGlinkRxInProgress = 0;
    return status;
}

static NTSTATUS
PmicGlink_SyncSendReceive(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PUCHAR InputBuffer,
    _In_ SIZE_T InputBufferSize
    )
{
    ULONG effectiveIoctl;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    effectiveIoctl = PmicGlinkNormalizeLegacyIoctl(IoControlCode);
    switch (effectiveIoctl)
    {
    case IOCTL_PMICGLINK_ABD_UCSI_READ_LEGACY:
        effectiveIoctl = IOCTL_PMICGLINK_UCSI_READ;
        break;

    case IOCTL_PMICGLINK_ABD_USBC_READ_LEGACY:
        effectiveIoctl = IOCTL_PMICGLINK_PLATFORM_USBC_READ;
        break;

    case IOCTL_PMICGLINK_ABD_GET_OEM_MSG_LEGACY:
        effectiveIoctl = IOCTL_PMICGLINK_GET_OEM_MSG;
        break;

    case IOCTL_PMICGLINK_ABD_USBC_NOTIFY_LEGACY:
        effectiveIoctl = IOCTL_PMICGLINK_PLATFORM_USBC_NOTIFY;
        break;

    default:
        break;
    }

    switch (effectiveIoctl)
    {
    case IOCTL_PMICGLINK_UCSI_WRITE:
    case IOCTL_PMICGLINK_ABD_UCSI_WRITE_LEGACY:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            UCHAR WriteBuffer[PMICGLINK_UCSI_BUFFER_SIZE];
        } ucsiWriteRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        ucsiWriteRequest.Header = 0x10000800Bull;
        ucsiWriteRequest.MessageOp = 18u;
        RtlZeroMemory(ucsiWriteRequest.WriteBuffer, sizeof(ucsiWriteRequest.WriteBuffer));
        RtlCopyMemory(ucsiWriteRequest.WriteBuffer, InputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);

        RtlCopyMemory(Context->UCSIDataBuffer, InputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
        return PmicGlink_SendData(Context, 18u, &ucsiWriteRequest, sizeof(ucsiWriteRequest), TRUE);
    }

    case IOCTL_PMICGLINK_UCSI_READ:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
        } ucsiReadRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        ucsiReadRequest.Header = 0x10000800Bull;
        ucsiReadRequest.MessageOp = 17u;
        return PmicGlink_SendData(Context, 17u, &ucsiReadRequest, sizeof(ucsiReadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_PLATFORM_USBC_READ:
    {
        UCHAR usbcReadRequest[12];
        const ULONGLONG header = 0x10000800Aull;
        const ULONG messageOp = 34u;

        RtlZeroMemory(usbcReadRequest, sizeof(usbcReadRequest));
        RtlCopyMemory(usbcReadRequest, &header, sizeof(header));
        RtlCopyMemory(usbcReadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        return PmicGlink_SendData(Context, 34u, usbcReadRequest, sizeof(usbcReadRequest), FALSE);
    }

    case IOCTL_PMICGLINK_PLATFORM_USBC_NOTIFY:
    {
        const PMICGLINK_OEM_SET_PROP_INPUT* oemSetRequest;
        UCHAR oemSetPropRequest[276];
        const ULONGLONG header = 0x10000800Eull;
        const ULONG messageOp = 258u;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        oemSetRequest = (const PMICGLINK_OEM_SET_PROP_INPUT*)InputBuffer;

        RtlZeroMemory(oemSetPropRequest, sizeof(oemSetPropRequest));
        RtlCopyMemory(oemSetPropRequest, &header, sizeof(header));
        RtlCopyMemory(oemSetPropRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(oemSetPropRequest + 12, &oemSetRequest->property_id, sizeof(oemSetRequest->property_id));
        RtlCopyMemory(oemSetPropRequest + 16, &oemSetRequest->data_size, sizeof(oemSetRequest->data_size));
        RtlCopyMemory(oemSetPropRequest + 20, oemSetRequest->data, sizeof(oemSetRequest->data));

        return PmicGlink_SendData(Context, 258u, oemSetPropRequest, sizeof(oemSetPropRequest), TRUE);
    }

    case IOCTL_PMICGLINK_GET_OEM_MSG:
    {
        const PMICGLINK_OEM_GET_PROP_INPUT* request;
        UCHAR oemGetPropRequest[20];
        const ULONGLONG header = 0x10000800Eull;
        const ULONG messageOp = 257u;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const PMICGLINK_OEM_GET_PROP_INPUT*)InputBuffer;
        Context->OemPropData[0] = request->property_id;
        Context->OemPropData[1] = request->data_size;

        RtlZeroMemory(oemGetPropRequest, sizeof(oemGetPropRequest));
        RtlCopyMemory(oemGetPropRequest, &header, sizeof(header));
        RtlCopyMemory(oemGetPropRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(oemGetPropRequest + 12, &request->property_id, sizeof(request->property_id));
        RtlCopyMemory(oemGetPropRequest + 16, &request->data_size, sizeof(request->data_size));

        return PmicGlink_SendData(Context, 257u, oemGetPropRequest, sizeof(oemGetPropRequest), TRUE);
    }

    case IOCTL_PMICGLINK_OEM_READ_BUFFER:
    case IOCTL_PMICGLINK_OEM_WRITE_BUFFER:
    {
        UCHAR oemSendReceiveRequest[84];
        const ULONGLONG header = 0x10000800Eull;
        const ULONG messageOp = 260u;
        ULONG dataSize;
        SIZE_T copyLength;

        if ((InputBuffer == NULL) || (InputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (InputBufferSize > PMICGLINK_OEM_SEND_BUFFER_SIZE)
        {
            return STATUS_BUFFER_OVERFLOW;
        }

        dataSize = (ULONG)InputBufferSize;
        copyLength = (SIZE_T)dataSize;

        RtlZeroMemory(oemSendReceiveRequest, sizeof(oemSendReceiveRequest));
        RtlCopyMemory(oemSendReceiveRequest, &header, sizeof(header));
        RtlCopyMemory(oemSendReceiveRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(oemSendReceiveRequest + 12, &dataSize, sizeof(dataSize));
        RtlCopyMemory(oemSendReceiveRequest + 16, InputBuffer, copyLength);

        return PmicGlink_SendData(Context, 260u, oemSendReceiveRequest, sizeof(oemSendReceiveRequest), TRUE);
    }

    case IOCTL_PMICGLINK_GET_CHARGER_PORTS:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
        } chargerPortsRequest;

        chargerPortsRequest.Header = 0x10000800Aull;
        chargerPortsRequest.MessageOp = 73u;
        return PmicGlink_SendData(Context, 73u, &chargerPortsRequest, sizeof(chargerPortsRequest), TRUE);
    }

    case IOCTL_PMICGLINK_GET_USB_CHG_STATUS:
    {
        UCHAR chargerPowerRequest[12];
        const ULONGLONG header = 0x10000800Aull;
        const ULONG messageOp = 74u;

        RtlZeroMemory(chargerPowerRequest, sizeof(chargerPowerRequest));
        RtlCopyMemory(chargerPowerRequest, &header, sizeof(header));
        RtlCopyMemory(chargerPowerRequest + sizeof(header), &messageOp, sizeof(messageOp));
        return PmicGlink_SendData(Context, 74u, chargerPowerRequest, sizeof(chargerPowerRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_GWS:
    {
        UCHAR tadRequest[16];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 96u;
        ULONG timerId;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        timerId = ((const PMICGLINK_TAD_GWS_INBUF*)InputBuffer)->TimerId;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &timerId, sizeof(timerId));
        return PmicGlink_SendData(Context, 96u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_CWS:
    {
        UCHAR tadRequest[16];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 97u;
        ULONG timerId;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        timerId = ((const PMICGLINK_TAD_CWS_INBUF*)InputBuffer)->TimerId;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &timerId, sizeof(timerId));
        return PmicGlink_SendData(Context, 97u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_STP:
    {
        UCHAR tadRequest[20];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 98u;
        const PMICGLINK_TAD_STP_INBUF* request;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const PMICGLINK_TAD_STP_INBUF*)InputBuffer;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &request->TimerId, sizeof(request->TimerId));
        RtlCopyMemory(tadRequest + 16, &request->PolicyValue, sizeof(request->PolicyValue));
        return PmicGlink_SendData(Context, 98u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_STV:
    {
        UCHAR tadRequest[20];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 99u;
        const PMICGLINK_TAD_STV_INBUF* request;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const PMICGLINK_TAD_STV_INBUF*)InputBuffer;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &request->TimerId, sizeof(request->TimerId));
        RtlCopyMemory(tadRequest + 16, &request->TimerValue, sizeof(request->TimerValue));
        return PmicGlink_SendData(Context, 99u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_TIP:
    {
        UCHAR tadRequest[16];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 100u;
        ULONG timerId;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        timerId = ((const PMICGLINK_TAD_TIP_INBUF*)InputBuffer)->TimerId;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &timerId, sizeof(timerId));
        return PmicGlink_SendData(Context, 100u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_TAD_TIV:
    {
        UCHAR tadRequest[16];
        const ULONGLONG header = 0x100008010ull;
        const ULONG messageOp = 101u;
        ULONG timerId;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        timerId = ((const PMICGLINK_TAD_TIV_INBUF*)InputBuffer)->TimerId;
        RtlZeroMemory(tadRequest, sizeof(tadRequest));
        RtlCopyMemory(tadRequest, &header, sizeof(header));
        RtlCopyMemory(tadRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(tadRequest + 12, &timerId, sizeof(timerId));
        return PmicGlink_SendData(Context, 101u, tadRequest, sizeof(tadRequest), TRUE);
    }

    case IOCTL_PMICGLINK_I2C_WRITE:
    case IOCTL_PMICGLINK_I2C_READ:
    {
        UCHAR i2cRequest[100];
        const ULONGLONG header = 0x10000800Eull;
        const ULONG messageOp = 79u;
        ULONG busId;
        ULONG transferType;
        ULONG readWriteFlag;
        ULONG deviceAddress;
        ULONG registerOffset;
        ULONG transferLength;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        busId = (ULONG)InputBuffer[0];
        transferType = (ULONG)((InputBuffer[1] >> 1) & 0x7Bu);
        readWriteFlag = (ULONG)(InputBuffer[1] & 0x1u);
        deviceAddress = (ULONG)InputBuffer[2] | ((ULONG)InputBuffer[3] << 8);
        registerOffset = (ULONG)InputBuffer[4];
        transferLength = (ULONG)InputBuffer[5];

        if ((deviceAddress == 0u) || (registerOffset == 0u) || (transferLength == 0u))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(i2cRequest, sizeof(i2cRequest));
        RtlCopyMemory(i2cRequest, &header, sizeof(header));
        RtlCopyMemory(i2cRequest + sizeof(header), &messageOp, sizeof(messageOp));
        RtlCopyMemory(i2cRequest + 12, &busId, sizeof(busId));
        RtlCopyMemory(i2cRequest + 16, &transferType, sizeof(transferType));
        RtlCopyMemory(i2cRequest + 20, &readWriteFlag, sizeof(readWriteFlag));
        RtlCopyMemory(i2cRequest + 24, &deviceAddress, sizeof(deviceAddress));
        RtlCopyMemory(i2cRequest + 28, &registerOffset, sizeof(registerOffset));
        RtlCopyMemory(i2cRequest + 32, &transferLength, sizeof(transferLength));

        if (readWriteFlag == 0u)
        {
            SIZE_T safeCopyLength;

            safeCopyLength = (transferLength <= PMICGLINK_I2C_DATA_SIZE)
                ? (SIZE_T)transferLength
                : PMICGLINK_I2C_DATA_SIZE;
            RtlCopyMemory(i2cRequest + 36, InputBuffer + PMICGLINK_I2C_HEADER_SIZE, safeCopyLength);
        }

        return PmicGlink_SendData(Context, 79u, i2cRequest, sizeof(i2cRequest), TRUE);
    }

    case IOCTL_BATTMNGR_GET_BATT_ID:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
        } battIdRequest;

        battIdRequest.Header = 0x10000800Aull;
        battIdRequest.MessageOp = 0u;
        return PmicGlink_SendData(Context, 0u, &battIdRequest, sizeof(battIdRequest), TRUE);
    }

    case IOCTL_BATTMNGR_GET_CHARGER_STATUS:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG BatteryId;
        } chgStatusRequest;

        chgStatusRequest.Header = 0x10000800Aull;
        chgStatusRequest.MessageOp = 1u;
        chgStatusRequest.BatteryId = 0u;
        return PmicGlink_SendData(Context, 1u, &chgStatusRequest, sizeof(chgStatusRequest), TRUE);
    }

    case IOCTL_BATTMNGR_GET_BATT_INFO:
    {
        const BATT_MNGR_GET_BATT_INFO* battInfoIn;
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG BatteryId;
            ULONG RateOfDrain;
        } battInfoRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        battInfoIn = (const BATT_MNGR_GET_BATT_INFO*)InputBuffer;
        battInfoRequest.Header = 0x10000800Aull;
        battInfoRequest.MessageOp = 9u;
        battInfoRequest.BatteryId = battInfoIn->batt_id;
        battInfoRequest.RateOfDrain = battInfoIn->rate_of_drain;
        return PmicGlink_SendData(Context, 9u, &battInfoRequest, sizeof(battInfoRequest), TRUE);
    }

    case IOCTL_BATTMNGR_SET_STATUS_CRITERIA:
    {
        const BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA* request;
        NTSTATUS sendStatus;
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG HighCapacity;
            ULONG LowCapacity;
            ULONG PowerState;
        } statusCriteriaRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA*)InputBuffer;
        if (!Context->Notify
            || (Context->LegacyStatusCriteria.batt_notify_criteria.power_state
                != request->batt_notify_criteria.power_state)
            || (Context->LegacyStatusCriteria.batt_notify_criteria.low_capacity
                != request->batt_notify_criteria.low_capacity)
            || (Context->LegacyStatusCriteria.batt_notify_criteria.high_capacity
                != request->batt_notify_criteria.high_capacity))
        {
            statusCriteriaRequest.Header = 0x10000800Aull;
            statusCriteriaRequest.MessageOp = 4u;
            statusCriteriaRequest.HighCapacity = request->batt_notify_criteria.high_capacity;
            statusCriteriaRequest.LowCapacity = request->batt_notify_criteria.low_capacity;
            statusCriteriaRequest.PowerState = request->batt_notify_criteria.power_state;
            sendStatus = PmicGlink_SendData(Context, 4u, &statusCriteriaRequest, sizeof(statusCriteriaRequest), TRUE);
            if (NT_SUCCESS(sendStatus))
            {
                Context->LegacyStatusCriteria = *request;
                Context->Notify = TRUE;
            }
            return sendStatus;
        }

        return STATUS_SUCCESS;
    }

    case IOCTL_BATTMNGR_GET_BATT_PRESENT:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
        } battPresentRequest;

        battPresentRequest.Header = 0x10000800Aull;
        battPresentRequest.MessageOp = 5u;
        return PmicGlink_SendData(Context, 5u, &battPresentRequest, sizeof(battPresentRequest), TRUE);
    }

    case IOCTL_BATTMNGR_SET_OPERATION_MODE:
    {
        const BATT_MNGR_SET_OPERATIONAL_MODE* modeRequest;
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG OperationalMode;
        } setModeRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        modeRequest = (const BATT_MNGR_SET_OPERATIONAL_MODE*)InputBuffer;
        RtlCopyMemory(
            &Context->LegacyOperationalMode,
            InputBuffer,
            sizeof(BATT_MNGR_SET_OPERATIONAL_MODE));

        setModeRequest.Header = 0x10000800Aull;
        setModeRequest.MessageOp = 3u;
        setModeRequest.OperationalMode = modeRequest->mode_type;
        return PmicGlink_SendData(Context, 3u, &setModeRequest, sizeof(setModeRequest), TRUE);
    }

    case IOCTL_BATTMNGR_SET_CHARGE_RATE:
    {
        const BATT_MNGR_SET_CHARGE_RATE* chargeRateRequest;
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG BatteryId;
            ULONG ChargeRate;
            ULONG ChargingPath;
        } setChargeRateRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        chargeRateRequest = (const BATT_MNGR_SET_CHARGE_RATE*)InputBuffer;
        RtlCopyMemory(
            &Context->LegacyChargeRate,
            InputBuffer,
            sizeof(BATT_MNGR_SET_CHARGE_RATE));

        setChargeRateRequest.Header = 0x10000800Aull;
        setChargeRateRequest.MessageOp = 6u;
        setChargeRateRequest.BatteryId = 0u;
        setChargeRateRequest.ChargeRate = chargeRateRequest->charge_perc;
        setChargeRateRequest.ChargingPath = chargeRateRequest->device_name;
        return PmicGlink_SendData(Context, 6u, &setChargeRateRequest, sizeof(setChargeRateRequest), TRUE);
    }

    case IOCTL_BATTMNGR_GET_TEST_INFO:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG BatteryId;
            ULONG RequestType;
            ULONG RequestBufferLength;
            ULONG RequestBuffer[64];
        } genericTestInfoRequest;

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(&genericTestInfoRequest, sizeof(genericTestInfoRequest));
        genericTestInfoRequest.Header = 0x10000800Aull;
        genericTestInfoRequest.MessageOp = 32u;
        genericTestInfoRequest.BatteryId = 0u;
        genericTestInfoRequest.RequestType = *(ULONG*)InputBuffer;
        genericTestInfoRequest.RequestBufferLength = 0u;
        Context->LegacyLastTestInfoRequestType = genericTestInfoRequest.RequestType;
        return PmicGlink_SendData(Context, 32u, &genericTestInfoRequest, sizeof(genericTestInfoRequest), TRUE);
    }

    case IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT:
    {
        struct
        {
            ULONGLONG Header;
            ULONG MessageOp;
            ULONG Enable;
            ULONG TargetSoc;
            ULONG DeltaSoc;
        } chargeLimitRequest;

        if (!Context->GlinkChannelConnected)
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (InputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        chargeLimitRequest.Header = 0x10000800Aull;
        chargeLimitRequest.MessageOp = 72u;
        chargeLimitRequest.Enable = ((ULONG*)InputBuffer)[0];
        chargeLimitRequest.TargetSoc = ((ULONG*)InputBuffer)[1];
        chargeLimitRequest.DeltaSoc = ((ULONG*)InputBuffer)[2];
        return PmicGlink_SendData(Context, 72u, &chargeLimitRequest, sizeof(chargeLimitRequest), TRUE);
    }

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS
PmicGlinkPlatformQcmb_WriteMBToBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_writes_bytes_(BufferSize) PUCHAR Buffer,
    _In_ SIZE_T BufferSize
    )
{
    if ((Context == NULL) || (Buffer == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferSize < 64u)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(Buffer, BufferSize);
    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    ((ULONG*)Buffer)[0] = Context->QcmbStatus;
    ((ULONG*)Buffer)[1] = Context->QcmbCurrentChargerPowerUW;
    ((ULONG*)Buffer)[2] = Context->QcmbGoodChargerThresholdUW;
    ((ULONG*)Buffer)[3] = Context->QcmbChargerStatusInfo;

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkPlatformQcmb_GetStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ PULONG QcmbStatus
    )
{
    NTSTATUS status;
    struct
    {
        ULONGLONG Header;
        ULONG MessageOp;
    } requestMessage;

    if (Context == NULL)
    {
        return (NTSTATUS)0xC00000EFL;
    }

    if (QcmbStatus == NULL)
    {
        return (NTSTATUS)0xC00000F0L;
    }

    *QcmbStatus = 0;

    requestMessage.Header = 0x100008010ull;
    requestMessage.MessageOp = 128u;

    status = PmicGlink_SendData(Context, 0x80u, &requestMessage, sizeof(requestMessage), TRUE);
    if (NT_SUCCESS(status))
    {
        if (Context->StateLock != NULL)
        {
            WdfSpinLockAcquire(Context->StateLock);
        }

        *QcmbStatus = Context->QcmbStatus;

        if (Context->StateLock != NULL)
        {
            WdfSpinLockRelease(Context->StateLock);
        }
    }

    return status;
}

static VOID
PmicGlinkPlatformQcmb_SetStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG QcmbStatus
    )
{
    if (Context == NULL)
    {
        return;
    }

    if (Context->StateLock != NULL)
    {
        WdfSpinLockAcquire(Context->StateLock);
    }

    Context->QcmbStatus = QcmbStatus;

    if (Context->StateLock != NULL)
    {
        WdfSpinLockRelease(Context->StateLock);
    }
}

static NTSTATUS
PmicGlinkPlatformQcmb_SendMailboxCommand(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG QcmbStatus,
    _In_ ULONG WaitInMs,
    _Out_opt_ PULONG ObservedStatus
    )
{
    NTSTATUS status;
    UCHAR qcmbMessage[64];

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (ObservedStatus != NULL)
    {
        *ObservedStatus = 0u;
    }

    PmicGlinkPlatformQcmb_SetStatus(Context, QcmbStatus);

    status = PmicGlinkPlatformQcmb_WriteMBToBuffer(Context, qcmbMessage, sizeof(qcmbMessage));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PmicGlink_SendData(Context, 0x81u, qcmbMessage, sizeof(qcmbMessage), TRUE);
    if (NT_SUCCESS(status) && (WaitInMs > 0u))
    {
        PmicGlinkPlatformQcmb_WaitCmdStatus(Context, WaitInMs);
    }

    if (ObservedStatus != NULL)
    {
        NTSTATUS statusRead;
        ULONG qcmbStatusRead;

        qcmbStatusRead = 0u;
        statusRead = PmicGlinkPlatformQcmb_GetStatus(Context, &qcmbStatusRead);
        if (NT_SUCCESS(statusRead))
        {
            *ObservedStatus = qcmbStatusRead;
        }
    }

    return status;
}

static VOID
PmicGlinkPlatformQcmb_WaitCmdStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG WaitInMs
    )
{
    NTSTATUS status;
    ULONG qcmbStatus;

    if (Context == NULL)
    {
        return;
    }

    status = PmicGlinkPlatformQcmb_GetStatus(Context, &qcmbStatus);
    if (NT_SUCCESS(status) && ((qcmbStatus & 0xEu) == 0x2u) && (WaitInMs > 0))
    {
        LARGE_INTEGER timeout;

        timeout.QuadPart = -10000ll * (LONGLONG)WaitInMs;
        (VOID)KeWaitForSingleObject(
            &Context->QcmbNotifyEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeout);
        KeClearEvent(&Context->QcmbNotifyEvent);

        status = PmicGlinkPlatformQcmb_GetStatus(Context, &qcmbStatus);
    }

}

static NTSTATUS
PmicGlinkPlatformQcmb_PreShutdown_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG CmdBitMask
    )
{
    NTSTATUS status;
    ULONG qcmbStatus;
    ULONG commandStatus;
    ULONG secondaryStatus;

    if (Context == NULL)
    {
        return (NTSTATUS)0xC00000EFL;
    }

    if (!Context->QcmbConnected)
    {
        return STATUS_SUCCESS;
    }

    commandStatus = (Context->QcmbConnected ? 1u : 0u) | 0x2u;
    status = PmicGlinkPlatformQcmb_SendMailboxCommand(
        Context,
        commandStatus,
        100u,
        &qcmbStatus);
    UNREFERENCED_PARAMETER(qcmbStatus);

    if ((CmdBitMask & 0xFFu) < 4u)
    {
        secondaryStatus = (Context->QcmbConnected ? 1u : 0u) | 0x4u;
        (VOID)PmicGlinkPlatformQcmb_SendMailboxCommand(Context, secondaryStatus, 0u, NULL);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkPlatformQcmb_GetChargerInfo_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA* ChargerInfo
    )
{
    NTSTATUS status;
    ULONG qcmbStatus;
    ULONG commandStatus;

    if (Context == NULL)
    {
        return (NTSTATUS)0xC00000EFL;
    }

    if (ChargerInfo == NULL)
    {
        return (NTSTATUS)0xC00000F0L;
    }

    RtlZeroMemory(ChargerInfo, sizeof(*ChargerInfo));

    if (!Context->QcmbConnected)
    {
        return STATUS_SUCCESS;
    }

    commandStatus = (Context->QcmbConnected ? 1u : 0u) | 0x2u;
    (VOID)PmicGlinkPlatformQcmb_SendMailboxCommand(Context, commandStatus, 100u, NULL);

    status = PmicGlinkPlatformQcmb_GetStatus(Context, &qcmbStatus);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((qcmbStatus & 0xEu) == 0x4u)
    {
        WdfSpinLockAcquire(Context->StateLock);
        ChargerInfo->currentChargerPowerUW = Context->QcmbCurrentChargerPowerUW;
        ChargerInfo->goodChargerThresholdUW = Context->QcmbGoodChargerThresholdUW;
        ChargerInfo->chargerStatusInfo.AsUINT32 = Context->QcmbChargerStatusInfo;
        WdfSpinLockRelease(Context->StateLock);
    }

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkPower_ModernStandby_Callback(
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PEP_Modern_Standby_Notif_Struct* standbyNotification;

    UNREFERENCED_PARAMETER(Argument2);

    context = (PPMIC_GLINK_DEVICE_CONTEXT)CallbackContext;
    standbyNotification = (PEP_Modern_Standby_Notif_Struct*)Argument1;

    if (context->ModernStandbyState != standbyNotification->ModernStandbyState)
    {
        context->ModernStandbyState = standbyNotification->ModernStandbyState;
        (VOID)PmicGlinkPlatformQcmb_PreShutdown_Cmd(
            context,
            context->ModernStandbyState ? 2u : 1u);
    }
}

static NTSTATUS
PmicGlinkRegistryQuery(
    _In_ WDFKEY RegKey,
    _In_ PCUNICODE_STRING RegName,
    _Out_ PULONG ReadData
    )
{
    ULONG valueLength;
    ULONG valueType;

    valueLength = 0;
    valueType = 0;
    return WdfRegistryQueryValue(
        RegKey,
        RegName,
        sizeof(ULONG),
        ReadData,
        &valueLength,
        &valueType);
}

static NTSTATUS
PmicGlinkNotify_PingBattMiniClass(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 1);
    return STATUS_SUCCESS;
}

static VOID
PmicGlinkNotifyBattMiniStatusFromGlink(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotificationData
    )
{
    SIZE_T notifyBytesReturned;
    NTSTATUS notifyStatus;

    PmicGlinkNotify_PingBattMiniClass(Context);
    if (InterlockedCompareExchange(&gPmicGlinkNotifyGo, 1, 1) == 0)
    {
        return;
    }

    WdfWaitLockAcquire(Context->BattMiniNotifyLock, NULL);

    if (Context->BattMiniDeviceLoaded)
    {
        notifyBytesReturned = 0u;
        notifyStatus = PmicGlinkSendDriverRequestWithTimeout(
            Context->BattMiniIoTarget,
            PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE,
            NULL,
            0u,
            NULL,
            0u,
            PMICGLINK_BATTMINI_NOTIFY_TIMEOUT_100NS,
            &notifyBytesReturned);
        (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
        if (NT_SUCCESS(notifyStatus))
        {
            Context->LegacyStatusNotificationPending = TRUE;
        }

        if ((NotificationData & 0xFFu) == 0x83u)
        {
            ULONG battMiniNotifyArgument;

            battMiniNotifyArgument = (NotificationData >> 8);
            notifyBytesReturned = 0u;
            notifyStatus = PmicGlinkSendDriverRequestWithTimeout(
                Context->BattMiniIoTarget,
                PMICGLINK_BATTMINI_IOCTL_NOTIFY_STATUS,
                &battMiniNotifyArgument,
                sizeof(battMiniNotifyArgument),
                NULL,
                0u,
                PMICGLINK_BATTMINI_NOTIFY_TIMEOUT_100NS,
                &notifyBytesReturned);
            (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
            if (NT_SUCCESS(notifyStatus))
            {
                Context->LegacyStatusNotificationPending = TRUE;
            }
        }
    }

    WdfWaitLockRelease(Context->BattMiniNotifyLock);
}

static VOID
PmicGlinkInitAbdConnections(
    VOID
    )
{
    RtlZeroMemory(gPmicGlinkAbdConnections, sizeof(gPmicGlinkAbdConnections));
    gPmicGlinkAbdConnectionMax = RTL_NUMBER_OF(gPmicGlinkAbdConnections);

    gPmicGlinkAbdConnections[0].InterfaceClassGuid = GUID_DEVINTERFACE_PMICGLINK;
    gPmicGlinkAbdConnections[0].ConnectionType = 3;
    gPmicGlinkAbdConnections[0].PrimaryIoctl = IOCTL_PMICGLINK_ABD_UCSI_READ_LEGACY;
    gPmicGlinkAbdConnections[0].SecondaryIoctl = IOCTL_PMICGLINK_ABD_UCSI_WRITE_LEGACY;
    gPmicGlinkAbdConnections[0].MessageOpcode = 56;

    gPmicGlinkAbdConnections[1].InterfaceClassGuid = GUID_DEVINTERFACE_BATT_MNGR;
    gPmicGlinkAbdConnections[1].ConnectionType = 4;
    gPmicGlinkAbdConnections[1].PrimaryIoctl = IOCTL_PMICGLINK_OEM_READ_BUFFER;
    gPmicGlinkAbdConnections[1].SecondaryIoctl = IOCTL_PMICGLINK_OEM_WRITE_BUFFER;
    gPmicGlinkAbdConnections[1].MessageOpcode = 74;

    gPmicGlinkAbdConnections[2].InterfaceClassGuid = GUID_DEVINTERFACE_PMICGLINK;
    gPmicGlinkAbdConnections[2].ConnectionType = 6;
    gPmicGlinkAbdConnections[2].PrimaryIoctl = IOCTL_PMICGLINK_I2C_READ;
    gPmicGlinkAbdConnections[2].SecondaryIoctl = IOCTL_PMICGLINK_I2C_WRITE;
    gPmicGlinkAbdConnections[2].MessageOpcode = 78;

    gPmicGlinkAbdConnections[3].InterfaceClassGuid = GUID_DEVINTERFACE_PMICGLINK;
    gPmicGlinkAbdConnections[3].ConnectionType = 7;
    gPmicGlinkAbdConnections[3].PrimaryIoctl = IOCTL_PMICGLINK_QCMB_GET_CHARGER_INFO;
    gPmicGlinkAbdConnections[3].SecondaryIoctl = IOCTL_PMICGLINK_QCMB_GET_CHARGER_INFO;
    gPmicGlinkAbdConnections[3].MessageOpcode = 20;
}

static NTSTATUS
PmicGlinkEvaluateAbdConnectionCount(
    _In_ WDFDEVICE Device
    )
{
    NTSTATUS status;
    WDFIOTARGET ioTarget;
    ACPI_EVAL_INPUT_BUFFER inputBuffer;
    UCHAR outputStorage[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG))];
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    PACPI_METHOD_ARGUMENT outputArg;
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    ULONG_PTR bytesReturned;
    ULONG rawValue;
    ULONG connectionCount;

    if (Device == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    RtlZeroMemory(&inputBuffer, sizeof(inputBuffer));
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    RtlCopyMemory(inputBuffer.MethodName, "GEPT", sizeof(inputBuffer.MethodName));

    RtlZeroMemory(outputStorage, sizeof(outputStorage));
    bytesReturned = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, &inputBuffer, sizeof(inputBuffer));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, outputStorage, sizeof(outputStorage));

    status = WdfIoTargetSendInternalIoctlSynchronously(
        ioTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        &bytesReturned);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)outputStorage;
    if ((bytesReturned < sizeof(ACPI_EVAL_OUTPUT_BUFFER))
        || (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
        || (outputBuffer->Count == 0))
    {
        return STATUS_UNSUCCESSFUL;
    }

    outputArg = &outputBuffer->Argument[0];
    if ((outputArg->Type != ACPI_METHOD_ARGUMENT_INTEGER)
        || (outputArg->DataLength < sizeof(ULONG)))
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    rawValue = outputArg->Argument;
    connectionCount = (rawValue >> 16) & 0xFFu;
    gPmicGlinkAbdConnectionMax = (UCHAR)connectionCount;
    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkNotifyLinkStateAcpi(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG LinkState
    )
{
    NTSTATUS status;
    WDFIOTARGET ioTarget;
    ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER inputBuffer;
    UCHAR outputStorage[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG))];
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    ULONG_PTR bytesReturned;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ioTarget = WdfDeviceGetIoTarget(Context->Device);
    if (ioTarget == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    RtlZeroMemory(&inputBuffer, sizeof(inputBuffer));
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    RtlCopyMemory(inputBuffer.MethodName, "LKST", sizeof(inputBuffer.MethodName));
    inputBuffer.IntegerArgument = LinkState;

    RtlZeroMemory(outputStorage, sizeof(outputStorage));
    outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)outputStorage;
    outputBuffer->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;

    bytesReturned = 0;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, &inputBuffer, sizeof(inputBuffer));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, outputStorage, sizeof(outputStorage));

    status = WdfIoTargetSendInternalIoctlSynchronously(
        ioTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        &bytesReturned);

    return status;
}

static NTSTATUS
PmicGlinkEvaluateAcpiMethodInteger(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_(4) const CHAR MethodName[4],
    _In_ ULONG Value
    )
{
    NTSTATUS status;
    WDFIOTARGET ioTarget;
    ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER inputBuffer;
    UCHAR outputStorage[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG))];
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    ULONG_PTR bytesReturned;

    if ((Context == NULL) || (Context->Device == NULL) || (MethodName == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ioTarget = WdfDeviceGetIoTarget(Context->Device);
    if (ioTarget == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    RtlZeroMemory(&inputBuffer, sizeof(inputBuffer));
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    RtlCopyMemory(inputBuffer.MethodName, MethodName, 4u);
    inputBuffer.IntegerArgument = Value;

    RtlZeroMemory(outputStorage, sizeof(outputStorage));
    bytesReturned = 0;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, &inputBuffer, sizeof(inputBuffer));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, outputStorage, sizeof(outputStorage));

    status = WdfIoTargetSendInternalIoctlSynchronously(
        ioTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        &bytesReturned);

    return status;
}

static NTSTATUS
PmicGlinkEvaluateAcpiMethodBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_(4) const CHAR MethodName[4],
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize
    )
{
    NTSTATUS status;
    WDFIOTARGET ioTarget;
    SIZE_T inputSize;
    USHORT dataSize16;
    ACPI_EVAL_INPUT_BUFFER_COMPLEX* inputBuffer;
    PACPI_METHOD_ARGUMENT methodArg;
    UCHAR inputStorage[sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + ACPI_METHOD_ARGUMENT_LENGTH(PMICGLINK_OEM_BUFFER_SIZE)];
    UCHAR outputStorage[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG))];
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    ULONG_PTR bytesReturned;

    if ((Context == NULL)
        || (Context->Device == NULL)
        || (MethodName == NULL)
        || (Data == NULL)
        || (DataSize == 0u)
        || (DataSize > PMICGLINK_OEM_BUFFER_SIZE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ioTarget = WdfDeviceGetIoTarget(Context->Device);
    if (ioTarget == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + ACPI_METHOD_ARGUMENT_LENGTH(DataSize);
    dataSize16 = (USHORT)DataSize;
    RtlZeroMemory(inputStorage, sizeof(inputStorage));
    inputBuffer = (ACPI_EVAL_INPUT_BUFFER_COMPLEX*)inputStorage;
    inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    RtlCopyMemory(inputBuffer->MethodName, MethodName, 4u);
    inputBuffer->Size = (ULONG)inputSize;
    inputBuffer->ArgumentCount = 1u;

    methodArg = &inputBuffer->Argument[0];
    ACPI_METHOD_SET_ARGUMENT_BUFFER(methodArg, Data, dataSize16);

    RtlZeroMemory(outputStorage, sizeof(outputStorage));
    bytesReturned = 0;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, inputBuffer, (ULONG)inputSize);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, outputStorage, sizeof(outputStorage));

    status = WdfIoTargetSendInternalIoctlSynchronously(
        ioTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        &bytesReturned);

    return status;
}

static NTSTATUS
PmicGlinkSendDriverRequest(
    _In_ WDFIOTARGET IoTarget,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_opt_ SIZE_T* BytesReturned
    )
{
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    PWDF_MEMORY_DESCRIPTOR pInputDescriptor;
    PWDF_MEMORY_DESCRIPTOR pOutputDescriptor;

    if (IoTarget == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pInputDescriptor = NULL;
    if ((InputBuffer != NULL) && (InputBufferSize != 0))
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, InputBuffer, InputBufferSize);
        pInputDescriptor = &inputDescriptor;
    }

    pOutputDescriptor = NULL;
    if ((OutputBuffer != NULL) && (OutputBufferSize != 0))
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, OutputBuffer, OutputBufferSize);
        pOutputDescriptor = &outputDescriptor;
    }

    return WdfIoTargetSendIoctlSynchronously(
        IoTarget,
        NULL,
        IoControlCode,
        pInputDescriptor,
        pOutputDescriptor,
        NULL,
        (PULONG_PTR)BytesReturned);
}

static NTSTATUS
PmicGlinkSendDriverRequestWithTimeout(
    _In_ WDFIOTARGET IoTarget,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _In_ LONGLONG Timeout100ns,
    _Out_opt_ SIZE_T* BytesReturned
    )
{
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    PWDF_MEMORY_DESCRIPTOR pInputDescriptor;
    PWDF_MEMORY_DESCRIPTOR pOutputDescriptor;
    WDF_REQUEST_SEND_OPTIONS requestOptions;

    if (IoTarget == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pInputDescriptor = NULL;
    if ((InputBuffer != NULL) && (InputBufferSize != 0))
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, InputBuffer, InputBufferSize);
        pInputDescriptor = &inputDescriptor;
    }

    pOutputDescriptor = NULL;
    if ((OutputBuffer != NULL) && (OutputBufferSize != 0))
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, OutputBuffer, OutputBufferSize);
        pOutputDescriptor = &outputDescriptor;
    }

    WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    requestOptions.Timeout = Timeout100ns;

    return WdfIoTargetSendIoctlSynchronously(
        IoTarget,
        NULL,
        IoControlCode,
        pInputDescriptor,
        pOutputDescriptor,
        &requestOptions,
        (PULONG_PTR)BytesReturned);
}

static NTSTATUS
PmicGlinkAbdUpdateConnections(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ BOOLEAN Register
    )
{
    NTSTATUS status;
    ULONG ioControlCode;
    ULONG inputBufferSize;
    ULONG connectionCount;
    ULONG index;

    if ((Context == NULL) || (Context->AbdIoTarget == NULL) || !Context->ABDAttached)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ioControlCode = Register
        ? PMICGLINK_ABD_IOCTL_REGISTER_CONNECTION
        : PMICGLINK_ABD_IOCTL_UNREGISTER_CONNECTION;
    inputBufferSize = Register
        ? (ULONG)sizeof(PMICGLINK_ABD_CONNECTION_ENTRY)
        : (ULONG)sizeof(GUID);
    connectionCount = gPmicGlinkAbdConnectionMax;
    if (connectionCount > RTL_NUMBER_OF(gPmicGlinkAbdConnections))
    {
        connectionCount = RTL_NUMBER_OF(gPmicGlinkAbdConnections);
    }

    status = STATUS_SUCCESS;
    for (index = 0; index < connectionCount; index++)
    {
        NTSTATUS requestStatus;
        SIZE_T bytesReturned;

        bytesReturned = 0;
        requestStatus = PmicGlinkSendDriverRequest(
            Context->AbdIoTarget,
            ioControlCode,
            &gPmicGlinkAbdConnections[index],
            inputBufferSize,
            NULL,
            0,
            &bytesReturned);
        status |= requestStatus;
    }

    return status;
}

static VOID
CrashDumpGuidToHexKey(
    _In_ const GUID* Guid,
    _Out_writes_(33) CHAR* Key
    )
{
    static const CHAR kHexDigits[] = "0123456789ABCDEF";
    const UCHAR* guidBytes;
    ULONG index;

    if (Key == NULL)
    {
        return;
    }

    RtlZeroMemory(Key, 33);
    if (Guid == NULL)
    {
        return;
    }

    guidBytes = (const UCHAR*)Guid;
    for (index = 0; index < 16; index++)
    {
        Key[index * 2] = kHexDigits[(guidBytes[index] >> 4) & 0x0Fu];
        Key[(index * 2) + 1] = kHexDigits[guidBytes[index] & 0x0Fu];
    }
}

static BOOLEAN
CrashDumpGuidIsZero(
    _In_ const GUID* Guid
    )
{
    GUID zeroGuid;

    if (Guid == NULL)
    {
        return TRUE;
    }

    RtlZeroMemory(&zeroGuid, sizeof(zeroGuid));
    return (RtlCompareMemory(Guid, &zeroGuid, sizeof(zeroGuid)) == sizeof(zeroGuid)) ? TRUE : FALSE;
}

static LONG
CrashDump_FileHandleSlotFind(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject,
    _In_ ULONG DataSourceMode
    )
{
    ULONG index;

    if ((Context == NULL)
        || (FileObject == NULL)
        || (DataSourceMode == 0)
        || (DataSourceMode >= RTL_NUMBER_OF(Context->CrashDumpDataSources[0].FileObject)))
    {
        return -1;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        if (Context->CrashDumpDataSources[index].FileObject[DataSourceMode] == FileObject)
        {
            return (LONG)index;
        }
    }

    return -1;
}

static LONG
CrashDump_FileHandleSlotFindForDestroy(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    ULONG index;

    if ((Context == NULL) || (FileObject == NULL))
    {
        return -1;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        if ((Context->CrashDumpDataSources[index].FileObject[1] == FileObject)
            || (Context->CrashDumpDataSources[index].FileObject[2] == FileObject))
        {
            return (LONG)index;
        }
    }

    return -1;
}

static LONG
CrashDump_FileHandleSlotFree(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    LONG slotIndex;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    slotIndex = CrashDump_FileHandleSlotFindForDestroy(Context, FileObject);
    if (slotIndex < 0)
    {
        return -1;
    }

    source = &Context->CrashDumpDataSources[(ULONG)slotIndex];
    if (source->FileObject[2] == FileObject)
    {
        source->FileObject[2] = NULL;
    }
    else if (source->FileObject[1] == FileObject)
    {
        source->FileObject[1] = NULL;
    }

    return slotIndex;
}

static VOID
CrashDump_DmfDestroy_RingBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    if ((Context == NULL) || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->RingBufferData != NULL)
    {
        ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        source->RingBufferData = NULL;
    }
    if (source->BugCheckSnapshotBuffer != NULL)
    {
        ExFreePoolWithTag(source->BugCheckSnapshotBuffer, PMICGLINK_POOLTAG_CRASHDUMP);
        source->BugCheckSnapshotBuffer = NULL;
    }

    RtlZeroMemory(&source->RingBufferGuid, sizeof(source->RingBufferGuid));
    RtlZeroMemory(source->RingBufferEncryptionKey, sizeof(source->RingBufferEncryptionKey));
    source->RingBufferEncryptionKeySize = 0;
    source->RingBufferSize = 0;
    source->RingBufferSizeOfEachEntry = 0;
    source->EntriesCount = 0;
    source->CurrentRingBufferIndex = 0;
    source->ValidEntryCount = 0;
    source->BugCheckBufferPointer = NULL;
    source->RingBufferEnumerationOffset = 0;
}

static NTSTATUS
CrashDump_DataSourceCreateInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _In_ ULONG ItemCount,
    _In_ ULONG SizeOfEachEntry,
    _In_ const GUID* Guid
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG totalBytes;

    if ((Context == NULL)
        || (Guid == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount)
        || (ItemCount == 0)
        || (SizeOfEachEntry == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    totalBytes = (ULONGLONG)ItemCount * (ULONGLONG)SizeOfEachEntry;
    if ((totalBytes == 0)
        || (totalBytes > PMICGLINK_CRASHDUMP_RINGBUFFER_MAX_BYTES)
        || (totalBytes > MAXULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->RingBufferData != NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    source->RingBufferData = (PUCHAR)ExAllocatePoolZero(
        NonPagedPoolNx,
        (SIZE_T)totalBytes,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (source->RingBufferData == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    source->BugCheckSnapshotBuffer = (PUCHAR)ExAllocatePoolZero(
        NonPagedPoolNx,
        (SIZE_T)totalBytes,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (source->BugCheckSnapshotBuffer == NULL)
    {
        ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        source->RingBufferData = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    source->EntriesCount = ItemCount;
    source->RingBufferSizeOfEachEntry = SizeOfEachEntry;
    source->RingBufferSize = (ULONG)totalBytes;
    source->CurrentRingBufferIndex = 0;
    source->ValidEntryCount = 0;
    source->BugCheckBufferPointer = NULL;
    source->RingBufferEnumerationOffset = 0;
    source->RingBufferGuid = *Guid;
    source->RingBufferEncryptionKeySize = 32;
    CrashDumpGuidToHexKey(Guid, source->RingBufferEncryptionKey);
    KeInitializeCallbackRecord(&source->BugCheckCallbackRecord);
    source->BugCheckCallbackRegistered = FALSE;
    if (!KeRegisterBugCheckReasonCallback(
        &source->BugCheckCallbackRecord,
        CrashDump_BugCheckSecondaryDumpDataCallbackRingBuffer,
        KbCallbackSecondaryDumpData,
        gCrashDumpBugCheckComponent))
    {
        CrashDump_DmfDestroy_RingBuffer(Context, DataSourceIndex);
        return STATUS_INVALID_PARAMETER;
    }

    source->BugCheckCallbackRegistered = TRUE;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceDestroyInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    if ((Context == NULL) || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->BugCheckCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&source->BugCheckCallbackRecord);
        source->BugCheckCallbackRegistered = FALSE;
    }

    CrashDump_DmfDestroy_RingBuffer(Context, DataSourceIndex);
    RtlZeroMemory(&source->BugCheckCallbackRecord, sizeof(source->BugCheckCallbackRecord));
    source->FileObject[1] = NULL;
    source->FileObject[2] = NULL;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceDestroyAuxiliaryInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    LONG slotIndex;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    slotIndex = CrashDump_FileHandleSlotFree(Context, FileObject);
    if (slotIndex < 0)
    {
        return STATUS_INVALID_HANDLE;
    }

    return CrashDump_DataSourceDestroyInternal(Context, (ULONG)slotIndex);
}

static NTSTATUS
CrashDump_DataSourceWriteInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _In_reads_bytes_(BufferLength) const UCHAR* Buffer,
    _In_ SIZE_T BufferLength
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    PUCHAR entry;
    ULONG writeIndex;
    SIZE_T bytesToCopy;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength > source->RingBufferSizeOfEachEntry)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    writeIndex = source->CurrentRingBufferIndex;
    entry = source->RingBufferData + ((SIZE_T)writeIndex * source->RingBufferSizeOfEachEntry);
    RtlZeroMemory(entry, source->RingBufferSizeOfEachEntry);

    bytesToCopy = (BufferLength < source->RingBufferSizeOfEachEntry)
        ? BufferLength
        : source->RingBufferSizeOfEachEntry;
    if (bytesToCopy > 0)
    {
        RtlCopyMemory(entry, Buffer, bytesToCopy);
    }

    source->CurrentRingBufferIndex = (writeIndex + 1u) % source->EntriesCount;
    if (source->ValidEntryCount < source->EntriesCount)
    {
        source->ValidEntryCount++;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceReadInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _Out_writes_bytes_(BufferLength) UCHAR* Buffer,
    _In_ SIZE_T BufferLength
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG readIndex;
    PUCHAR entry;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength < source->RingBufferSizeOfEachEntry)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (source->ValidEntryCount == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    readIndex = (source->CurrentRingBufferIndex + source->EntriesCount - source->ValidEntryCount) % source->EntriesCount;
    entry = source->RingBufferData + ((SIZE_T)readIndex * source->RingBufferSizeOfEachEntry);
    RtlCopyMemory(Buffer, entry, source->RingBufferSizeOfEachEntry);
    source->ValidEntryCount--;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceCaptureInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _Out_writes_bytes_(BufferLength) UCHAR* Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ PULONG BytesWritten
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG written;
    NTSTATUS status;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (BytesWritten == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength < source->RingBufferSize)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    written = 0;
    while ((source->ValidEntryCount > 0)
           && ((written + source->RingBufferSizeOfEachEntry) <= BufferLength))
    {
        status = CrashDump_DataSourceReadInternal(
            Context,
            DataSourceIndex,
            Buffer + written,
            BufferLength - written);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        written += source->RingBufferSizeOfEachEntry;
    }

    *BytesWritten = written;
    return STATUS_SUCCESS;
}

static PMICGLINK_CRASHDUMP_DATA_SOURCE*
CrashDump_FindActiveDataSource(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG index;

    if (Context == NULL)
    {
        return NULL;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if ((source->RingBufferData != NULL)
            && (source->RingBufferSizeOfEachEntry != 0)
            && (source->RingBufferSize != 0))
        {
            return source;
        }
    }

    return NULL;
}

static PMICGLINK_CRASHDUMP_DATA_SOURCE*
CrashDump_FindSourceFromCallbackRecord(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ PKBUGCHECK_REASON_CALLBACK_RECORD Record
    )
{
    ULONG index;

    if ((Context == NULL) || (Record == NULL))
    {
        return NULL;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if (&source->BugCheckCallbackRecord == Record)
        {
            return source;
        }
    }

    return NULL;
}

static NTSTATUS
CrashDump_InitializeTriageDataArray(
    _Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    UNICODE_STRING routineName;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((Context->CrashDumpTriageDataArray != NULL) && (Context->CrashDumpTriageDataArraySize != 0))
    {
        return STATUS_SUCCESS;
    }

    if (gKeInitializeTriageDumpDataArray == NULL)
    {
        RtlInitUnicodeString(&routineName, L"KeInitializeTriageDumpDataArray");
        gKeInitializeTriageDumpDataArray = MmGetSystemRoutineAddress(&routineName);
        if (gKeInitializeTriageDumpDataArray == NULL)
        {
            return STATUS_PROCEDURE_NOT_FOUND;
        }
    }

    Context->CrashDumpTriageDataArraySize = PMICGLINK_CRASHDUMP_TRIAGE_DATA_ARRAY_SIZE;
    Context->CrashDumpTriageDataArray = ExAllocatePoolZero(
        NonPagedPoolNx,
        Context->CrashDumpTriageDataArraySize,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (Context->CrashDumpTriageDataArray == NULL)
    {
        Context->CrashDumpTriageDataArraySize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ((PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY)gKeInitializeTriageDumpDataArray)(
        Context->CrashDumpTriageDataArray,
        Context->CrashDumpTriageDataArraySize);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
        Context->CrashDumpTriageDataArray = NULL;
        Context->CrashDumpTriageDataArraySize = 0;
        return status;
    }

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkLkmdTelInitializeMaxSizes(
    VOID
    )
{
    HANDLE regHandle;
    NTSTATUS status;
    ULONG reportSizeMb;
    ULONG resultLength;
    UNICODE_STRING keyPath;
    UNICODE_STRING valueName;
    OBJECT_ATTRIBUTES objectAttributes;
    UCHAR valueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;

    if (InterlockedCompareExchange(&gPmicGlinkLkmdTelMaxSizeInitialized, 1, 0) != 0)
    {
        return;
    }

    reportSizeMb = PMICGLINK_LKMDTEL_DEFAULT_MAX_REPORT_MB;
    regHandle = NULL;
    valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)valueBuffer;
    resultLength = 0;

    RtlInitUnicodeString(
        &keyPath,
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\LKMDTel");
    InitializeObjectAttributes(
        &objectAttributes,
        &keyPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    status = ZwOpenKey(&regHandle, KEY_READ, &objectAttributes);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&valueName, L"MaxReportSizeInMB");
        status = ZwQueryValueKey(
            regHandle,
            &valueName,
            KeyValuePartialInformation,
            valueInfo,
            sizeof(valueBuffer),
            &resultLength);
        if (NT_SUCCESS(status)
            && (valueInfo->Type == REG_DWORD)
            && (valueInfo->DataLength == sizeof(ULONG)))
        {
            reportSizeMb = *(ULONG*)valueInfo->Data;
        }

        ZwClose(regHandle);
    }

    if (reportSizeMb > PMICGLINK_LKMDTEL_DEFAULT_MAX_REPORT_MB)
    {
        if (reportSizeMb > PMICGLINK_LKMDTEL_MAX_REPORT_MB)
        {
            reportSizeMb = PMICGLINK_LKMDTEL_MAX_REPORT_MB;
        }

        gPmicGlinkLkmdTelMaxReportSize = (reportSizeMb << 20);
        if (gPmicGlinkLkmdTelMaxReportSize > PMICGLINK_LKMDTEL_SECONDARY_OVERHEAD_BYTES)
        {
            gPmicGlinkLkmdTelMaxSecondarySize =
                gPmicGlinkLkmdTelMaxReportSize - PMICGLINK_LKMDTEL_SECONDARY_OVERHEAD_BYTES;
        }
        else
        {
            gPmicGlinkLkmdTelMaxSecondarySize = 0;
        }
    }
}

static VOID
CrashDump_PopulateTriageDataArray(
    _Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG remainingSecondarySize;
    ULONG index;
    UNICODE_STRING routineName;
    PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY initializeTriageDataArray;
    PFN_KE_ADD_TRIAGE_DUMP_DATA_BLOCK addTriageDataBlock;

    if ((Context == NULL)
        || (Context->CrashDumpTriageDataArray == NULL)
        || (Context->CrashDumpTriageDataArraySize == 0))
    {
        return;
    }

    initializeTriageDataArray =
        (PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY)gKeInitializeTriageDumpDataArray;
    if (initializeTriageDataArray == NULL)
    {
        return;
    }

    (VOID)initializeTriageDataArray(
        Context->CrashDumpTriageDataArray,
        Context->CrashDumpTriageDataArraySize);

    if (gKeAddTriageDumpDataBlock == NULL)
    {
        RtlInitUnicodeString(&routineName, L"KeAddTriageDumpDataBlock");
        gKeAddTriageDumpDataBlock = MmGetSystemRoutineAddress(&routineName);
    }

    addTriageDataBlock = (PFN_KE_ADD_TRIAGE_DUMP_DATA_BLOCK)gKeAddTriageDumpDataBlock;
    if (addTriageDataBlock == NULL)
    {
        return;
    }

    PmicGlinkLkmdTelInitializeMaxSizes();
    remainingSecondarySize = gPmicGlinkLkmdTelMaxSecondarySize;

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        ULONG dataSize;
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
        PVOID dataBuffer;

        source = &Context->CrashDumpDataSources[index];
        if ((source->RingBufferData == NULL)
            || (source->RingBufferSize == 0)
            || (source->RingBufferSizeOfEachEntry == 0))
        {
            continue;
        }

        CrashDump_PrepareBugCheckSnapshot(source);
        dataBuffer = (source->BugCheckBufferPointer != NULL)
            ? (PVOID)source->BugCheckBufferPointer
            : (PVOID)source->RingBufferData;
        if (dataBuffer == NULL)
        {
            continue;
        }

        dataSize = source->RingBufferSize;
        if (dataSize > remainingSecondarySize)
        {
            dataSize = remainingSecondarySize;
        }
        if (dataSize == 0)
        {
            break;
        }

        (VOID)addTriageDataBlock(
            Context->CrashDumpTriageDataArray,
            dataBuffer,
            dataSize);

        if (remainingSecondarySize > dataSize)
        {
            remainingSecondarySize -= dataSize;
        }
        else
        {
            remainingSecondarySize = 0;
            break;
        }
    }
}

static NTSTATUS
CrashDump_RegisterGlobalCallbacks(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    BOOLEAN additionalRegisteredHere;
    NTSTATUS status;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    additionalRegisteredHere = FALSE;
    if (!Context->CrashDumpAdditionalCallbackRegistered)
    {
        KeInitializeCallbackRecord(&Context->CrashDumpAdditionalCallbackRecord);
        if (!KeRegisterBugCheckReasonCallback(
            &Context->CrashDumpAdditionalCallbackRecord,
            CrashDump_BugCheckSecondaryDumpDataCallbackAdditional,
            KbCallbackSecondaryDumpData,
            gCrashDumpBugCheckComponent))
        {
            RtlZeroMemory(
                &Context->CrashDumpAdditionalCallbackRecord,
                sizeof(Context->CrashDumpAdditionalCallbackRecord));
            return STATUS_INVALID_PARAMETER;
        }

        Context->CrashDumpAdditionalCallbackRegistered = TRUE;
        additionalRegisteredHere = TRUE;
    }

    if (!Context->CrashDumpTriageCallbackRegistered)
    {
        status = CrashDump_InitializeTriageDataArray(Context);
        if (!NT_SUCCESS(status))
        {
            if (additionalRegisteredHere)
            {
                (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
                Context->CrashDumpAdditionalCallbackRegistered = FALSE;
                RtlZeroMemory(
                    &Context->CrashDumpAdditionalCallbackRecord,
                    sizeof(Context->CrashDumpAdditionalCallbackRecord));
            }
            return status;
        }

        KeInitializeCallbackRecord(&Context->CrashDumpTriageCallbackRecord);
        if (!KeRegisterBugCheckReasonCallback(
            &Context->CrashDumpTriageCallbackRecord,
            CrashDump_BugCheckTriageDumpDataCallback,
            KbCallbackTriageDumpData,
            gCrashDumpBugCheckComponent))
        {
            RtlZeroMemory(
                &Context->CrashDumpTriageCallbackRecord,
                sizeof(Context->CrashDumpTriageCallbackRecord));

            if (additionalRegisteredHere)
            {
                (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
                Context->CrashDumpAdditionalCallbackRegistered = FALSE;
                RtlZeroMemory(
                    &Context->CrashDumpAdditionalCallbackRecord,
                    sizeof(Context->CrashDumpAdditionalCallbackRecord));
            }

            if (Context->CrashDumpTriageDataArray != NULL)
            {
                ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
                Context->CrashDumpTriageDataArray = NULL;
                Context->CrashDumpTriageDataArraySize = 0;
            }

            return STATUS_INVALID_PARAMETER;
        }

        Context->CrashDumpTriageCallbackRegistered = TRUE;
    }

    return STATUS_SUCCESS;
}

static VOID
CrashDump_ResetState(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG index;
    ULONG sourceCount;

    if (Context == NULL)
    {
        return;
    }

    sourceCount = Context->CrashDumpDataSourceCount;
    if ((sourceCount == 0) || (sourceCount > PMICGLINK_CRASHDUMP_MAX_SOURCES))
    {
        sourceCount = PMICGLINK_CRASHDUMP_MAX_SOURCES;
    }

    if (Context->CrashDumpLock != NULL)
    {
        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);
    }

    for (index = 0; index < sourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if (source->BugCheckCallbackRegistered)
        {
            (VOID)KeDeregisterBugCheckReasonCallback(&source->BugCheckCallbackRecord);
            source->BugCheckCallbackRegistered = FALSE;
        }

        if (source->RingBufferData != NULL)
        {
            ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        }
        if (source->BugCheckSnapshotBuffer != NULL)
        {
            ExFreePoolWithTag(source->BugCheckSnapshotBuffer, PMICGLINK_POOLTAG_CRASHDUMP);
        }

        RtlZeroMemory(source, sizeof(*source));
    }

    if (Context->CrashDumpAdditionalCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
        Context->CrashDumpAdditionalCallbackRegistered = FALSE;
    }

    if (Context->CrashDumpTriageCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpTriageCallbackRecord);
        Context->CrashDumpTriageCallbackRegistered = FALSE;
    }

    if (Context->CrashDumpTriageDataArray != NULL)
    {
        ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
        Context->CrashDumpTriageDataArray = NULL;
        Context->CrashDumpTriageDataArraySize = 0;
    }

    RtlZeroMemory(&Context->CrashDumpAdditionalCallbackRecord, sizeof(Context->CrashDumpAdditionalCallbackRecord));
    RtlZeroMemory(&Context->CrashDumpTriageCallbackRecord, sizeof(Context->CrashDumpTriageCallbackRecord));

    if (Context->CrashDumpLock != NULL)
    {
        WdfWaitLockRelease(Context->CrashDumpLock);
    }
}

static NTSTATUS
HandleCrashDumpRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    WDFFILEOBJECT fileObject;

    if ((Context == NULL) || (Request == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;
    fileObject = WdfRequestGetFileObject(Request);

    switch (IoControlCode)
    {
    case IOCTL_CRASHDUMP_DATA_SOURCE_CREATE:
    {
        const DATA_SOURCE_CREATE* createInfo;
        LONG slotIndex;
        LONG existingIndex;
        ULONG index;

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(DATA_SOURCE_CREATE)))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        createInfo = (const DATA_SOURCE_CREATE*)InputBuffer;

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        existingIndex = -1;
        if (!CrashDumpGuidIsZero(&createInfo->Guid))
        {
            for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
            {
                if (RtlCompareMemory(
                    &Context->CrashDumpDataSources[index].RingBufferGuid,
                    &createInfo->Guid,
                    sizeof(GUID)) == sizeof(GUID))
                {
                    WDFFILEOBJECT ownerFileObject;

                    ownerFileObject = Context->CrashDumpDataSources[index].FileObject[1];
                    if ((ownerFileObject == NULL)
                        || (ownerFileObject == PMICGLINK_CRASHDUMP_STALE_FILE_OBJECT))
                    {
                        existingIndex = (LONG)index;
                        break;
                    }
                }
            }
        }

        if (existingIndex >= 0)
        {
            Context->CrashDumpDataSources[(ULONG)existingIndex].FileObject[1] = fileObject;
            status = STATUS_SUCCESS;
            *BytesReturned = sizeof(DATA_SOURCE_CREATE);
        }
        else
        {
            if (CrashDump_FileHandleSlotFind(Context, fileObject, 1) >= 0)
            {
                status = STATUS_NO_MORE_FILES;
            }
            else
            {
                slotIndex = -1;
                for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
                {
                    if ((Context->CrashDumpDataSources[index].FileObject[1] == NULL)
                        && (Context->CrashDumpDataSources[index].RingBufferData == NULL))
                    {
                        slotIndex = (LONG)index;
                        break;
                    }
                }

                if (slotIndex < 0)
                {
                    status = STATUS_NO_MORE_FILES;
                }
                else
                {
                    Context->CrashDumpDataSources[(ULONG)slotIndex].FileObject[1] = fileObject;
                    status = CrashDump_DataSourceCreateInternal(
                        Context,
                        (ULONG)slotIndex,
                        createInfo->EntriesCount,
                        createInfo->EntrySize,
                        &createInfo->Guid);
                    if (!NT_SUCCESS(status))
                    {
                        Context->CrashDumpDataSources[(ULONG)slotIndex].FileObject[1] = NULL;
                    }
                    else
                    {
                        *BytesReturned = sizeof(DATA_SOURCE_CREATE);
                    }
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_DESTROY:
        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);
        status = CrashDump_DataSourceDestroyAuxiliaryInternal(Context, fileObject);
        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;

    case IOCTL_CRASHDUMP_DATA_SOURCE_WRITE:
    {
        LONG slotIndex;
        ULONG entrySize;

        if ((InputBuffer == NULL) || (InputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 1);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            entrySize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSizeOfEachEntry;
            if (InputBufferSize > entrySize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = CrashDump_DataSourceWriteInternal(
                    Context,
                    (ULONG)slotIndex,
                    (const UCHAR*)InputBuffer,
                    InputBufferSize);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = InputBufferSize;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_QUERY:
    {
        const DATA_SOURCE_CREATE* queryIn;
        DATA_SOURCE_CREATE* queryOut;
        LONG slotIndex;
        ULONG index;

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(DATA_SOURCE_CREATE))
            || (OutputBuffer == NULL) || (OutputBufferSize < sizeof(DATA_SOURCE_CREATE)))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        queryIn = (const DATA_SOURCE_CREATE*)InputBuffer;
        queryOut = (DATA_SOURCE_CREATE*)OutputBuffer;
        if (CrashDumpGuidIsZero(&queryIn->Guid))
        {
            return STATUS_INVALID_PARAMETER;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = -1;
        for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
        {
            if (RtlCompareMemory(
                &Context->CrashDumpDataSources[index].RingBufferGuid,
                &queryIn->Guid,
                sizeof(GUID)) == sizeof(GUID))
            {
                slotIndex = (LONG)index;
                break;
            }
        }

        if (slotIndex < 0)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

            source = &Context->CrashDumpDataSources[(ULONG)slotIndex];
            source->FileObject[2] = fileObject;
            queryOut->EntriesCount = (source->RingBufferSizeOfEachEntry == 0)
                ? 0
                : (source->RingBufferSize / source->RingBufferSizeOfEachEntry);
            queryOut->EntrySize = source->RingBufferSizeOfEachEntry;
            queryOut->Guid = source->RingBufferGuid;
            *BytesReturned = sizeof(DATA_SOURCE_CREATE);
            status = STATUS_SUCCESS;
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_READ:
    {
        LONG slotIndex;
        ULONG entrySize;

        if ((OutputBuffer == NULL) || (OutputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 2);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            entrySize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSizeOfEachEntry;
            if (OutputBufferSize < entrySize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = CrashDump_DataSourceReadInternal(
                    Context,
                    (ULONG)slotIndex,
                    (UCHAR*)OutputBuffer,
                    OutputBufferSize);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = OutputBufferSize;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_CAPTURE:
    {
        LONG slotIndex;
        ULONG captureBytes;
        ULONG ringBufferSize;

        if ((OutputBuffer == NULL) || (OutputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 2);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            ringBufferSize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSize;
            if (OutputBufferSize < ringBufferSize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                captureBytes = 0;
                status = CrashDump_DataSourceCaptureInternal(
                    Context,
                    (ULONG)slotIndex,
                    (UCHAR*)OutputBuffer,
                    OutputBufferSize,
                    &captureBytes);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = captureBytes;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS
CrashDump_IoctlHandler(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    return HandleCrashDumpRequest(
        Context,
        Request,
        IoControlCode,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutputBufferSize,
        BytesReturned);
}

static VOID
CrashDump_PrepareBugCheckSnapshot(
    _Inout_ PMICGLINK_CRASHDUMP_DATA_SOURCE* Source
    )
{
    ULONG validEntries;
    ULONG oldestIndex;
    ULONG index;

    if (Source == NULL)
    {
        return;
    }

    Source->BugCheckBufferPointer = NULL;
    if ((Source->BugCheckSnapshotBuffer == NULL)
        || (Source->RingBufferSize == 0)
        || (Source->EntriesCount == 0)
        || (Source->RingBufferSizeOfEachEntry == 0))
    {
        return;
    }

    RtlZeroMemory(Source->BugCheckSnapshotBuffer, Source->RingBufferSize);

    if ((Source->RingBufferData == NULL) || (Source->ValidEntryCount == 0))
    {
        Source->BugCheckBufferPointer = Source->BugCheckSnapshotBuffer;
        return;
    }

    validEntries = Source->ValidEntryCount;
    if (validEntries > Source->EntriesCount)
    {
        validEntries = Source->EntriesCount;
    }

    oldestIndex = (Source->CurrentRingBufferIndex + Source->EntriesCount - validEntries) % Source->EntriesCount;
    for (index = 0; index < validEntries; index++)
    {
        ULONG sourceIndex;

        sourceIndex = (oldestIndex + index) % Source->EntriesCount;
        RtlCopyMemory(
            Source->BugCheckSnapshotBuffer + ((SIZE_T)index * Source->RingBufferSizeOfEachEntry),
            Source->RingBufferData + ((SIZE_T)sourceIndex * Source->RingBufferSizeOfEachEntry),
            Source->RingBufferSizeOfEachEntry);
    }

    Source->BugCheckBufferPointer = Source->BugCheckSnapshotBuffer;
}

static NTSTATUS
CrashDump_RingBufferElementsFirstBufferGet(
    _In_opt_ PVOID DmfModule,
    _Out_writes_bytes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _Inout_opt_ PVOID CallbackContext
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    UNREFERENCED_PARAMETER(DmfModule);
    UNREFERENCED_PARAMETER(BufferSize);

    if ((Buffer == NULL) || (CallbackContext == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = (PMICGLINK_CRASHDUMP_DATA_SOURCE*)CallbackContext;
    source->BugCheckBufferPointer = Buffer;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_RingBufferElementsXor(
    _In_opt_ PVOID DmfModule,
    _Inout_updates_bytes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _Inout_opt_ PVOID CallbackContext
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG index;
    ULONG keySize;

    UNREFERENCED_PARAMETER(DmfModule);

    if ((Buffer == NULL) || (CallbackContext == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = (PMICGLINK_CRASHDUMP_DATA_SOURCE*)CallbackContext;
    keySize = source->RingBufferEncryptionKeySize;
    if ((keySize == 0) || (keySize > 32))
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (index = source->RingBufferEnumerationOffset; index < BufferSize; index++)
    {
        Buffer[index] ^= (UCHAR)source->RingBufferEncryptionKey[index % keySize];
    }
    source->RingBufferEnumerationOffset = BufferSize;

    return STATUS_SUCCESS;
}

static VOID
CrashDump_BugCheckSecondaryDumpDataCallbackRingBuffer(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG* reason64;
    ULONG* reason32;
    ULONG bytesReported;
    UCHAR* xorBuffer;

    UNREFERENCED_PARAMETER(Reason);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < 44))
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    source = CrashDump_FindSourceFromCallbackRecord(context, Record);
    if (source == NULL)
    {
        source = CrashDump_FindActiveDataSource(context);
    }
    if (source == NULL)
    {
        WdfWaitLockRelease(context->CrashDumpLock);
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    reason32 = (ULONG*)ReasonSpecificData;

    if (reason64[4] != 0)
    {
        if (reason64[4] == reason64[0])
        {
            RtlCopyMemory(&reason64[2], &source->RingBufferGuid, sizeof(GUID));
            source->RingBufferEnumerationOffset = 0;
            xorBuffer = (source->BugCheckBufferPointer != NULL)
                ? source->BugCheckBufferPointer
                : source->RingBufferData;
            if (xorBuffer != NULL)
            {
                (VOID)CrashDump_RingBufferElementsXor(
                    NULL,
                    xorBuffer,
                    source->RingBufferSize,
                    source);
            }
            source->RingBufferEnumerationOffset = 0;
        }
    }
    else
    {
        CrashDump_PrepareBugCheckSnapshot(source);
        if (source->BugCheckBufferPointer != NULL)
        {
            (VOID)CrashDump_RingBufferElementsFirstBufferGet(
                NULL,
                source->BugCheckBufferPointer,
                source->RingBufferSize,
                source);
        }
    }

    if (source->BugCheckBufferPointer != NULL)
    {
        reason64[4] = (ULONGLONG)(ULONG_PTR)source->BugCheckBufferPointer;
        bytesReported = reason32[3];
        if (source->RingBufferSize < bytesReported)
        {
            bytesReported = source->RingBufferSize;
        }
    }
    else
    {
        reason64[4] = 0;
        bytesReported = 0;
    }

    reason32[10] = bytesReported;
    WdfWaitLockRelease(context->CrashDumpLock);
}

static VOID
CrashDump_BugCheckSecondaryDumpDataCallbackAdditional(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG* reason64;
    ULONG* reason32;
    ULONG bytesReported;

    UNREFERENCED_PARAMETER(Reason);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < 44))
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    source = CrashDump_FindSourceFromCallbackRecord(context, Record);
    if (source == NULL)
    {
        source = CrashDump_FindActiveDataSource(context);
    }
    if (source == NULL)
    {
        WdfWaitLockRelease(context->CrashDumpLock);
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    reason32 = (ULONG*)ReasonSpecificData;

    if (reason64[4] != 0)
    {
        if (reason64[4] == reason64[0])
        {
            RtlCopyMemory(&reason64[2], &source->RingBufferGuid, sizeof(GUID));
            bytesReported = reason32[3];
            if (source->RingBufferSize < bytesReported)
            {
                bytesReported = source->RingBufferSize;
            }
            reason32[10] = bytesReported;
        }
    }
    else
    {
        CrashDump_PrepareBugCheckSnapshot(source);
        if (source->BugCheckBufferPointer != NULL)
        {
            reason64[4] = (ULONGLONG)(ULONG_PTR)source->BugCheckBufferPointer;
            bytesReported = reason32[3];
            if (source->RingBufferSize < bytesReported)
            {
                bytesReported = source->RingBufferSize;
            }
        }
        else
        {
            reason64[4] = 0;
            bytesReported = 0;
        }
        reason32[10] = bytesReported;
    }

    WdfWaitLockRelease(context->CrashDumpLock);
}

static VOID
CrashDump_BugCheckTriageDumpDataCallback(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONGLONG* reason64;

    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(Record);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < sizeof(ULONGLONG) * 2))
    {
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    if ((reason64[1] & 1ULL) == 0)
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    CrashDump_PopulateTriageDataArray(context);
    if (context->CrashDumpTriageDataArray != NULL)
    {
        reason64[0] = (ULONGLONG)(ULONG_PTR)context->CrashDumpTriageDataArray;
    }
    else
    {
        reason64[0] = 0;
    }
    WdfWaitLockRelease(context->CrashDumpLock);
}

static VOID
PmicGlinkEvtDmfDeviceModulesAdd(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
{
    WDFDEVICE device;
    UCHAR dummy;

    device = WdfIoQueueGetDevice(Queue);
    if (device != NULL)
    {
        gCrashDumpContext = PmicGlinkGetDeviceContext(device);
    }

    dummy = 0;
    (VOID)CrashDump_RingBufferElementsFirstBufferGet(NULL, &dummy, sizeof(dummy), NULL);
    CrashDump_BugCheckTriageDumpDataCallback((KBUGCHECK_CALLBACK_REASON)0, NULL, &dummy, sizeof(dummy));

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

static BOOLEAN
PmicGlinkGuidEquals(
    _In_ const GUID* Left,
    _In_ const GUID* Right
    )
{
    if ((Left == NULL) || (Right == NULL))
    {
        return FALSE;
    }

    return (RtlCompareMemory(Left, Right, sizeof(GUID)) == sizeof(GUID)) ? TRUE : FALSE;
}

static ULONG
PmicGlinkResolveProprietaryChargerCurrent(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ const GUID* ChargerGuid
    )
{
    if ((Context == NULL) || (ChargerGuid == NULL))
    {
        return 0;
    }

    if (Context->HvdcpCharger.ChargerSupported
        && PmicGlinkGuidEquals(ChargerGuid, &Context->HvdcpCharger.ChargerGUID))
    {
        return Context->HvdcpCharger.ChargerCurrent;
    }

    if (Context->HvdcpV3Charger.ChargerSupported
        && PmicGlinkGuidEquals(ChargerGuid, &Context->HvdcpV3Charger.ChargerGUID))
    {
        return Context->HvdcpV3Charger.ChargerCurrent;
    }

    if (Context->IWallCharger.ChargerSupported
        && PmicGlinkGuidEquals(ChargerGuid, &Context->IWallCharger.ChargerGUID))
    {
        return Context->IWallCharger.ChargerCurrent;
    }

    return 0;
}

static BOOLEAN
PmicGlinkIsBattStateCharging(
    _In_ ULONG BatteryState
    )
{
    switch (BatteryState)
    {
    case 2u:  /* FAST */
    case 3u:  /* TOP_OFF */
    case 5u:  /* RECHARGE */
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN
PmicGlinkIsBattStateNotCharging(
    _In_ ULONG BatteryState
    )
{
    switch (BatteryState)
    {
    case 1u:   /* NO_CHG */
    case 4u:   /* DONE */
    case 6u:   /* TDONE */
    case 7u:   /* NOT_CHARGING_THERMAL */
    case 8u:   /* ERROR */
    case 9u:   /* TEST */
    case 0xBu: /* INVALID */
    case 0xCu: /* NA */
        return TRUE;

    default:
        return FALSE;
    }
}

static NTSTATUS
PmicGlinkCreateDeviceWorkItem(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ PFN_WDF_WORKITEM WorkItemRoutine
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_WORKITEM_CONFIG workItemConfig;
    WDFWORKITEM workItem;

    if ((Context == NULL) || (Context->Device == NULL) || (WorkItemRoutine == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, WorkItemRoutine);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Context->Device;

    status = WdfWorkItemCreate(&workItemConfig, &attributes, &workItem);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WdfWorkItemEnqueue(workItem);
    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkPlatformUsbc_Request_Write(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ const USBPD_DPM_USBC_WRITE_BUFFER* Request
    )
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Request == NULL)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    gPmicGlinkPendingUsbcWriteRequest = *Request;
    return PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkPlatformUsbc_Request_Write_WorkItem);
}

static VOID
PmicGlinkPlatformUsbc_Request_Write_WorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_USBC_WRITE_REQ_MESSAGE message;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if (context != NULL)
    {
        RtlZeroMemory(&message, sizeof(message));
        message.Header = 0x10000800Cull;
        message.MessageOp = 21u;
        message.Request = gPmicGlinkPendingUsbcWriteRequest;

        (VOID)PmicGlink_SendData(context, 0x15u, &message, sizeof(message), TRUE);
    }

    WdfObjectDelete(WorkItem);
}

static VOID
PmicGlinkPlatformSetState_Request_Write_WorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_USBC_SET_STATE_MESSAGE message;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if (context != NULL)
    {
        RtlZeroMemory(&message, sizeof(message));
        message.Header = 0x1000080E8ull;
        message.MessageOp = 82u;
        message.State = gPmicGlinkPendingPlatformState;

        (VOID)PmicGlink_SendData(context, 0x52u, &message, sizeof(message), TRUE);
    }

    WdfObjectDelete(WorkItem);
}

static NTSTATUS
PmicGlinkPlatformUsbc_HandlePanAckNotification(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotifyValue
    )
{
    USBPD_DPM_USBC_WRITE_BUFFER request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (gPmicGlinkPendingPan >= PMICGLINK_MAX_PORTS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&request, sizeof(request));
    request.cmd_payload.pan_ack.port_index = gPmicGlinkPendingPan;
    request.cmd_type = (NotifyValue == PMICGLINK_USBC_NOTIFY_PAN_ACK_CONNECTOR)
        ? PMICGLINK_USBC_CMD_PAN_ACK_CONNECTOR
        : PMICGLINK_USBC_CMD_PAN_ACK;

    return PmicGlinkPlatformUsbc_Request_Write(Context, &request);
}

static NTSTATUS
PmicGlinkPlatformUsbc_HandleReadSelectNotification(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotifyValue
    )
{
    USBPD_DPM_USBC_WRITE_BUFFER request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    RtlZeroMemory(&request, sizeof(request));
    request.cmd_type = PMICGLINK_USBC_CMD_READ_SELECTION;
    request.cmd_payload.read_sel = (ULONG)(UCHAR)(NotifyValue + 12u);
    return PmicGlinkPlatformUsbc_Request_Write(Context, &request);
}

static NTSTATUS
PmicGlinkPlatformUsbc_HandleStateNotification(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotifyValue
    )
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    gPmicGlinkPendingPlatformState = (UCHAR)NotifyValue;
    return PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkPlatformSetState_Request_Write_WorkItem);
}

static VOID
PmicGlinkPlatformUsbc_AcpiNotificationHandler(
    _In_opt_ PVOID Context,
    _In_ ULONG NotifyValue
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;

    if (Context == NULL)
    {
        return;
    }

    deviceContext = PmicGlinkGetDeviceContext((WDFDEVICE)Context);
    if (deviceContext == NULL)
    {
        return;
    }

    status = STATUS_SUCCESS;
    if ((NotifyValue == PMICGLINK_USBC_NOTIFY_PAN_ACK)
        || (NotifyValue == PMICGLINK_USBC_NOTIFY_PAN_ACK_CONNECTOR))
    {
        status = PmicGlinkPlatformUsbc_HandlePanAckNotification(deviceContext, NotifyValue);
    }
    else if ((NotifyValue >= PMICGLINK_USBC_NOTIFY_READ_SEL_BASE)
        && (NotifyValue <= PMICGLINK_USBC_NOTIFY_READ_SEL_LAST))
    {
        status = PmicGlinkPlatformUsbc_HandleReadSelectNotification(deviceContext, NotifyValue);
    }
    else
    {
        status = PmicGlinkPlatformUsbc_HandleStateNotification(deviceContext, NotifyValue);
    }

    UNREFERENCED_PARAMETER(status);
}

static NTSTATUS
PmicGlink_OemHandleCommand(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(PMICGLINK_OEM_SEND_BUFFER_SIZE) UCHAR* InputBuffer,
    _Out_ BOOLEAN* IsOEMCmd
    )
{
    if ((Context == NULL) || (InputBuffer == NULL) || (IsOEMCmd == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *IsOEMCmd = FALSE;
    if (InputBuffer[0] == 0xFFu)
    {
        *IsOEMCmd = TRUE;
    }

    return STATUS_SUCCESS;
}

static SIZE_T
PmicGlinkGetBoundedOemPayloadSize(
    _In_reads_bytes_(InputBufferSize) const UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize
    )
{
    SIZE_T payloadSize;

    if ((InputBuffer == NULL) || (InputBufferSize == 0u))
    {
        return 0u;
    }

    payloadSize = (InputBuffer[1] <= PMICGLINK_OEM_BUFFER_SIZE)
        ? (SIZE_T)(InputBuffer[1] + 2u)
        : PMICGLINK_OEM_SEND_BUFFER_SIZE;

    if (payloadSize > InputBufferSize)
    {
        payloadSize = InputBufferSize;
    }

    return payloadSize;
}

static ULONG
PmicGlinkGetUcsiRequestedLength(
    _In_reads_bytes_opt_(InputBufferSize) const UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize
    )
{
    ULONG requestedLength;

    requestedLength = PMICGLINK_UCSI_BUFFER_SIZE;
    if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(ULONG)))
    {
        RtlCopyMemory(&requestedLength, InputBuffer, sizeof(requestedLength));
    }

    return requestedLength;
}

static VOID
PmicGlinkBuildOfflineUcsiResponse(
    _Out_writes_bytes_(PMICGLINK_UCSI_BUFFER_SIZE) UCHAR* OutputBuffer
    )
{
    PUSHORT words;

    if (OutputBuffer == NULL)
    {
        return;
    }

    RtlZeroMemory(OutputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
    words = (PUSHORT)OutputBuffer;
    words[0] = 0x0100;
    words[3] = 0x0800;
}

static VOID
PmicGlinkUpdateOfflineUcsiCommandState(
    _In_reads_bytes_opt_(OutputSize) const UCHAR* OutputBuffer,
    _In_ SIZE_T OutputSize
    )
{
    if ((OutputBuffer != NULL)
        && (OutputSize >= (sizeof(USHORT) * 4u))
        && ((((const PUSHORT)OutputBuffer)[3] & (USHORT)0xA000u) != 0u))
    {
        *(ULONGLONG*)&gLatestUcsiCmd.data[8] = 0ull;
    }
}

static NTSTATUS
PmicGlinkUCSIWriteBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    SIZE_T cachedCopySize;

    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if (BytesReturned == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (InputBuffer != NULL)
    {
        if ((Context != NULL) && Context->GlinkChannelConnected)
        {
            (VOID)PmicGlink_SyncSendReceive(
                Context,
                IOCTL_PMICGLINK_UCSI_WRITE,
                (PUCHAR)InputBuffer,
                InputBufferSize);
        }
        else
        {
            cachedCopySize = InputBufferSize;
            if (cachedCopySize > PMICGLINK_UCSI_BUFFER_SIZE)
            {
                cachedCopySize = PMICGLINK_UCSI_BUFFER_SIZE;
            }

            RtlZeroMemory(gLatestUcsiCmd.data, PMICGLINK_UCSI_BUFFER_SIZE);
            if (cachedCopySize > 0u)
            {
                RtlCopyMemory(gLatestUcsiCmd.data, InputBuffer, cachedCopySize);
            }
        }
    }

    *BytesReturned = 0;
    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkUCSIReadBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    ULONG requestedLength;
    SIZE_T returnedCopySize;
    SIZE_T outputCapacity;

    status = STATUS_SUCCESS;
    requestedLength = PMICGLINK_UCSI_BUFFER_SIZE;
    returnedCopySize = 0u;
    outputCapacity = OutputBufferSize;

    if ((Context == NULL) || (OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;
    if (outputCapacity > 0u)
    {
        RtlZeroMemory(OutputBuffer, outputCapacity);
    }

    if (Context->GlinkChannelConnected)
    {
        status = PmicGlink_SyncSendReceive(
            Context,
            IOCTL_PMICGLINK_UCSI_READ,
            InputBuffer,
            InputBufferSize);

        requestedLength = PmicGlinkGetUcsiRequestedLength(InputBuffer, InputBufferSize);

        if ((requestedLength >= PMICGLINK_UCSI_BUFFER_SIZE)
            && (outputCapacity >= PMICGLINK_UCSI_BUFFER_SIZE))
        {
            RtlCopyMemory(OutputBuffer, Context->UCSIDataBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
            returnedCopySize = PMICGLINK_UCSI_BUFFER_SIZE;
        }
        else if ((requestedLength > 0u) && (outputCapacity > 0u))
        {
            returnedCopySize = requestedLength;
            if (returnedCopySize > outputCapacity)
            {
                returnedCopySize = outputCapacity;
            }
        }

        *BytesReturned = requestedLength;
    }
    else if ((*(ULONGLONG*)&gLatestUcsiCmd.data[8] == 1ull)
        && (outputCapacity >= PMICGLINK_UCSI_BUFFER_SIZE))
    {
        PmicGlinkBuildOfflineUcsiResponse((UCHAR*)OutputBuffer);
        returnedCopySize = PMICGLINK_UCSI_BUFFER_SIZE;
        *BytesReturned = PMICGLINK_UCSI_BUFFER_SIZE;
    }

    PmicGlinkUpdateOfflineUcsiCommandState((const UCHAR*)OutputBuffer, returnedCopySize);

    return status;
}

static NTSTATUS
PmicGlinkGetOemMsg(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PMICGLINK_OEM_GET_PROP_OUTPUT* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(
        Context,
        IOCTL_PMICGLINK_GET_OEM_MSG,
        InputBuffer,
        InputBufferSize);

    if (!NT_SUCCESS(status))
    {
        status = STATUS_SUCCESS;
    }

    RtlCopyMemory(OutputBuffer->data, Context->OemPropData, sizeof(Context->OemPropData));
    *BytesReturned = sizeof(*OutputBuffer);

    return status;
}

static NTSTATUS
PmicGlinkReadOemBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PMICGLINK_OEM_SET_DATA_BUF_TYPE* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(
        Context,
        IOCTL_PMICGLINK_OEM_READ_BUFFER,
        Context->CachedOemSendBuffer,
        sizeof(Context->CachedOemSendBuffer));

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(OutputBuffer->data, Context->OemReceivedData, sizeof(OutputBuffer->data));
        *BytesReturned = sizeof(*OutputBuffer);
    }

    return status;
}

static NTSTATUS
PmicGlinkWriteOemBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    BOOLEAN isOemCmd;
    SIZE_T payloadSize;

    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(InputBufferSize);

    if ((Context == NULL) || (InputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    status = PmicGlink_OemHandleCommand(Context, InputBuffer, &isOemCmd);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    payloadSize = PmicGlinkGetBoundedOemPayloadSize(InputBuffer, InputBufferSize);
    if (payloadSize == 0u)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (isOemCmd)
    {
        RtlZeroMemory(Context->CachedOemSendBuffer, sizeof(Context->CachedOemSendBuffer));
        RtlCopyMemory(Context->CachedOemSendBuffer, InputBuffer, payloadSize);
        return STATUS_SUCCESS;
    }

    return PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_OEM_WRITE_BUFFER, InputBuffer, payloadSize);
}

static NTSTATUS
PmicGlinkGetChargerPorts(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) ULONG* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((Context == NULL) || (OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    status = STATUS_SUCCESS;
    if (Context->NumPorts == 0)
    {
        status = PmicGlink_SyncSendReceive(
            Context,
            IOCTL_PMICGLINK_GET_CHARGER_PORTS,
            InputBuffer,
            InputBufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *OutputBuffer = Context->NumPorts;
    *BytesReturned = sizeof(ULONG);

    return status;
}

static NTSTATUS
PmicGlinkGetUSBBattMngrChgStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) LONG* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    ULONG portIndex;
    LARGE_INTEGER now;
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((Context == NULL) || (InputBuffer == NULL) || (OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    portIndex = *(ULONG*)InputBuffer;
    *BytesReturned = 0;

    if (portIndex >= PMICGLINK_MAX_PORTS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    now = PmicGlinkQuerySystemTime();
    status = STATUS_SUCCESS;

    if ((now.QuadPart - gPmicGlinkLastUsbIoctlEvent.QuadPart) > 10000000ll)
    {
        status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_GET_USB_CHG_STATUS, InputBuffer, InputBufferSize);
    }

    gPmicGlinkLastUsbIoctlEvent = now;

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *OutputBuffer = Context->UsbinPower[portIndex];
    *BytesReturned = sizeof(ULONGLONG);

    return status;
}

static NTSTATUS
PmicGlinkI2CWriteBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((Context == NULL) || (InputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    if ((InputBuffer[1] & 0x1u) != 0)
    {
        Context->I2CDataLength = 0;
        RtlZeroMemory(Context->I2CHeader, sizeof(Context->I2CHeader));
        RtlCopyMemory(Context->I2CHeader, InputBuffer, PMICGLINK_I2C_HEADER_SIZE);
        return STATUS_SUCCESS;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_I2C_WRITE, InputBuffer, InputBufferSize);
    return status;
}

static NTSTATUS
PmicGlinkI2CReadBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) UCHAR* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((Context == NULL) || (OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    if (Context->I2CHeader[0] == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(
        Context,
        IOCTL_PMICGLINK_I2C_READ,
        Context->I2CHeader,
        PMICGLINK_I2C_HEADER_SIZE);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (Context->I2CDataLength == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(OutputBuffer + PMICGLINK_I2C_HEADER_SIZE, Context->I2CData, Context->I2CDataLength);

    *BytesReturned = PMICGLINK_I2C_HEADER_SIZE + Context->I2CDataLength;
    return status;
}

static NTSTATUS
PmicGlinkSendTad_GWS(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_GWS_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_GWS, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->gws_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static NTSTATUS
PmicGlinkSendTad_CWS(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_CWS_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_CWS, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->cws_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static NTSTATUS
PmicGlinkSendTad_STP(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_STP_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_STP, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->stp_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static NTSTATUS
PmicGlinkSendTad_STV(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_STV_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_STV, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->stv_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static NTSTATUS
PmicGlinkSendTad_TIP(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_TIP_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_TIP, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->tip_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static NTSTATUS
PmicGlinkSendTad_TIV(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_opt_(InputBufferSize) UCHAR* InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize) PMICGLINK_TAD_TIV_OUTBUF* OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_TIV, InputBuffer, InputBufferSize);
    *OutputBuffer = Context->tiv_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return status;
}

static ULONG
PmicGlinkNormalizeLegacyIoctl(
    _In_ ULONG IoControlCode
    )
{
    switch (IoControlCode)
    {
    case IOCTL_BATTMNGR_GET_CHARGER_STATUS_V1:
        return IOCTL_BATTMNGR_GET_CHARGER_STATUS;

    case IOCTL_BATTMNGR_GET_BATT_INFO_V1:
        return IOCTL_BATTMNGR_GET_BATT_INFO;

    case IOCTL_BATTMNGR_CONTROL_CHARGING_V1:
        return IOCTL_BATTMNGR_CONTROL_CHARGING;

    case IOCTL_BATTMNGR_SET_STATUS_CRITERIA_V1:
        return IOCTL_BATTMNGR_SET_STATUS_CRITERIA;

    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V1_ALIAS0:
        return IOCTL_BATTMNGR_SET_OPERATION_MODE;

    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V1_ALIAS1:
        return IOCTL_BATTMNGR_GET_BATT_PRESENT;

    case IOCTL_BATTMNGR_SET_OPERATION_MODE_V1:
        return IOCTL_BATTMNGR_SET_CHARGE_RATE;

    case IOCTL_BATTMNGR_SET_CHARGE_RATE_V1:
    case IOCTL_BATTMNGR_NOTIFY_IFACE_FREE_V1:
        return IOCTL_BATTMNGR_NOTIFY_IFACE_FREE;

    case IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS_V1:
        return IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS;

    case IOCTL_BATTMNGR_GET_TEST_INFO_V1:
        return IOCTL_BATTMNGR_GET_TEST_INFO;

    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V2:
        return IOCTL_BATTMNGR_GET_BATT_PRESENT_V1;

    default:
        return IoControlCode;
    }
}

static ULONGLONG
PmicGlink_Helper_get_rel_time_msec(
    VOID
    )
{
    LARGE_INTEGER now;

    now = PmicGlinkQuerySystemTime();
    if (!gPmicGlinkRelTimeInitialized)
    {
        gPmicGlinkRelTimeStartTicks = (ULONGLONG)now.QuadPart;
        gPmicGlinkRelTimeInitialized = TRUE;
        return 0;
    }

    if ((ULONGLONG)now.QuadPart <= gPmicGlinkRelTimeStartTicks)
    {
        return 0;
    }

    return ((ULONGLONG)now.QuadPart - gPmicGlinkRelTimeStartTicks) / 10000ull;
}

static NTSTATUS
PmicGlinkNotify_Interface_Free(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    PmicGlinkClearBattMiniAttachment(Context);
    return STATUS_SUCCESS;
}

static NTSTATUS
BattMngrControlCharging(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    const BATT_MNGR_CONTROL_CHARGING* request;
    GUID zeroGuid;
    BOOLEAN guidIsZero;
    BOOLEAN legacySimpleRequest;

    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if ((Context == NULL) || (InputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    if (InputBufferSize < sizeof(BATT_MNGR_CONTROL_CHARGING))
    {
        return STATUS_INVALID_PARAMETER;
    }

    request = (const BATT_MNGR_CONTROL_CHARGING*)InputBuffer;
    RtlZeroMemory(&zeroGuid, sizeof(zeroGuid));

    switch (request->batt_command)
    {
    case 1:
    case 2:
        return STATUS_SUCCESS;

    case 3:
        if (Context->LegacyOperationalMode.mode_type != 1)
        {
            return STATUS_SUCCESS;
        }

        if (request->batt_max_charge_current == 0xFFFFFFFF)
        {
            return STATUS_SUCCESS;
        }

        if ((request->batt_max_charge_current == Context->LegacyChargeRate.charge_perc)
            && (request->batt_max_charge_current != 900))
        {
            return STATUS_SUCCESS;
        }

        Context->LegacyChargeRate.charge_perc = request->batt_max_charge_current;
        if (Context->LegacyChargeRate.charge_perc == 0)
        {
            RtlZeroMemory(&Context->LegacyControlCharging.charger_guid, sizeof(GUID));
        }
        break;

    case 4:
        if (Context->LegacyOperationalMode.mode_type != 1)
        {
            return STATUS_SUCCESS;
        }

        RtlCopyMemory(
            &Context->LegacyControlCharging.charger_guid,
            &request->charger_guid,
            sizeof(GUID));

        guidIsZero = (RtlCompareMemory(&request->charger_guid, &zeroGuid, sizeof(GUID)) == sizeof(GUID));
        if (guidIsZero)
        {
            Context->LegacyChargeRate.charge_perc = 0;
        }
        else
        {
            Context->LegacyChargeRate.charge_perc = PmicGlinkResolveProprietaryChargerCurrent(
                Context,
                &request->charger_guid);
        }
        break;

    case 5:
        if (Context->LegacyOperationalMode.mode_type != 1)
        {
            return STATUS_SUCCESS;
        }

        guidIsZero = (RtlCompareMemory(&request->charger_guid, &zeroGuid, sizeof(GUID)) == sizeof(GUID));
        legacySimpleRequest = guidIsZero
            && (request->batt_charger_source_type == 0)
            && (request->batt_charger_flags == 0)
            && (request->batt_charger_voltage == 0)
            && (request->batt_charger_port_type == 0)
            && (request->batt_charger_port_id == 0);
        if (legacySimpleRequest)
        {
            if (request->batt_max_charge_current == 0xFFFFFFFF)
            {
                return STATUS_SUCCESS;
            }

            if ((request->batt_max_charge_current == Context->LegacyChargeRate.charge_perc)
                && (request->batt_max_charge_current != 900))
            {
                return STATUS_SUCCESS;
            }

            Context->LegacyChargeRate.charge_perc = request->batt_max_charge_current;
            if (Context->LegacyChargeRate.charge_perc == 0)
            {
                RtlZeroMemory(&Context->LegacyControlCharging.charger_guid, sizeof(GUID));
            }
        }
        else
        {
            RtlCopyMemory(
                &Context->LegacyControlCharging.charger_guid,
                &request->charger_guid,
                sizeof(GUID));

            if (guidIsZero)
            {
                Context->LegacyChargeRate.charge_perc = 0;
            }
            else
            {
                Context->LegacyChargeRate.charge_perc = PmicGlinkResolveProprietaryChargerCurrent(
                    Context,
                    &request->charger_guid);
            }
        }

        Context->LastChargerVoltage = request->batt_charger_voltage;
        Context->LastChargerPortType = request->batt_charger_port_type;
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
HandleLegacyBattMngrRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    ULONG effectiveIoctl;

    if ((Context == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;
    effectiveIoctl = PmicGlinkNormalizeLegacyIoctl(IoControlCode);

    switch (effectiveIoctl)
    {
    case IOCTL_BATTMNGR_GET_CAPABILITIES:
    {
        BATT_MNGR_GET_CAPABILITIES_OUT* outCapabilities;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outCapabilities)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        outCapabilities = (BATT_MNGR_GET_CAPABILITIES_OUT*)OutputBuffer;
        outCapabilities->batt_mngr_capabilities = 0x1BEFEDF7Full;
        *BytesReturned = sizeof(*outCapabilities);
        return STATUS_SUCCESS;
    }

    case IOCTL_BATTMNGR_GET_BATT_ID:
    {
        BATT_MNGR_GET_BATT_ID_OUT* outBattId;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outBattId)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        outBattId = (BATT_MNGR_GET_BATT_ID_OUT*)OutputBuffer;
        outBattId->batt_id = 0;
        *BytesReturned = sizeof(*outBattId);

        if (Context->GlinkChannelRestart || !Context->GlinkChannelConnected)
        {
            return STATUS_SUCCESS;
        }

        status = STATUS_SUCCESS;
        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        if ((gPmicGlinkLastBattIdQueryMsec == 0)
            || (nowMsec < gPmicGlinkLastBattIdQueryMsec)
            || ((nowMsec - gPmicGlinkLastBattIdQueryMsec) >= 500))
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_BATT_ID,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            gPmicGlinkLastBattIdQueryMsec = nowMsec;
            if (!NT_SUCCESS(status))
            {
                status = STATUS_SUCCESS;
            }
        }

        outBattId->batt_id = Context->LegacyBattId.batt_id;
        return status;
    }

    case IOCTL_BATTMNGR_GET_CHARGER_STATUS:
    {
        BATT_MNGR_CHG_STATUS_OUT* outChgStatus;
        BOOLEAN usbPowerPresent;
        ULONG portIndex;
        UCHAR usbStatusQueryPort;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outChgStatus)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        status = STATUS_SUCCESS;
        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        if ((gPmicGlinkLastChargeStatusQueryMsec == 0)
            || (nowMsec < gPmicGlinkLastChargeStatusQueryMsec)
            || ((nowMsec - gPmicGlinkLastChargeStatusQueryMsec) >= 250))
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_CHARGER_STATUS,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            gPmicGlinkLastChargeStatusQueryMsec = nowMsec;
            if (!NT_SUCCESS(status))
            {
                status = STATUS_SUCCESS;
            }
        }

        outChgStatus = (BATT_MNGR_CHG_STATUS_OUT*)OutputBuffer;
        *outChgStatus = Context->LegacyChargeStatus;
        outChgStatus->charging_source = 2u;

        usbStatusQueryPort = 0u;
        if ((PmicGlinkQuerySystemTime().QuadPart - gPmicGlinkLastUsbIoctlEvent.QuadPart) > 10000000ll)
        {
            (VOID)PmicGlink_SyncSendReceive(
                Context,
                IOCTL_PMICGLINK_GET_USB_CHG_STATUS,
                &usbStatusQueryPort,
                sizeof(usbStatusQueryPort));
            gPmicGlinkLastUsbIoctlEvent = PmicGlinkQuerySystemTime();
        }

        usbPowerPresent = FALSE;
        for (portIndex = 0u; portIndex < PMICGLINK_MAX_PORTS; portIndex++)
        {
            if (Context->UsbinPower[portIndex] > 0)
            {
                usbPowerPresent = TRUE;
                break;
            }
        }

        if (!usbPowerPresent)
        {
            outChgStatus->power_state &= ~(1u | 4u);
            outChgStatus->power_state |= 2u;
            if (outChgStatus->rate > 0)
            {
                outChgStatus->rate = -outChgStatus->rate;
            }
        }

        if ((gPmicGlinkLastChargeStatusTraceMsec == 0)
            || (nowMsec < gPmicGlinkLastChargeStatusTraceMsec)
            || ((nowMsec - gPmicGlinkLastChargeStatusTraceMsec) >= 2000))
        {
            gPmicGlinkLastChargeStatusTraceMsec = nowMsec;
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                DPFLTR_INFO_LEVEL,
                "pmicglink: qstatus ps=0x%08lx cap=%lu volt=%lu rate=%ld usb=[%ld,%ld,%ld,%ld]\n",
                outChgStatus->power_state,
                outChgStatus->capacity,
                outChgStatus->voltage,
                outChgStatus->rate,
                Context->UsbinPower[0],
                Context->UsbinPower[1],
                Context->UsbinPower[2],
                Context->UsbinPower[3]);
        }

        *BytesReturned = sizeof(*outChgStatus);

        if (Context->LegacyBattId.batt_id == 0)
        {
            return STATUS_NO_SUCH_DEVICE;
        }

        Context->LegacyStateChangePending = FALSE;
        return status;
    }

    case IOCTL_BATTMNGR_GET_BATT_INFO:
    {
        const BATT_MNGR_GET_BATT_INFO* request;
        BATT_MNGR_GET_BATT_INFO_OUT* outBattInfo;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != PMICGLINK_BATTMNGR_BATT_INFO_OUT_SIZE))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(*request)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const BATT_MNGR_GET_BATT_INFO*)InputBuffer;
        status = STATUS_SUCCESS;
        nowMsec = PmicGlink_Helper_get_rel_time_msec();

        if ((gPmicGlinkLastBattInfoQueryMsec == 0)
            || (nowMsec < gPmicGlinkLastBattInfoQueryMsec)
            || ((nowMsec - gPmicGlinkLastBattInfoQueryMsec) >= 250)
            || (request->batt_info_type == 2)
            || ((request->rate_of_drain != 0) && (request->batt_info_type == 3)))
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_BATT_INFO,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            if (NT_SUCCESS(status))
            {
                gPmicGlinkLastBattInfoQueryMsec = nowMsec;
            }
            else
            {
                status = STATUS_SUCCESS;
            }
        }

        outBattInfo = (BATT_MNGR_GET_BATT_INFO_OUT*)OutputBuffer;
        RtlZeroMemory(outBattInfo, sizeof(*outBattInfo));

        switch (request->batt_info_type)
        {
        case 0:
            outBattInfo->batt_info = Context->LegacyBattInfo;
            break;

        case 1:
            ((ULONG*)outBattInfo)[0] = Context->LegacyReportingScale[0].granularity;
            ((ULONG*)outBattInfo)[1] = Context->LegacyBattInfo.full_charged_capacity;
            break;

        case 2:
            outBattInfo->batt_temperature = Context->LegacyBattTemperature;
            break;

        case 3:
            outBattInfo->batt_estimated_time = Context->LegacyBattEstimatedTime;
            break;

        case 4:
            RtlCopyMemory(
                outBattInfo->batt_device_name,
                Context->LegacyBattDeviceName,
                sizeof(Context->LegacyBattDeviceName));
            break;

        case 5:
            outBattInfo->batt_manufacture_date = Context->LegacyBattManufactureDate;
            break;

        case 6:
            RtlCopyMemory(
                outBattInfo->batt_manufacture_name,
                Context->LegacyBattManufactureName,
                sizeof(Context->LegacyBattManufactureName));
            break;

        case 7:
            RtlCopyMemory(
                outBattInfo->batt_unique_id,
                Context->LegacyBattUniqueId,
                sizeof(Context->LegacyBattUniqueId));
            break;

        case 8:
            RtlCopyMemory(
                outBattInfo->batt_serial_num,
                Context->LegacyBattSerialNumber,
                sizeof(Context->LegacyBattSerialNumber));
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

        *BytesReturned = PMICGLINK_BATTMNGR_BATT_INFO_OUT_SIZE;
        return status;
    }

    case IOCTL_BATTMNGR_CONTROL_CHARGING:
        if (InputBufferSize != sizeof(BATT_MNGR_CONTROL_CHARGING))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return BattMngrControlCharging(
            Context,
            InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_BATTMNGR_SET_STATUS_CRITERIA:
        return PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_SET_STATUS_CRITERIA,
            (PUCHAR)InputBuffer,
            InputBufferSize);

    case IOCTL_BATTMNGR_GET_BATT_PRESENT:
        if (!Context->Notify)
        {
            return STATUS_SUCCESS;
        }

        status = PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_GET_BATT_PRESENT,
            (PUCHAR)InputBuffer,
            InputBufferSize);
        if (NT_SUCCESS(status))
        {
            Context->Notify = FALSE;
        }

        return status;

    case IOCTL_BATTMNGR_SET_OPERATION_MODE:
        return PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_SET_OPERATION_MODE,
            (PUCHAR)InputBuffer,
            InputBufferSize);

    case IOCTL_BATTMNGR_SET_CHARGE_RATE:
    {
        const BATT_MNGR_SET_CHARGE_RATE* request;

        if ((InputBuffer == NULL) || (InputBufferSize != sizeof(BATT_MNGR_SET_CHARGE_RATE)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        request = (const BATT_MNGR_SET_CHARGE_RATE*)InputBuffer;

        if ((request->charge_perc == Context->LegacyChargeRate.charge_perc)
            && (request->charge_perc != 900))
        {
            Context->LegacyChargeRate.device_name = request->device_name;
            return STATUS_SUCCESS;
        }

        Context->LegacyChargeRate = *request;
        return PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_SET_CHARGE_RATE,
            (PUCHAR)InputBuffer,
            InputBufferSize);
    }

    case IOCTL_BATTMNGR_NOTIFY_IFACE_FREE:
        return PmicGlinkNotify_Interface_Free(Context);

    case IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS:
    {
        BATT_MNGR_GET_BATTERY_PRESENT_STATUS* outPresentStatus;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outPresentStatus)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        outPresentStatus = (BATT_MNGR_GET_BATTERY_PRESENT_STATUS*)OutputBuffer;
        RtlZeroMemory(outPresentStatus, sizeof(*outPresentStatus));
        outPresentStatus->IsBatteryPresent = (Context->LegacyBattId.batt_id != 0) ? 1 : 0;
        outPresentStatus->battPercentage = Context->LegacyBattPercentage;
        outPresentStatus->capacity = Context->LegacyChargeStatus.capacity;
        outPresentStatus->battTemperature = (LONG)Context->LegacyBattTemperature;
        outPresentStatus->battRate = Context->LegacyChargeStatus.rate;

        *BytesReturned = sizeof(*outPresentStatus);
        return STATUS_SUCCESS;
    }

    case IOCTL_BATTMNGR_GET_TEST_INFO:
    {
        BATT_MNGR_GENERIC_TEST_INFO_OUTPUT* outTestInfo;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outTestInfo)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        outTestInfo = (BATT_MNGR_GENERIC_TEST_INFO_OUTPUT*)OutputBuffer;
        RtlZeroMemory(outTestInfo, sizeof(*outTestInfo));

        status = PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_GET_TEST_INFO,
            (PUCHAR)InputBuffer,
            InputBufferSize);

        if (NT_SUCCESS(status))
        {
            *outTestInfo = Context->LegacyTestInfo;
        }

        *BytesReturned = sizeof(*outTestInfo);
        return status;
    }

    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V1:
        if (OutputBufferSize != 24)
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (OutputBuffer == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        *BytesReturned = 24;
        return STATUS_UNSUCCESSFUL;

    case IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT:
        if (!Context->GlinkChannelConnected)
        {
            return STATUS_UNSUCCESSFUL;
        }

        return PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT,
            (PUCHAR)InputBuffer,
            InputBufferSize);

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
HandlePmicGlinkRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;

    if (BytesReturned == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    if (!Context->AllReqIntfArrived)
    {
        status = PMICGLINK_STATUS_DEVICE_NOT_READY;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    switch (IoControlCode)
    {
    case IOCTL_PMICGLINK_GET_FEATURE_MASK:
        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(ULONGLONG)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        *(ULONGLONG*)OutputBuffer = 0x3FFFFFULL;
        *BytesReturned = sizeof(ULONGLONG);
        return status;

    case IOCTL_PMICGLINK_UCSI_WRITE:
    case IOCTL_PMICGLINK_ABD_UCSI_WRITE_LEGACY:
        if (InputBufferSize != PMICGLINK_UCSI_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkUCSIWriteBuffer(
            Context,
            InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_UCSI_READ:
    case IOCTL_PMICGLINK_ABD_UCSI_READ_LEGACY:
        if (OutputBufferSize != PMICGLINK_UCSI_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkUCSIReadBuffer(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_PLATFORM_USBC_READ:
    case IOCTL_PMICGLINK_ABD_USBC_READ_LEGACY:
    case IOCTL_PMICGLINK_PLATFORM_USBC_NOTIFY:
    case IOCTL_PMICGLINK_ABD_USBC_NOTIFY_LEGACY:
        if (!Context->GlinkChannelConnected)
        {
            *BytesReturned = 0;
            return STATUS_UNSUCCESSFUL;
        }

        status = PmicGlink_SyncSendReceive(
            Context,
            IoControlCode,
            (PUCHAR)InputBuffer,
            InputBufferSize);
        *BytesReturned = 0;
        return status;

    case IOCTL_PMICGLINK_GET_OEM_MSG:
    case IOCTL_PMICGLINK_ABD_GET_OEM_MSG_LEGACY:
        if (OutputBufferSize != sizeof(PMICGLINK_OEM_GET_PROP_OUTPUT))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkGetOemMsg(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_OEM_GET_PROP_OUTPUT*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_GWS:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_GWS_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_GWS(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_GWS_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_CWS:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_CWS_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_CWS(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_CWS_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_STP:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_STP_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_STP(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_STP_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_STV:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_STV_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_STV(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_STV_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_TIP:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_TIP_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_TIP(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_TIP_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_TAD_TIV:
        if (OutputBufferSize != sizeof(PMICGLINK_TAD_TIV_OUTBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkSendTad_TIV(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (PMICGLINK_TAD_TIV_OUTBUF*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_OEM_READ_BUFFER:
        if (OutputBufferSize != PMICGLINK_OEM_SEND_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkReadOemBuffer(
            Context,
            InputBuffer,
            InputBufferSize,
            (PMICGLINK_OEM_SET_DATA_BUF_TYPE*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_OEM_WRITE_BUFFER:
        if (InputBufferSize != PMICGLINK_OEM_SEND_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkWriteOemBuffer(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_GET_CHARGER_PORTS:
        if (OutputBufferSize != sizeof(ULONG))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkGetChargerPorts(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (ULONG*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_GET_USB_CHG_STATUS:
        if ((InputBufferSize != sizeof(ULONG)) || (OutputBufferSize != sizeof(ULONG)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkGetUSBBattMngrChgStatus(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            (LONG*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_PRESHUTDOWN_CMD:
        if (InputBufferSize != sizeof(PMICGLINK_PRESHUTDOWN_CMD_INBUF))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (InputBuffer == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        return PmicGlinkPlatformQcmb_PreShutdown_Cmd(
            Context,
            ((PMICGLINK_PRESHUTDOWN_CMD_INBUF*)InputBuffer)->CmdBitMask);

    case IOCTL_PMICGLINK_I2C_WRITE:
        if (InputBufferSize != 70)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkI2CWriteBuffer(
            Context,
            (UCHAR*)InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_I2C_READ:
        if (OutputBufferSize != 70)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkI2CReadBuffer(
            Context,
            InputBuffer,
            InputBufferSize,
            (UCHAR*)OutputBuffer,
            OutputBufferSize,
            BytesReturned);

    case IOCTL_PMICGLINK_QCMB_GET_CHARGER_INFO:
        if (OutputBufferSize != sizeof(QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (OutputBuffer == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        *BytesReturned = sizeof(QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA);
        return PmicGlinkPlatformQcmb_GetChargerInfo_Cmd(
            Context,
            (QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA*)OutputBuffer);

    default:
        return status;
    }
}

VOID
PmicGlink_GetStringFromBuffer(
    _In_reads_bytes_opt_(BufferSize) const UCHAR* Buffer,
    _In_ UCHAR BufferSize,
    _Out_writes_bytes_(StringSize) CHAR* StringBuffer,
    _In_ UCHAR StringSize
    )
{
    static const CHAR hexDigits[] = "0123456789ABCDEF";
    ULONG i;
    ULONG outIndex;

    if ((StringBuffer == NULL) || (StringSize == 0))
    {
        return;
    }

    if ((Buffer == NULL) || ((ULONG)StringSize < ((ULONG)BufferSize * 3u)))
    {
        StringBuffer[0] = '\0';
        return;
    }

    outIndex = 0;
    StringBuffer[0] = '\0';

    for (i = 0; i < BufferSize; i++)
    {
        UCHAR value;

        value = Buffer[i];
        StringBuffer[outIndex++] = hexDigits[(value >> 4) & 0x0Fu];
        StringBuffer[outIndex++] = hexDigits[value & 0x0Fu];

        if ((i + 1u) < BufferSize)
        {
            StringBuffer[outIndex++] = ' ';
        }
    }

    StringBuffer[outIndex] = '\0';
}

NTSTATUS
PmicGlink_ANSIToUniString(
    _In_z_ const CHAR* AnsiString,
    _Out_writes_(UnicodeChars) WCHAR* UnicodeString,
    _In_ SIZE_T UnicodeChars
    )
{
    ANSI_STRING ansi;
    UNICODE_STRING unicode;

    if ((AnsiString == NULL) || (UnicodeString == NULL) || (UnicodeChars == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (UnicodeChars > ((SIZE_T)0xFFFFu / sizeof(WCHAR)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitAnsiString(&ansi, AnsiString);

    unicode.Buffer = UnicodeString;
    unicode.Length = 0;
    unicode.MaximumLength = (USHORT)(UnicodeChars * sizeof(WCHAR));

    return RtlAnsiStringToUnicodeString(&unicode, &ansi, FALSE);
}

NTSTATUS
PmicGlink_RetrieveRxData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _Out_opt_ PBOOLEAN ExpectedReceived
    )
{
    PMICGLINK_COMM_DATA* slot;
    const UCHAR* Buffer;
    SIZE_T BufferSize;
    SIZE_T slotSize;
    ULONG packetOpCode;
    ULONG opCode;
    NTSTATUS status;

    if (ExpectedReceived != NULL)
    {
        *ExpectedReceived = FALSE;
    }

    if ((Context == NULL) || (OpCode >= PMICGLINK_COMM_DATA_SLOTS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    slot = &Context->CommData[OpCode];
    Buffer = slot->Buffer;
    BufferSize = slot->Size;
    slotSize = BufferSize;
    if ((Buffer == NULL) || (BufferSize == 0u))
    {
        return STATUS_SUCCESS;
    }

    status = STATUS_SUCCESS;
    opCode = OpCode;
    Context->LastRxOpcode = 0;
    Context->LastRxStatus = STATUS_SUCCESS;
    Context->LastRxValid = FALSE;

    if (BufferSize < (sizeof(ULONGLONG) + sizeof(ULONG)))
    {
        if (BufferSize >= sizeof(gPmicGlinkUsbcNotification.AsUINT8))
        {
            RtlCopyMemory(
                gPmicGlinkUsbcNotification.AsUINT8,
                Buffer,
                sizeof(gPmicGlinkUsbcNotification.AsUINT8));
            gPmicGlinkPendingPan = (UCHAR)gPmicGlinkUsbcNotification.detail.common.port_index;
        }
        else
        {
            SIZE_T copySize;

            copySize = (BufferSize < sizeof(Context->OemReceivedData))
                ? BufferSize
                : sizeof(Context->OemReceivedData);
            RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));
            RtlCopyMemory(Context->OemReceivedData, Buffer, copySize);
        }

        Context->LastRxOpcode = 0xFFFFFFFFu;
        Context->LastRxStatus = STATUS_SUCCESS;
        Context->LastRxValid = FALSE;
        return STATUS_SUCCESS;
    }

    packetOpCode = 0xFFFFFFFFu;
    RtlCopyMemory(&packetOpCode, Buffer + sizeof(ULONGLONG), sizeof(packetOpCode));
    if (packetOpCode != OpCode)
    {
        Context->LastRxValid = FALSE;
        return STATUS_SUCCESS;
    }

    Context->LastRxOpcode = opCode;
    switch (opCode)
    {
    case 0u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->LegacyBattId.batt_id, Buffer + 12, sizeof(Context->LegacyBattId.batt_id));
            if ((Context->LegacyBattId.batt_id != 0u) && (gPmicGlinkCachedBatteryStatus == 0u))
            {
                gPmicGlinkCachedBatteryStatus = 1u;
            }
        }
        break;

    case 1u:
        if (BufferSize >= 40u)
        {
            RtlCopyMemory(&Context->LegacyBattStateId, Buffer + 12, sizeof(Context->LegacyBattStateId));
            RtlCopyMemory(&Context->LegacyChargeStatus.capacity, Buffer + 16, sizeof(Context->LegacyChargeStatus.capacity));
            RtlCopyMemory(&Context->LegacyChargeStatus.rate, Buffer + 20, sizeof(Context->LegacyChargeStatus.rate));
            RtlCopyMemory(&Context->LegacyChargeStatus.voltage, Buffer + 24, sizeof(Context->LegacyChargeStatus.voltage));
            RtlCopyMemory(&Context->LegacyChargeStatus.power_state, Buffer + 28, sizeof(Context->LegacyChargeStatus.power_state));
            RtlCopyMemory(&Context->LegacyBattTemperature, Buffer + 36, sizeof(Context->LegacyBattTemperature));
            Context->LegacyBattPercentage = (Context->LegacyChargeStatus.capacity > 100u)
                ? 100u
                : (UCHAR)Context->LegacyChargeStatus.capacity;
        }
        break;

    case 3u:
    case 4u:
    case 5u:
    case 6u:
    case 72u:
    {
        ULONG fwStatus;

        if (BufferSize < 16u)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
        status = (fwStatus == 0u) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        break;
    }

    case 9u:
        if (BufferSize >= 740u)
        {
            CHAR battDeviceNameA[129];
            CHAR battManufacturerNameA[129];
            CHAR battSerialA[129];
            CHAR battUniqueA[129];
            SIZE_T mfgNameLen;
            SIZE_T uniqueTailBytes;

            RtlCopyMemory(&Context->LegacyBattInfo.designed_capacity, Buffer + 16, sizeof(Context->LegacyBattInfo.designed_capacity));
            RtlCopyMemory(&Context->LegacyBattInfo.full_charged_capacity, Buffer + 20, sizeof(Context->LegacyBattInfo.full_charged_capacity));
            RtlCopyMemory(&Context->LegacyBattInfo.default_alert2, Buffer + 32, sizeof(Context->LegacyBattInfo.default_alert2));
            RtlCopyMemory(&Context->LegacyBattInfo.default_alert1, Buffer + 36, sizeof(Context->LegacyBattInfo.default_alert1));
            RtlCopyMemory(&Context->LegacyBattInfo.cycle_count, Buffer + 40, sizeof(Context->LegacyBattInfo.cycle_count));
            RtlCopyMemory(&Context->LegacyBattInfo.capabilities, Buffer + 76, sizeof(Context->LegacyBattInfo.capabilities));
            RtlCopyMemory(&Context->LegacyBattInfo.chemistry, Buffer + 592, sizeof(Context->LegacyBattInfo.chemistry));
            RtlCopyMemory(&Context->LegacyBattInfo.critical_bias, Buffer + 724, sizeof(Context->LegacyBattInfo.critical_bias));
            RtlCopyMemory(&Context->LegacyBattEstimatedTime, Buffer + 736, sizeof(Context->LegacyBattEstimatedTime));
            Context->LegacyBattInfo.technology = *(Buffer + 24);

            Context->LegacyBattManufactureDate.day = *(Buffer + 728);
            Context->LegacyBattManufactureDate.month = *(Buffer + 729);
            RtlCopyMemory(&Context->LegacyBattManufactureDate.year, Buffer + 730, sizeof(Context->LegacyBattManufactureDate.year));

            Context->LegacyReportingScale[0].capacity = Context->LegacyBattInfo.designed_capacity;
            RtlCopyMemory(
                &Context->LegacyReportingScale[0].granularity,
                Buffer + 64,
                sizeof(Context->LegacyReportingScale[0].granularity));

            RtlZeroMemory(battDeviceNameA, sizeof(battDeviceNameA));
            RtlZeroMemory(battManufacturerNameA, sizeof(battManufacturerNameA));
            RtlZeroMemory(battSerialA, sizeof(battSerialA));
            RtlZeroMemory(battUniqueA, sizeof(battUniqueA));

            RtlCopyMemory(battDeviceNameA, Buffer + 80, 128);
            RtlCopyMemory(battManufacturerNameA, Buffer + 464, 128);
            RtlCopyMemory(battSerialA, Buffer + 208, 128);

            mfgNameLen = 0;
            while ((mfgNameLen < 128u) && (battManufacturerNameA[mfgNameLen] != '\0'))
            {
                mfgNameLen++;
            }

            RtlCopyMemory(battUniqueA, battManufacturerNameA, mfgNameLen);
            uniqueTailBytes = 128u - mfgNameLen;
            RtlCopyMemory(battUniqueA + mfgNameLen, battSerialA, uniqueTailBytes);
            battUniqueA[128] = '\0';

            (VOID)PmicGlink_ANSIToUniString(
                battDeviceNameA,
                Context->LegacyBattDeviceName,
                ARRAYSIZE(Context->LegacyBattDeviceName));
            (VOID)PmicGlink_ANSIToUniString(
                battManufacturerNameA,
                Context->LegacyBattManufactureName,
                ARRAYSIZE(Context->LegacyBattManufactureName));
            (VOID)PmicGlink_ANSIToUniString(
                battSerialA,
                Context->LegacyBattSerialNumber,
                ARRAYSIZE(Context->LegacyBattSerialNumber));
            (VOID)PmicGlink_ANSIToUniString(
                battUniqueA,
                Context->LegacyBattUniqueId,
                ARRAYSIZE(Context->LegacyBattUniqueId));
        }
        break;

    case 17u:
        if (BufferSize >= (12u + PMICGLINK_UCSI_BUFFER_SIZE))
        {
            RtlCopyMemory(Context->UCSIDataBuffer, Buffer + 12, PMICGLINK_UCSI_BUFFER_SIZE);
        }

        if (BufferSize >= 64u)
        {
            ULONG fwStatus;

            RtlCopyMemory(&fwStatus, Buffer + 60, sizeof(fwStatus));
            status = (NTSTATUS)(LONG)fwStatus;
        }
        break;

    case 18u:
    case 21u:
    {
        ULONG fwStatus;

        if (BufferSize < 16u)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
        status = (NTSTATUS)(LONG)fwStatus;
        break;
    }

    case 32u:
    {
        ULONG fwStatus;
        ULONG responseType;

        if (BufferSize < 20u)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
        if (fwStatus == 0u)
        {
            SIZE_T copySize;

            responseType = 0xFFFFFFFFu;
            if (BufferSize >= 24u)
            {
                RtlCopyMemory(&responseType, Buffer + 20, sizeof(responseType));
            }

            if ((Context->LegacyLastTestInfoRequestType < 0x10u)
                || (Context->LegacyLastTestInfoRequestType == responseType))
            {
                copySize = BufferSize - 20u;
                if (copySize > 0x100u)
                {
                    copySize = 0x100u;
                }

                if (copySize > sizeof(Context->LegacyTestInfo.data))
                {
                    copySize = sizeof(Context->LegacyTestInfo.data);
                }

                RtlZeroMemory(&Context->LegacyTestInfo, sizeof(Context->LegacyTestInfo));
                RtlCopyMemory(Context->LegacyTestInfo.data, Buffer + 20, copySize);
            }

            status = STATUS_SUCCESS;
        }
        else if (fwStatus == 1u)
        {
            status = (NTSTATUS)0xC0000463L;
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
        break;
    }

    case 73u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->NumPorts, Buffer + 12, sizeof(Context->NumPorts));
        }
        break;

    case 74u:
        if (BufferSize >= 44u)
        {
            ULONG voltage[PMICGLINK_MAX_PORTS];
            ULONG current[PMICGLINK_MAX_PORTS];
            ULONG index;

            RtlCopyMemory(voltage, Buffer + 12, sizeof(voltage));
            RtlCopyMemory(current, Buffer + 28, sizeof(current));
            for (index = 0; index < PMICGLINK_MAX_PORTS; index++)
            {
                Context->UsbinPower[index] = (LONG)((current[index] * voltage[index]) / 1000u);
            }
        }
        break;

    case 79u:
        if (BufferSize >= 16u)
        {
            ULONG dataLength;

            RtlCopyMemory(&dataLength, Buffer + 12, sizeof(dataLength));
            Context->I2CDataLength = 0;
            RtlZeroMemory(Context->I2CData, sizeof(Context->I2CData));

            if ((dataLength <= PMICGLINK_I2C_DATA_SIZE)
                && (BufferSize >= (16u + (SIZE_T)dataLength)))
            {
                Context->I2CDataLength = (UCHAR)dataLength;
                if (dataLength > 0u)
                {
                    RtlCopyMemory(Context->I2CData, Buffer + 16, dataLength);
                }
            }
        }
        break;

    case 82u:
        if (BufferSize >= 16u)
        {
            ULONG fwStatus;

            RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
            status = (NTSTATUS)(LONG)fwStatus;
        }
        break;

    case 96u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->gws_out.AlarmStatus, Buffer + 12, sizeof(Context->gws_out.AlarmStatus));
        }
        break;

    case 97u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->cws_out.Status, Buffer + 12, sizeof(Context->cws_out.Status));
        }
        break;

    case 98u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->stp_out.Status, Buffer + 12, sizeof(Context->stp_out.Status));
        }
        break;

    case 99u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->stv_out.Status, Buffer + 12, sizeof(Context->stv_out.Status));
        }
        break;

    case 100u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->tip_out.PolicySetting, Buffer + 12, sizeof(Context->tip_out.PolicySetting));
        }
        break;

    case 101u:
        if (BufferSize >= 16u)
        {
            RtlCopyMemory(&Context->tiv_out.TimerValueRemain, Buffer + 12, sizeof(Context->tiv_out.TimerValueRemain));
        }
        break;

    case 128u:
        if (BufferSize >= 44u)
        {
            if (Context->StateLock != NULL)
            {
                WdfSpinLockAcquire(Context->StateLock);
            }

            RtlCopyMemory(&Context->QcmbStatus, Buffer + 16, sizeof(Context->QcmbStatus));
            RtlCopyMemory(&Context->QcmbCurrentChargerPowerUW, Buffer + 28, sizeof(Context->QcmbCurrentChargerPowerUW));
            RtlCopyMemory(&Context->QcmbGoodChargerThresholdUW, Buffer + 32, sizeof(Context->QcmbGoodChargerThresholdUW));
            RtlCopyMemory(&Context->QcmbChargerStatusInfo, Buffer + 36, sizeof(Context->QcmbChargerStatusInfo));

            if (Context->StateLock != NULL)
            {
                WdfSpinLockRelease(Context->StateLock);
            }

            KeSetEvent(&Context->QcmbNotifyEvent, IO_NO_INCREMENT, FALSE);
        }

        if (BufferSize >= 64u)
        {
            ULONG fwStatus;

            RtlCopyMemory(&fwStatus, Buffer + 60, sizeof(fwStatus));
            status = (NTSTATUS)(LONG)fwStatus;
        }
        break;

    case 129u:
        if (BufferSize >= 16u)
        {
            ULONG fwStatus;

            RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
            status = (NTSTATUS)(LONG)fwStatus;

            if (Context->StateLock != NULL)
            {
                WdfSpinLockAcquire(Context->StateLock);
            }

            RtlCopyMemory(&Context->QcmbStatus, Buffer + 12, sizeof(Context->QcmbStatus));
            Context->QcmbConnected = ((Context->QcmbStatus & 1u) != 0u) ? TRUE : FALSE;

            if (Context->StateLock != NULL)
            {
                WdfSpinLockRelease(Context->StateLock);
            }

            KeSetEvent(&Context->QcmbNotifyEvent, IO_NO_INCREMENT, FALSE);
        }
        break;

    case 257u:
        if (BufferSize >= 16u + sizeof(Context->OemPropData))
        {
            RtlCopyMemory(Context->OemPropData, Buffer + 16, sizeof(Context->OemPropData));
        }
        break;

    case 258u:
        if (BufferSize >= 16u)
        {
            ULONG fwStatus;

            RtlCopyMemory(&fwStatus, Buffer + 12, sizeof(fwStatus));
            status = (NTSTATUS)(LONG)fwStatus;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case 260u:
        if (BufferSize >= 16u)
        {
            ULONG dataLength;

            RtlCopyMemory(&dataLength, Buffer + 12, sizeof(dataLength));
            RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));

            if ((dataLength <= PMICGLINK_OEM_BUFFER_SIZE)
                && (BufferSize >= (16u + (SIZE_T)dataLength)))
            {
                if (dataLength > 0u)
                {
                    RtlCopyMemory(Context->OemReceivedData, Buffer + 16, dataLength);
                }
            }
        }
        break;

    case 7u:
    case 19u:
    case 22u:
    case 130u:
    case 259u:
    {
        ULONG notificationId;
        ULONG notificationType;
        ULONG notificationData;
        ULONG notificationAux;

        notificationId = 0u;
        notificationType = 0u;
        notificationData = 0u;
        notificationAux = 0u;

        if (BufferSize >= sizeof(ULONG))
        {
            RtlCopyMemory(&notificationId, Buffer, sizeof(notificationId));
        }

        if (BufferSize >= (sizeof(ULONG) * 2u))
        {
            RtlCopyMemory(&notificationType, Buffer + sizeof(ULONG), sizeof(notificationType));
        }

        if (BufferSize >= (sizeof(ULONG) * 4u))
        {
            RtlCopyMemory(&notificationData, Buffer + (sizeof(ULONG) * 3u), sizeof(notificationData));
        }

        if (BufferSize >= (sizeof(ULONG) * 5u))
        {
            RtlCopyMemory(&notificationAux, Buffer + (sizeof(ULONG) * 4u), sizeof(notificationAux));
        }

        if (notificationType == 2u)
        {
            switch (notificationId)
            {
            case 0x800Au:
                PmicGlinkNotifyBattMiniStatusFromGlink(Context, notificationData);
                break;

            case 0x800Bu:
                if (notificationAux == 0u)
                {
                    (VOID)PmicGlinkEvaluateAcpiMethodInteger(Context, "USBN", notificationData);
                    PmicGlinkDDI_NotifyUcsiAlert(Context, notificationId, notificationData);
                }
                break;

            case 0x800Cu:
                if (BufferSize >= (12u + sizeof(gPmicGlinkUsbcNotification.AsUINT8)))
                {
                    RtlCopyMemory(
                        gPmicGlinkUsbcNotification.AsUINT8,
                        Buffer + 12,
                        sizeof(gPmicGlinkUsbcNotification.AsUINT8));
                    gPmicGlinkPendingPan = (UCHAR)gPmicGlinkUsbcNotification.detail.common.port_index;
                    (VOID)PmicGlinkEvaluateAcpiMethodBuffer(
                        Context,
                        "UPAN",
                        gPmicGlinkUsbcNotification.AsUINT8,
                        (ULONG)sizeof(gPmicGlinkUsbcNotification.AsUINT8));
                }
                break;

            case 0x800Eu:
                Context->EventID = notificationData;
                if ((notificationData >> 16) == 1u)
                {
                    if (Context->BclCriticalCallbackObject != NULL)
                    {
                        ULONG bclArgument;

                        bclArgument = notificationData & 0xFFFFu;
                        ExNotifyCallback(Context->BclCriticalCallbackObject, &bclArgument, NULL);
                    }
                }
                else
                {
                    (VOID)PmicGlinkEvaluateAcpiMethodInteger(Context, "OEMN", notificationData);
                }
                break;

            case 0x8010u:
                if (notificationData == 128u)
                {
                    KeSetEvent(&Context->QcmbNotifyEvent, IO_NO_INCREMENT, FALSE);
                }
                break;

            default:
                break;
            }
        }

        Context->LastRxStatus = STATUS_SUCCESS;
        Context->LastRxValid = FALSE;
        if (ExpectedReceived != NULL)
        {
            *ExpectedReceived = FALSE;
        }
        return STATUS_SUCCESS;
    }

    default:
        if (BufferSize >= sizeof(gPmicGlinkUsbcNotification.AsUINT8))
        {
            RtlCopyMemory(
                gPmicGlinkUsbcNotification.AsUINT8,
                Buffer,
                sizeof(gPmicGlinkUsbcNotification.AsUINT8));
            gPmicGlinkPendingPan = (UCHAR)gPmicGlinkUsbcNotification.detail.common.port_index;
        }
        else
        {
            SIZE_T copySize;

            copySize = (BufferSize < sizeof(Context->OemReceivedData))
                ? BufferSize
                : sizeof(Context->OemReceivedData);
            RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));
            RtlCopyMemory(Context->OemReceivedData, Buffer, copySize);
        }

        Context->LastRxStatus = STATUS_SUCCESS;
        Context->LastRxValid = FALSE;
        if (ExpectedReceived != NULL)
        {
            *ExpectedReceived = FALSE;
        }
        return STATUS_SUCCESS;
    }

    if ((slot->Buffer != NULL) && (slotSize != 0u))
    {
        RtlZeroMemory(slot->Buffer, slotSize);
        slot->Size = 0u;
    }

    Context->LastRxStatus = status;
    Context->LastRxValid = TRUE;
    if (ExpectedReceived != NULL)
    {
        *ExpectedReceived = TRUE;
    }
    return status;
}

static VOID
PmicGlinkRxNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ SIZE_T IntentUsed
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;
    ULONG messageOp;
    BOOLEAN queueWorkItem;

    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(IntentUsed);

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkMainChannelHandle;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext == NULL)
    {
        return;
    }

    messageOp = MAXULONG;
    queueWorkItem = FALSE;
    if ((Buffer != NULL) && (BufferSize > 0u))
    {
        (VOID)PmicGlinkTryExtractMessageOp(Buffer, BufferSize, &messageOp);
        if (messageOp < PMICGLINK_COMM_DATA_SLOTS)
        {
            status = PmicGlinkStoreCommDataPacket(
                deviceContext,
                messageOp,
                Buffer,
                BufferSize,
                TRUE);
            if (NT_SUCCESS(status))
            {
                queueWorkItem = PmicGlinkIsDeferredNotificationOp(messageOp);

                if (!queueWorkItem)
                {
                    (VOID)KeSetEvent(&gPmicGlinkRxNotificationEvent, IO_NO_INCREMENT, FALSE);
                }
            }
        }
    }

    if ((channelHandle != NULL)
        && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
        && (gPmicGlinkApiInterface.GLinkRxDone != NULL))
    {
        (VOID)gPmicGlinkApiInterface.GLinkRxDone(channelHandle, Buffer, TRUE);
    }

    if (queueWorkItem)
    {
        status = PmicGlinkCreateDeviceWorkItem(deviceContext, PmicGlinkRxNotificationWorkItem);
        if (!NT_SUCCESS(status))
        {
            (VOID)KeSetEvent(&gPmicGlinkRxNotificationEvent, IO_NO_INCREMENT, FALSE);
        }
    }
}

BOOLEAN
PmicGlinkNotifyRxIntentReqCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T RequestedSize
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkMainChannelHandle;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext == NULL)
    {
        return FALSE;
    }

    if ((channelHandle == NULL)
        || (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference == NULL)
        || (gPmicGlinkApiInterface.GLinkQueueRxIntent == NULL))
    {
        return FALSE;
    }

    status = gPmicGlinkApiInterface.GLinkQueueRxIntent(
        channelHandle,
        deviceContext,
        RequestedSize);
    return (status == STATUS_SUCCESS) ? TRUE : FALSE;
}

VOID
PmicGlinkNotifyRxIntentCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T Size
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(Size);

    if (Handle != NULL)
    {
        gPmicGlinkMainChannelHandle = (GLINK_CHANNEL_CTX*)Handle;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext != NULL)
    {
        deviceContext->GlinkRxIntent += 1;
    }
}

VOID
PmicGlinkTxNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    if (Handle != NULL)
    {
        gPmicGlinkMainChannelHandle = (GLINK_CHANNEL_CTX*)Handle;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext != NULL)
    {
        (VOID)KeSetEvent(&gPmicGlinkTxNotificationEvent, IO_NO_INCREMENT, FALSE);
    }
}

VOID
PmicGlinkRxNotificationWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONG pendingOp;
    BOOLEAN expectedReceived;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if ((context != NULL)
        && PmicGlinkGetPendingNotificationOp(context, &pendingOp))
    {
        expectedReceived = PmicGlinkConsumeCommDataPacket(context, pendingOp);
        if (expectedReceived)
        {
            (VOID)KeSetEvent(&gPmicGlinkRxNotificationEvent, IO_NO_INCREMENT, FALSE);
        }

        if (PmicGlinkGetPendingNotificationOp(context, &pendingOp))
        {
            if (!NT_SUCCESS(PmicGlinkCreateDeviceWorkItem(context, PmicGlinkRxNotificationWorkItem)))
            {
                (VOID)KeSetEvent(&gPmicGlinkRxNotificationEvent, IO_NO_INCREMENT, FALSE);
            }
        }
    }

    WdfObjectDelete(WorkItem);
}

static VOID
PmicGLinkRegisterLinkStateCb(
    _In_opt_ PMIC_GLINK_LINK_INFO* LinkInfo,
    _In_opt_ PVOID Context
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    BOOLEAN linkMatches;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if ((deviceContext == NULL)
        || (LinkInfo == NULL)
        || (LinkInfo->Xport == NULL)
        || (LinkInfo->RemoteSs == NULL))
    {
        return;
    }

    linkMatches = FALSE;
    if ((RtlCompareMemory(LinkInfo->Xport, "SMEM", 4) == 4)
        && (LinkInfo->Xport[4] == '\0')
        && (RtlCompareMemory(LinkInfo->RemoteSs, "lpass", 5) == 5)
        && (LinkInfo->RemoteSs[5] == '\0'))
    {
        linkMatches = TRUE;
    }

    if (!linkMatches)
    {
        return;
    }

    if (LinkInfo->LinkState == 1u)
    {
        deviceContext->GlinkLinkStateUp = TRUE;
        if (NT_SUCCESS(PmicGlink_OpenGlinkChannel(deviceContext)))
        {
            deviceContext->GlinkChannelRestart = FALSE;
        }

        if ((deviceContext->UlogInitEn != 0)
            && NT_SUCCESS(PmicGlinkUlog_OpenGlinkChannelUlog(deviceContext)))
        {
            deviceContext->GlinkChannelUlogRestart = FALSE;
        }
    }
    else if (LinkInfo->LinkState == 0u)
    {
        deviceContext->GlinkLinkStateUp = FALSE;
        deviceContext->GlinkChannelConnected = FALSE;
        deviceContext->GlinkChannelRestart = TRUE;
        deviceContext->GlinkChannelUlogConnected = FALSE;
        deviceContext->GlinkChannelUlogRestart = TRUE;
    }
}

VOID
PmicGlinkRpeADSPStateNotificationCallback(
    _In_opt_ PVOID Context,
    _In_ ULONG PreviousState,
    _In_opt_ PULONG CurrentState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;
    PMIC_GLINK_LINK_ID linkId;

    UNREFERENCED_PARAMETER(PreviousState);

    if ((Context == NULL) || (CurrentState == NULL))
    {
        return;
    }

    deviceContext = PmicGlinkGetDeviceContext((WDFDEVICE)Context);
    if (deviceContext == NULL)
    {
        return;
    }

    if ((*CurrentState == PMICGLINK_RPE_STATE_ID_PDR_READY_FOR_COMMANDS)
        && !deviceContext->RpeInitialized)
    {
        status = STATUS_SUCCESS;
        if (!deviceContext->Hibernate)
        {
            status = PmicGlinkEnsureApiInterface(deviceContext);
        }

        if (NT_SUCCESS(status))
        {
            if (gPmicGlinkLinkStateHandle == NULL)
            {
                RtlZeroMemory(&linkId, sizeof(linkId));
                linkId.Version = 1u;
                linkId.Xport = "SMEM";
                linkId.RemoteSs = "lpass";
                linkId.LinkNotifier = PmicGLinkRegisterLinkStateCb;
                linkId.Handle = NULL;
                if ((gPmicGlinkApiInterface.GLinkRegisterLinkStateCb != NULL)
                    && (gPmicGlinkApiInterface.GLinkRegisterLinkStateCb(&linkId, deviceContext) == STATUS_SUCCESS))
                {
                    gPmicGlinkLinkStateHandle = linkId.Handle;
                }
            }

            if (gPmicGlinkLinkStateHandle != NULL)
            {
                deviceContext->RpeInitialized = TRUE;
            }
        }
    }
}

NTSTATUS
PmicGlinkAppStateNotificationCallback(
    _In_opt_ PVOID Context,
    _In_ ULONG PreviousState,
    _In_opt_ PULONG CurrentState
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PreviousState);
    UNREFERENCED_PARAMETER(CurrentState);

    return (NTSTATUS)1;
}

VOID
PmicGlinkUlogRxNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID PacketContext,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T BufferSize
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;
    ULONG messageOp;

    UNREFERENCED_PARAMETER(PacketContext);

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkUlogChannelHandle;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if ((deviceContext != NULL) && (Buffer != NULL) && (BufferSize > 0))
    {
        messageOp = MAXULONG;
        if (PmicGlinkTryExtractMessageOp(Buffer, BufferSize, &messageOp))
        {
            ((PUSHORT)Buffer)[5] = 0u;
        }

        if ((messageOp == PMICGLINK_ULOG_SET_PROPERTIES_OPCODE)
            || (messageOp == PMICGLINK_ULOG_GET_LOG_BUFFER_OPCODE)
            || (messageOp == PMICGLINK_ULOG_GET_BUFFER_OPCODE))
        {
            status = PmicGlinkStoreCommDataPacket(
                deviceContext,
                messageOp,
                Buffer,
                BufferSize,
                TRUE);
            if (NT_SUCCESS(status))
            {
                (VOID)KeSetEvent(&gPmicGlinkUlogRxNotificationEvent, IO_NO_INCREMENT, FALSE);
            }
        }
    }

    if ((channelHandle != NULL)
        && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
        && (gPmicGlinkApiInterface.GLinkRxDone != NULL))
    {
        (VOID)gPmicGlinkApiInterface.GLinkRxDone(channelHandle, Buffer, TRUE);
    }
}

VOID
PmicGlinkUlogTxNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID PacketContext,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T BufferSize
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    PUSHORT txWords;

    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(BufferSize);

    if (Handle != NULL)
    {
        gPmicGlinkUlogChannelHandle = (GLINK_CHANNEL_CTX*)Handle;
    }

    if (Buffer != NULL)
    {
        txWords = (PUSHORT)Buffer;
        txWords[5] = 0u;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext != NULL)
    {
        (VOID)KeSetEvent(&gPmicGlinkUlogTxNotificationEvent, IO_NO_INCREMENT, FALSE);
    }
}

VOID
PmicGlinkUlogRegisterInterfaceWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    NTSTATUS status;
    LONGLONG dueTime100ns;
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);
    if (context != NULL)
    {
        if (context->GlinkChannelUlogConnected)
        {
            if ((context->UlogInterval != 0u) || (context->UlogInitEn != 0u))
            {
                if (context->UlogInterval != 0u)
                {
                    (VOID)PmicGlinkUlogSendSetPropertiesRequest(context);
                }

                if (context->UlogTimer == NULL)
                {
                    WDF_TIMER_CONFIG_INIT(&timerConfig, PmicGlinkUlogTimerFunction);
                    timerConfig.AutomaticSerialization = FALSE;

                    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
                    timerAttributes.ParentObject = parentObject;
                    timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

                    status = WdfTimerCreate(&timerConfig, &timerAttributes, &context->UlogTimer);
                    if (!NT_SUCCESS(status))
                    {
                        context->UlogTimer = NULL;
                    }

                    if (context->UlogTimer != NULL)
                    {
                        dueTime100ns = (context->UlogInterval != 0u)
                            ? (-((LONGLONG)context->UlogInterval * PMICGLINK_100NS_PER_SECOND))
                            : PMICGLINK_ULOG_DEFAULT_TIMER_DUE_TIME_100NS;
                        (VOID)WdfTimerStart(context->UlogTimer, dueTime100ns);
                    }
                }
            }
        }

    }

    WdfObjectDelete(WorkItem);
}

static NTSTATUS
PmicGlinkUlogSendSetPropertiesRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    PMICGLINK_ULOG_SET_PROPERTIES_REQUEST request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->UlogCategories == 0ull)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->UlogLevel > 5u)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&request, sizeof(request));
    request.Header = PMICGLINK_ULOG_MSG_HEADER;
    request.MessageOp = PMICGLINK_ULOG_SET_PROPERTIES_OPCODE;
    request.Categories = Context->UlogCategories;
    request.Level = Context->UlogLevel;

    return PmicGlinkUlog_SendData(Context, &request, sizeof(request));
}

static NTSTATUS
PmicGlinkUlogSendGetLogBufferRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    PMICGLINK_ULOG_GET_BUFFER_REQUEST request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&request, sizeof(request));
    request.Header = PMICGLINK_ULOG_MSG_HEADER;
    request.MessageOp = PMICGLINK_ULOG_GET_LOG_BUFFER_OPCODE;

    return PmicGlinkUlog_SendData(Context, &request, sizeof(request));
}

static NTSTATUS
PmicGlinkUlogSendGetBufferRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    PMICGLINK_ULOG_GET_BUFFER_REQUEST request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&request, sizeof(request));
    request.Header = PMICGLINK_ULOG_MSG_HEADER;
    request.MessageOp = PMICGLINK_ULOG_GET_BUFFER_OPCODE;

    return PmicGlinkUlog_SendData(Context, &request, sizeof(request));
}

static NTSTATUS
PmicGlinkUlogPrintBuffer(
    _In_ BOOLEAN IsInitLog
    )
{
    SIZE_T usedLength;
    SIZE_T index;
    SIZE_T lineStart;
    const CHAR* sourceBuffer;
    CHAR localBuffer[PMICGLINK_ULOG_BUFFER_CAPACITY];
    CHAR lineBuffer[PMICGLINK_ULOG_LINE_PRINT_BUFFER];

    sourceBuffer = IsInitLog ? gPmicGlinkUlogInitData : gPmicGlinkUlogData;
    RtlZeroMemory(localBuffer, sizeof(localBuffer));
    RtlCopyMemory(localBuffer, sourceBuffer, sizeof(localBuffer));

    usedLength = 0;
    while ((usedLength < sizeof(localBuffer)) && (localBuffer[usedLength] != '\0'))
    {
        usedLength++;
    }

    if (usedLength == 0u)
    {
        if (IsInitLog)
        {
            gPmicGlinkUlogInitPrinted = 1u;
        }

        return STATUS_INVALID_MESSAGE;
    }

    lineStart = 0u;
    for (index = 0u; index < usedLength; index++)
    {
        if ((localBuffer[index] != '\n') && (localBuffer[index] != '\0'))
        {
            continue;
        }

        if ((index - lineStart + 1u) >= PMICGLINK_ULOG_LINE_CHUNK_WITH_TERM)
        {
            SIZE_T chunkStart;

            chunkStart = lineStart;
            while (chunkStart < index)
            {
                SIZE_T remainingWithTerm;
                SIZE_T chunkBytes;

                remainingWithTerm = (index - chunkStart) + 1u;
                chunkBytes = (remainingWithTerm > PMICGLINK_ULOG_LINE_CHUNK_WITH_TERM)
                    ? PMICGLINK_ULOG_LINE_CHUNK_WITH_TERM
                    : remainingWithTerm;

                RtlZeroMemory(lineBuffer, sizeof(lineBuffer));
                RtlCopyMemory(lineBuffer, &localBuffer[chunkStart], chunkBytes);
                lineBuffer[chunkBytes - 1u] = '\0';
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "pmicglink: %s\n", lineBuffer);

                chunkStart += chunkBytes;
            }
        }
        else
        {
            SIZE_T lineBytes;

            lineBytes = (index - lineStart) + 1u;
            RtlZeroMemory(lineBuffer, sizeof(lineBuffer));
            RtlCopyMemory(lineBuffer, &localBuffer[lineStart], lineBytes);
            lineBuffer[index - lineStart] = '\0';
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "pmicglink: %s\n", lineBuffer);
        }

        lineStart = index + 1u;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkUlogGetLogBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG attempts;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelUlogConnected)
    {
        return STATUS_SUCCESS;
    }

    attempts = 4u;
    status = STATUS_SUCCESS;
    while (attempts > 0u)
    {
        status = PmicGlinkUlogSendGetLogBufferRequest(Context);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        status = PmicGlinkUlogPrintBuffer(FALSE);
        attempts--;

        if ((attempts == 0u) || ((gPmicGlinkUlogInitPrinted != 0u) && (status != STATUS_SUCCESS)))
        {
            return status;
        }
    }

    return status;
}

static VOID
PmicGlinkUlogTimerFunction(
    _In_ WDFTIMER Timer
    )
{
    NTSTATUS status;
    ULONG attempts;
    LONGLONG dueTime100ns;
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    parentObject = WdfTimerGetParentObject(Timer);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);
    if (context == NULL)
    {
        return;
    }

    if ((gPmicGlinkUlogInitPrinted == 0u)
        && ((context->UlogInterval != 0u) || (context->UlogInitEn != 0u)))
    {
        if (context->GlinkChannelUlogConnected)
        {
            attempts = 4u;
            while (attempts > 0u)
            {
                if (!NT_SUCCESS(PmicGlinkUlogSendGetBufferRequest(context)))
                {
                    break;
                }

                status = PmicGlinkUlogPrintBuffer(TRUE);
                attempts--;

                if ((attempts == 0u) || ((gPmicGlinkUlogInitPrinted != 0u) && (status != STATUS_SUCCESS)))
                {
                    break;
                }
            }
        }

        if (context->UlogInterval == 0u)
        {
            (VOID)PmicGlinkUlogGetLogBuffer(context);
        }
    }

    if (context->UlogInterval != 0u)
    {
        (VOID)PmicGlinkUlogGetLogBuffer(context);
    }

    if ((context->UlogInterval != 0u) && (context->UlogTimer != NULL))
    {
        dueTime100ns = -((LONGLONG)context->UlogInterval * PMICGLINK_100NS_PER_SECOND);
        (VOID)WdfTimerStart(context->UlogTimer, dueTime100ns);
    }
}

VOID
PmicGlinkUlogStateNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ PMICGLINK_CHANNEL_EVENT Event
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkUlogChannelHandle;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext == NULL)
    {
        return;
    }

    switch (Event)
    {
    case PmicGlinkChannelConnected:
        deviceContext->GlinkChannelUlogFirstConnect = TRUE;
        deviceContext->GlinkChannelUlogConnected = TRUE;
        deviceContext->GlinkChannelUlogRestart = FALSE;
        if (channelHandle != NULL)
        {
            gPmicGlinkUlogChannelHandle = channelHandle;
        }
        if ((channelHandle != NULL)
            && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkQueueRxIntent != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkQueueRxIntent(channelHandle, deviceContext, 12288u);
        }
        deviceContext->GlinkUlogRxIntent += 1;
        (VOID)PmicGlinkCreateDeviceWorkItem(deviceContext, PmicGlinkUlogRegisterInterfaceWorkItem);
        break;

    case PmicGlinkChannelLocalDisconnected:
        deviceContext->GlinkChannelUlogConnected = FALSE;
        if (deviceContext->GlinkChannelUlogRestart
            && deviceContext->GlinkLinkStateUp)
        {
            (VOID)PmicGlinkUlog_OpenGlinkChannelUlog(deviceContext);
        }
        break;

    case PmicGlinkChannelRemoteDisconnected:
        if ((channelHandle != NULL)
            && (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
            && (gPmicGlinkApiInterface.GLinkClose != NULL))
        {
            (VOID)gPmicGlinkApiInterface.GLinkClose(channelHandle);

            gPmicGlinkUlogChannelHandle = NULL;
            deviceContext->GlinkChannelUlogConnected = FALSE;
            deviceContext->GlinkChannelUlogRestart = TRUE;
        }
        break;

    default:
        break;
    }
}

BOOLEAN
PmicGlinkUlogNotifyRxIntentReqCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T RequestedSize
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    NTSTATUS status;

    channelHandle = (Handle != NULL)
        ? (GLINK_CHANNEL_CTX*)Handle
        : gPmicGlinkUlogChannelHandle;

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext == NULL)
    {
        return FALSE;
    }

    if ((channelHandle == NULL)
        || (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference == NULL)
        || (gPmicGlinkApiInterface.GLinkQueueRxIntent == NULL))
    {
        return FALSE;
    }

    status = gPmicGlinkApiInterface.GLinkQueueRxIntent(
        channelHandle,
        deviceContext,
        RequestedSize);
    return (status == STATUS_SUCCESS) ? TRUE : FALSE;
}

VOID
PmicGlinkUlogNotifyRxIntentCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T Size
    )
{
    UNREFERENCED_PARAMETER(Size);

    if (Handle != NULL)
    {
        gPmicGlinkUlogChannelHandle = (GLINK_CHANNEL_CTX*)Handle;
    }

    if (Context != NULL)
    {
        ((PPMIC_GLINK_DEVICE_CONTEXT)Context)->GlinkUlogRxIntent += 1;
    }
}

static VOID
PmicGlinkUlogRxNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ SIZE_T IntentUsed
    )
{
    UNREFERENCED_PARAMETER(IntentUsed);

    PmicGlinkUlogRxNotificationCb(
        Channel,
        (PVOID)Context,
        (PVOID)PacketContext,
        (PVOID)Buffer,
        BufferSize);
}

static VOID
PmicGlinkUlogTxNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_opt_ const VOID* PacketContext,
    _In_opt_ const VOID* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    PmicGlinkUlogTxNotificationCb(
        Channel,
        (PVOID)Context,
        (PVOID)PacketContext,
        (PVOID)Buffer,
        BufferSize);
}

static VOID
PmicGlinkUlogStateNotificationShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ ULONG Event
    )
{
    PmicGlinkUlogStateNotificationCb(Channel, (PVOID)Context, (PMICGLINK_CHANNEL_EVENT)Event);
}

static BOOLEAN
PmicGlinkUlogNotifyRxIntentReqShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ SIZE_T RequestedSize
    )
{
    return PmicGlinkUlogNotifyRxIntentReqCb(Channel, (PVOID)Context, RequestedSize);
}

static VOID
PmicGlinkUlogNotifyRxIntentShim(
    _In_opt_ GLINK_CHANNEL_CTX* Channel,
    _In_opt_ const VOID* Context,
    _In_ SIZE_T Size
    )
{
    PmicGlinkUlogNotifyRxIntentCb(Channel, (PVOID)Context, Size);
}

NTSTATUS
PmicGlinkUlog_OpenGlinkChannelUlog(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    PMIC_GLINK_OPEN_CONFIG openConfig;
    GLINK_CHANNEL_CTX* channelHandle;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&gPmicGlinkUlogTxNotificationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&gPmicGlinkUlogRxNotificationEvent, NotificationEvent, FALSE);

    status = PmicGlinkEnsureApiInterface(Context);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SUCCESS;
    }

    if (gPmicGlinkApiInterface.GLinkOpen == NULL)
    {
        return STATUS_SUCCESS;
    }

    gPmicGlinkUlogChannelHandle = NULL;
    channelHandle = NULL;
    RtlZeroMemory(&openConfig, sizeof(openConfig));
    openConfig.Transport = "SMEM";
    openConfig.RemoteSs = "lpass";
    openConfig.Name = "PMIC_LOGS_ADSP_APPS";
    openConfig.Options = 0;
    openConfig.Context = Context;
    openConfig.NotifyRx = PmicGlinkUlogRxNotificationShim;
    openConfig.NotifyTxDone = PmicGlinkUlogTxNotificationShim;
    openConfig.NotifyState = PmicGlinkUlogStateNotificationShim;
    openConfig.NotifyRxIntentReq = PmicGlinkUlogNotifyRxIntentReqShim;
    openConfig.NotifyRxIntent = PmicGlinkUlogNotifyRxIntentShim;
    openConfig.NotifyRxSigs = PmicGlinkNotifyRxSigsShim;
    openConfig.NotifyRxAbort = PmicGlinkNotifyRxAbortShim;
    openConfig.NotifyTxAbort = PmicGlinkNotifyTxAbortShim;

    status = gPmicGlinkApiInterface.GLinkOpen(&openConfig, &channelHandle);
    if (status != STATUS_SUCCESS)
    {
        gPmicGlinkUlogChannelHandle = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    gPmicGlinkUlogChannelHandle = channelHandle;
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkUlog_RetrieveRxData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG OpCode,
    _Out_opt_ PBOOLEAN ExpectedReceived
    )
{
    PMICGLINK_COMM_DATA* slot;
    const CHAR* Buffer;
    SIZE_T BufferSize;
    SIZE_T slotSize;
    ULONG packetOpCode;
    ULONG opCode;
    NTSTATUS status;
    BOOLEAN responseMatched;

    if (ExpectedReceived != NULL)
    {
        *ExpectedReceived = FALSE;
    }

    if ((Context == NULL) || (OpCode >= PMICGLINK_COMM_DATA_SLOTS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    slot = &Context->CommData[OpCode];
    Buffer = (const CHAR*)slot->Buffer;
    BufferSize = slot->Size;
    slotSize = BufferSize;
    if ((Buffer == NULL) || (BufferSize < (sizeof(ULONGLONG) + sizeof(ULONG))))
    {
        return STATUS_SUCCESS;
    }

    slot->Size = 0u;
    status = STATUS_SUCCESS;
    responseMatched = FALSE;
    opCode = OpCode;
    Context->LastUlogRxOpcode = 0;
    Context->LastUlogRxStatus = STATUS_SUCCESS;
    Context->LastUlogRxValid = FALSE;

    packetOpCode = *(const ULONG*)((const UCHAR*)Buffer + sizeof(ULONGLONG));
    if (packetOpCode != OpCode)
    {
        Context->LastUlogRxValid = FALSE;
        return STATUS_SUCCESS;
    }

    Context->LastUlogRxOpcode = opCode;
    switch (opCode)
    {
    case 24u:
    case 35u:
    {
        const UCHAR* payload;
        SIZE_T payloadSize;
        CHAR* targetBuffer;
        ULONG* targetLength;
        SIZE_T copySize;
        SIZE_T i;

        payload = (const UCHAR*)Buffer + sizeof(ULONGLONG) + sizeof(ULONG);
        payloadSize = BufferSize - (sizeof(ULONGLONG) + sizeof(ULONG));

        if (opCode == 24u)
        {
            targetBuffer = gPmicGlinkUlogData;
            targetLength = &gPmicGlinkUlogDataLength;
        }
        else
        {
            targetBuffer = gPmicGlinkUlogInitData;
            targetLength = &gPmicGlinkUlogInitDataLength;
        }

        RtlZeroMemory(targetBuffer, 8192);
        *targetLength = 0;

        copySize = payloadSize;
        for (i = 0; i < payloadSize; i++)
        {
            if (payload[i] == '\0')
            {
                copySize = i;
                break;
            }
        }

        if (copySize >= 8192u)
        {
            copySize = 8191u;
        }

        if (copySize > 0)
        {
            RtlCopyMemory(targetBuffer, payload, copySize);
        }

        targetBuffer[copySize] = '\0';
        *targetLength = (ULONG)copySize;

        RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));
        if (copySize > 0)
        {
            SIZE_T oemCopySize;

            oemCopySize = copySize;
            if (oemCopySize >= sizeof(Context->OemReceivedData))
            {
                oemCopySize = sizeof(Context->OemReceivedData) - 1u;
            }

            RtlCopyMemory(Context->OemReceivedData, payload, oemCopySize);
        }

        status = STATUS_SUCCESS;
        responseMatched = TRUE;
        break;
    }

    case 25u:
        if (BufferSize >= (sizeof(ULONGLONG) + (sizeof(ULONG) * 2u)))
        {
            ULONG statusCode;

            statusCode = *(const ULONG*)((const UCHAR*)Buffer + sizeof(ULONGLONG) + sizeof(ULONG));
            status = (statusCode == 0u) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            responseMatched = TRUE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }
        break;

    default:
        status = STATUS_SUCCESS;
        break;
    }

    if (responseMatched && (slot->Buffer != NULL) && (slotSize != 0u))
    {
        RtlZeroMemory(slot->Buffer, slotSize);
    }

    Context->LastUlogRxStatus = status;
    Context->LastUlogRxValid = responseMatched;
    if (ExpectedReceived != NULL)
    {
        *ExpectedReceived = responseMatched;
    }
    return status;
}

NTSTATUS
PmicGlinkUlog_SendData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(BufferSize) const VOID* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    GLINK_CHANNEL_CTX* channelHandle;
    NTSTATUS status;
    LARGE_INTEGER pollInterval;
    ULONG waitCount;
    ULONG waitStatus;
    ULONG waitIndex;
    ULONG waitObjectCount;
    PVOID waitObjects[2];
    KWAIT_BLOCK waitBlocks[2];
    LONG txCount;
    ULONG opCode;
    BOOLEAN matchedResponse;
    BOOLEAN expectedReceived;

    if (Buffer == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    txCount = ++gPmicGlinkUlogTxCount;

    if (Context == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Context->GlinkChannelUlogRestart
        || !Context->GlinkChannelUlogConnected)
    {
        return STATUS_RETRY;
    }

    if (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference == NULL)
    {
        return STATUS_RETRY;
    }

    if (gPmicGlinkApiInterface.GLinkTx == NULL)
    {
        return STATUS_RETRY;
    }

    channelHandle = gPmicGlinkUlogChannelHandle;
    if (channelHandle == NULL)
    {
        return STATUS_RETRY;
    }

    waitCount = 0;
    pollInterval.QuadPart = -20000ll;
    while (gPmicGlinkUlogRxInProgress == 1)
    {
        if (waitCount >= 0x3E8u)
        {
            return STATUS_RETRY;
        }

        (VOID)KeDelayExecutionThread(KernelMode, FALSE, &pollInterval);
        waitCount++;
    }

    gPmicGlinkUlogRxInProgress = 1;

    opCode = 0;
    if (BufferSize >= (sizeof(ULONGLONG) + sizeof(ULONG)))
    {
        opCode = *(const ULONG*)((const UCHAR*)Buffer + sizeof(ULONGLONG));
    }

    status = STATUS_SUCCESS;
    Context->NotificationFlag = FALSE;
    Context->LastUlogRxOpcode = 0;
    Context->LastUlogRxStatus = STATUS_SUCCESS;
    Context->LastUlogRxValid = FALSE;
    (VOID)KeClearEvent(&gPmicGlinkUlogTxNotificationEvent);
    (VOID)KeClearEvent(&gPmicGlinkUlogRxNotificationEvent);
    if (opCode < PMICGLINK_COMM_DATA_SLOTS)
    {
        PmicGlinkClearCommDataSlot(Context, opCode);
    }

    status = gPmicGlinkApiInterface.GLinkTx(
        channelHandle,
        (PVOID)(ULONG_PTR)(ULONG)txCount,
        Buffer,
        BufferSize,
        1u);
    if (status != STATUS_SUCCESS)
    {
        gPmicGlinkUlogRxInProgress = 0;
        return STATUS_UNSUCCESSFUL;
    }

    waitObjects[0] = &gPmicGlinkUlogTxNotificationEvent;
    waitObjects[1] = &gPmicGlinkUlogRxNotificationEvent;
    waitObjectCount = RTL_NUMBER_OF(waitObjects);

    matchedResponse = FALSE;
    waitCount = 0;
    pollInterval.QuadPart = -200000ll;
    while (waitCount < 50u)
    {
        waitStatus = KeWaitForMultipleObjects(
            waitObjectCount,
            waitObjects,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            &pollInterval,
            waitBlocks);
        if (waitStatus < (STATUS_WAIT_0 + waitObjectCount))
        {
            waitIndex = waitStatus - STATUS_WAIT_0;
            (VOID)KeClearEvent((PKEVENT)waitObjects[waitIndex]);
            if (waitIndex == 0u)
            {
                break;
            }
        }

        if ((waitStatus < (STATUS_WAIT_0 + waitObjectCount)) && (waitIndex == 1u))
        {
            expectedReceived = FALSE;
            status = PmicGlinkUlog_RetrieveRxData(Context, opCode, &expectedReceived);
            if (expectedReceived)
            {
                matchedResponse = TRUE;
                gPmicGlinkUlogRxInProgress = 0;
                break;
            }
        }

        waitCount++;
    }

    if (!matchedResponse && (waitCount >= 50u))
    {
        gPmicGlinkUlogRxInProgress = 0;
    }

    if (!NT_SUCCESS(status))
    {
        gPmicGlinkUlogRxInProgress = 0;
        return status;
    }

    waitCount = 0;
    while (waitCount < 50u)
    {
        waitStatus = KeWaitForMultipleObjects(
            waitObjectCount,
            waitObjects,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            &pollInterval,
            waitBlocks);
        if (waitStatus < (STATUS_WAIT_0 + waitObjectCount))
        {
            waitIndex = waitStatus - STATUS_WAIT_0;
            (VOID)KeClearEvent((PKEVENT)waitObjects[waitIndex]);
            if (waitIndex == 0u)
            {
                waitCount++;
                continue;
            }
        }

        if ((waitStatus < (STATUS_WAIT_0 + waitObjectCount)) && (waitIndex == 1u))
        {
            expectedReceived = FALSE;
            status = PmicGlinkUlog_RetrieveRxData(Context, opCode, &expectedReceived);
            if (expectedReceived)
            {
                matchedResponse = TRUE;
                gPmicGlinkUlogRxInProgress = 0;
                break;
            }
        }

        waitCount++;
    }

    if (!matchedResponse)
    {
        status = STATUS_TIMEOUT;
    }

    gPmicGlinkUlogRxInProgress = 0;
    return status;
}

static NTSTATUS
PmicGlinkDeviceCreate(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    return PmicGlinkEvtDeviceAdd(Driver, DeviceInit);
}

static NTSTATUS
PmicGlinkOnDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    return PmicGlinkDeviceCreate(Driver, DeviceInit);
}

static VOID
PmicGlinkOnIoQueueContextDestroy(
    _In_ WDFOBJECT Object
    )
{
    PPMICGLINK_QUEUE_CONTEXT queueContext;

    queueContext = PmicGlinkGetQueueContext(Object);
    if ((queueContext != NULL) && (queueContext->Buffer != NULL))
    {
        ExFreePoolWithTag(queueContext->Buffer, 0);
        queueContext->Buffer = NULL;
    }
}

static VOID
PmicGlinkOnIoQueueTimer(
    _In_ WDFTIMER Timer
    )
{
    WDFOBJECT parent;
    PPMICGLINK_QUEUE_CONTEXT queueContext;
    WDFREQUEST request;
    NTSTATUS status;

    parent = WdfTimerGetParentObject(Timer);
    queueContext = PmicGlinkGetQueueContext(parent);
    if (queueContext == NULL)
    {
        return;
    }

    request = queueContext->CurrentRequest;
    if (request == NULL)
    {
        return;
    }

    status = WdfRequestUnmarkCancelable(request);
    if (status != STATUS_CANCELLED)
    {
        queueContext->CurrentRequest = NULL;
        WdfRequestComplete(request, queueContext->CurrentStatus);
    }
}

static NTSTATUS
PmicGlinkQueueTimerCreate(
    _Out_ WDFTIMER* Timer,
    _In_ ULONG PeriodMs,
    _In_ WDFQUEUE Queue
    )
{
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;

    if ((Timer == NULL) || (Queue == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, PmicGlinkOnIoQueueTimer, PeriodMs);
    timerConfig.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = Queue;

    return WdfTimerCreate(&timerConfig, &timerAttributes, Timer);
}

NTSTATUS
PmicGlinkQueueInitialize(
    _In_ WDFDEVICE Device
    )
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDFQUEUE queue;
    PPMICGLINK_QUEUE_CONTEXT queueContext;
    ULONG timerPeriodMs;

    if (Device == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = PmicGlinkEvtIoDeviceControl;
    queueConfig.EvtIoInternalDeviceControl = PmicGlinkEvtIoInternalDeviceControl;
    queueConfig.EvtIoDefault = PmicGlinkEvtDmfDeviceModulesAdd;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, PMICGLINK_QUEUE_CONTEXT);
    queueAttributes.EvtDestroyCallback = PmicGlinkOnIoQueueContextDestroy;

    queue = NULL;
    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        &queueAttributes,
        &queue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    queueContext = PmicGlinkGetQueueContext(queue);
    if (queueContext == NULL)
    {
        return status;
    }

    queueContext->Buffer = NULL;
    queueContext->Length = 0;
    queueContext->Timer = NULL;
    queueContext->CurrentRequest = NULL;
    queueContext->CurrentStatus = STATUS_INVALID_DEVICE_REQUEST;
    queueContext->DeviceContext = PmicGlinkGetDeviceContext(Device);

    timerPeriodMs = PMICGLINK_QUEUE_TIMER_PERIOD_MS;
    status = PmicGlinkQueueTimerCreate(&queueContext->Timer, timerPeriodMs, queue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;
}

static VOID
PmicGlinkOnDriverCleanup(
    _In_ WDFOBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
}

static VOID
PmicGlinkOnDriverUnload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
}

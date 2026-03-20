#include "pmicglink8550_internal.h"

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

typedef struct _PMIC_GLINK_TRACER_PKT_INIT_CONFIG
{
    UCHAR QdssTraceEnable;
    USHORT ClientEventConfig;
    ULONG GlinkEventConfig;
    ULONG PacketPrivate[3];
    ULONG PacketPrivateLength;
} PMIC_GLINK_TRACER_PKT_INIT_CONFIG;

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
    NTSTATUS(*TracerPktInit)(_In_opt_ PVOID TracerContext, _In_ ULONG ClientId, _In_opt_ PMIC_GLINK_TRACER_PKT_INIT_CONFIG* InitConfig);
    NTSTATUS(*TracerPktSetEventCfg)(_In_opt_ PVOID TracerContext, _In_ USHORT ClientEventConfig, _In_ ULONG GlinkEventConfig);
    NTSTATUS(*TracerPktLogEvent)(_In_opt_ PVOID TracerContext, _In_ USHORT EventId);
    ULONG(*TracerPktCalcHexDumpSize)(_In_opt_ PVOID TracerContext, _In_ ULONG DataLength);
    NTSTATUS(*TracerPktHexDump)(_In_opt_ PVOID TracerContext, _In_ ULONG Flags, _In_reads_bytes_opt_(DataLength) const VOID* Data, _In_ ULONG DataLength);
} PMIC_GLINK_API_INTERFACE;

C_ASSERT(sizeof(PMIC_GLINK_API_INTERFACE) == 192u);

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
    0xdc424aec,
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
static ULONGLONG gPmicGlinkLastChannelRecoverAttemptMsec;
static ULONGLONG gPmicGlinkLastBattMiniAttachAttemptMsec;
static UNICODE_STRING gPmicGlinkApiInterfaceSymbolicLinkName;
static PWSTR gPmicGlinkApiInterfaceSymbolicLinkBuffer;
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
#define PMICGLINK_GLINK_QUERY_ACCESS_MASK 0x001F0000u
#define PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE 0x800A2008u
#define PMICGLINK_BATTMINI_IOCTL_NOTIFY_STATUS 0x800A0FB0u
#define PMICGLINK_BATTMINI_NOTIFY_TIMEOUT_100NS (-100000000ll)
#define PMICGLINK_USBN_NOTIFY_IOCTL 0x0032C004u
#define PMICGLINK_USBN_NOTIFY_SIG_IN 0x4E42535549696541ull
#define PMICGLINK_USBN_NOTIFY_SIG_OUT 0x426F6541u
#define PMICGLINK_TRACE_LEVEL 31
#define PMICGLINK_BC_BATTERY_STATUS_GET_OPCODE 0x30u
#define PMICGLINK_BC_PROP_BATT_CAPACITY 4u
#define PMICGLINK_BC_ADSP_DEBUG_OPCODE 0x100u
#define PMICGLINK_BC_DEBUG_MSG_OPCODE 0x101u
#define PMICGLINK_BC_DEBUG_MSG_MAX_CHARS 256u
#define PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS 3000u
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
static VOID PmicGlinkSetApiInterfaceSymbolicLink(_In_opt_ const UNICODE_STRING* SymbolicLinkName);
static NTSTATUS PmicGlinkEnsureApiInterfaceMemory(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
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
static NTSTATUS PmicGlinkTryAttachBattMiniNoPnp(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkTryAttachBattMiniFromIoctl(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ PCSTR Reason);
static NTSTATUS PmicGlinkSendDriverRequest(_In_ WDFIOTARGET IoTarget, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _Out_opt_ SIZE_T* BytesReturned);
static NTSTATUS PmicGlinkSendDriverRequestWithTimeout(_In_ WDFIOTARGET IoTarget, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize, _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _In_ LONGLONG Timeout100ns, _Out_opt_ SIZE_T* BytesReturned);
static NTSTATUS PmicGlinkSendDriverRequestAsync(_In_ WDFIOTARGET IoTarget, _In_ ULONG IoControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize);
static VOID PmicGlinkCloseIoTargetIfOpen(_Inout_ WDFIOTARGET* IoTarget);
static VOID PmicGlinkClearGlinkAttachment(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkClearAbdAttachment(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN UnregisterConnections);
static VOID PmicGlinkClearBattMiniAttachment(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkUnregisterPnPNotificationEntry(_Inout_ PVOID* Entry);
static NTSTATUS PmicGlinkAbdUpdateConnections(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN Register);
static VOID PmicGlinkInitAbdConnections(VOID);
static NTSTATUS PmicGlinkEvaluateAbdConnectionCount(_In_ WDFDEVICE Device);
static NTSTATUS PmicGlinkNotifyLinkStateAcpi(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG LinkState);
static NTSTATUS PmicGlinkEvaluateAcpiMethodInteger(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_reads_(4) const CHAR MethodName[4], _In_ ULONG Value);
static NTSTATUS PmicGlinkEvaluateAcpiMethodBuffer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_reads_(4) const CHAR MethodName[4], _In_reads_bytes_(DataSize) const VOID* Data, _In_ ULONG DataSize);
static NTSTATUS PmicGlinkNotifyUsbnIoctl(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
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
static VOID PmicGlinkBootBatteryRefreshWorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkLegacyBatteryRefreshTimerFunction(_In_ WDFTIMER Timer);
static NTSTATUS PmicGlinkEnsureLegacyBatteryRefreshTimer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkStartLegacyBatteryRefreshTimer(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ PCSTR Reason);
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

static ULONG
PmicGlinkGetLegacyReportedFullCapacity(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacityHint
    );

static UCHAR
PmicGlinkComputeLegacyBattPercentage(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacity
    );

static ULONG
PmicGlinkNormalizeLegacyRemainingCapacity(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacity
    );

static BOOLEAN
PmicGlinkTryConvertModernSocX100(
    _In_ ULONG RawValue,
    _Out_ PULONG SocX100
    );

static BOOLEAN
PmicGlinkTryExtractModernSocFromDebugMsg(
    _In_reads_bytes_(TextLength) const CHAR* Text,
    _In_ SIZE_T TextLength,
    _Out_ PULONG RawValue,
    _Out_ PULONG SocX100
    );

static VOID
PmicGlinkApplyModernSocToLegacy(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG SocX100
    );

static NTSTATUS
PmicGlinkQueryModernBatterySoc(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    );

static VOID
PmicGlinkRefreshModernBatterySoc(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONGLONG NowMsec,
    _In_ BOOLEAN Force
    );

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

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: build tag 2026-03-16 commit decomp-criteria-fallback\n");

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

    PmicGlinkClearGlinkAttachment(context);
    PmicGlinkResetCommDataSlots(context, TRUE);
    PmicGlinkSetApiInterfaceSymbolicLink(NULL);

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
    if (context == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    context->Hibernate = FALSE;

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
    PmicGlinkStartLegacyBatteryRefreshTimer(context, "D0Entry");
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
    if (context->LegacyBatteryRefreshTimer != NULL)
    {
        (VOID)WdfTimerStop(context->LegacyBatteryRefreshTimer, TRUE);
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
    PPMIC_GLINK_DEVICE_CONTEXT context;
    WDFIOTARGET ioTarget;

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    (VOID)WdfIoTargetStart(ioTarget);
    context = PmicGlinkGetDeviceContext(Device);
    if (context != NULL)
    {
        context->Hibernate = FALSE;
        PmicGlinkStartLegacyBatteryRefreshTimer(context, "SelfManagedIoInit");
    }
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoRestart(
    _In_ WDFDEVICE Device
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    WDFIOTARGET ioTarget;

    ioTarget = WdfDeviceGetIoTarget(Device);
    if (ioTarget == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    (VOID)WdfIoTargetStart(ioTarget);
    context = PmicGlinkGetDeviceContext(Device);
    if (context != NULL)
    {
        context->Hibernate = FALSE;
        PmicGlinkStartLegacyBatteryRefreshTimer(context, "SelfManagedIoRestart");
    }
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
PmicGlinkClearGlinkAttachment(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if (Context == NULL)
    {
        return;
    }

    PmicGlinkCloseIoTargetIfOpen(&Context->GlinkIoTarget);
    Context->GlinkDeviceLoaded = FALSE;
    Context->AllReqIntfArrived = FALSE;
    Context->GlinkLinkStateUp = FALSE;
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
    PUNICODE_STRING symbolicLinkName;
    BOOLEAN arrival;
    NTSTATUS status;
    ULONG currentState;

    if (Context == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
    symbolicLinkName = notification->SymbolicLinkName;

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
            WDF_IO_TARGET_OPEN_PARAMS openParams;

            if (deviceContext->GlinkDeviceLoaded)
            {
                return STATUS_SUCCESS;
            }

            if (deviceContext->GlinkIoTarget == NULL)
            {
                status = WdfIoTargetCreate(
                    deviceContext->Device,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &deviceContext->GlinkIoTarget);
            }

            if (!NT_SUCCESS(status) || (deviceContext->GlinkIoTarget == NULL))
            {
                return status;
            }

            if ((symbolicLinkName == NULL) || (symbolicLinkName->Buffer == NULL))
            {
                return STATUS_INVALID_PARAMETER;
            }

            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: glink_iface arrival nameLen=%hu target=%p\n",
                symbolicLinkName->Length,
                deviceContext->GlinkIoTarget);

            WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                &openParams,
                symbolicLinkName,
                PMICGLINK_GLINK_QUERY_ACCESS_MASK);
            openParams.ShareAccess = FILE_SHARE_READ;
            status = WdfIoTargetOpen(deviceContext->GlinkIoTarget, &openParams);
            if (!NT_SUCCESS(status))
            {
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: glink_iface open failed status=0x%08lx\n",
                    (ULONG)status);
                PmicGlinkCloseIoTargetIfOpen(&deviceContext->GlinkIoTarget);
                return status;
            }

            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: glink_iface open ok target=%p\n",
                deviceContext->GlinkIoTarget);

            deviceContext->GlinkDeviceLoaded = TRUE;
            PmicGlinkSetApiInterfaceSymbolicLink(symbolicLinkName);
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

            PmicGlinkClearGlinkAttachment(deviceContext);
            PmicGlinkSetApiInterfaceSymbolicLink(NULL);
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

                    if ((symbolicLinkName == NULL) || (symbolicLinkName->Buffer == NULL))
                    {
                        return STATUS_INVALID_PARAMETER;
                    }

                    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                        &openParams,
                        symbolicLinkName,
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
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini iface event=%s loaded=%u target=%p nameLen=%hu\n",
            arrival ? "arrival" : "removal",
            deviceContext->BattMiniDeviceLoaded ? 1u : 0u,
            deviceContext->BattMiniIoTarget,
            ((symbolicLinkName != NULL) ? symbolicLinkName->Length : 0u));

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

                    if ((symbolicLinkName == NULL) || (symbolicLinkName->Buffer == NULL))
                    {
                        status = STATUS_INVALID_PARAMETER;
                        goto BattMiniExit;
                    }

                    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                        &openParams,
                        symbolicLinkName,
                        PMICGLINK_GLINK_QUERY_ACCESS_MASK);
                    openParams.ShareAccess = FILE_SHARE_READ;
                    status = WdfIoTargetOpen(deviceContext->BattMiniIoTarget, &openParams);
                    if (NT_SUCCESS(status))
                    {
                        deviceContext->BattMiniDeviceLoaded = TRUE;
                        DbgPrintEx(
                            DPFLTR_IHVDRIVER_ID,
                            PMICGLINK_TRACE_LEVEL,
                            "pmicglink: battmini iface open ok target=%p\n",
                            deviceContext->BattMiniIoTarget);
                        status = PmicGlinkEnsureBclCriticalCallback(deviceContext);
                        if (NT_SUCCESS(status) && deviceContext->GlinkChannelConnected)
                        {
                            (VOID)PmicGlinkCreateDeviceWorkItem(
                                deviceContext,
                                PmicGlinkBootBatteryRefreshWorkItem);
                        }
                    }
                    else
                    {
                        DbgPrintEx(
                            DPFLTR_IHVDRIVER_ID,
                            PMICGLINK_TRACE_LEVEL,
                            "pmicglink: battmini iface open failed status=0x%08lx\n",
                            (ULONG)status);
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
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini iface removed target=%p\n",
                deviceContext->BattMiniIoTarget);
        }

BattMiniExit:
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
        PmicGlinkClearGlinkAttachment(Context);

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

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini iface notify registered entry=%p status=0x%08lx\n",
            Context->BattMiniNotificationEntry,
            (ULONG)status);
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
    Context->GlinkApiMemory = NULL;
    Context->GlinkApiInterfaceBuffer = NULL;
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

    Context->LegacyChargeStatus.power_state = 1;
    Context->LegacyChargeStatus.capacity = 0xFFFFFFFF;
    Context->LegacyChargeStatus.voltage = 0xFFFFFFFF;
    Context->LegacyChargeStatus.rate = (LONG)0x80000000;
    Context->LegacyChargeStatus.charging_source = 0;
    Context->LegacyChargeStatusRawCapacity = 0xFFFFFFFF;
    Context->LegacyChargeStatusAux = 0xFFFFFFFF;
    Context->ModernSocX100 = 0xFFFFFFFF;
    Context->ModernSocRaw = 0xFFFFFFFF;
    Context->ModernSocValid = FALSE;

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
    Context->LegacyLastModernSocQueryMsec = 0;
    Context->LegacyLastAdspBatteryNotifyMsec = 0;
    Context->LegacyLastTestInfoRequestType = 0;
    Context->LegacyReserved = 0;

    Context->ModernStandbyState = 0;
    Context->ModernStandbyCallbackObject = NULL;
    Context->ModernStandbyCallbackHandle = NULL;
    Context->BclCriticalCallbackObject = NULL;
    Context->BclCriticalCallbackEnabled = FALSE;
    Context->GlinkNotificationEntry = NULL;
    Context->GlinkIoTarget = NULL;
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
    Context->LegacyBatteryRefreshTimer = NULL;
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
    Context->LegacyLastAdspBatteryNotifyMsec = 0;
    Context->LegacyBatteryRefreshTimer = NULL;
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
PmicGlinkSetApiInterfaceSymbolicLink(
    _In_opt_ const UNICODE_STRING* SymbolicLinkName
    )
{
    PWSTR newBuffer;
    SIZE_T newSize;

    newBuffer = NULL;
    newSize = 0u;
    if ((SymbolicLinkName != NULL)
        && (SymbolicLinkName->Buffer != NULL)
        && (SymbolicLinkName->Length != 0u))
    {
        newSize = (SIZE_T)SymbolicLinkName->Length + sizeof(WCHAR);
        newBuffer = (PWSTR)ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            newSize,
            PMICGLINK_POOLTAG_COMMDATA);
        if (newBuffer != NULL)
        {
            RtlZeroMemory(newBuffer, newSize);
            RtlCopyMemory(newBuffer, SymbolicLinkName->Buffer, SymbolicLinkName->Length);
        }
    }

    if (gPmicGlinkApiInterfaceSymbolicLinkBuffer != NULL)
    {
        ExFreePoolWithTag(gPmicGlinkApiInterfaceSymbolicLinkBuffer, PMICGLINK_POOLTAG_COMMDATA);
        gPmicGlinkApiInterfaceSymbolicLinkBuffer = NULL;
    }

    RtlZeroMemory(&gPmicGlinkApiInterfaceSymbolicLinkName, sizeof(gPmicGlinkApiInterfaceSymbolicLinkName));
    if (newBuffer != NULL)
    {
        gPmicGlinkApiInterfaceSymbolicLinkBuffer = newBuffer;
        gPmicGlinkApiInterfaceSymbolicLinkName.Buffer = newBuffer;
        gPmicGlinkApiInterfaceSymbolicLinkName.Length = SymbolicLinkName->Length;
        gPmicGlinkApiInterfaceSymbolicLinkName.MaximumLength = (USHORT)newSize;
    }
}

static NTSTATUS
PmicGlinkEnsureApiInterfaceMemory(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    SIZE_T allocatedSize;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((Context->GlinkApiMemory != NULL) && (Context->GlinkApiInterfaceBuffer != NULL))
    {
        allocatedSize = 0u;
        (VOID)WdfMemoryGetBuffer(Context->GlinkApiMemory, &allocatedSize);
        if (allocatedSize >= sizeof(gPmicGlinkApiInterface))
        {
            RtlZeroMemory(Context->GlinkApiInterfaceBuffer, sizeof(gPmicGlinkApiInterface));
            return STATUS_SUCCESS;
        }

        WdfObjectDelete(Context->GlinkApiMemory);
        Context->GlinkApiMemory = NULL;
        Context->GlinkApiInterfaceBuffer = NULL;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Context->Device;
    status = WdfMemoryCreate(
        &attributes,
        NonPagedPoolNx,
        PMICGLINK_POOLTAG_COMMDATA,
        sizeof(gPmicGlinkApiInterface),
        &Context->GlinkApiMemory,
        &Context->GlinkApiInterfaceBuffer);
    if (!NT_SUCCESS(status))
    {
        Context->GlinkApiMemory = NULL;
        Context->GlinkApiInterfaceBuffer = NULL;
        return status;
    }

    RtlZeroMemory(Context->GlinkApiInterfaceBuffer, sizeof(gPmicGlinkApiInterface));
    return STATUS_SUCCESS;
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
    WDFIOTARGET ioTarget;
    ULONG sizeIndex;
    ULONG versionIndex;
    static const USHORT querySizes[] = {
        (USHORT)sizeof(gPmicGlinkApiInterface),
        200u,
        208u,
        216u,
        224u,
        232u,
        240u,
        184u,
        176u,
        168u,
        160u
    };
    static const USHORT queryVersions[] = { 1u, 2u, 3u, 4u, 0u };

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (gPmicGlinkApiInterface.InterfaceHeader.InterfaceReference != NULL)
    {
        return STATUS_SUCCESS;
    }

    PmicGlinkResetApiInterface();
    status = PmicGlinkEnsureApiInterfaceMemory(Context);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ioTarget = WdfDeviceGetIoTarget(Context->Device);
    if (Context->GlinkIoTarget != NULL)
    {
        ioTarget = Context->GlinkIoTarget;
    }

    if (ioTarget != NULL)
    {
        for (versionIndex = 0u; versionIndex < RTL_NUMBER_OF(queryVersions); versionIndex++)
        {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: ensure_api iotarget_query probing version=%hu\n",
                queryVersions[versionIndex]);

            for (sizeIndex = 0u; sizeIndex < RTL_NUMBER_OF(querySizes); sizeIndex++)
            {
                PmicGlinkResetApiInterface();
                RtlZeroMemory(Context->GlinkApiInterfaceBuffer, sizeof(gPmicGlinkApiInterface));
                status = WdfIoTargetQueryForInterface(
                    ioTarget,
                    &GUID_GLINK_API_INTERFACE,
                    (PINTERFACE)Context->GlinkApiInterfaceBuffer,
                    querySizes[sizeIndex],
                    queryVersions[versionIndex],
                    NULL);
                if (NT_SUCCESS(status))
                {
                    RtlCopyMemory(
                        &gPmicGlinkApiInterface,
                        Context->GlinkApiInterfaceBuffer,
                        sizeof(gPmicGlinkApiInterface));
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: ensure_api iotarget_query ok version=%hu size=%lu\n",
                        queryVersions[versionIndex],
                        querySizes[sizeIndex]);
                    return STATUS_SUCCESS;
                }
            }
        }

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: ensure_api iotarget_query failed status=0x%08lx\n",
            (ULONG)status);
    }

    for (versionIndex = 0u; versionIndex < RTL_NUMBER_OF(queryVersions); versionIndex++)
    {
        for (sizeIndex = 0u; sizeIndex < RTL_NUMBER_OF(querySizes); sizeIndex++)
        {
            PmicGlinkResetApiInterface();
            RtlZeroMemory(Context->GlinkApiInterfaceBuffer, sizeof(gPmicGlinkApiInterface));
            status = WdfFdoQueryForInterface(
                Context->Device,
                &GUID_GLINK_API_INTERFACE,
                (PINTERFACE)Context->GlinkApiInterfaceBuffer,
                querySizes[sizeIndex],
                queryVersions[versionIndex],
                NULL);
            if (NT_SUCCESS(status))
            {
                RtlCopyMemory(
                    &gPmicGlinkApiInterface,
                    Context->GlinkApiInterfaceBuffer,
                    sizeof(gPmicGlinkApiInterface));
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: ensure_api fdo_query ok version=%hu size=%lu\n",
                    queryVersions[versionIndex],
                    querySizes[sizeIndex]);
                return STATUS_SUCCESS;
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        WDFIOTARGET namedIoTarget;
        WDF_IO_TARGET_OPEN_PARAMS openParams;

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: ensure_api fdo_query failed status=0x%08lx nameLen=%hu\n",
            (ULONG)status,
            gPmicGlinkApiInterfaceSymbolicLinkName.Length);

        if ((gPmicGlinkApiInterfaceSymbolicLinkName.Buffer == NULL)
            || (gPmicGlinkApiInterfaceSymbolicLinkName.Length == 0u))
        {
            return status;
        }

        namedIoTarget = NULL;
        status = WdfIoTargetCreate(Context->Device, WDF_NO_OBJECT_ATTRIBUTES, &namedIoTarget);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
            &openParams,
            &gPmicGlinkApiInterfaceSymbolicLinkName,
            PMICGLINK_GLINK_QUERY_ACCESS_MASK);
        openParams.ShareAccess = FILE_SHARE_READ;

        status = WdfIoTargetOpen(namedIoTarget, &openParams);
        if (!NT_SUCCESS(status))
        {
            WdfObjectDelete(namedIoTarget);
            return status;
        }

        for (versionIndex = 0u; versionIndex < RTL_NUMBER_OF(queryVersions); versionIndex++)
        {
            for (sizeIndex = 0u; sizeIndex < RTL_NUMBER_OF(querySizes); sizeIndex++)
            {
                PmicGlinkResetApiInterface();
                RtlZeroMemory(Context->GlinkApiInterfaceBuffer, sizeof(gPmicGlinkApiInterface));
                status = WdfIoTargetQueryForInterface(
                    namedIoTarget,
                    &GUID_GLINK_API_INTERFACE,
                    (PINTERFACE)Context->GlinkApiInterfaceBuffer,
                    querySizes[sizeIndex],
                    queryVersions[versionIndex],
                    NULL);
                if (NT_SUCCESS(status))
                {
                    RtlCopyMemory(
                        &gPmicGlinkApiInterface,
                        Context->GlinkApiInterfaceBuffer,
                        sizeof(gPmicGlinkApiInterface));
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: ensure_api named_iotarget_query ok version=%hu size=%lu\n",
                        queryVersions[versionIndex],
                        querySizes[sizeIndex]);
                    break;
                }
            }

            if (NT_SUCCESS(status))
            {
                break;
            }
        }

        WdfIoTargetClose(namedIoTarget);
        WdfObjectDelete(namedIoTarget);
        if (!NT_SUCCESS(status))
        {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: ensure_api named_iotarget_query failed status=0x%08lx\n",
                (ULONG)status);
            return status;
        }

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: ensure_api named_iotarget_query ok size=%lu\n",
            (ULONG)sizeof(gPmicGlinkApiInterface));
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
        || (OpCode == 259u)
        || (OpCode == PMICGLINK_BC_ADSP_DEBUG_OPCODE)
        || (OpCode == PMICGLINK_BC_DEBUG_MSG_OPCODE));
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
    static const ULONG notificationOps[] = {
        PMICGLINK_BC_ADSP_DEBUG_OPCODE,
        PMICGLINK_BC_DEBUG_MSG_OPCODE,
        19u,
        7u,
        22u,
        259u,
        130u
    };
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

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: open_channel attempt allReq=%u glinkLoaded=%u linkUp=%u restart=%u hibernate=%u handle=%p\n",
        Context->AllReqIntfArrived ? 1u : 0u,
        Context->GlinkDeviceLoaded ? 1u : 0u,
        Context->GlinkLinkStateUp ? 1u : 0u,
        Context->GlinkChannelRestart ? 1u : 0u,
        Context->Hibernate ? 1u : 0u,
        gPmicGlinkMainChannelHandle);

    status = PmicGlinkEnsureApiInterface(Context);
    if (!NT_SUCCESS(status))
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: open_channel ensure_api failed status=0x%08lx\n",
            (ULONG)status);
        return STATUS_SUCCESS;
    }

    if (gPmicGlinkApiInterface.GLinkOpen == NULL)
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: open_channel GLinkOpen is NULL\n");
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
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: open_channel GLinkOpen failed status=0x%08lx\n",
            (ULONG)status);
        gPmicGlinkMainChannelHandle = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    gPmicGlinkMainChannelHandle = channelHandle;
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: open_channel success handle=%p\n",
        channelHandle);
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
PmicGlinkBootBatteryRefreshWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    BATT_MNGR_GET_BATT_INFO battInfoRequest;
    ULONGLONG nowMsec;
    NTSTATUS battIdStatus;
    NTSTATUS battInfoStatus;
    NTSTATUS chgStatus;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if ((context != NULL) && context->GlinkChannelConnected)
    {
        gPmicGlinkLastBattIdQueryMsec = 0;
        gPmicGlinkLastChargeStatusQueryMsec = 0;
        gPmicGlinkLastBattInfoQueryMsec = 0;
        context->LegacyLastModernSocQueryMsec = 0;
        context->LegacyStatusNotificationPending = TRUE;
        context->LegacyStateChangePending = TRUE;
        context->Notify = TRUE;

        PmicGlinkTryAttachBattMiniFromIoctl(context, "BOOT_REFRESH");
        battIdStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_BATT_ID,
            NULL,
            0u);

        RtlZeroMemory(&battInfoRequest, sizeof(battInfoRequest));
        battInfoRequest.batt_id = context->LegacyBattId.batt_id;
        battInfoRequest.rate_of_drain = context->LegacyChargeStatus.rate;
        battInfoStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_BATT_INFO,
            (PUCHAR)&battInfoRequest,
            sizeof(battInfoRequest));
        chgStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_CHARGER_STATUS,
            NULL,
            0u);
        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        PmicGlinkRefreshModernBatterySoc(context, nowMsec, TRUE);

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: boot_refresh connected=%u battmini=%u target=%p battId=0x%08lx info=0x%08lx chg=0x%08lx pct=%u state=0x%08lx\n",
            context->GlinkChannelConnected ? 1u : 0u,
            context->BattMiniDeviceLoaded ? 1u : 0u,
            context->BattMiniIoTarget,
            (ULONG)battIdStatus,
            (ULONG)battInfoStatus,
            (ULONG)chgStatus,
            context->LegacyBattPercentage,
            context->LegacyChargeStatus.power_state);

        PmicGlinkNotifyBattMiniStatusFromGlink(context, 0x80u);
    }

    WdfObjectDelete(WorkItem);
}

static NTSTATUS
PmicGlinkEnsureLegacyBatteryRefreshTimer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->LegacyBatteryRefreshTimer != NULL)
    {
        return STATUS_SUCCESS;
    }

    WDF_TIMER_CONFIG_INIT_PERIODIC(
        &timerConfig,
        PmicGlinkLegacyBatteryRefreshTimerFunction,
        PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS);
    timerConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = Context->Device;
    timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        &Context->LegacyBatteryRefreshTimer);
    if (!NT_SUCCESS(status))
    {
        Context->LegacyBatteryRefreshTimer = NULL;
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer create failed status=0x%08lx\n",
            (ULONG)status);
    }
    else
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer created periodMs=%u\n",
            PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS);
    }

    return status;
}

static VOID
PmicGlinkStartLegacyBatteryRefreshTimer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ PCSTR Reason
    )
{
    NTSTATUS status;

    if (Context == NULL)
    {
        return;
    }

    status = PmicGlinkEnsureLegacyBatteryRefreshTimer(Context);
    if (!NT_SUCCESS(status) || (Context->LegacyBatteryRefreshTimer == NULL))
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer start failed reason=%s status=0x%08lx timer=%p\n",
            (Reason != NULL) ? Reason : "unknown",
            (ULONG)status,
            Context->LegacyBatteryRefreshTimer);
        return;
    }

    (VOID)WdfTimerStart(
        Context->LegacyBatteryRefreshTimer,
        -((LONGLONG)PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS * 10000ll));
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: batt_fallback_timer started reason=%s timer=%p connected=%u hibernate=%u\n",
        (Reason != NULL) ? Reason : "unknown",
        Context->LegacyBatteryRefreshTimer,
        Context->GlinkChannelConnected ? 1u : 0u,
        Context->Hibernate ? 1u : 0u);
}

static VOID
PmicGlinkLegacyBatteryRefreshTimerFunction(
    _In_ WDFTIMER Timer
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONGLONG nowMsec;

    parentObject = WdfTimerGetParentObject(Timer);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);
    if ((context == NULL) || !context->GlinkChannelConnected || context->Hibernate)
    {
        return;
    }

    nowMsec = PmicGlink_Helper_get_rel_time_msec();
    if ((context->LegacyLastAdspBatteryNotifyMsec != 0)
        && (nowMsec >= context->LegacyLastAdspBatteryNotifyMsec)
        && ((nowMsec - context->LegacyLastAdspBatteryNotifyMsec)
            < (ULONGLONG)PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS))
    {
        return;
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: batt_fallback_timer ping lastNotify=%I64u now=%I64u connected=%u battmini=%u\n",
        context->LegacyLastAdspBatteryNotifyMsec,
        nowMsec,
        context->GlinkChannelConnected ? 1u : 0u,
        context->BattMiniDeviceLoaded ? 1u : 0u);

    gPmicGlinkLastChargeStatusQueryMsec = 0;
    gPmicGlinkLastBattInfoQueryMsec = 0;
    context->LegacyLastModernSocQueryMsec = 0;
    context->LegacyStatusNotificationPending = TRUE;
    context->LegacyStateChangePending = TRUE;
    context->Notify = TRUE;
    PmicGlinkTryAttachBattMiniFromIoctl(context, "TIMER");
    PmicGlinkNotifyBattMiniStatusFromGlink(context, 0x80u);
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

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: state_notify event=%lu ctx=%p handle=%p connected=%u restart=%u linkUp=%u\n",
        (ULONG)Event,
        Context,
        channelHandle,
        Context->GlinkChannelConnected ? 1u : 0u,
        Context->GlinkChannelRestart ? 1u : 0u,
        Context->GlinkLinkStateUp ? 1u : 0u);

    switch (Event)
    {
    case PmicGlinkChannelConnected:
        Context->GlinkChannelFirstConnect = TRUE;
        Context->GlinkChannelConnected = TRUE;
        Context->GlinkChannelRestart = FALSE;
        Context->LegacyLastAdspBatteryNotifyMsec = 0;
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
        PmicGlinkStartLegacyBatteryRefreshTimer(Context, "ChannelConnected");
        (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
        (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkBootBatteryRefreshWorkItem);
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: state_notify connected done handle=%p rxIntent=%lu\n",
            gPmicGlinkMainChannelHandle,
            Context->GlinkRxIntent);
        break;

    case PmicGlinkChannelLocalDisconnected:
        Context->GlinkChannelConnected = FALSE;
        if (Context->LegacyBatteryRefreshTimer != NULL)
        {
            (VOID)WdfTimerStop(Context->LegacyBatteryRefreshTimer, FALSE);
        }
        (VOID)KeClearEvent(&gPmicGlinkConnectedEvent);
        (VOID)KeSetEvent(&gPmicGlinkLocalDisconnectedEvent, IO_NO_INCREMENT, FALSE);
        if (Context->GlinkChannelRestart && Context->GlinkLinkStateUp)
        {
            (VOID)PmicGlink_OpenGlinkChannel(Context);
        }
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: state_notify local_disconnected restart=%u linkUp=%u\n",
            Context->GlinkChannelRestart ? 1u : 0u,
            Context->GlinkLinkStateUp ? 1u : 0u);
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
            if (Context->LegacyBatteryRefreshTimer != NULL)
            {
                (VOID)WdfTimerStop(Context->LegacyBatteryRefreshTimer, FALSE);
            }
            (VOID)KeClearEvent(&gPmicGlinkConnectedEvent);
            (VOID)KeSetEvent(&gPmicGlinkRemoteDisconnectedEvent, IO_NO_INCREMENT, FALSE);
            (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
        }
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: state_notify remote_disconnected restart=%u handle=%p\n",
            Context->GlinkChannelRestart ? 1u : 0u,
            gPmicGlinkMainChannelHandle);
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

static NTSTATUS
PmicGlinkTryAttachBattMiniNoPnp(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    NTSTATUS openStatus;
    PWSTR interfaceList;
    PWSTR currentInterface;
    UNICODE_STRING targetName;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    ULONG interfaceFlags;
    ULONG interfaceIndex;
    ULONG flagPass;
    ULONG guidIndex;
    BOOLEAN attached;
    BOOLEAN notifyCapable;
    NTSTATUS probeStatus;
    SIZE_T probeBytesReturned;
    const GUID* interfaceGuids[2];
    const CHAR* interfaceNames[2];

#ifndef DEVICE_INTERFACE_INCLUDE_NONACTIVE
#define DEVICE_INTERFACE_INCLUDE_NONACTIVE 0x00000001ul
#endif

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->BattMiniDeviceLoaded && (Context->BattMiniIoTarget != NULL))
    {
        return STATUS_SUCCESS;
    }

    attached = FALSE;
    status = STATUS_NOT_FOUND;
    openStatus = STATUS_NOT_FOUND;

    interfaceGuids[0] = &GUID_DEVINTERFACE_PMIC_BATT_MINI;
    interfaceNames[0] = "pmic_batt_mini";
    interfaceGuids[1] = &GUID_DEVICE_BATTERY;
    interfaceNames[1] = "device_battery";

    for (guidIndex = 0ul; guidIndex < RTL_NUMBER_OF(interfaceGuids); guidIndex++)
    {
        for (flagPass = 0ul; flagPass < 2ul; flagPass++)
        {
            interfaceFlags = (flagPass == 0ul) ? 0ul : DEVICE_INTERFACE_INCLUDE_NONACTIVE;
            interfaceList = NULL;
            status = IoGetDeviceInterfaces(
                (LPGUID)interfaceGuids[guidIndex],
                NULL,
                interfaceFlags,
                &interfaceList);
            if (!NT_SUCCESS(status))
            {
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini attach query guid=%s flags=0x%lx failed status=0x%08lx\n",
                    interfaceNames[guidIndex],
                    interfaceFlags,
                    (ULONG)status);
                continue;
            }

            if ((interfaceList == NULL) || (interfaceList[0] == L'\0'))
            {
                if (interfaceList != NULL)
                {
                    ExFreePool(interfaceList);
                }
                continue;
            }

            interfaceIndex = 0ul;
            currentInterface = interfaceList;
            while (currentInterface[0] != L'\0')
            {
                if (Context->BattMiniIoTarget == NULL)
                {
                    status = WdfIoTargetCreate(
                        Context->Device,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &Context->BattMiniIoTarget);
                    if (!NT_SUCCESS(status))
                    {
                        DbgPrintEx(
                            DPFLTR_IHVDRIVER_ID,
                            PMICGLINK_TRACE_LEVEL,
                            "pmicglink: battmini attach create target failed status=0x%08lx\n",
                            (ULONG)status);
                        break;
                    }
                }

                RtlInitUnicodeString(&targetName, currentInterface);
                WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                    &openParams,
                    &targetName,
                    PMICGLINK_GLINK_QUERY_ACCESS_MASK);
                openParams.ShareAccess = FILE_SHARE_READ;
                openStatus = WdfIoTargetOpen(Context->BattMiniIoTarget, &openParams);
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini attach try guid=%s[%lu] flags=0x%lx status=0x%08lx name=%ws\n",
                    interfaceNames[guidIndex],
                    interfaceIndex,
                    interfaceFlags,
                    (ULONG)openStatus,
                    currentInterface);
                if (NT_SUCCESS(openStatus))
                {
                    probeBytesReturned = 0u;
                    probeStatus = PmicGlinkSendDriverRequestWithTimeout(
                        Context->BattMiniIoTarget,
                        PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE,
                        NULL,
                        0u,
                        NULL,
                        0u,
                        -1000000ll,
                        &probeBytesReturned);
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: battmini attach probe guid=%s[%lu] status=0x%08lx bytes=%Iu\n",
                        interfaceNames[guidIndex],
                        interfaceIndex,
                        (ULONG)probeStatus,
                        probeBytesReturned);

                    notifyCapable = NT_SUCCESS(probeStatus) ? TRUE : FALSE;

                    if (notifyCapable)
                    {
                        attached = TRUE;
                        status = STATUS_SUCCESS;
                        break;
                    }

                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: battmini attach reject guid=%s[%lu] status=0x%08lx\n",
                        interfaceNames[guidIndex],
                        interfaceIndex,
                        (ULONG)probeStatus);
                }

                PmicGlinkCloseIoTargetIfOpen(&Context->BattMiniIoTarget);
                currentInterface += wcslen(currentInterface) + 1;
                interfaceIndex++;
            }

            ExFreePool(interfaceList);
            if (attached)
            {
                break;
            }
        }

        if (attached)
        {
            break;
        }
    }

    if (attached)
    {
        Context->BattMiniDeviceLoaded = TRUE;
        Context->NotificationFlag = TRUE;
        (VOID)PmicGlinkEnsureBclCriticalCallback(Context);
        return STATUS_SUCCESS;
    }

    if (Context->BattMiniIoTarget != NULL)
    {
        PmicGlinkCloseIoTargetIfOpen(&Context->BattMiniIoTarget);
    }

    if (!NT_SUCCESS(status) && NT_SUCCESS(openStatus))
    {
        return status;
    }

    if (!NT_SUCCESS(openStatus))
    {
        return openStatus;
    }

    return STATUS_NOT_FOUND;
}

static VOID
PmicGlinkTryAttachBattMiniFromIoctl(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ PCSTR Reason
    )
{
    NTSTATUS attachStatus;
    ULONGLONG nowMsec;

    if ((Context == NULL) || Context->BattMiniDeviceLoaded)
    {
        return;
    }

    nowMsec = PmicGlink_Helper_get_rel_time_msec();
    if ((nowMsec >= gPmicGlinkLastBattMiniAttachAttemptMsec)
        && ((nowMsec - gPmicGlinkLastBattMiniAttachAttemptMsec) < 2000ull))
    {
        return;
    }
    gPmicGlinkLastBattMiniAttachAttemptMsec = nowMsec;

    attachStatus = PmicGlinkTryAttachBattMiniNoPnp(Context);
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini ioctl-attach reason=%s status=0x%08lx loaded=%u target=%p\n",
        (Reason != NULL) ? Reason : "unknown",
        (ULONG)attachStatus,
        Context->BattMiniDeviceLoaded ? 1u : 0u,
        Context->BattMiniIoTarget);
}

static VOID
PmicGlinkNotifyBattMiniStatusFromGlink(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotificationData
    )
{
    NTSTATUS notifyStatus;
    ULONG battMiniNotifyArgument;
    BOOLEAN battMiniLoaded;
    WDFIOTARGET battMiniTarget;

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify enter notif=0x%08lx loaded=%u target=%p go=%ld lock=%p\n",
        NotificationData,
        (Context != NULL && Context->BattMiniDeviceLoaded) ? 1u : 0u,
        (Context != NULL) ? Context->BattMiniIoTarget : NULL,
        InterlockedCompareExchange(&gPmicGlinkNotifyGo, 0, 0),
        (Context != NULL) ? Context->BattMiniNotifyLock : NULL);

    if (Context == NULL)
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify exit null-context notif=0x%08lx\n",
            NotificationData);
        return;
    }

    PmicGlinkNotify_PingBattMiniClass(Context);
    if (InterlockedCompareExchange(&gPmicGlinkNotifyGo, 1, 1) == 0)
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify early-exit go=0 notif=0x%08lx\n",
            NotificationData);
        return;
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify no-lockpath notif=0x%08lx\n",
        NotificationData);

    if (!Context->BattMiniDeviceLoaded)
    {
        Context->LegacyStatusNotificationPending = TRUE;
        Context->LegacyStateChangePending = TRUE;
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify defer attach notif=0x%08lx loaded=0 target=%p\n",
            NotificationData,
            Context->BattMiniIoTarget);
        return;
    }

    battMiniLoaded = Context->BattMiniDeviceLoaded ? TRUE : FALSE;
    battMiniTarget = Context->BattMiniIoTarget;
    if (battMiniTarget != NULL)
    {
        WdfObjectReference(battMiniTarget);
    }

    if (battMiniLoaded && (battMiniTarget != NULL))
    {
        notifyStatus = PmicGlinkSendDriverRequestAsync(
            battMiniTarget,
            PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE,
            NULL,
            0u);
        (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify_presence status=0x%08lx notif=0x%08lx\n",
            (ULONG)notifyStatus,
            NotificationData);
        if ((notifyStatus == STATUS_NOT_SUPPORTED)
            || (notifyStatus == STATUS_INVALID_DEVICE_REQUEST))
        {
            PmicGlinkClearBattMiniAttachment(Context);
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_presence detach unsupported status=0x%08lx\n",
                (ULONG)notifyStatus);
        }
        if (NT_SUCCESS(notifyStatus))
        {
            Context->LegacyStatusNotificationPending = TRUE;
            Context->LegacyStateChangePending = TRUE;
        }

        if ((NotificationData & 0xFFu) == 0x83u)
        {
            battMiniNotifyArgument = (NotificationData >> 8);
            notifyStatus = PmicGlinkSendDriverRequestAsync(
                battMiniTarget,
                PMICGLINK_BATTMINI_IOCTL_NOTIFY_STATUS,
                &battMiniNotifyArgument,
                sizeof(battMiniNotifyArgument));
            (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_status status=0x%08lx arg=0x%08lx notif=0x%08lx\n",
                (ULONG)notifyStatus,
                battMiniNotifyArgument,
                NotificationData);
            if ((notifyStatus == STATUS_NOT_SUPPORTED)
                || (notifyStatus == STATUS_INVALID_DEVICE_REQUEST))
            {
                PmicGlinkClearBattMiniAttachment(Context);
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini notify_status detach unsupported status=0x%08lx\n",
                    (ULONG)notifyStatus);
            }
            if (NT_SUCCESS(notifyStatus))
            {
                Context->LegacyStatusNotificationPending = TRUE;
                Context->LegacyStateChangePending = TRUE;
            }
        }
        else
        {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_status skip notif=0x%08lx low=0x%02lx\n",
                NotificationData,
                NotificationData & 0xFFu);
        }
    }
    else
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify skipped loaded=%u target=%p notif=0x%08lx\n",
            battMiniLoaded ? 1u : 0u,
            battMiniTarget,
            NotificationData);
    }

    if (battMiniTarget != NULL)
    {
        WdfObjectDereference(battMiniTarget);
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify exit notif=0x%08lx loaded=%u target=%p\n",
        NotificationData,
        Context->BattMiniDeviceLoaded ? 1u : 0u,
        Context->BattMiniIoTarget);
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
PmicGlinkNotifyUsbnIoctl(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDFIOTARGET ioTarget;
    ULONG_PTR bytesReturned;
    UCHAR inputBuffer[12];
    ULONG outputBuffer[5];
    ULONGLONG inputSignature;
    ULONG inputValue;
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    WDF_MEMORY_DESCRIPTOR outputDescriptor;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ioTarget = WdfDeviceGetIoTarget(Context->Device);
    if (ioTarget == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    inputSignature = PMICGLINK_USBN_NOTIFY_SIG_IN;
    inputValue = 0x80u;
    RtlZeroMemory(inputBuffer, sizeof(inputBuffer));
    RtlCopyMemory(inputBuffer, &inputSignature, sizeof(inputSignature));
    RtlCopyMemory(inputBuffer + sizeof(inputSignature), &inputValue, sizeof(inputValue));

    outputBuffer[0] = PMICGLINK_USBN_NOTIFY_SIG_OUT;
    outputBuffer[1] = 0u;
    outputBuffer[2] = 0u;
    outputBuffer[3] = 0u;
    outputBuffer[4] = 0u;

    bytesReturned = 0;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor, inputBuffer, sizeof(inputBuffer));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor, outputBuffer, sizeof(outputBuffer));

    status = WdfIoTargetSendIoctlSynchronously(
        ioTarget,
        NULL,
        PMICGLINK_USBN_NOTIFY_IOCTL,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        &bytesReturned);

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: usbn_notify ioctl status=0x%08lx bytes=%Iu out=[0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx]\n",
        (ULONG)status,
        (SIZE_T)bytesReturned,
        outputBuffer[0],
        outputBuffer[1],
        outputBuffer[2],
        outputBuffer[3],
        outputBuffer[4]);

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
    NTSTATUS status;

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

    status = WdfIoTargetSendIoctlSynchronously(
        IoTarget,
        NULL,
        IoControlCode,
        pInputDescriptor,
        pOutputDescriptor,
        NULL,
        (PULONG_PTR)BytesReturned);

    if ((status == STATUS_INVALID_DEVICE_REQUEST)
        || (status == STATUS_NOT_SUPPORTED)
        || (status == STATUS_NOT_IMPLEMENTED))
    {
        NTSTATUS fallbackStatus;

        fallbackStatus = WdfIoTargetSendInternalIoctlSynchronously(
            IoTarget,
            NULL,
            IoControlCode,
            pInputDescriptor,
            pOutputDescriptor,
            NULL,
            (PULONG_PTR)BytesReturned);

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: sendreq fallback internal ioctl=0x%08lx status=0x%08lx->0x%08lx\n",
            IoControlCode,
            (ULONG)status,
            (ULONG)fallbackStatus);

        status = fallbackStatus;
    }

    return status;
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
    NTSTATUS status;

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

    status = WdfIoTargetSendIoctlSynchronously(
        IoTarget,
        NULL,
        IoControlCode,
        pInputDescriptor,
        pOutputDescriptor,
        &requestOptions,
        (PULONG_PTR)BytesReturned);

    if ((status == STATUS_INVALID_DEVICE_REQUEST)
        || (status == STATUS_NOT_SUPPORTED)
        || (status == STATUS_NOT_IMPLEMENTED))
    {
        NTSTATUS fallbackStatus;

        fallbackStatus = WdfIoTargetSendInternalIoctlSynchronously(
            IoTarget,
            NULL,
            IoControlCode,
            pInputDescriptor,
            pOutputDescriptor,
            &requestOptions,
            (PULONG_PTR)BytesReturned);

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: sendreq_timeout fallback internal ioctl=0x%08lx status=0x%08lx->0x%08lx\n",
            IoControlCode,
            (ULONG)status,
            (ULONG)fallbackStatus);

        status = fallbackStatus;
    }

    return status;
}

static NTSTATUS
PmicGlinkSendDriverRequestAsync(
    _In_ WDFIOTARGET IoTarget,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize
    )
{
    return PmicGlinkSendDriverRequestWithTimeout(
        IoTarget,
        IoControlCode,
        InputBuffer,
        InputBufferSize,
        NULL,
        0u,
        -100000000ll,
        NULL);
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

#include "pmicglink8550_crashdump.c"

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

#include "pmicglink8550_requests.c"


#include "pmicglink8550_ulog.c"

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

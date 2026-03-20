#include "internal.h"

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
static VOID PmicGlinkPollBattMiniClass(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_opt_ PCSTR Reason);
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


#include "device.c"
#include "interfaces.c"
#include "channel.c"
#include "transport.c"
#include "crashdump.c"
#include "battery.c"
#include "platform.c"
#include "requests.c"
#include "ulog.c"
#include "queue.c"

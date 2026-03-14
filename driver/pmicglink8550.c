#include "pmicglink8550.h"

#include <initguid.h>

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

static PMICGLINK_UCSI_WRITE_DATA_BUF_TYPE gLatestUcsiCmd;
static ULONGLONG gPmicGlinkRelTimeStartTicks;
static BOOLEAN gPmicGlinkRelTimeInitialized;

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

static NTSTATUS PmicGlink_Init(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkDevice_InitContext(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS RegisterDeviceInterfaces(_In_ WDFDEVICE Device, _In_ BOOLEAN Register);
static NTSTATUS PmicGlinkDevice_RegisterForPnPNotifications(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ BOOLEAN Register);
static VOID PmicGlinkPower_ModernStandby_Callback(_In_opt_ PVOID CallbackContext, _In_opt_ PVOID Argument1, _In_opt_ PVOID Argument2);
static VOID PmicGlinkDDI_InterfaceReference(_In_opt_ PVOID Context);
static VOID PmicGlinkDDI_InterfaceDereference(_In_opt_ PVOID Context);
static NTSTATUS PmicGlinkDDI_EvtDeviceProcessQueryInterfaceRequest(_In_ WDFDEVICE Device, _In_ const GUID* InterfaceType, _Inout_ PINTERFACE ExposedInterface, _In_opt_ PVOID ExposedInterfaceSpecificData);
static VOID PmicGlinkDDI_NotifyUcsiAlert(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ ULONG EventCode, _In_ ULONG EventData);
static NTSTATUS PmicGlinkCreateDeviceWorkItem(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ PFN_WDF_WORKITEM WorkItemRoutine);
static NTSTATUS PmicGlinkPlatformUsbc_Request_Write(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ const USBPD_DPM_USBC_WRITE_BUFFER* Request);
static VOID PmicGlinkPlatformUsbc_Request_Write_WorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkPlatformSetState_Request_Write_WorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkPlatformUsbc_AcpiNotificationHandler(_In_opt_ PVOID Context, _In_ ULONG NotifyValue);
static VOID PmicGlinkNotify_PingBattMiniClass(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static NTSTATUS PmicGlinkRegistryQuery(_In_ WDFKEY RegKey, _In_ PCUNICODE_STRING RegName, _Out_ PULONG ReadData);
static NTSTATUS PmicGlink_OpenGlinkChannel(_In_ PPMIC_GLINK_DEVICE_CONTEXT Context);
static VOID PmicGlinkRegisterInterfaceWorkItem(_In_ WDFWORKITEM WorkItem);
static VOID PmicGlinkStateNotificationCb(_In_opt_ PVOID Handle, _In_ PPMIC_GLINK_DEVICE_CONTEXT Context, _In_ PMICGLINK_CHANNEL_EVENT Event);

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

static NTSTATUS
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
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(&config, PmicGlinkEvtDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);

    return status;
}

NTSTATUS
PmicGlinkEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    UNREFERENCED_PARAMETER(Driver);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = PmicGlinkEvtPrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = PmicGlinkEvtReleaseHardware;
    pnpCallbacks.EvtDeviceD0Entry = PmicGlinkEvtD0Entry;
    pnpCallbacks.EvtDeviceD0Exit = PmicGlinkEvtD0Exit;
    pnpCallbacks.EvtDeviceSelfManagedIoInit = PmicGlinkEvtSelfManagedIoInit;
    pnpCallbacks.EvtDeviceSelfManagedIoRestart = PmicGlinkEvtSelfManagedIoRestart;
    pnpCallbacks.EvtDeviceSelfManagedIoCleanup = PmicGlinkEvtSelfManagedIoCleanup;
    pnpCallbacks.EvtDeviceQueryRemove = PmicGlinkEvtQueryRemove;
    pnpCallbacks.EvtDeviceQueryStop = PmicGlinkEvtQueryStop;
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

    status = RegisterDeviceInterfaces(device, TRUE);
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
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = PmicGlinkEvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE);

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
    WDFDEVICE device;

    bytesReturned = 0;
    inputBuffer = NULL;
    outputBuffer = NULL;

    device = WdfIoQueueGetDevice(Queue);
    context = PmicGlinkGetDeviceContext(device);

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

    WdfRequestCompleteWithInformation(Request, status, NT_SUCCESS(status) ? bytesReturned : 0);
}

NTSTATUS
PmicGlinkEvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    context = PmicGlinkGetDeviceContext(Device);
    context->GlinkDeviceLoaded = TRUE;
    context->AllReqIntfArrived = TRUE;
    context->GlinkLinkStateUp = TRUE;
    PmicGlinkStateNotificationCb(NULL, context, PmicGlinkChannelConnected);

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    context = PmicGlinkGetDeviceContext(Device);
    context->GlinkDeviceLoaded = FALSE;
    context->GlinkLinkStateUp = FALSE;
    PmicGlinkStateNotificationCb(NULL, context, PmicGlinkChannelLocalDisconnected);
    context->GlinkChannelConnected = FALSE;

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;

    UNREFERENCED_PARAMETER(PreviousState);

    context = PmicGlinkGetDeviceContext(Device);
    context->Hibernate = FALSE;

    if (!context->GlinkChannelConnected)
    {
        (VOID)PmicGlink_OpenGlinkChannel(context);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;

    context = PmicGlinkGetDeviceContext(Device);

    if (TargetState == WdfPowerDeviceD3Final)
    {
        context->Hibernate = TRUE;
    }

    PmicGlinkStateNotificationCb(NULL, context, PmicGlinkChannelLocalDisconnected);
    context->GlinkChannelConnected = FALSE;
    context->GlinkChannelRestart = FALSE;

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoInit(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkEvtSelfManagedIoRestart(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);

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

    context = PmicGlinkGetDeviceContext(Device);

    if (!Register)
    {
        context->DdiInterface.PmicGlinkUCSIAlertCallback = NULL;
        context->DeviceInterfacesRegistered = FALSE;
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

    status = WdfDeviceCreateDeviceInterface(Device, &GUID_DEVINTERFACE_PMIC_BATT_MINI, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->DeviceInterfacesRegistered = TRUE;

    return STATUS_SUCCESS;
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

    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    if ((Device == NULL) || (ExposedInterface == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    context = PmicGlinkGetDeviceContext(Device);
    if (context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    exposedDdi = (PPMICGLINK_DEVICE_DDIINTERFACE_TYPE)ExposedInterface;
    if (exposedDdi->PmicGlinkUCSIAlertCallback == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    context->DdiInterface.PmicGlinkUCSIAlertCallback = exposedDdi->PmicGlinkUCSIAlertCallback;
    *exposedDdi = context->DdiInterface;

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

    if ((Context == NULL) || (Context->DdiInterfaceLock == NULL))
    {
        return;
    }

    callback = NULL;
    WdfWaitLockAcquire(Context->DdiInterfaceLock, NULL);
    callback = Context->DdiInterface.PmicGlinkUCSIAlertCallback;
    WdfWaitLockRelease(Context->DdiInterfaceLock);

    if (callback != NULL)
    {
        callback(Context->Device, EventCode, EventData);
    }
}

static NTSTATUS
PmicGlinkDevice_RegisterForPnPNotifications(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ BOOLEAN Register
    )
{
    NTSTATUS status;
    UNICODE_STRING callbackName;
    OBJECT_ATTRIBUTES callbackAttributes;

    if (Context == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!Register)
    {
        if (Context->ModernStandbyCallbackHandle != NULL)
        {
            ExUnregisterCallback(Context->ModernStandbyCallbackHandle);
            Context->ModernStandbyCallbackHandle = NULL;
        }

        if (Context->ModernStandbyCallbackObject != NULL)
        {
            ObDereferenceObject(Context->ModernStandbyCallbackObject);
            Context->ModernStandbyCallbackObject = NULL;
        }

        Context->AllReqIntfArrived = FALSE;
        Context->GlinkChannelConnected = FALSE;
        return STATUS_SUCCESS;
    }

    if (Context->ModernStandbyCallbackHandle == NULL)
    {
        RtlInitUnicodeString(&callbackName, L"\\Callback\\QcModernStandby");
        InitializeObjectAttributes(
            &callbackAttributes,
            &callbackName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL);

        status = ExCreateCallback(
            &Context->ModernStandbyCallbackObject,
            &callbackAttributes,
            TRUE,
            TRUE);
        if (!NT_SUCCESS(status))
        {
            Context->ModernStandbyCallbackObject = NULL;
            return status;
        }

        Context->ModernStandbyCallbackHandle = ExRegisterCallback(
            Context->ModernStandbyCallbackObject,
            PmicGlinkPower_ModernStandby_Callback,
            Context);
        if (Context->ModernStandbyCallbackHandle == NULL)
        {
            ObDereferenceObject(Context->ModernStandbyCallbackObject);
            Context->ModernStandbyCallbackObject = NULL;
            return STATUS_UNSUCCESSFUL;
        }

        Context->ModernStandbyState = 0;
    }

    Context->AllReqIntfArrived = TRUE;
    Context->GlinkChannelConnected = TRUE;

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
    Context->AllReqIntfArrived = TRUE;
    Context->GlinkDeviceLoaded = FALSE;
    Context->GlinkChannelConnected = TRUE;
    Context->GlinkChannelRestart = FALSE;
    Context->GlinkChannelFirstConnect = FALSE;
    Context->GlinkLinkStateUp = FALSE;
    Context->RpeInitialized = FALSE;
    Context->Hibernate = FALSE;
    Context->Notify = FALSE;
    Context->NotificationFlag = FALSE;
    Context->EventID = 0;

    Context->NumPorts = 1;
    Context->UsbinPower[0] = 5000000;
    Context->UsbinPower[1] = 0;
    Context->UsbinPower[2] = 0;
    Context->UsbinPower[3] = 0;

    Context->LastUsbBattMngrQueryTime.QuadPart = 0;

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

    Context->QcmbConnected = TRUE;
    Context->QcmbStatus = 0x4;
    Context->QcmbCurrentChargerPowerUW = 12000000;
    Context->QcmbGoodChargerThresholdUW = 4500000;
    Context->QcmbChargerStatusInfo = 0x1;

    Context->LegacyBattId.batt_id = 0xFFFF;

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

    Context->LegacyBattPercentage = 0;
    Context->LegacyStatusNotificationPending = FALSE;
    Context->LegacyStateChangePending = FALSE;

    Context->LegacyLastBattIdQueryMsec = 0;
    Context->LegacyLastChargeStatusQueryMsec = 0;
    Context->LegacyLastBattInfoQueryMsec = 0;

    Context->ModernStandbyState = 0;
    Context->ModernStandbyCallbackObject = NULL;
    Context->ModernStandbyCallbackHandle = NULL;

    Context->DdiInterfaceRefCount = 0;
    Context->DdiInterfacePadding = 0;
    RtlZeroMemory(&Context->DdiInterface, sizeof(Context->DdiInterface));

    RtlZeroMemory(&Context->LastUsbcWriteRequest, sizeof(Context->LastUsbcWriteRequest));
    RtlZeroMemory(&Context->LastUsbcNotification, sizeof(Context->LastUsbcNotification));
    Context->PendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
    Context->PlatformState = 0;
    Context->Reserved2[0] = 0;
    Context->Reserved2[1] = 0;

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

    RtlZeroMemory(&gLatestUcsiCmd, sizeof(gLatestUcsiCmd));

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlink_Init(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDFKEY regKey;
    UNICODE_STRING regValueName;
    ULONG regValueData;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->GlinkChannelRestart = FALSE;
    Context->GlinkChannelFirstConnect = FALSE;
    Context->GlinkLinkStateUp = FALSE;
    Context->RpeInitialized = FALSE;
    Context->Hibernate = FALSE;

    regValueData = 0;
    status = WdfDeviceOpenRegistryKey(
        Context->Device,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &regKey);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&regValueName, L"PmicGlinkPlatformState");
        status = PmicGlinkRegistryQuery(regKey, &regValueName, &regValueData);
        if (NT_SUCCESS(status))
        {
            Context->PlatformState = (UCHAR)regValueData;
        }

        WdfRegistryClose(regKey);
    }

    status = PmicGlink_OpenGlinkChannel(Context);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlink_OpenGlinkChannel(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->GlinkChannelFirstConnect = TRUE;
    Context->GlinkChannelConnected = TRUE;
    Context->GlinkChannelRestart = FALSE;
    Context->GlinkLinkStateUp = TRUE;

    return PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
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

        context->PendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
        RtlZeroMemory(&context->LastUsbcNotification, sizeof(context->LastUsbcNotification));
        RtlZeroMemory(&context->LastUsbcWriteRequest, sizeof(context->LastUsbcWriteRequest));

        if (context->GlinkChannelFirstConnect)
        {
            RtlZeroMemory(&request, sizeof(request));
            request.cmd_type = 16u;
            request.cmd_payload.read_sel = 0u;
            (VOID)PmicGlinkPlatformUsbc_Request_Write(context, &request);
        }
    }

    WdfObjectDelete(WorkItem);
}

static VOID
PmicGlinkStateNotificationCb(
    _In_opt_ PVOID Handle,
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ PMICGLINK_CHANNEL_EVENT Event
    )
{
    UNREFERENCED_PARAMETER(Handle);

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
        Context->GlinkLinkStateUp = TRUE;
        (VOID)PmicGlinkCreateDeviceWorkItem(Context, PmicGlinkRegisterInterfaceWorkItem);
        break;

    case PmicGlinkChannelLocalDisconnected:
        Context->GlinkChannelConnected = FALSE;
        if (Context->GlinkChannelRestart && Context->GlinkLinkStateUp)
        {
            (VOID)PmicGlink_OpenGlinkChannel(Context);
        }
        break;

    case PmicGlinkChannelRemoteDisconnected:
        Context->GlinkChannelConnected = FALSE;
        Context->GlinkChannelRestart = TRUE;
        if (Context->GlinkLinkStateUp)
        {
            (VOID)PmicGlink_OpenGlinkChannel(Context);
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
    UNREFERENCED_PARAMETER(WaitForRx);

    if ((Context == NULL) || (Buffer == NULL) || (BufferLen == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (OpCode)
    {
    case 0x15:
        if (BufferLen >= sizeof(PMICGLINK_USBC_WRITE_REQ_MESSAGE))
        {
            const PMICGLINK_USBC_WRITE_REQ_MESSAGE* requestMessage;

            requestMessage = (const PMICGLINK_USBC_WRITE_REQ_MESSAGE*)Buffer;
            Context->LastUsbcWriteRequest = requestMessage->Request;

            switch (requestMessage->Request.cmd_type)
            {
            case 16u:
                if (Context->PendingPan >= PMICGLINK_MAX_PORTS)
                {
                    Context->PendingPan = 0;
                }

                RtlZeroMemory(&Context->LastUsbcNotification, sizeof(Context->LastUsbcNotification));
                Context->LastUsbcNotification.detail.common.port_index = Context->PendingPan;
                break;

            case 17u:
            case 19u:
                Context->PendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
                break;

            case 20u:
                if (requestMessage->Request.cmd_payload.read_sel < PMICGLINK_MAX_PORTS)
                {
                    Context->PendingPan = (UCHAR)requestMessage->Request.cmd_payload.read_sel;
                }
                break;

            default:
                break;
            }
        }
        return STATUS_SUCCESS;

    case 0x80:
        KeSetEvent(&Context->QcmbNotifyEvent, IO_NO_INCREMENT, FALSE);
        return STATUS_SUCCESS;

    case 0x81:
        Context->QcmbStatus = 0x4;
        KeSetEvent(&Context->QcmbNotifyEvent, IO_NO_INCREMENT, FALSE);
        return STATUS_SUCCESS;

    case 0x52:
        if (BufferLen >= sizeof(PMICGLINK_USBC_SET_STATE_MESSAGE))
        {
            const PMICGLINK_USBC_SET_STATE_MESSAGE* setStateMessage;

            setStateMessage = (const PMICGLINK_USBC_SET_STATE_MESSAGE*)Buffer;
            Context->PlatformState = (UCHAR)setStateMessage->State;
        }

        return STATUS_SUCCESS;

    default:
        return STATUS_SUCCESS;
    }
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
        if ((InputBuffer == NULL) || (InputBufferSize < PMICGLINK_UCSI_BUFFER_SIZE))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlCopyMemory(Context->UCSIDataBuffer, InputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_UCSI_READ:
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_PLATFORM_USBC_READ:
        if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(USBPD_DPM_USBC_WRITE_BUFFER)))
        {
            const USBPD_DPM_USBC_WRITE_BUFFER* request;

            request = (const USBPD_DPM_USBC_WRITE_BUFFER*)InputBuffer;
            Context->LastUsbcWriteRequest = *request;

            if (request->cmd_type == 16u)
            {
                Context->PendingPan = (UCHAR)Context->LastUsbcNotification.detail.common.port_index;
            }
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_PLATFORM_USBC_NOTIFY:
        if ((InputBuffer != NULL) && (InputBufferSize == sizeof(ULONG)))
        {
            PmicGlinkPlatformUsbc_AcpiNotificationHandler(Context, *(ULONG*)InputBuffer);
            return STATUS_SUCCESS;
        }

        if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(USBPD_DPM_USBC_WRITE_BUFFER)))
        {
            return PmicGlinkPlatformUsbc_Request_Write(
                Context,
                (const USBPD_DPM_USBC_WRITE_BUFFER*)InputBuffer);
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_GET_OEM_MSG:
        if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(PMICGLINK_OEM_GET_PROP_INPUT)))
        {
            PMICGLINK_OEM_GET_PROP_INPUT* req;

            req = (PMICGLINK_OEM_GET_PROP_INPUT*)InputBuffer;
            Context->OemPropData[0] = req->property_id;
            Context->OemPropData[1] = req->data_size;
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_OEM_READ_BUFFER:
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_OEM_WRITE_BUFFER:
        if ((InputBuffer == NULL) || (InputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(Context->OemReceivedData, sizeof(Context->OemReceivedData));
        RtlCopyMemory(
            Context->OemReceivedData,
            InputBuffer,
            (InputBufferSize < sizeof(Context->OemReceivedData)) ? InputBufferSize : sizeof(Context->OemReceivedData));
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_GET_CHARGER_PORTS:
        if (Context->NumPorts == 0)
        {
            Context->NumPorts = 1;
        }
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_GET_USB_CHG_STATUS:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(ULONG)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (*(ULONG*)InputBuffer >= PMICGLINK_MAX_PORTS)
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (Context->UsbinPower[*(ULONG*)InputBuffer] == 0)
        {
            Context->UsbinPower[*(ULONG*)InputBuffer] = 5000000;
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_GWS:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_GWS_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->gws_out.AlarmStatus = ((PMICGLINK_TAD_GWS_INBUF*)InputBuffer)->TimerId;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_CWS:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_CWS_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->cws_out.Status = ((PMICGLINK_TAD_CWS_INBUF*)InputBuffer)->TimerId;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_STP:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_STP_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->stp_out.Status = ((PMICGLINK_TAD_STP_INBUF*)InputBuffer)->PolicyValue;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_STV:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_STV_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->stv_out.Status = ((PMICGLINK_TAD_STV_INBUF*)InputBuffer)->TimerValue;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_TIP:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_TIP_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->tip_out.PolicySetting = ((PMICGLINK_TAD_TIP_INBUF*)InputBuffer)->TimerId;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_TAD_TIV:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_TAD_TIV_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Context->tiv_out.TimerValueRemain = ((PMICGLINK_TAD_TIV_INBUF*)InputBuffer)->TimerId;
        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_I2C_WRITE:
        if ((InputBuffer == NULL) || (InputBufferSize < PMICGLINK_I2C_HEADER_SIZE))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if ((InputBuffer[1] & 0x1u) == 0x1u)
        {
            RtlCopyMemory(Context->I2CHeader, InputBuffer, PMICGLINK_I2C_HEADER_SIZE);
            Context->I2CDataLength = 0;
        }
        else
        {
            ULONG payloadLength;

            payloadLength = (ULONG)(InputBuffer[1] >> 1);
            if (payloadLength > PMICGLINK_I2C_DATA_SIZE)
            {
                payloadLength = PMICGLINK_I2C_DATA_SIZE;
            }

            if (InputBufferSize < PMICGLINK_I2C_HEADER_SIZE + payloadLength)
            {
                return STATUS_INVALID_PARAMETER;
            }

            Context->I2CDataLength = payloadLength;
            RtlZeroMemory(Context->I2CData, sizeof(Context->I2CData));
            RtlCopyMemory(Context->I2CData, InputBuffer + PMICGLINK_I2C_HEADER_SIZE, payloadLength);
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_I2C_READ:
        if (Context->I2CHeader[0] == 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (Context->I2CDataLength == 0)
        {
            Context->I2CDataLength = 1;
            Context->I2CData[0] = 0;
        }

        return STATUS_SUCCESS;

    case IOCTL_PMICGLINK_PRESHUTDOWN_CMD:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(PMICGLINK_PRESHUTDOWN_CMD_INBUF)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        return PmicGlinkPlatformQcmb_PreShutdown_Cmd(
            Context,
            ((PMICGLINK_PRESHUTDOWN_CMD_INBUF*)InputBuffer)->CmdBitMask);

    case IOCTL_BATTMNGR_GET_BATT_ID:
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_GET_CHARGER_STATUS:
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_GET_BATT_INFO:
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_CONTROL_CHARGING:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(BATT_MNGR_CONTROL_CHARGING)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlCopyMemory(
            &Context->LegacyControlCharging,
            InputBuffer,
            sizeof(BATT_MNGR_CONTROL_CHARGING));
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_SET_STATUS_CRITERIA:
        if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA)))
        {
            RtlCopyMemory(
                &Context->LegacyStatusCriteria,
                InputBuffer,
                sizeof(BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA));
        }
        Context->LegacyStatusNotificationPending = TRUE;
        PmicGlinkNotify_PingBattMiniClass(Context);
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_GET_BATT_PRESENT:
        Context->LegacyStateChangePending = TRUE;
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_SET_OPERATION_MODE:
        if ((InputBuffer != NULL) && (InputBufferSize >= sizeof(BATT_MNGR_SET_OPERATIONAL_MODE)))
        {
            RtlCopyMemory(
                &Context->LegacyOperationalMode,
                InputBuffer,
                sizeof(BATT_MNGR_SET_OPERATIONAL_MODE));
        }
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_SET_CHARGE_RATE:
        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(BATT_MNGR_SET_CHARGE_RATE)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlCopyMemory(
            &Context->LegacyChargeRate,
            InputBuffer,
            sizeof(BATT_MNGR_SET_CHARGE_RATE));
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_GET_TEST_INFO:
        RtlZeroMemory(&Context->LegacyTestInfo, sizeof(Context->LegacyTestInfo));
        if (InputBuffer != NULL)
        {
            SIZE_T copySize;

            copySize = (InputBufferSize < sizeof(Context->LegacyTestInfo.data))
                ? InputBufferSize
                : sizeof(Context->LegacyTestInfo.data);

            RtlCopyMemory(
                Context->LegacyTestInfo.data,
                InputBuffer,
                copySize);
        }
        return STATUS_SUCCESS;

    case IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT:
        if (!Context->GlinkChannelConnected)
        {
            return STATUS_UNSUCCESSFUL;
        }
        return STATUS_SUCCESS;

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
    ((ULONG*)Buffer)[0] = Context->QcmbStatus;
    ((ULONG*)Buffer)[1] = Context->QcmbCurrentChargerPowerUW;
    ((ULONG*)Buffer)[2] = Context->QcmbGoodChargerThresholdUW;
    ((ULONG*)Buffer)[3] = Context->QcmbChargerStatusInfo;

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

    if (!Context->QcmbConnected)
    {
        return STATUS_SUCCESS;
    }

    requestMessage.Header = 0x100008010ull;
    requestMessage.MessageOp = 128u;

    status = PmicGlink_SendData(Context, 0x80u, &requestMessage, sizeof(requestMessage), TRUE);
    if (NT_SUCCESS(status))
    {
        *QcmbStatus = Context->QcmbStatus;
    }

    return status;
}

static NTSTATUS
PmicGlinkPlatformQcmb_WaitCmdStatus(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG WaitInMs
    )
{
    NTSTATUS status;
    ULONG qcmbStatus;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
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

    return status;
}

static NTSTATUS
PmicGlinkPlatformQcmb_PreShutdown_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG CmdBitMask
    )
{
    NTSTATUS status;
    UCHAR qcmbMessage[64];

    if (Context == NULL)
    {
        return (NTSTATUS)0xC00000EFL;
    }

    if (!Context->QcmbConnected)
    {
        return STATUS_SUCCESS;
    }

    WdfSpinLockAcquire(Context->StateLock);
    Context->QcmbStatus = 0x2;
    if ((CmdBitMask & 0x3u) != 0)
    {
        Context->ModernStandbyState = ((CmdBitMask & 0x2u) != 0) ? 1u : 0u;
    }
    WdfSpinLockRelease(Context->StateLock);

    status = PmicGlinkPlatformQcmb_WriteMBToBuffer(Context, qcmbMessage, sizeof(qcmbMessage));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PmicGlink_SendData(Context, 0x81u, qcmbMessage, sizeof(qcmbMessage), TRUE);
    if (NT_SUCCESS(status))
    {
        (VOID)PmicGlinkPlatformQcmb_WaitCmdStatus(Context, 50u);
    }

    Context->QcmbStatus = 0x4;

    return status;
}

static NTSTATUS
PmicGlinkPlatformQcmb_GetChargerInfo_Cmd(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _Out_ QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA* ChargerInfo
    )
{
    NTSTATUS status;
    ULONG qcmbStatus;
    UCHAR qcmbMessage[64];

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

    Context->QcmbStatus = 0x2;
    status = PmicGlinkPlatformQcmb_WriteMBToBuffer(Context, qcmbMessage, sizeof(qcmbMessage));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PmicGlink_SendData(Context, 0x81u, qcmbMessage, sizeof(qcmbMessage), TRUE);
    if (NT_SUCCESS(status))
    {
        (VOID)PmicGlinkPlatformQcmb_WaitCmdStatus(Context, 50u);
    }

    status = PmicGlinkPlatformQcmb_GetStatus(Context, &qcmbStatus);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((qcmbStatus & 0xEu) != 0x4u)
    {
        return STATUS_UNSUCCESSFUL;
    }

    WdfSpinLockAcquire(Context->StateLock);
    ChargerInfo->currentChargerPowerUW = Context->QcmbCurrentChargerPowerUW;
    ChargerInfo->goodChargerThresholdUW = Context->QcmbGoodChargerThresholdUW;
    ChargerInfo->chargerStatusInfo.AsUINT32 = Context->QcmbChargerStatusInfo;
    WdfSpinLockRelease(Context->StateLock);

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

    if ((CallbackContext == NULL) || (Argument1 == NULL))
    {
        return;
    }

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
    ULONG value;
    NTSTATUS status;

    if ((RegName == NULL) || (ReadData == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    value = 0;
    status = WdfRegistryQueryULong(RegKey, RegName, &value);
    if (NT_SUCCESS(status))
    {
        *ReadData = value;
    }

    return status;
}

static VOID
PmicGlinkNotify_PingBattMiniClass(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if (Context != NULL)
    {
        Context->NotificationFlag = TRUE;
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
        return STATUS_INVALID_HANDLE;
    }

    if (Request == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->LastUsbcWriteRequest = *Request;
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
        message.Request = context->LastUsbcWriteRequest;

        if ((message.Request.cmd_type == 17u) || (message.Request.cmd_type == 19u))
        {
            context->PendingPan = (UCHAR)PMICGLINK_MAX_PORTS;
        }

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
        message.State = context->PlatformState;

        (VOID)PmicGlink_SendData(context, 0x52u, &message, sizeof(message), TRUE);
    }

    WdfObjectDelete(WorkItem);
}

static VOID
PmicGlinkPlatformUsbc_AcpiNotificationHandler(
    _In_opt_ PVOID Context,
    _In_ ULONG NotifyValue
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;
    USBPD_DPM_USBC_WRITE_BUFFER request;

    if (Context == NULL)
    {
        return;
    }

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    RtlZeroMemory(&request, sizeof(request));

    if ((NotifyValue == 240u) || (NotifyValue == 241u))
    {
        if (deviceContext->PendingPan >= PMICGLINK_MAX_PORTS)
        {
            return;
        }

        request.cmd_payload.pan_ack.port_index = deviceContext->PendingPan;
        request.cmd_type = (NotifyValue == 241u) ? 19u : 17u;
        (VOID)PmicGlinkPlatformUsbc_Request_Write(deviceContext, &request);
        return;
    }

    if ((NotifyValue >= 244u) && (NotifyValue <= 247u))
    {
        request.cmd_type = 20u;
        request.cmd_payload.read_sel = (ULONG)(UCHAR)(NotifyValue + 12u);
        (VOID)PmicGlinkPlatformUsbc_Request_Write(deviceContext, &request);
        return;
    }

    deviceContext->PlatformState = (UCHAR)NotifyValue;
    (VOID)PmicGlinkCreateDeviceWorkItem(deviceContext, PmicGlinkPlatformSetState_Request_Write_WorkItem);
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
        return STATUS_UNSUCCESSFUL;
    }

    *IsOEMCmd = (InputBuffer[0] == 0xFFu) ? TRUE : FALSE;

    return STATUS_SUCCESS;
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
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    if (BytesReturned == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (InputBuffer != NULL)
    {
        if (Context->GlinkChannelConnected)
        {
            (VOID)PmicGlink_SyncSendReceive(
                Context,
                IOCTL_PMICGLINK_UCSI_WRITE,
                (PUCHAR)InputBuffer,
                InputBufferSize);
        }
        else if (InputBufferSize >= PMICGLINK_UCSI_BUFFER_SIZE)
        {
            RtlCopyMemory(gLatestUcsiCmd.data, InputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
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

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferSize);

    status = STATUS_SUCCESS;

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    if (Context->GlinkChannelConnected)
    {
        status = PmicGlink_SyncSendReceive(
            Context,
            IOCTL_PMICGLINK_UCSI_READ,
            InputBuffer,
            InputBufferSize);

        if (OutputBufferSize >= PMICGLINK_UCSI_BUFFER_SIZE)
        {
            RtlCopyMemory(OutputBuffer, Context->UCSIDataBuffer, PMICGLINK_UCSI_BUFFER_SIZE);
            *BytesReturned = PMICGLINK_UCSI_BUFFER_SIZE;
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (*(ULONGLONG*)&gLatestUcsiCmd.data[8] == 1ull)
    {
        USHORT* words;

        if (OutputBufferSize < PMICGLINK_UCSI_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(OutputBuffer, PMICGLINK_UCSI_BUFFER_SIZE);

        words = (USHORT*)OutputBuffer;
        words[0] = 0x0100;
        words[3] = 0x0800;

        *BytesReturned = PMICGLINK_UCSI_BUFFER_SIZE;
    }

    if (((USHORT*)OutputBuffer)[3] & (USHORT)0xA000u)
    {
        *(ULONGLONG*)&gLatestUcsiCmd.data[8] = 0;
    }

    if (*BytesReturned >= (sizeof(USHORT) * 4u))
    {
        USHORT* words;

        words = (USHORT*)OutputBuffer;
        PmicGlinkDDI_NotifyUcsiAlert(
            Context,
            (ULONG)words[3],
            (ULONG)words[0]);
    }

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

    if (NT_SUCCESS(status))
    {
        if (OutputBufferSize < sizeof(*OutputBuffer))
        {
            return STATUS_INVALID_PARAMETER;
        }

        RtlCopyMemory(OutputBuffer->data, Context->OemPropData, sizeof(Context->OemPropData));
        *BytesReturned = sizeof(*OutputBuffer);
    }

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
        if (OutputBufferSize < sizeof(*OutputBuffer))
        {
            return STATUS_INVALID_PARAMETER;
        }

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

    payloadSize = (InputBuffer[1] <= PMICGLINK_OEM_BUFFER_SIZE)
        ? (SIZE_T)(InputBuffer[1] + 2u)
        : PMICGLINK_OEM_SEND_BUFFER_SIZE;

    if (payloadSize > InputBufferSize)
    {
        payloadSize = InputBufferSize;
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

    if (OutputBufferSize < sizeof(ULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->NumPorts;
    *BytesReturned = sizeof(ULONG);

    return STATUS_SUCCESS;
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

    if ((Context == NULL) || (InputBuffer == NULL) || (OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((InputBufferSize < sizeof(ULONG)) || (OutputBufferSize < sizeof(LONG)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    portIndex = *(ULONG*)InputBuffer;
    if (portIndex >= PMICGLINK_MAX_PORTS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;

    now = PmicGlinkQuerySystemTime();
    status = STATUS_SUCCESS;

    if ((now.QuadPart - Context->LastUsbBattMngrQueryTime.QuadPart) > 10000000ll)
    {
        status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_GET_USB_CHG_STATUS, InputBuffer, InputBufferSize);
    }

    Context->LastUsbBattMngrQueryTime = now;

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *OutputBuffer = Context->UsbinPower[portIndex];
    *BytesReturned = sizeof(ULONGLONG);

    return STATUS_SUCCESS;
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
        if (InputBufferSize < PMICGLINK_I2C_HEADER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

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

    if (OutputBufferSize < PMICGLINK_I2C_HEADER_SIZE + Context->I2CDataLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(OutputBuffer, Context->I2CHeader, PMICGLINK_I2C_HEADER_SIZE);
    RtlCopyMemory(OutputBuffer + PMICGLINK_I2C_HEADER_SIZE, Context->I2CData, Context->I2CDataLength);

    *BytesReturned = PMICGLINK_I2C_HEADER_SIZE + Context->I2CDataLength;
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_GWS, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->gws_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_CWS, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->cws_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_STP, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->stp_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_STV, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->stv_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_TIP, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->tip_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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

    if ((OutputBuffer == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Context->GlinkChannelConnected)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = PmicGlink_SyncSendReceive(Context, IOCTL_PMICGLINK_TAD_TIV, InputBuffer, InputBufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OutputBufferSize < sizeof(*OutputBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OutputBuffer = Context->tiv_out;
    *BytesReturned = sizeof(*OutputBuffer);
    return STATUS_SUCCESS;
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
    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V1_ALIAS1:
        return IOCTL_BATTMNGR_GET_BATT_PRESENT;

    case IOCTL_BATTMNGR_SET_OPERATION_MODE_V1:
        return IOCTL_BATTMNGR_SET_OPERATION_MODE;

    case IOCTL_BATTMNGR_SET_CHARGE_RATE_V1:
        return IOCTL_BATTMNGR_SET_CHARGE_RATE;

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
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->NotificationFlag = FALSE;
    Context->Notify = FALSE;
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

    RtlCopyMemory(
        &Context->LegacyControlCharging,
        request,
        sizeof(Context->LegacyControlCharging));

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

        guidIsZero = (RtlCompareMemory(&request->charger_guid, &zeroGuid, sizeof(GUID)) == sizeof(GUID));
        if (guidIsZero)
        {
            Context->LegacyChargeRate.charge_perc = 0;
        }
        else
        {
            switch (request->batt_charger_source_type)
            {
            case 0:
                Context->LegacyChargeRate.charge_perc = 500;
                break;
            case 1:
                Context->LegacyChargeRate.charge_perc = 1500;
                break;
            default:
                Context->LegacyChargeRate.charge_perc = 0;
                break;
            }
        }
        break;

    case 5:
        if (Context->LegacyOperationalMode.mode_type != 1)
        {
            return STATUS_SUCCESS;
        }

        guidIsZero = (RtlCompareMemory(&request->charger_guid, &zeroGuid, sizeof(GUID)) == sizeof(GUID));
        if (guidIsZero
            && (request->batt_charger_source_type == 0)
            && (request->batt_charger_flags == 0)
            && (request->batt_charger_voltage == 0)
            && (request->batt_charger_port_type == 0)
            && (request->batt_charger_port_id == 0))
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
            switch (request->batt_charger_source_type)
            {
            case 0:
                Context->LegacyChargeRate.charge_perc = 500;
                break;
            case 1:
                Context->LegacyChargeRate.charge_perc = 1500;
                break;
            default:
                Context->LegacyChargeRate.charge_perc = 0;
                break;
            }
        }
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    Context->LegacyChargeStatus.rate = (LONG)Context->LegacyChargeRate.charge_perc;
    Context->LegacyStatusNotificationPending = TRUE;
    PmicGlinkNotify_PingBattMiniClass(Context);
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
        if ((Context->LegacyLastBattIdQueryMsec == 0)
            || (nowMsec < Context->LegacyLastBattIdQueryMsec)
            || ((nowMsec - Context->LegacyLastBattIdQueryMsec) >= 500))
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_BATT_ID,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            Context->LegacyLastBattIdQueryMsec = nowMsec;
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
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outChgStatus)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        status = STATUS_SUCCESS;
        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        if ((Context->LegacyLastChargeStatusQueryMsec == 0)
            || (nowMsec < Context->LegacyLastChargeStatusQueryMsec)
            || ((nowMsec - Context->LegacyLastChargeStatusQueryMsec) >= 250))
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_CHARGER_STATUS,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            Context->LegacyLastChargeStatusQueryMsec = nowMsec;
            if (!NT_SUCCESS(status))
            {
                status = STATUS_SUCCESS;
            }
        }

        outChgStatus = (BATT_MNGR_CHG_STATUS_OUT*)OutputBuffer;
        *outChgStatus = Context->LegacyChargeStatus;
        outChgStatus->charging_source = 2;
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

        if ((Context->LegacyLastBattInfoQueryMsec == 0)
            || (nowMsec < Context->LegacyLastBattInfoQueryMsec)
            || ((nowMsec - Context->LegacyLastBattInfoQueryMsec) >= 250)
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
                Context->LegacyLastBattInfoQueryMsec = nowMsec;
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
        status = STATUS_SUCCESS;
        if (Context->LegacyStatusNotificationPending)
        {
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_GET_BATT_PRESENT,
                (PUCHAR)InputBuffer,
                InputBufferSize);

            if (NT_SUCCESS(status))
            {
                Context->LegacyStatusNotificationPending = FALSE;
            }
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
        return STATUS_SUCCESS;
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
            return STATUS_INVALID_PARAMETER;
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
            return STATUS_INVALID_PARAMETER;
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

    outIndex = 0;
    StringBuffer[0] = '\0';

    if (Buffer == NULL)
    {
        return;
    }

    for (i = 0; i < BufferSize; i++)
    {
        UCHAR value;

        if ((outIndex + 3u) >= StringSize)
        {
            break;
        }

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
    SIZE_T i;

    if ((AnsiString == NULL) || (UnicodeString == NULL) || (UnicodeChars == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    i = 0;
    while ((i + 1u) < UnicodeChars && (AnsiString[i] != '\0'))
    {
        UnicodeString[i] = (WCHAR)(UCHAR)AnsiString[i];
        i++;
    }

    UnicodeString[i] = L'\0';
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlink_RetrieveRxData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(BufferSize) const UCHAR* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    if ((Context == NULL) || (Buffer == NULL) || (BufferSize == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferSize >= sizeof(Context->LastUsbcNotification.AsUINT8))
    {
        RtlCopyMemory(
            Context->LastUsbcNotification.AsUINT8,
            Buffer,
            sizeof(Context->LastUsbcNotification.AsUINT8));
        Context->PendingPan = (UCHAR)Context->LastUsbcNotification.detail.common.port_index;
    }
    else
    {
        SIZE_T copySize;

        copySize = (BufferSize < sizeof(Context->OemReceivedData))
            ? BufferSize
            : sizeof(Context->OemReceivedData);
        RtlCopyMemory(Context->OemReceivedData, Buffer, copySize);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
PmicGlinkNotifyRxIntentReqCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T RequestedSize
    )
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(RequestedSize);
    return TRUE;
}

VOID
PmicGlinkNotifyRxIntentCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T Size
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Size);

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext != NULL)
    {
        deviceContext->EventID += 1;
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

    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if (deviceContext != NULL)
    {
        deviceContext->NotificationFlag = TRUE;
    }
}

VOID
PmicGlinkRxNotificationWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);
    if (context != NULL)
    {
        (VOID)PmicGlink_RetrieveRxData(
            context,
            context->LastUsbcNotification.AsUINT8,
            sizeof(context->LastUsbcNotification.AsUINT8));
    }

    WdfObjectDelete(WorkItem);
}

VOID
PmicGlinkRpeADSPStateNotificationCallback(
    _In_opt_ PVOID Context,
    _In_ ULONG PreviousState,
    _In_opt_ PULONG CurrentState
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(PreviousState);

    deviceContext = (PPMIC_GLINK_DEVICE_CONTEXT)Context;
    if ((deviceContext == NULL) || (CurrentState == NULL))
    {
        return;
    }

    if (*CurrentState != 0)
    {
        deviceContext->GlinkLinkStateUp = TRUE;
        if (!deviceContext->GlinkChannelConnected)
        {
            (VOID)PmicGlink_OpenGlinkChannel(deviceContext);
        }
    }
    else
    {
        deviceContext->GlinkLinkStateUp = FALSE;
        deviceContext->GlinkChannelConnected = FALSE;
        deviceContext->GlinkChannelRestart = TRUE;
    }
}

VOID
PmicGlinkAppStateNotificationCallback(
    _In_opt_ PVOID Context,
    _In_ ULONG PreviousState,
    _In_opt_ PULONG CurrentState
    )
{
    PmicGlinkRpeADSPStateNotificationCallback(Context, PreviousState, CurrentState);
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
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);
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
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);
}

VOID
PmicGlinkUlogRegisterInterfaceWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WdfObjectDelete(WorkItem);
}

VOID
PmicGlinkUlogStateNotificationCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ PMICGLINK_CHANNEL_EVENT Event
    )
{
    if (Context != NULL)
    {
        PmicGlinkStateNotificationCb(Handle, (PPMIC_GLINK_DEVICE_CONTEXT)Context, Event);
    }
}

BOOLEAN
PmicGlinkUlogNotifyRxIntentReqCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T RequestedSize
    )
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(RequestedSize);
    return TRUE;
}

VOID
PmicGlinkUlogNotifyRxIntentCb(
    _In_opt_ PVOID Handle,
    _In_opt_ PVOID Context,
    _In_ SIZE_T Size
    )
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Size);
}

NTSTATUS
PmicGlinkUlog_OpenGlinkChannelUlog(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->RpeInitialized = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
PmicGlinkUlog_RetrieveRxData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(BufferSize) const CHAR* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    if ((Context == NULL) || (Buffer == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return PmicGlink_RetrieveRxData(Context, (const UCHAR*)Buffer, BufferSize);
}

NTSTATUS
PmicGlinkUlog_SendData(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_reads_bytes_(BufferSize) const VOID* Buffer,
    _In_ SIZE_T BufferSize
    )
{
    if ((Context == NULL) || (Buffer == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return PmicGlink_SendData(Context, 0x15u, (PVOID)Buffer, BufferSize, TRUE);
}

NTSTATUS
PmicGlinkDeviceCreate(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    return PmicGlinkEvtDeviceAdd(Driver, DeviceInit);
}

NTSTATUS
PmicGlinkOnDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    return PmicGlinkDeviceCreate(Driver, DeviceInit);
}

NTSTATUS
PmicGlinkQueueInitialize(
    _In_ WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);
    return STATUS_SUCCESS;
}

VOID
PmicGlinkOnDriverCleanup(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
}

VOID
PmicGlinkOnDriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
}

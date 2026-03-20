/* Platform glue, UCSI/OEM helpers, USB-C work items, and queue module stub entrypoint. Included by main.c. */

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


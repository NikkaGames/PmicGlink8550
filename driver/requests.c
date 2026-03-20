/* Request dispatch, battery handling, and GLINK RX path. Included by main.c. */

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
        return IOCTL_BATTMNGR_SET_STATUS_CRITERIA;

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

static const CHAR*
PmicGlinkLegacyBattIoctlName(
    _In_ ULONG IoControlCode
    )
{
    switch (IoControlCode)
    {
    case IOCTL_BATTMNGR_GET_CAPABILITIES:
        return "GET_CAPABILITIES";
    case IOCTL_BATTMNGR_GET_BATT_ID:
        return "GET_BATT_ID";
    case IOCTL_BATTMNGR_GET_CHARGER_STATUS:
        return "GET_CHARGER_STATUS";
    case IOCTL_BATTMNGR_GET_BATT_INFO:
        return "GET_BATT_INFO";
    case IOCTL_BATTMNGR_CONTROL_CHARGING:
        return "CONTROL_CHARGING";
    case IOCTL_BATTMNGR_SET_STATUS_CRITERIA:
        return "SET_STATUS_CRITERIA";
    case IOCTL_BATTMNGR_GET_BATT_PRESENT:
        return "GET_BATT_PRESENT";
    case IOCTL_BATTMNGR_SET_OPERATION_MODE:
        return "SET_OPERATION_MODE";
    case IOCTL_BATTMNGR_SET_CHARGE_RATE:
        return "SET_CHARGE_RATE";
    case IOCTL_BATTMNGR_NOTIFY_IFACE_FREE:
        return "NOTIFY_IFACE_FREE";
    case IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS:
        return "GET_BATTERY_PRESENT_STATUS";
    case IOCTL_BATTMNGR_GET_TEST_INFO:
        return "GET_TEST_INFO";
    case IOCTL_BATTMNGR_GET_BATT_PRESENT_V1:
        return "GET_BATT_PRESENT_V1";
    case IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT:
        return "ENABLE_CHARGE_LIMIT";
    default:
        return "UNKNOWN";
    }
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
    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy ioctl in=0x%08lx normalized=0x%08lx (%s) inLen=%Iu outLen=%Iu\n",
        IoControlCode,
        effectiveIoctl,
        PmicGlinkLegacyBattIoctlName(effectiveIoctl),
        InputBufferSize,
        OutputBufferSize);

    switch (effectiveIoctl)
    {
    case IOCTL_BATTMNGR_GET_CAPABILITIES:
    {
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_CAPABILITIES\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_BATT_ID\n");
        BATT_MNGR_GET_BATT_ID_OUT* outBattId;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outBattId)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        outBattId = (BATT_MNGR_GET_BATT_ID_OUT*)OutputBuffer;
        outBattId->batt_id = 0;
        *BytesReturned = sizeof(*outBattId);
        PmicGlinkTryAttachBattMiniFromIoctl(Context, "GET_BATT_ID");

        if (Context->GlinkChannelRestart || !Context->GlinkChannelConnected)
        {
            ULONGLONG recoverNowMsec;

            recoverNowMsec = PmicGlink_Helper_get_rel_time_msec();
            if ((recoverNowMsec >= gPmicGlinkLastChannelRecoverAttemptMsec)
                && ((recoverNowMsec - gPmicGlinkLastChannelRecoverAttemptMsec) >= 2000ull))
            {
                NTSTATUS recoverStatus;

                gPmicGlinkLastChannelRecoverAttemptMsec = recoverNowMsec;
                recoverStatus = PmicGlink_OpenGlinkChannel(Context);
                Trace(TRACE_LEVEL_INFORMATION, "pmicglink: GET_BATT_ID recover attempt status=0x%08lx linkUp=%u restart=%u connected=%u\n",
                    (ULONG)recoverStatus,
                    Context->GlinkLinkStateUp ? 1u : 0u,
                    Context->GlinkChannelRestart ? 1u : 0u,
                    Context->GlinkChannelConnected ? 1u : 0u);
            }

            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: GET_BATT_ID no-glink restart=%u connected=%u linkUp=%u allReq=%u loaded=%u rpe=%u handle=%p cachedBattId=%lu\n",
                Context->GlinkChannelRestart ? 1u : 0u,
                Context->GlinkChannelConnected ? 1u : 0u,
                Context->GlinkLinkStateUp ? 1u : 0u,
                Context->AllReqIntfArrived ? 1u : 0u,
                Context->GlinkDeviceLoaded ? 1u : 0u,
                Context->RpeInitialized ? 1u : 0u,
                gPmicGlinkMainChannelHandle,
                Context->LegacyBattId.batt_id);
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: GET_BATT_ID result status=0x%08lx battId=%lu queryTick=%I64u\n",
            (ULONG)status,
            outBattId->batt_id,
            nowMsec);
        return status;
    }

    case IOCTL_BATTMNGR_GET_CHARGER_STATUS:
    {
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_CHARGER_STATUS\n");
        BATT_MNGR_CHG_STATUS_OUT* outChgStatus;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outChgStatus)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        PmicGlinkTryAttachBattMiniFromIoctl(Context, "GET_CHARGER_STATUS");
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

        PmicGlinkRefreshModernBatterySoc(Context, nowMsec, FALSE);

        outChgStatus = (BATT_MNGR_CHG_STATUS_OUT*)OutputBuffer;
        *outChgStatus = Context->LegacyChargeStatus;
        outChgStatus->charging_source = 2u;

        if ((gPmicGlinkLastChargeStatusTraceMsec == 0)
            || (nowMsec < gPmicGlinkLastChargeStatusTraceMsec)
            || ((nowMsec - gPmicGlinkLastChargeStatusTraceMsec) >= 2000))
        {
            gPmicGlinkLastChargeStatusTraceMsec = nowMsec;
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: qstatus ps=0x%08lx src=%lu cap=%lu volt=%lu rate=%ld modern=[raw:%lu x100:%lu valid:%u] usb=[%ld,%ld,%ld,%ld]\n",
                outChgStatus->power_state,
                outChgStatus->charging_source,
                outChgStatus->capacity,
                outChgStatus->voltage,
                outChgStatus->rate,
                Context->ModernSocRaw,
                Context->ModernSocX100,
                Context->ModernSocValid ? 1u : 0u,
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_BATT_INFO\n");
        const BATT_MNGR_GET_BATT_INFO* request;
        BATT_MNGR_GET_BATT_INFO_OUT* outBattInfo;
        ULONG reportedFullCapacity;
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

        PmicGlinkRefreshModernBatterySoc(Context, nowMsec, FALSE);

        outBattInfo = (BATT_MNGR_GET_BATT_INFO_OUT*)OutputBuffer;
        RtlZeroMemory(outBattInfo, sizeof(*outBattInfo));
        reportedFullCapacity = PmicGlinkGetLegacyReportedFullCapacity(
            Context,
            Context->LegacyChargeStatus.capacity);

        switch (request->batt_info_type)
        {
        case 0:
            outBattInfo->batt_info = Context->LegacyBattInfo;
            outBattInfo->batt_info.full_charged_capacity = reportedFullCapacity;
            break;

        case 1:
            ((ULONG*)outBattInfo)[0] = Context->LegacyReportingScale[0].granularity;
            ((ULONG*)outBattInfo)[1] = reportedFullCapacity;
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: GET_BATT_INFO type=%lu status=0x%08lx temp=%lu est=%lu design=%lu full=%lu cycle=%lu\n",
            request->batt_info_type,
            (ULONG)status,
            Context->LegacyBattTemperature,
            Context->LegacyBattEstimatedTime,
            Context->LegacyBattInfo.designed_capacity,
            reportedFullCapacity,
            Context->LegacyBattInfo.cycle_count);
        return status;
    }

    case IOCTL_BATTMNGR_CONTROL_CHARGING:
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case CONTROL_CHARGING\n");
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
    {
        const BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA* request;

        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case SET_STATUS_CRITERIA\n");
        PmicGlinkTryAttachBattMiniFromIoctl(Context, "SET_STATUS_CRITERIA");

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(*request)))
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
            status = PmicGlink_SyncSendReceive(
                Context,
                IOCTL_BATTMNGR_SET_STATUS_CRITERIA,
                (PUCHAR)InputBuffer,
                InputBufferSize);
            if (NT_SUCCESS(status))
            {
                Context->LegacyStatusCriteria = *request;
                Context->Notify = TRUE;
            }

            return status;
        }

        return STATUS_SUCCESS;
    }

    case IOCTL_BATTMNGR_GET_BATT_PRESENT:
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_BATT_PRESENT\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case SET_OPERATION_MODE\n");
        return PmicGlink_SyncSendReceive(
            Context,
            IOCTL_BATTMNGR_SET_OPERATION_MODE,
            (PUCHAR)InputBuffer,
            InputBufferSize);

    case IOCTL_BATTMNGR_SET_CHARGE_RATE:
    {
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case SET_CHARGE_RATE\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case NOTIFY_IFACE_FREE\n");
        return PmicGlinkNotify_Interface_Free(Context);

    case IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS:
    {
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_BATTERY_PRESENT_STATUS\n");
        BATT_MNGR_GET_BATTERY_PRESENT_STATUS* outPresentStatus;
        ULONGLONG nowMsec;

        if ((OutputBuffer == NULL) || (OutputBufferSize != sizeof(*outPresentStatus)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        PmicGlinkRefreshModernBatterySoc(Context, nowMsec, TRUE);

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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_TEST_INFO\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case GET_BATT_PRESENT_V1\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case ENABLE_CHARGE_LIMIT\n");
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: legacy case UNKNOWN normalized=0x%08lx\n",
            effectiveIoctl);
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

static
ULONG
PmicGlinkGetLegacyReportedFullCapacity(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacityHint
    )
{
    ULONG fullCapacity;
    ULONG designedCapacity;

    if (Context == NULL)
    {
        return 0u;
    }

    fullCapacity = Context->LegacyBattInfo.full_charged_capacity;
    if ((fullCapacity == 0u) || (fullCapacity == 0xFFFFFFFFu))
    {
        return Context->LegacyBattInfo.designed_capacity;
    }

    designedCapacity = Context->LegacyBattInfo.designed_capacity;
    if ((designedCapacity != 0u)
        && (designedCapacity != 0xFFFFFFFFu)
        && (designedCapacity > fullCapacity))
    {
        UNREFERENCED_PARAMETER(RawCapacityHint);
        fullCapacity = designedCapacity;
    }

    return fullCapacity;
}

static
ULONG
PmicGlinkNormalizeLegacyRemainingCapacity(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacity
    )
{
    ULONG fullCapacity;
    ULONGLONG scaledCapacity;

    if ((Context == NULL) || (RawCapacity == 0xFFFFFFFFu))
    {
        return RawCapacity;
    }

    fullCapacity = PmicGlinkGetLegacyReportedFullCapacity(Context, RawCapacity);
    if ((fullCapacity == 0u) || (fullCapacity == 0xFFFFFFFFu))
    {
        return RawCapacity;
    }

    if (RawCapacity <= 100u)
    {
        scaledCapacity = (((ULONGLONG)fullCapacity * (ULONGLONG)RawCapacity) + 50ull) / 100ull;
        return (ULONG)scaledCapacity;
    }

    if ((RawCapacity > 100u) && (RawCapacity <= 10000u))
    {
        scaledCapacity = (((ULONGLONG)fullCapacity * (ULONGLONG)RawCapacity) + 5000ull) / 10000ull;
        if ((scaledCapacity != 0ull) && (scaledCapacity <= (ULONGLONG)fullCapacity))
        {
            return (ULONG)scaledCapacity;
        }
    }

    if ((ULONGLONG)RawCapacity > ((ULONGLONG)fullCapacity * 4ull))
    {
        ULONG scaledBy1000;
        ULONG scaledBy100;

        scaledBy1000 = RawCapacity / 1000u;
        if ((scaledBy1000 != 0u) && ((ULONGLONG)scaledBy1000 <= ((ULONGLONG)fullCapacity * 2ull)))
        {
            return scaledBy1000;
        }

        scaledBy100 = RawCapacity / 100u;
        if ((scaledBy100 != 0u) && ((ULONGLONG)scaledBy100 <= ((ULONGLONG)fullCapacity * 2ull)))
        {
            return scaledBy100;
        }
    }

    return RawCapacity;
}

static
BOOLEAN
PmicGlinkTryConvertModernSocX100(
    _In_ ULONG RawValue,
    _Out_ PULONG SocX100
    )
{
    ULONG normalized;

    if (SocX100 == NULL)
    {
        return FALSE;
    }

    if (RawValue == 0xFFFFFFFFu)
    {
        return FALSE;
    }

    normalized = 0u;
    if (RawValue <= 100u)
    {
        normalized = RawValue * 100u;
    }
    else if (RawValue <= 10000u)
    {
        normalized = RawValue;
    }
    else if (RawValue <= 100000u)
    {
        normalized = (RawValue + 5u) / 10u;
    }
    else if (RawValue <= 1000000u)
    {
        normalized = (RawValue + 50u) / 100u;
    }

    if ((normalized == 0u) || (normalized > 10000u))
    {
        return FALSE;
    }

    *SocX100 = normalized;
    return TRUE;
}

static
BOOLEAN
PmicGlinkTryExtractModernSocFromDebugMsg(
    _In_reads_bytes_(TextLength) const CHAR* Text,
    _In_ SIZE_T TextLength,
    _Out_ PULONG RawValue,
    _Out_ PULONG SocX100
    )
{
    static const CHAR* const patterns[] =
    {
        "ui_capacity",
        "fake_soc",
        "rsoc",
        "ssoc",
        "soc",
        "capacity"
    };
    SIZE_T patternIndex;

    if ((Text == NULL) || (RawValue == NULL) || (SocX100 == NULL) || (TextLength == 0u))
    {
        return FALSE;
    }

    for (patternIndex = 0; patternIndex < ARRAYSIZE(patterns); patternIndex++)
    {
        const CHAR* pattern;
        SIZE_T patternLength;
        SIZE_T i;

        pattern = patterns[patternIndex];
        patternLength = 0u;
        while (pattern[patternLength] != '\0')
        {
            patternLength++;
        }

        if ((patternLength == 0u) || (patternLength >= TextLength))
        {
            continue;
        }

        for (i = 0; (i + patternLength) <= TextLength; i++)
        {
            SIZE_T matchIndex;
            SIZE_T valueIndex;
            ULONG value;
            SIZE_T digitCount;
            BOOLEAN matched;

            if ((i != 0u)
                && (((Text[i - 1] >= '0') && (Text[i - 1] <= '9'))
                    || ((Text[i - 1] >= 'A') && (Text[i - 1] <= 'Z'))
                    || ((Text[i - 1] >= 'a') && (Text[i - 1] <= 'z'))
                    || (Text[i - 1] == '_')))
            {
                continue;
            }

            matched = TRUE;
            for (matchIndex = 0; matchIndex < patternLength; matchIndex++)
            {
                CHAR hay;
                CHAR needle;

                hay = Text[i + matchIndex];
                needle = pattern[matchIndex];
                if ((hay >= 'A') && (hay <= 'Z'))
                {
                    hay = (CHAR)(hay + ('a' - 'A'));
                }

                if (hay != needle)
                {
                    matched = FALSE;
                    break;
                }
            }

            if (!matched)
            {
                continue;
            }

            valueIndex = i + patternLength;
            while ((valueIndex < TextLength)
                && ((Text[valueIndex] == ' ')
                    || (Text[valueIndex] == '\t')
                    || (Text[valueIndex] == '=')
                    || (Text[valueIndex] == ':')
                    || (Text[valueIndex] == '-')
                    || (Text[valueIndex] == '_')
                    || (Text[valueIndex] == '[')
                    || (Text[valueIndex] == ']')))
            {
                valueIndex++;
            }

            if ((valueIndex >= TextLength)
                || (Text[valueIndex] < '0')
                || (Text[valueIndex] > '9'))
            {
                continue;
            }

            value = 0u;
            digitCount = 0u;
            while ((valueIndex < TextLength)
                && (Text[valueIndex] >= '0')
                && (Text[valueIndex] <= '9')
                && (digitCount < 8u))
            {
                value = (value * 10u) + (ULONG)(Text[valueIndex] - '0');
                valueIndex++;
                digitCount++;
            }

            if ((digitCount == 0u) || !PmicGlinkTryConvertModernSocX100(value, SocX100))
            {
                continue;
            }

            *RawValue = value;
            return TRUE;
        }
    }

    return FALSE;
}

static
VOID
PmicGlinkApplyModernSocToLegacy(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG SocX100
    )
{
    ULONG reportedFullCapacity;
    ULONGLONG liveCapacity;

    if (Context == NULL)
    {
        return;
    }

    if (SocX100 > 10000u)
    {
        SocX100 = 10000u;
    }

    Context->ModernSocX100 = SocX100;
    Context->ModernSocValid = TRUE;
    Context->LegacyBattPercentage = (UCHAR)((SocX100 + 50u) / 100u);
    if (Context->LegacyBattPercentage > 100u)
    {
        Context->LegacyBattPercentage = 100u;
    }

    reportedFullCapacity = PmicGlinkGetLegacyReportedFullCapacity(Context, Context->LegacyChargeStatusRawCapacity);
    if ((reportedFullCapacity == 0u) || (reportedFullCapacity == 0xFFFFFFFFu))
    {
        return;
    }

    liveCapacity = (((ULONGLONG)reportedFullCapacity * (ULONGLONG)SocX100) + 5000ull) / 10000ull;
    if (liveCapacity > (ULONGLONG)reportedFullCapacity)
    {
        liveCapacity = (ULONGLONG)reportedFullCapacity;
    }

    Context->LegacyChargeStatus.capacity = (ULONG)liveCapacity;
}

static
NTSTATUS
PmicGlinkQueryModernBatterySoc(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    struct
    {
        ULONGLONG Header;
        ULONG MessageOp;
        ULONG BatteryId;
        ULONG PropertyId;
        ULONG Value;
    } request;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    request.Header = 0x10000800Aull;
    request.MessageOp = PMICGLINK_BC_BATTERY_STATUS_GET_OPCODE;
    request.BatteryId = 0u;
    request.PropertyId = PMICGLINK_BC_PROP_BATT_CAPACITY;
    request.Value = 0u;
    return PmicGlink_SendData(
        Context,
        PMICGLINK_BC_BATTERY_STATUS_GET_OPCODE,
        &request,
        sizeof(request),
        TRUE);
}

static
VOID
PmicGlinkRefreshModernBatterySoc(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONGLONG NowMsec,
    _In_ BOOLEAN Force
    )
{
    NTSTATUS status;

    if ((Context == NULL) || !Context->GlinkChannelConnected)
    {
        return;
    }

    if (!Force
        && (Context->LegacyLastModernSocQueryMsec != 0)
        && (NowMsec >= Context->LegacyLastModernSocQueryMsec)
        && ((NowMsec - Context->LegacyLastModernSocQueryMsec) < 250ull))
    {
        return;
    }

    status = PmicGlinkQueryModernBatterySoc(Context);
    Context->LegacyLastModernSocQueryMsec = NowMsec;
    if (!NT_SUCCESS(status))
    {
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: modern_soc refresh failed status=0x%08lx force=%u\n",
            (ULONG)status,
            Force ? 1u : 0u);
    }
}

static
UCHAR
PmicGlinkComputeLegacyBattPercentage(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG RawCapacity
    )
{
    ULONG fullCapacity;
    ULONGLONG scaledPercent;

    if (Context == NULL)
    {
        return 100u;
    }

    if (RawCapacity == 0xFFFFFFFFu)
    {
        return Context->LegacyBattPercentage;
    }

    if (RawCapacity <= 100u)
    {
        return (UCHAR)RawCapacity;
    }

    fullCapacity = PmicGlinkGetLegacyReportedFullCapacity(Context, RawCapacity);

    if ((fullCapacity != 0u) && (fullCapacity != 0xFFFFFFFFu))
    {
        scaledPercent = (((ULONGLONG)RawCapacity * 100ull) + ((ULONGLONG)fullCapacity / 2ull))
            / (ULONGLONG)fullCapacity;
        if (scaledPercent > 100ull)
        {
            scaledPercent = 100ull;
        }

        return (UCHAR)scaledPercent;
    }

    if (RawCapacity <= 10000u)
    {
        return (UCHAR)((RawCapacity + 50u) / 100u);
    }

    return 100u;
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
        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX opcode mismatch expected=%lu got=%lu size=%Iu\n",
            OpCode,
            packetOpCode,
            BufferSize);
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
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX batt_id=%lu size=%Iu\n",
                Context->LegacyBattId.batt_id,
                BufferSize);
        }
        else
        {
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX batt_id short packet size=%Iu\n",
                BufferSize);
        }
        break;

    case 1u:
        if (BufferSize >= 40u)
        {
            ULONG rawCapacity;
            ULONG auxValue;

            RtlCopyMemory(&Context->LegacyBattStateId, Buffer + 12, sizeof(Context->LegacyBattStateId));
            RtlCopyMemory(&rawCapacity, Buffer + 16, sizeof(rawCapacity));
            RtlCopyMemory(&auxValue, Buffer + 32, sizeof(auxValue));
            Context->LegacyChargeStatusRawCapacity = rawCapacity;
            Context->LegacyChargeStatusAux = auxValue;
            Context->LegacyChargeStatus.capacity = PmicGlinkNormalizeLegacyRemainingCapacity(
                Context,
                rawCapacity);
            RtlCopyMemory(&Context->LegacyChargeStatus.rate, Buffer + 20, sizeof(Context->LegacyChargeStatus.rate));
            RtlCopyMemory(&Context->LegacyChargeStatus.voltage, Buffer + 24, sizeof(Context->LegacyChargeStatus.voltage));
            RtlCopyMemory(&Context->LegacyChargeStatus.power_state, Buffer + 28, sizeof(Context->LegacyChargeStatus.power_state));
            RtlCopyMemory(&Context->LegacyBattTemperature, Buffer + 36, sizeof(Context->LegacyBattTemperature));
            Context->LegacyBattPercentage = PmicGlinkComputeLegacyBattPercentage(
                Context,
                Context->LegacyChargeStatus.capacity);
            if (Context->ModernSocValid)
            {
                PmicGlinkApplyModernSocToLegacy(Context, Context->ModernSocX100);
            }
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX chg battState=%lu ps=0x%08lx capRaw=%lu capAux=%lu cap=%lu pct=%u volt=%lu rate=%ld temp=%lu\n",
                Context->LegacyBattStateId,
                Context->LegacyChargeStatus.power_state,
                Context->LegacyChargeStatusRawCapacity,
                Context->LegacyChargeStatusAux,
                Context->LegacyChargeStatus.capacity,
                Context->LegacyBattPercentage,
                Context->LegacyChargeStatus.voltage,
                Context->LegacyChargeStatus.rate,
                Context->LegacyBattTemperature);
        }
        else
        {
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX chg short packet size=%Iu\n",
                BufferSize);
        }
        break;

    case PMICGLINK_BC_BATTERY_STATUS_GET_OPCODE:
        if (BufferSize >= 24u)
        {
            ULONG propertyId;
            ULONG value;
            ULONG retCode;

            RtlCopyMemory(&propertyId, Buffer + 12, sizeof(propertyId));
            RtlCopyMemory(&value, Buffer + 16, sizeof(value));
            RtlCopyMemory(&retCode, Buffer + 20, sizeof(retCode));
            status = (retCode == 0u) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            if ((retCode == 0u) && (propertyId == PMICGLINK_BC_PROP_BATT_CAPACITY))
            {
                ULONG socX100;

                if (PmicGlinkTryConvertModernSocX100(value, &socX100))
                {
                    if ((value == 10000u)
                        && Context->ModernSocValid
                        && (Context->ModernSocRaw != 0xFFFFFFFFu)
                        && (Context->ModernSocRaw != 10000u))
                    {
                        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX modern_soc raw=%lu ignored keeping raw=%lu x100=%lu pct=%u\n",
                            value,
                            Context->ModernSocRaw,
                            Context->ModernSocX100,
                            Context->LegacyBattPercentage);
                        break;
                    }

                    Context->ModernSocRaw = value;
                    PmicGlinkApplyModernSocToLegacy(Context, socX100);
                    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX modern_soc raw=%lu x100=%lu pct=%u cap=%lu\n",
                        value,
                        socX100,
                        Context->LegacyBattPercentage,
                        Context->LegacyChargeStatus.capacity);
                }
                else
                {
                    if (!Context->ModernSocValid
                        || (Context->ModernSocRaw == 0xFFFFFFFFu)
                        || (Context->ModernSocRaw == 10000u))
                    {
                        Context->ModernSocValid = FALSE;
                    }
                    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX modern_soc invalid raw=%lu\n",
                        value);
                }
            }
            else
            {
                Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX modern_resp prop=%lu value=%lu ret=0x%08lx\n",
                    propertyId,
                    value,
                    retCode);
            }
        }
        else
        {
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX modern_resp short packet size=%Iu\n",
                BufferSize);
        }
        break;

    case PMICGLINK_BC_ADSP_DEBUG_OPCODE:
        if (BufferSize >= 16u)
        {
            ULONG debugValue;

            RtlCopyMemory(&debugValue, Buffer + 12, sizeof(debugValue));
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX adsp_debug value=0x%08lx\n",
                debugValue);
        }
        else
        {
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX adsp_debug short packet size=%Iu\n",
                BufferSize);
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
        if (BufferSize >= 732u)
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
            if (BufferSize >= 740u)
            {
                RtlCopyMemory(&Context->LegacyBattEstimatedTime, Buffer + 736, sizeof(Context->LegacyBattEstimatedTime));
            }
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
            Context->LegacyChargeStatus.capacity = PmicGlinkNormalizeLegacyRemainingCapacity(
                Context,
                Context->LegacyChargeStatusRawCapacity);
            Context->LegacyBattPercentage = PmicGlinkComputeLegacyBattPercentage(
                Context,
                Context->LegacyChargeStatus.capacity);
            if (Context->ModernSocValid)
            {
                PmicGlinkApplyModernSocToLegacy(Context, Context->ModernSocX100);
            }
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX batt_info design=%lu full=%lu cycle=%lu tech=%u crit=%lu est=%lu capRaw=%lu capAux=%lu cap=%lu pct=%u\n",
                Context->LegacyBattInfo.designed_capacity,
                Context->LegacyBattInfo.full_charged_capacity,
                Context->LegacyBattInfo.cycle_count,
                Context->LegacyBattInfo.technology,
                Context->LegacyBattInfo.critical_bias,
                Context->LegacyBattEstimatedTime,
                Context->LegacyChargeStatusRawCapacity,
                Context->LegacyChargeStatusAux,
                Context->LegacyChargeStatus.capacity,
                Context->LegacyBattPercentage);
        }
        else
        {
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX batt_info short packet size=%Iu\n",
                BufferSize);
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
    {
        CHAR debugText[PMICGLINK_BC_DEBUG_MSG_MAX_CHARS + 1u];
        ULONG debugRawSoc;
        ULONG debugSocX100;
        ULONG textLength;
        SIZE_T copyLength;
        SIZE_T i;

        RtlZeroMemory(debugText, sizeof(debugText));
        debugRawSoc = 0u;
        debugSocX100 = 0u;
        textLength = 0u;

        if (BufferSize >= (12u + PMICGLINK_BC_DEBUG_MSG_MAX_CHARS + sizeof(ULONG)))
        {
            RtlCopyMemory(
                &textLength,
                Buffer + 12u + PMICGLINK_BC_DEBUG_MSG_MAX_CHARS,
                sizeof(textLength));
        }

        if (BufferSize >= 16u + sizeof(Context->OemPropData))
        {
            RtlCopyMemory(Context->OemPropData, Buffer + 16, sizeof(Context->OemPropData));
        }

        if ((textLength == 0u) || (textLength > PMICGLINK_BC_DEBUG_MSG_MAX_CHARS))
        {
            textLength = (ULONG)((BufferSize > 12u) ? (BufferSize - 12u) : 0u);
            if (textLength > PMICGLINK_BC_DEBUG_MSG_MAX_CHARS)
            {
                textLength = PMICGLINK_BC_DEBUG_MSG_MAX_CHARS;
            }
        }

        copyLength = textLength;
        if (copyLength > PMICGLINK_BC_DEBUG_MSG_MAX_CHARS)
        {
            copyLength = PMICGLINK_BC_DEBUG_MSG_MAX_CHARS;
        }

        if ((copyLength != 0u) && (BufferSize >= (12u + copyLength)))
        {
            RtlCopyMemory(debugText, Buffer + 12, copyLength);
            for (i = 0; i < copyLength; i++)
            {
                if ((debugText[i] == '\0') || (debugText[i] == '\r') || (debugText[i] == '\n'))
                {
                    debugText[i] = '\0';
                    break;
                }

                if (((UCHAR)debugText[i] < 0x20u) || ((UCHAR)debugText[i] > 0x7Eu))
                {
                    debugText[i] = ' ';
                }
            }
            debugText[PMICGLINK_BC_DEBUG_MSG_MAX_CHARS] = '\0';
        }

        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX debug_msg size=%Iu text=%s\n",
            BufferSize,
            debugText);

        if (PmicGlinkTryExtractModernSocFromDebugMsg(
            debugText,
            copyLength,
            &debugRawSoc,
            &debugSocX100))
        {
            Context->ModernSocRaw = debugRawSoc;
            PmicGlinkApplyModernSocToLegacy(Context, debugSocX100);
            Trace(TRACE_LEVEL_INFORMATION, "pmicglink: RX debug_soc raw=%lu x100=%lu pct=%u cap=%lu\n",
                debugRawSoc,
                debugSocX100,
                Context->LegacyBattPercentage,
                Context->LegacyChargeStatus.capacity);
        }
        break;
    }

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

        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: notify op=%lu id=0x%08lx type=%lu data=0x%08lx aux=0x%08lx size=%Iu\n",
            opCode,
            notificationId,
            notificationType,
            notificationData,
            notificationAux,
            BufferSize);

        if (notificationType == 2u)
        {
            switch (notificationId)
            {
            case 0x800Au:
                Context->LegacyLastAdspBatteryNotifyMsec = PmicGlink_Helper_get_rel_time_msec();
                gPmicGlinkLastChargeStatusQueryMsec = 0;
                gPmicGlinkLastBattInfoQueryMsec = 0;
                Context->Notify = TRUE;
                Context->LegacyStatusNotificationPending = TRUE;
                Context->LegacyStateChangePending = TRUE;
                Trace(TRACE_LEVEL_INFORMATION, "pmicglink: notify dispatch battmini id=0x%08lx data=0x%08lx\n",
                    notificationId,
                    notificationData);
                PmicGlinkNotifyBattMiniStatusFromGlink(Context, notificationData);
                break;

            case 0x800Bu:
                if (notificationAux == 0u)
                {
                    (VOID)PmicGlinkEvaluateAcpiMethodInteger(Context, "USBN", notificationData);
                    PmicGlinkDDI_NotifyUcsiAlert(Context, notificationId, notificationData);
                    (VOID)PmicGlinkNotifyUsbnIoctl(Context);
                    gPmicGlinkLastChargeStatusQueryMsec = 0;
                    gPmicGlinkLastBattInfoQueryMsec = 0;
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

    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: link_state_cb state=%lu xport=%s remote=%s\n",
        LinkInfo->LinkState,
        LinkInfo->Xport,
        LinkInfo->RemoteSs);

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

    if ((Context == NULL) || (CurrentState == NULL))
    {
        return;
    }

    deviceContext = PmicGlinkGetDeviceContext((WDFDEVICE)Context);
    if (deviceContext == NULL)
    {
        return;
    }

    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: rpe_adsp_state prev=%lu curr=%lu rpeInit=%u hibernate=%u\n",
        PreviousState,
        *CurrentState,
        deviceContext->RpeInitialized ? 1u : 0u,
        deviceContext->Hibernate ? 1u : 0u);

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
                    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: rpe_adsp_state link-state-cb registered handle=%p\n",
                        gPmicGlinkLinkStateHandle);
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

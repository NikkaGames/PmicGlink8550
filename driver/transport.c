/* Transport send/receive helpers, QCMB, ABD, and ACPI link-state plumbing. Included by main.c. */

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

    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: usbn_notify ioctl status=0x%08lx bytes=%Iu out=[0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx]\n",
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

        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: sendreq fallback internal ioctl=0x%08lx status=0x%08lx->0x%08lx\n",
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

        Trace(TRACE_LEVEL_INFORMATION, "pmicglink: sendreq_timeout fallback internal ioctl=0x%08lx status=0x%08lx->0x%08lx\n",
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


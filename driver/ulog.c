/* ULOG channel and log collection subsystem. Included by main.c. */

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
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, PMICGLINK_TRACE_LEVEL, "pmicglink: %s\n", lineBuffer);

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
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, PMICGLINK_TRACE_LEVEL, "pmicglink: %s\n", lineBuffer);
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

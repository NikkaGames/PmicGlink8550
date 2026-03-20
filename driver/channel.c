/* API interface state, comm slots, GLINK channel open/close, and state shims. Included by main.c. */

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


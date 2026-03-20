/* Device lifecycle, I/O entrypoints, and power callbacks. Included by main.c. */

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

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    WDF_DRIVER_CONFIG_INIT(&config, PmicGlinkOnDeviceAdd);
    config.EvtDriverUnload = PmicGlinkOnDriverUnload;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&driverAttributes, PMIC_GLINK_DRIVER_CONTEXT);
    driverAttributes.EvtCleanupCallback = PmicGlinkOnDriverCleanup;

    Trace(TRACE_LEVEL_INFORMATION, "pmicglink: build tag 2026-03-16 commit decomp-criteria-fallback\n");

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverAttributes,
        &config,
        &driver);
    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
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

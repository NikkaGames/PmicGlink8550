/* Queue wrappers and delayed-request timer. Included by main.c. */

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
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}

static VOID
PmicGlinkOnDriverUnload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
}

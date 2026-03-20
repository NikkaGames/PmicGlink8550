/* Interface exposure, attachment callbacks, and driver context initialization. Included by main.c. */

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
                            PmicGlinkStartLegacyBatteryRefreshTimer(deviceContext, "BattMiniIface");
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

/* Legacy battery refresh, battmini attach/notify, and charge-state helpers. Included by main.c. */

static VOID
PmicGlinkBootBatteryRefreshWorkItem(
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    BATT_MNGR_GET_BATT_INFO battInfoRequest;
    ULONGLONG nowMsec;
    NTSTATUS battIdStatus;
    NTSTATUS battInfoStatus;
    NTSTATUS chgStatus;

    parentObject = WdfWorkItemGetParentObject(WorkItem);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);

    if ((context != NULL) && context->GlinkChannelConnected)
    {
        gPmicGlinkLastBattIdQueryMsec = 0;
        gPmicGlinkLastChargeStatusQueryMsec = 0;
        gPmicGlinkLastBattInfoQueryMsec = 0;
        context->LegacyLastModernSocQueryMsec = 0;
        context->LegacyStatusNotificationPending = TRUE;
        context->LegacyStateChangePending = TRUE;
        context->Notify = TRUE;

        PmicGlinkTryAttachBattMiniFromIoctl(context, "BOOT_REFRESH");
        battIdStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_BATT_ID,
            NULL,
            0u);

        RtlZeroMemory(&battInfoRequest, sizeof(battInfoRequest));
        battInfoRequest.batt_id = context->LegacyBattId.batt_id;
        battInfoRequest.rate_of_drain = context->LegacyChargeStatus.rate;
        battInfoStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_BATT_INFO,
            (PUCHAR)&battInfoRequest,
            sizeof(battInfoRequest));
        chgStatus = PmicGlink_SyncSendReceive(
            context,
            IOCTL_BATTMNGR_GET_CHARGER_STATUS,
            NULL,
            0u);
        nowMsec = PmicGlink_Helper_get_rel_time_msec();
        PmicGlinkRefreshModernBatterySoc(context, nowMsec, TRUE);

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: boot_refresh connected=%u battmini=%u target=%p battId=0x%08lx info=0x%08lx chg=0x%08lx pct=%u state=0x%08lx\n",
            context->GlinkChannelConnected ? 1u : 0u,
            context->BattMiniDeviceLoaded ? 1u : 0u,
            context->BattMiniIoTarget,
            (ULONG)battIdStatus,
            (ULONG)battInfoStatus,
            (ULONG)chgStatus,
            context->LegacyBattPercentage,
            context->LegacyChargeStatus.power_state);

        PmicGlinkNotifyBattMiniStatusFromGlink(context, 0x80u);
    }

    WdfObjectDelete(WorkItem);
}

static NTSTATUS
PmicGlinkEnsureLegacyBatteryRefreshTimer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->LegacyBatteryRefreshTimer != NULL)
    {
        return STATUS_SUCCESS;
    }

    WDF_TIMER_CONFIG_INIT_PERIODIC(
        &timerConfig,
        PmicGlinkLegacyBatteryRefreshTimerFunction,
        PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS);
    timerConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = Context->Device;
    timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        &Context->LegacyBatteryRefreshTimer);
    if (!NT_SUCCESS(status))
    {
        Context->LegacyBatteryRefreshTimer = NULL;
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer create failed status=0x%08lx\n",
            (ULONG)status);
    }
    else
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer created periodMs=%u\n",
            PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS);
    }

    return status;
}

static VOID
PmicGlinkStartLegacyBatteryRefreshTimer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ PCSTR Reason
    )
{
    NTSTATUS status;

    if (Context == NULL)
    {
        return;
    }

    status = PmicGlinkEnsureLegacyBatteryRefreshTimer(Context);
    if (!NT_SUCCESS(status) || (Context->LegacyBatteryRefreshTimer == NULL))
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: batt_fallback_timer start failed reason=%s status=0x%08lx timer=%p\n",
            (Reason != NULL) ? Reason : "unknown",
            (ULONG)status,
            Context->LegacyBatteryRefreshTimer);
        return;
    }

    (VOID)WdfTimerStart(
        Context->LegacyBatteryRefreshTimer,
        -((LONGLONG)PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS * 10000ll));
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: batt_fallback_timer started reason=%s timer=%p connected=%u hibernate=%u\n",
        (Reason != NULL) ? Reason : "unknown",
        Context->LegacyBatteryRefreshTimer,
        Context->GlinkChannelConnected ? 1u : 0u,
        Context->Hibernate ? 1u : 0u);
}

static VOID
PmicGlinkLegacyBatteryRefreshTimerFunction(
    _In_ WDFTIMER Timer
    )
{
    WDFOBJECT parentObject;
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONGLONG nowMsec;

    parentObject = WdfTimerGetParentObject(Timer);
    context = PmicGlinkGetDeviceContext((WDFDEVICE)parentObject);
    if ((context == NULL) || !context->GlinkChannelConnected || context->Hibernate)
    {
        return;
    }

    nowMsec = PmicGlink_Helper_get_rel_time_msec();
    if ((context->LegacyLastAdspBatteryNotifyMsec != 0)
        && (nowMsec >= context->LegacyLastAdspBatteryNotifyMsec)
        && ((nowMsec - context->LegacyLastAdspBatteryNotifyMsec)
            < (ULONGLONG)PMICGLINK_BATT_FALLBACK_REFRESH_PERIOD_MS))
    {
        return;
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: batt_fallback_timer ping lastNotify=%I64u now=%I64u connected=%u battmini=%u\n",
        context->LegacyLastAdspBatteryNotifyMsec,
        nowMsec,
        context->GlinkChannelConnected ? 1u : 0u,
        context->BattMiniDeviceLoaded ? 1u : 0u);

    gPmicGlinkLastChargeStatusQueryMsec = 0;
    gPmicGlinkLastBattInfoQueryMsec = 0;
    context->LegacyLastModernSocQueryMsec = 0;
    context->LegacyStatusNotificationPending = TRUE;
    context->LegacyStateChangePending = TRUE;
    context->Notify = TRUE;
    PmicGlinkTryAttachBattMiniFromIoctl(context, "TIMER");
    PmicGlinkNotifyBattMiniStatusFromGlink(context, 0x80u);
}


static NTSTATUS
PmicGlinkNotify_PingBattMiniClass(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 1);
    return STATUS_SUCCESS;
}

static NTSTATUS
PmicGlinkTryAttachBattMiniNoPnp(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    NTSTATUS openStatus;
    PWSTR interfaceList;
    PWSTR currentInterface;
    UNICODE_STRING targetName;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    ULONG interfaceFlags;
    ULONG interfaceIndex;
    ULONG flagPass;
    ULONG guidIndex;
    BOOLEAN attached;
    BOOLEAN notifyCapable;
    NTSTATUS probeStatus;
    SIZE_T probeBytesReturned;
    const GUID* interfaceGuids[2];
    const CHAR* interfaceNames[2];

#ifndef DEVICE_INTERFACE_INCLUDE_NONACTIVE
#define DEVICE_INTERFACE_INCLUDE_NONACTIVE 0x00000001ul
#endif

    if ((Context == NULL) || (Context->Device == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->BattMiniDeviceLoaded && (Context->BattMiniIoTarget != NULL))
    {
        return STATUS_SUCCESS;
    }

    attached = FALSE;
    status = STATUS_NOT_FOUND;
    openStatus = STATUS_NOT_FOUND;

    interfaceGuids[0] = &GUID_DEVINTERFACE_PMIC_BATT_MINI;
    interfaceNames[0] = "pmic_batt_mini";
    interfaceGuids[1] = &GUID_DEVICE_BATTERY;
    interfaceNames[1] = "device_battery";

    for (guidIndex = 0ul; guidIndex < RTL_NUMBER_OF(interfaceGuids); guidIndex++)
    {
        for (flagPass = 0ul; flagPass < 2ul; flagPass++)
        {
            interfaceFlags = (flagPass == 0ul) ? 0ul : DEVICE_INTERFACE_INCLUDE_NONACTIVE;
            interfaceList = NULL;
            status = IoGetDeviceInterfaces(
                (LPGUID)interfaceGuids[guidIndex],
                NULL,
                interfaceFlags,
                &interfaceList);
            if (!NT_SUCCESS(status))
            {
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini attach query guid=%s flags=0x%lx failed status=0x%08lx\n",
                    interfaceNames[guidIndex],
                    interfaceFlags,
                    (ULONG)status);
                continue;
            }

            if ((interfaceList == NULL) || (interfaceList[0] == L'\0'))
            {
                if (interfaceList != NULL)
                {
                    ExFreePool(interfaceList);
                }
                continue;
            }

            interfaceIndex = 0ul;
            currentInterface = interfaceList;
            while (currentInterface[0] != L'\0')
            {
                if (Context->BattMiniIoTarget == NULL)
                {
                    status = WdfIoTargetCreate(
                        Context->Device,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &Context->BattMiniIoTarget);
                    if (!NT_SUCCESS(status))
                    {
                        DbgPrintEx(
                            DPFLTR_IHVDRIVER_ID,
                            PMICGLINK_TRACE_LEVEL,
                            "pmicglink: battmini attach create target failed status=0x%08lx\n",
                            (ULONG)status);
                        break;
                    }
                }

                RtlInitUnicodeString(&targetName, currentInterface);
                WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                    &openParams,
                    &targetName,
                    PMICGLINK_GLINK_QUERY_ACCESS_MASK);
                openParams.ShareAccess = FILE_SHARE_READ;
                openStatus = WdfIoTargetOpen(Context->BattMiniIoTarget, &openParams);
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini attach try guid=%s[%lu] flags=0x%lx status=0x%08lx name=%ws\n",
                    interfaceNames[guidIndex],
                    interfaceIndex,
                    interfaceFlags,
                    (ULONG)openStatus,
                    currentInterface);
                if (NT_SUCCESS(openStatus))
                {
                    probeBytesReturned = 0u;
                    probeStatus = PmicGlinkSendDriverRequestWithTimeout(
                        Context->BattMiniIoTarget,
                        PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE,
                        NULL,
                        0u,
                        NULL,
                        0u,
                        -1000000ll,
                        &probeBytesReturned);
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: battmini attach probe guid=%s[%lu] status=0x%08lx bytes=%Iu\n",
                        interfaceNames[guidIndex],
                        interfaceIndex,
                        (ULONG)probeStatus,
                        probeBytesReturned);

                    notifyCapable = NT_SUCCESS(probeStatus) ? TRUE : FALSE;

                    if (notifyCapable)
                    {
                        attached = TRUE;
                        status = STATUS_SUCCESS;
                        break;
                    }

                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID,
                        PMICGLINK_TRACE_LEVEL,
                        "pmicglink: battmini attach reject guid=%s[%lu] status=0x%08lx\n",
                        interfaceNames[guidIndex],
                        interfaceIndex,
                        (ULONG)probeStatus);
                }

                PmicGlinkCloseIoTargetIfOpen(&Context->BattMiniIoTarget);
                currentInterface += wcslen(currentInterface) + 1;
                interfaceIndex++;
            }

            ExFreePool(interfaceList);
            if (attached)
            {
                break;
            }
        }

        if (attached)
        {
            break;
        }
    }

    if (attached)
    {
        Context->BattMiniDeviceLoaded = TRUE;
        Context->NotificationFlag = TRUE;
        (VOID)PmicGlinkEnsureBclCriticalCallback(Context);
        return STATUS_SUCCESS;
    }

    if (Context->BattMiniIoTarget != NULL)
    {
        PmicGlinkCloseIoTargetIfOpen(&Context->BattMiniIoTarget);
    }

    if (!NT_SUCCESS(status) && NT_SUCCESS(openStatus))
    {
        return status;
    }

    if (!NT_SUCCESS(openStatus))
    {
        return openStatus;
    }

    return STATUS_NOT_FOUND;
}

static VOID
PmicGlinkTryAttachBattMiniFromIoctl(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ PCSTR Reason
    )
{
    NTSTATUS attachStatus;
    ULONGLONG nowMsec;

    if ((Context == NULL) || Context->BattMiniDeviceLoaded)
    {
        return;
    }

    nowMsec = PmicGlink_Helper_get_rel_time_msec();
    if ((nowMsec >= gPmicGlinkLastBattMiniAttachAttemptMsec)
        && ((nowMsec - gPmicGlinkLastBattMiniAttachAttemptMsec) < 2000ull))
    {
        return;
    }
    gPmicGlinkLastBattMiniAttachAttemptMsec = nowMsec;

    attachStatus = PmicGlinkTryAttachBattMiniNoPnp(Context);
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini ioctl-attach reason=%s status=0x%08lx loaded=%u target=%p\n",
        (Reason != NULL) ? Reason : "unknown",
        (ULONG)attachStatus,
        Context->BattMiniDeviceLoaded ? 1u : 0u,
        Context->BattMiniIoTarget);
}

static VOID
PmicGlinkNotifyBattMiniStatusFromGlink(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG NotificationData
    )
{
    NTSTATUS notifyStatus;
    ULONG battMiniNotifyArgument;
    BOOLEAN battMiniLoaded;
    WDFIOTARGET battMiniTarget;

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify enter notif=0x%08lx loaded=%u target=%p go=%ld lock=%p\n",
        NotificationData,
        (Context != NULL && Context->BattMiniDeviceLoaded) ? 1u : 0u,
        (Context != NULL) ? Context->BattMiniIoTarget : NULL,
        InterlockedCompareExchange(&gPmicGlinkNotifyGo, 0, 0),
        (Context != NULL) ? Context->BattMiniNotifyLock : NULL);

    if (Context == NULL)
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify exit null-context notif=0x%08lx\n",
            NotificationData);
        return;
    }

    PmicGlinkNotify_PingBattMiniClass(Context);
    if (InterlockedCompareExchange(&gPmicGlinkNotifyGo, 1, 1) == 0)
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify early-exit go=0 notif=0x%08lx\n",
            NotificationData);
        return;
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify no-lockpath notif=0x%08lx\n",
        NotificationData);

    if (!Context->BattMiniDeviceLoaded)
    {
        Context->LegacyStatusNotificationPending = TRUE;
        Context->LegacyStateChangePending = TRUE;
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify defer attach notif=0x%08lx loaded=0 target=%p\n",
            NotificationData,
            Context->BattMiniIoTarget);
        return;
    }

    battMiniLoaded = Context->BattMiniDeviceLoaded ? TRUE : FALSE;
    battMiniTarget = Context->BattMiniIoTarget;
    if (battMiniTarget != NULL)
    {
        WdfObjectReference(battMiniTarget);
    }

    if (battMiniLoaded && (battMiniTarget != NULL))
    {
        notifyStatus = PmicGlinkSendDriverRequestAsync(
            battMiniTarget,
            PMICGLINK_BATTMINI_IOCTL_NOTIFY_PRESENCE,
            NULL,
            0u);
        (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify_presence status=0x%08lx notif=0x%08lx\n",
            (ULONG)notifyStatus,
            NotificationData);
        if ((notifyStatus == STATUS_NOT_SUPPORTED)
            || (notifyStatus == STATUS_INVALID_DEVICE_REQUEST))
        {
            PmicGlinkClearBattMiniAttachment(Context);
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_presence detach unsupported status=0x%08lx\n",
                (ULONG)notifyStatus);
        }
        if (NT_SUCCESS(notifyStatus))
        {
            Context->LegacyStatusNotificationPending = TRUE;
            Context->LegacyStateChangePending = TRUE;
        }

        if ((NotificationData & 0xFFu) == 0x83u)
        {
            battMiniNotifyArgument = (NotificationData >> 8);
            notifyStatus = PmicGlinkSendDriverRequestAsync(
                battMiniTarget,
                PMICGLINK_BATTMINI_IOCTL_NOTIFY_STATUS,
                &battMiniNotifyArgument,
                sizeof(battMiniNotifyArgument));
            (VOID)InterlockedExchange(&gPmicGlinkNotifyGo, 0);
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_status status=0x%08lx arg=0x%08lx notif=0x%08lx\n",
                (ULONG)notifyStatus,
                battMiniNotifyArgument,
                NotificationData);
            if ((notifyStatus == STATUS_NOT_SUPPORTED)
                || (notifyStatus == STATUS_INVALID_DEVICE_REQUEST))
            {
                PmicGlinkClearBattMiniAttachment(Context);
                DbgPrintEx(
                    DPFLTR_IHVDRIVER_ID,
                    PMICGLINK_TRACE_LEVEL,
                    "pmicglink: battmini notify_status detach unsupported status=0x%08lx\n",
                    (ULONG)notifyStatus);
            }
            if (NT_SUCCESS(notifyStatus))
            {
                Context->LegacyStatusNotificationPending = TRUE;
                Context->LegacyStateChangePending = TRUE;
            }
        }
        else
        {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                PMICGLINK_TRACE_LEVEL,
                "pmicglink: battmini notify_status skip notif=0x%08lx low=0x%02lx\n",
                NotificationData,
                NotificationData & 0xFFu);
        }
    }
    else
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID,
            PMICGLINK_TRACE_LEVEL,
            "pmicglink: battmini notify skipped loaded=%u target=%p notif=0x%08lx\n",
            battMiniLoaded ? 1u : 0u,
            battMiniTarget,
            NotificationData);
    }

    if (battMiniTarget != NULL)
    {
        WdfObjectDereference(battMiniTarget);
    }

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID,
        PMICGLINK_TRACE_LEVEL,
        "pmicglink: battmini notify exit notif=0x%08lx loaded=%u target=%p\n",
        NotificationData,
        Context->BattMiniDeviceLoaded ? 1u : 0u,
        Context->BattMiniIoTarget);
}


static BOOLEAN
PmicGlinkIsBattStateCharging(
    _In_ ULONG BatteryState
    )
{
    switch (BatteryState)
    {
    case 2u:  /* FAST */
    case 3u:  /* TOP_OFF */
    case 5u:  /* RECHARGE */
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN
PmicGlinkIsBattStateNotCharging(
    _In_ ULONG BatteryState
    )
{
    switch (BatteryState)
    {
    case 1u:   /* NO_CHG */
    case 4u:   /* DONE */
    case 6u:   /* TDONE */
    case 7u:   /* NOT_CHARGING_THERMAL */
    case 8u:   /* ERROR */
    case 9u:   /* TEST */
    case 0xBu: /* INVALID */
    case 0xCu: /* NA */
        return TRUE;

    default:
        return FALSE;
    }
}


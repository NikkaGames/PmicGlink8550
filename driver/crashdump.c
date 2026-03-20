/* Crashdump subsystem. Included by main.c. */

static VOID
CrashDumpGuidToHexKey(
    _In_ const GUID* Guid,
    _Out_writes_(33) CHAR* Key
    )
{
    static const CHAR kHexDigits[] = "0123456789ABCDEF";
    const UCHAR* guidBytes;
    ULONG index;

    if (Key == NULL)
    {
        return;
    }

    RtlZeroMemory(Key, 33);
    if (Guid == NULL)
    {
        return;
    }

    guidBytes = (const UCHAR*)Guid;
    for (index = 0; index < 16; index++)
    {
        Key[index * 2] = kHexDigits[(guidBytes[index] >> 4) & 0x0Fu];
        Key[(index * 2) + 1] = kHexDigits[guidBytes[index] & 0x0Fu];
    }
}

static BOOLEAN
CrashDumpGuidIsZero(
    _In_ const GUID* Guid
    )
{
    GUID zeroGuid;

    if (Guid == NULL)
    {
        return TRUE;
    }

    RtlZeroMemory(&zeroGuid, sizeof(zeroGuid));
    return (RtlCompareMemory(Guid, &zeroGuid, sizeof(zeroGuid)) == sizeof(zeroGuid)) ? TRUE : FALSE;
}

static LONG
CrashDump_FileHandleSlotFind(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject,
    _In_ ULONG DataSourceMode
    )
{
    ULONG index;

    if ((Context == NULL)
        || (FileObject == NULL)
        || (DataSourceMode == 0)
        || (DataSourceMode >= RTL_NUMBER_OF(Context->CrashDumpDataSources[0].FileObject)))
    {
        return -1;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        if (Context->CrashDumpDataSources[index].FileObject[DataSourceMode] == FileObject)
        {
            return (LONG)index;
        }
    }

    return -1;
}

static LONG
CrashDump_FileHandleSlotFindForDestroy(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    ULONG index;

    if ((Context == NULL) || (FileObject == NULL))
    {
        return -1;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        if ((Context->CrashDumpDataSources[index].FileObject[1] == FileObject)
            || (Context->CrashDumpDataSources[index].FileObject[2] == FileObject))
        {
            return (LONG)index;
        }
    }

    return -1;
}

static LONG
CrashDump_FileHandleSlotFree(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    LONG slotIndex;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    slotIndex = CrashDump_FileHandleSlotFindForDestroy(Context, FileObject);
    if (slotIndex < 0)
    {
        return -1;
    }

    source = &Context->CrashDumpDataSources[(ULONG)slotIndex];
    if (source->FileObject[2] == FileObject)
    {
        source->FileObject[2] = NULL;
    }
    else if (source->FileObject[1] == FileObject)
    {
        source->FileObject[1] = NULL;
    }

    return slotIndex;
}

static VOID
CrashDump_DmfDestroy_RingBuffer(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    if ((Context == NULL) || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->RingBufferData != NULL)
    {
        ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        source->RingBufferData = NULL;
    }
    if (source->BugCheckSnapshotBuffer != NULL)
    {
        ExFreePoolWithTag(source->BugCheckSnapshotBuffer, PMICGLINK_POOLTAG_CRASHDUMP);
        source->BugCheckSnapshotBuffer = NULL;
    }

    RtlZeroMemory(&source->RingBufferGuid, sizeof(source->RingBufferGuid));
    RtlZeroMemory(source->RingBufferEncryptionKey, sizeof(source->RingBufferEncryptionKey));
    source->RingBufferEncryptionKeySize = 0;
    source->RingBufferSize = 0;
    source->RingBufferSizeOfEachEntry = 0;
    source->EntriesCount = 0;
    source->CurrentRingBufferIndex = 0;
    source->ValidEntryCount = 0;
    source->BugCheckBufferPointer = NULL;
    source->RingBufferEnumerationOffset = 0;
}

static NTSTATUS
CrashDump_DataSourceCreateInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _In_ ULONG ItemCount,
    _In_ ULONG SizeOfEachEntry,
    _In_ const GUID* Guid
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG totalBytes;

    if ((Context == NULL)
        || (Guid == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount)
        || (ItemCount == 0)
        || (SizeOfEachEntry == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    totalBytes = (ULONGLONG)ItemCount * (ULONGLONG)SizeOfEachEntry;
    if ((totalBytes == 0)
        || (totalBytes > PMICGLINK_CRASHDUMP_RINGBUFFER_MAX_BYTES)
        || (totalBytes > MAXULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->RingBufferData != NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    source->RingBufferData = (PUCHAR)ExAllocatePoolZero(
        NonPagedPoolNx,
        (SIZE_T)totalBytes,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (source->RingBufferData == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    source->BugCheckSnapshotBuffer = (PUCHAR)ExAllocatePoolZero(
        NonPagedPoolNx,
        (SIZE_T)totalBytes,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (source->BugCheckSnapshotBuffer == NULL)
    {
        ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        source->RingBufferData = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    source->EntriesCount = ItemCount;
    source->RingBufferSizeOfEachEntry = SizeOfEachEntry;
    source->RingBufferSize = (ULONG)totalBytes;
    source->CurrentRingBufferIndex = 0;
    source->ValidEntryCount = 0;
    source->BugCheckBufferPointer = NULL;
    source->RingBufferEnumerationOffset = 0;
    source->RingBufferGuid = *Guid;
    source->RingBufferEncryptionKeySize = 32;
    CrashDumpGuidToHexKey(Guid, source->RingBufferEncryptionKey);
    KeInitializeCallbackRecord(&source->BugCheckCallbackRecord);
    source->BugCheckCallbackRegistered = FALSE;
    if (!KeRegisterBugCheckReasonCallback(
        &source->BugCheckCallbackRecord,
        CrashDump_BugCheckSecondaryDumpDataCallbackRingBuffer,
        KbCallbackSecondaryDumpData,
        gCrashDumpBugCheckComponent))
    {
        CrashDump_DmfDestroy_RingBuffer(Context, DataSourceIndex);
        return STATUS_INVALID_PARAMETER;
    }

    source->BugCheckCallbackRegistered = TRUE;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceDestroyInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    if ((Context == NULL) || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if (source->BugCheckCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&source->BugCheckCallbackRecord);
        source->BugCheckCallbackRegistered = FALSE;
    }

    CrashDump_DmfDestroy_RingBuffer(Context, DataSourceIndex);
    RtlZeroMemory(&source->BugCheckCallbackRecord, sizeof(source->BugCheckCallbackRecord));
    source->FileObject[1] = NULL;
    source->FileObject[2] = NULL;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceDestroyAuxiliaryInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ WDFFILEOBJECT FileObject
    )
{
    LONG slotIndex;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    slotIndex = CrashDump_FileHandleSlotFree(Context, FileObject);
    if (slotIndex < 0)
    {
        return STATUS_INVALID_HANDLE;
    }

    return CrashDump_DataSourceDestroyInternal(Context, (ULONG)slotIndex);
}

static NTSTATUS
CrashDump_DataSourceWriteInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _In_reads_bytes_(BufferLength) const UCHAR* Buffer,
    _In_ SIZE_T BufferLength
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    PUCHAR entry;
    ULONG writeIndex;
    SIZE_T bytesToCopy;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength > source->RingBufferSizeOfEachEntry)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    writeIndex = source->CurrentRingBufferIndex;
    entry = source->RingBufferData + ((SIZE_T)writeIndex * source->RingBufferSizeOfEachEntry);
    RtlZeroMemory(entry, source->RingBufferSizeOfEachEntry);

    bytesToCopy = (BufferLength < source->RingBufferSizeOfEachEntry)
        ? BufferLength
        : source->RingBufferSizeOfEachEntry;
    if (bytesToCopy > 0)
    {
        RtlCopyMemory(entry, Buffer, bytesToCopy);
    }

    source->CurrentRingBufferIndex = (writeIndex + 1u) % source->EntriesCount;
    if (source->ValidEntryCount < source->EntriesCount)
    {
        source->ValidEntryCount++;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceReadInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _Out_writes_bytes_(BufferLength) UCHAR* Buffer,
    _In_ SIZE_T BufferLength
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG readIndex;
    PUCHAR entry;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength < source->RingBufferSizeOfEachEntry)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (source->ValidEntryCount == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    readIndex = (source->CurrentRingBufferIndex + source->EntriesCount - source->ValidEntryCount) % source->EntriesCount;
    entry = source->RingBufferData + ((SIZE_T)readIndex * source->RingBufferSizeOfEachEntry);
    RtlCopyMemory(Buffer, entry, source->RingBufferSizeOfEachEntry);
    source->ValidEntryCount--;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_DataSourceCaptureInternal(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG DataSourceIndex,
    _Out_writes_bytes_(BufferLength) UCHAR* Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ PULONG BytesWritten
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG written;
    NTSTATUS status;

    if ((Context == NULL)
        || (Buffer == NULL)
        || (BytesWritten == NULL)
        || (DataSourceIndex == 0)
        || (DataSourceIndex >= Context->CrashDumpDataSourceCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = &Context->CrashDumpDataSources[DataSourceIndex];
    if ((source->RingBufferData == NULL)
        || (source->EntriesCount == 0)
        || (source->RingBufferSizeOfEachEntry == 0))
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (BufferLength < source->RingBufferSize)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    written = 0;
    while ((source->ValidEntryCount > 0)
           && ((written + source->RingBufferSizeOfEachEntry) <= BufferLength))
    {
        status = CrashDump_DataSourceReadInternal(
            Context,
            DataSourceIndex,
            Buffer + written,
            BufferLength - written);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        written += source->RingBufferSizeOfEachEntry;
    }

    *BytesWritten = written;
    return STATUS_SUCCESS;
}

static PMICGLINK_CRASHDUMP_DATA_SOURCE*
CrashDump_FindActiveDataSource(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG index;

    if (Context == NULL)
    {
        return NULL;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if ((source->RingBufferData != NULL)
            && (source->RingBufferSizeOfEachEntry != 0)
            && (source->RingBufferSize != 0))
        {
            return source;
        }
    }

    return NULL;
}

static PMICGLINK_CRASHDUMP_DATA_SOURCE*
CrashDump_FindSourceFromCallbackRecord(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_opt_ PKBUGCHECK_REASON_CALLBACK_RECORD Record
    )
{
    ULONG index;

    if ((Context == NULL) || (Record == NULL))
    {
        return NULL;
    }

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if (&source->BugCheckCallbackRecord == Record)
        {
            return source;
        }
    }

    return NULL;
}

static NTSTATUS
CrashDump_InitializeTriageDataArray(
    _Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    NTSTATUS status;
    UNICODE_STRING routineName;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((Context->CrashDumpTriageDataArray != NULL) && (Context->CrashDumpTriageDataArraySize != 0))
    {
        return STATUS_SUCCESS;
    }

    if (gKeInitializeTriageDumpDataArray == NULL)
    {
        RtlInitUnicodeString(&routineName, L"KeInitializeTriageDumpDataArray");
        gKeInitializeTriageDumpDataArray = MmGetSystemRoutineAddress(&routineName);
        if (gKeInitializeTriageDumpDataArray == NULL)
        {
            return STATUS_PROCEDURE_NOT_FOUND;
        }
    }

    Context->CrashDumpTriageDataArraySize = PMICGLINK_CRASHDUMP_TRIAGE_DATA_ARRAY_SIZE;
    Context->CrashDumpTriageDataArray = ExAllocatePoolZero(
        NonPagedPoolNx,
        Context->CrashDumpTriageDataArraySize,
        PMICGLINK_POOLTAG_CRASHDUMP);
    if (Context->CrashDumpTriageDataArray == NULL)
    {
        Context->CrashDumpTriageDataArraySize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ((PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY)gKeInitializeTriageDumpDataArray)(
        Context->CrashDumpTriageDataArray,
        Context->CrashDumpTriageDataArraySize);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
        Context->CrashDumpTriageDataArray = NULL;
        Context->CrashDumpTriageDataArraySize = 0;
        return status;
    }

    return STATUS_SUCCESS;
}

static VOID
PmicGlinkLkmdTelInitializeMaxSizes(
    VOID
    )
{
    HANDLE regHandle;
    NTSTATUS status;
    ULONG reportSizeMb;
    ULONG resultLength;
    UNICODE_STRING keyPath;
    UNICODE_STRING valueName;
    OBJECT_ATTRIBUTES objectAttributes;
    UCHAR valueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;

    if (InterlockedCompareExchange(&gPmicGlinkLkmdTelMaxSizeInitialized, 1, 0) != 0)
    {
        return;
    }

    reportSizeMb = PMICGLINK_LKMDTEL_DEFAULT_MAX_REPORT_MB;
    regHandle = NULL;
    valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)valueBuffer;
    resultLength = 0;

    RtlInitUnicodeString(
        &keyPath,
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\LKMDTel");
    InitializeObjectAttributes(
        &objectAttributes,
        &keyPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    status = ZwOpenKey(&regHandle, KEY_READ, &objectAttributes);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&valueName, L"MaxReportSizeInMB");
        status = ZwQueryValueKey(
            regHandle,
            &valueName,
            KeyValuePartialInformation,
            valueInfo,
            sizeof(valueBuffer),
            &resultLength);
        if (NT_SUCCESS(status)
            && (valueInfo->Type == REG_DWORD)
            && (valueInfo->DataLength == sizeof(ULONG)))
        {
            reportSizeMb = *(ULONG*)valueInfo->Data;
        }

        ZwClose(regHandle);
    }

    if (reportSizeMb > PMICGLINK_LKMDTEL_DEFAULT_MAX_REPORT_MB)
    {
        if (reportSizeMb > PMICGLINK_LKMDTEL_MAX_REPORT_MB)
        {
            reportSizeMb = PMICGLINK_LKMDTEL_MAX_REPORT_MB;
        }

        gPmicGlinkLkmdTelMaxReportSize = (reportSizeMb << 20);
        if (gPmicGlinkLkmdTelMaxReportSize > PMICGLINK_LKMDTEL_SECONDARY_OVERHEAD_BYTES)
        {
            gPmicGlinkLkmdTelMaxSecondarySize =
                gPmicGlinkLkmdTelMaxReportSize - PMICGLINK_LKMDTEL_SECONDARY_OVERHEAD_BYTES;
        }
        else
        {
            gPmicGlinkLkmdTelMaxSecondarySize = 0;
        }
    }
}

static VOID
CrashDump_PopulateTriageDataArray(
    _Inout_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG remainingSecondarySize;
    ULONG index;
    UNICODE_STRING routineName;
    PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY initializeTriageDataArray;
    PFN_KE_ADD_TRIAGE_DUMP_DATA_BLOCK addTriageDataBlock;

    if ((Context == NULL)
        || (Context->CrashDumpTriageDataArray == NULL)
        || (Context->CrashDumpTriageDataArraySize == 0))
    {
        return;
    }

    initializeTriageDataArray =
        (PFN_KE_INITIALIZE_TRIAGE_DUMP_DATA_ARRAY)gKeInitializeTriageDumpDataArray;
    if (initializeTriageDataArray == NULL)
    {
        return;
    }

    (VOID)initializeTriageDataArray(
        Context->CrashDumpTriageDataArray,
        Context->CrashDumpTriageDataArraySize);

    if (gKeAddTriageDumpDataBlock == NULL)
    {
        RtlInitUnicodeString(&routineName, L"KeAddTriageDumpDataBlock");
        gKeAddTriageDumpDataBlock = MmGetSystemRoutineAddress(&routineName);
    }

    addTriageDataBlock = (PFN_KE_ADD_TRIAGE_DUMP_DATA_BLOCK)gKeAddTriageDumpDataBlock;
    if (addTriageDataBlock == NULL)
    {
        return;
    }

    PmicGlinkLkmdTelInitializeMaxSizes();
    remainingSecondarySize = gPmicGlinkLkmdTelMaxSecondarySize;

    for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
    {
        ULONG dataSize;
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
        PVOID dataBuffer;

        source = &Context->CrashDumpDataSources[index];
        if ((source->RingBufferData == NULL)
            || (source->RingBufferSize == 0)
            || (source->RingBufferSizeOfEachEntry == 0))
        {
            continue;
        }

        CrashDump_PrepareBugCheckSnapshot(source);
        dataBuffer = (source->BugCheckBufferPointer != NULL)
            ? (PVOID)source->BugCheckBufferPointer
            : (PVOID)source->RingBufferData;
        if (dataBuffer == NULL)
        {
            continue;
        }

        dataSize = source->RingBufferSize;
        if (dataSize > remainingSecondarySize)
        {
            dataSize = remainingSecondarySize;
        }
        if (dataSize == 0)
        {
            break;
        }

        (VOID)addTriageDataBlock(
            Context->CrashDumpTriageDataArray,
            dataBuffer,
            dataSize);

        if (remainingSecondarySize > dataSize)
        {
            remainingSecondarySize -= dataSize;
        }
        else
        {
            remainingSecondarySize = 0;
            break;
        }
    }
}

static NTSTATUS
CrashDump_RegisterGlobalCallbacks(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    BOOLEAN additionalRegisteredHere;
    NTSTATUS status;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    additionalRegisteredHere = FALSE;
    if (!Context->CrashDumpAdditionalCallbackRegistered)
    {
        KeInitializeCallbackRecord(&Context->CrashDumpAdditionalCallbackRecord);
        if (!KeRegisterBugCheckReasonCallback(
            &Context->CrashDumpAdditionalCallbackRecord,
            CrashDump_BugCheckSecondaryDumpDataCallbackAdditional,
            KbCallbackSecondaryDumpData,
            gCrashDumpBugCheckComponent))
        {
            RtlZeroMemory(
                &Context->CrashDumpAdditionalCallbackRecord,
                sizeof(Context->CrashDumpAdditionalCallbackRecord));
            return STATUS_INVALID_PARAMETER;
        }

        Context->CrashDumpAdditionalCallbackRegistered = TRUE;
        additionalRegisteredHere = TRUE;
    }

    if (!Context->CrashDumpTriageCallbackRegistered)
    {
        status = CrashDump_InitializeTriageDataArray(Context);
        if (!NT_SUCCESS(status))
        {
            if (additionalRegisteredHere)
            {
                (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
                Context->CrashDumpAdditionalCallbackRegistered = FALSE;
                RtlZeroMemory(
                    &Context->CrashDumpAdditionalCallbackRecord,
                    sizeof(Context->CrashDumpAdditionalCallbackRecord));
            }
            return status;
        }

        KeInitializeCallbackRecord(&Context->CrashDumpTriageCallbackRecord);
        if (!KeRegisterBugCheckReasonCallback(
            &Context->CrashDumpTriageCallbackRecord,
            CrashDump_BugCheckTriageDumpDataCallback,
            KbCallbackTriageDumpData,
            gCrashDumpBugCheckComponent))
        {
            RtlZeroMemory(
                &Context->CrashDumpTriageCallbackRecord,
                sizeof(Context->CrashDumpTriageCallbackRecord));

            if (additionalRegisteredHere)
            {
                (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
                Context->CrashDumpAdditionalCallbackRegistered = FALSE;
                RtlZeroMemory(
                    &Context->CrashDumpAdditionalCallbackRecord,
                    sizeof(Context->CrashDumpAdditionalCallbackRecord));
            }

            if (Context->CrashDumpTriageDataArray != NULL)
            {
                ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
                Context->CrashDumpTriageDataArray = NULL;
                Context->CrashDumpTriageDataArraySize = 0;
            }

            return STATUS_INVALID_PARAMETER;
        }

        Context->CrashDumpTriageCallbackRegistered = TRUE;
    }

    return STATUS_SUCCESS;
}

static VOID
CrashDump_ResetState(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context
    )
{
    ULONG index;
    ULONG sourceCount;

    if (Context == NULL)
    {
        return;
    }

    sourceCount = Context->CrashDumpDataSourceCount;
    if ((sourceCount == 0) || (sourceCount > PMICGLINK_CRASHDUMP_MAX_SOURCES))
    {
        sourceCount = PMICGLINK_CRASHDUMP_MAX_SOURCES;
    }

    if (Context->CrashDumpLock != NULL)
    {
        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);
    }

    for (index = 0; index < sourceCount; index++)
    {
        PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

        source = &Context->CrashDumpDataSources[index];
        if (source->BugCheckCallbackRegistered)
        {
            (VOID)KeDeregisterBugCheckReasonCallback(&source->BugCheckCallbackRecord);
            source->BugCheckCallbackRegistered = FALSE;
        }

        if (source->RingBufferData != NULL)
        {
            ExFreePoolWithTag(source->RingBufferData, PMICGLINK_POOLTAG_CRASHDUMP);
        }
        if (source->BugCheckSnapshotBuffer != NULL)
        {
            ExFreePoolWithTag(source->BugCheckSnapshotBuffer, PMICGLINK_POOLTAG_CRASHDUMP);
        }

        RtlZeroMemory(source, sizeof(*source));
    }

    if (Context->CrashDumpAdditionalCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpAdditionalCallbackRecord);
        Context->CrashDumpAdditionalCallbackRegistered = FALSE;
    }

    if (Context->CrashDumpTriageCallbackRegistered)
    {
        (VOID)KeDeregisterBugCheckReasonCallback(&Context->CrashDumpTriageCallbackRecord);
        Context->CrashDumpTriageCallbackRegistered = FALSE;
    }

    if (Context->CrashDumpTriageDataArray != NULL)
    {
        ExFreePoolWithTag(Context->CrashDumpTriageDataArray, PMICGLINK_POOLTAG_CRASHDUMP);
        Context->CrashDumpTriageDataArray = NULL;
        Context->CrashDumpTriageDataArraySize = 0;
    }

    RtlZeroMemory(&Context->CrashDumpAdditionalCallbackRecord, sizeof(Context->CrashDumpAdditionalCallbackRecord));
    RtlZeroMemory(&Context->CrashDumpTriageCallbackRecord, sizeof(Context->CrashDumpTriageCallbackRecord));

    if (Context->CrashDumpLock != NULL)
    {
        WdfWaitLockRelease(Context->CrashDumpLock);
    }
}

static NTSTATUS
HandleCrashDumpRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    NTSTATUS status;
    WDFFILEOBJECT fileObject;

    if ((Context == NULL) || (Request == NULL) || (BytesReturned == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesReturned = 0;
    fileObject = WdfRequestGetFileObject(Request);

    switch (IoControlCode)
    {
    case IOCTL_CRASHDUMP_DATA_SOURCE_CREATE:
    {
        const DATA_SOURCE_CREATE* createInfo;
        LONG slotIndex;
        LONG existingIndex;
        ULONG index;

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(DATA_SOURCE_CREATE)))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        createInfo = (const DATA_SOURCE_CREATE*)InputBuffer;

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        existingIndex = -1;
        if (!CrashDumpGuidIsZero(&createInfo->Guid))
        {
            for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
            {
                if (RtlCompareMemory(
                    &Context->CrashDumpDataSources[index].RingBufferGuid,
                    &createInfo->Guid,
                    sizeof(GUID)) == sizeof(GUID))
                {
                    WDFFILEOBJECT ownerFileObject;

                    ownerFileObject = Context->CrashDumpDataSources[index].FileObject[1];
                    if ((ownerFileObject == NULL)
                        || (ownerFileObject == PMICGLINK_CRASHDUMP_STALE_FILE_OBJECT))
                    {
                        existingIndex = (LONG)index;
                        break;
                    }
                }
            }
        }

        if (existingIndex >= 0)
        {
            Context->CrashDumpDataSources[(ULONG)existingIndex].FileObject[1] = fileObject;
            status = STATUS_SUCCESS;
            *BytesReturned = sizeof(DATA_SOURCE_CREATE);
        }
        else
        {
            if (CrashDump_FileHandleSlotFind(Context, fileObject, 1) >= 0)
            {
                status = STATUS_NO_MORE_FILES;
            }
            else
            {
                slotIndex = -1;
                for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
                {
                    if ((Context->CrashDumpDataSources[index].FileObject[1] == NULL)
                        && (Context->CrashDumpDataSources[index].RingBufferData == NULL))
                    {
                        slotIndex = (LONG)index;
                        break;
                    }
                }

                if (slotIndex < 0)
                {
                    status = STATUS_NO_MORE_FILES;
                }
                else
                {
                    Context->CrashDumpDataSources[(ULONG)slotIndex].FileObject[1] = fileObject;
                    status = CrashDump_DataSourceCreateInternal(
                        Context,
                        (ULONG)slotIndex,
                        createInfo->EntriesCount,
                        createInfo->EntrySize,
                        &createInfo->Guid);
                    if (!NT_SUCCESS(status))
                    {
                        Context->CrashDumpDataSources[(ULONG)slotIndex].FileObject[1] = NULL;
                    }
                    else
                    {
                        *BytesReturned = sizeof(DATA_SOURCE_CREATE);
                    }
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_DESTROY:
        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);
        status = CrashDump_DataSourceDestroyAuxiliaryInternal(Context, fileObject);
        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;

    case IOCTL_CRASHDUMP_DATA_SOURCE_WRITE:
    {
        LONG slotIndex;
        ULONG entrySize;

        if ((InputBuffer == NULL) || (InputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 1);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            entrySize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSizeOfEachEntry;
            if (InputBufferSize > entrySize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = CrashDump_DataSourceWriteInternal(
                    Context,
                    (ULONG)slotIndex,
                    (const UCHAR*)InputBuffer,
                    InputBufferSize);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = InputBufferSize;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_QUERY:
    {
        const DATA_SOURCE_CREATE* queryIn;
        DATA_SOURCE_CREATE* queryOut;
        LONG slotIndex;
        ULONG index;

        if ((InputBuffer == NULL) || (InputBufferSize < sizeof(DATA_SOURCE_CREATE))
            || (OutputBuffer == NULL) || (OutputBufferSize < sizeof(DATA_SOURCE_CREATE)))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        queryIn = (const DATA_SOURCE_CREATE*)InputBuffer;
        queryOut = (DATA_SOURCE_CREATE*)OutputBuffer;
        if (CrashDumpGuidIsZero(&queryIn->Guid))
        {
            return STATUS_INVALID_PARAMETER;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = -1;
        for (index = 1; index < Context->CrashDumpDataSourceCount; index++)
        {
            if (RtlCompareMemory(
                &Context->CrashDumpDataSources[index].RingBufferGuid,
                &queryIn->Guid,
                sizeof(GUID)) == sizeof(GUID))
            {
                slotIndex = (LONG)index;
                break;
            }
        }

        if (slotIndex < 0)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

            source = &Context->CrashDumpDataSources[(ULONG)slotIndex];
            source->FileObject[2] = fileObject;
            queryOut->EntriesCount = (source->RingBufferSizeOfEachEntry == 0)
                ? 0
                : (source->RingBufferSize / source->RingBufferSizeOfEachEntry);
            queryOut->EntrySize = source->RingBufferSizeOfEachEntry;
            queryOut->Guid = source->RingBufferGuid;
            *BytesReturned = sizeof(DATA_SOURCE_CREATE);
            status = STATUS_SUCCESS;
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_READ:
    {
        LONG slotIndex;
        ULONG entrySize;

        if ((OutputBuffer == NULL) || (OutputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 2);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            entrySize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSizeOfEachEntry;
            if (OutputBufferSize < entrySize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = CrashDump_DataSourceReadInternal(
                    Context,
                    (ULONG)slotIndex,
                    (UCHAR*)OutputBuffer,
                    OutputBufferSize);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = OutputBufferSize;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    case IOCTL_CRASHDUMP_DATA_SOURCE_CAPTURE:
    {
        LONG slotIndex;
        ULONG captureBytes;
        ULONG ringBufferSize;

        if ((OutputBuffer == NULL) || (OutputBufferSize == 0))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (fileObject == NULL)
        {
            return PMICGLINK_STATUS_INVALID_ADDRESS;
        }

        WdfWaitLockAcquire(Context->CrashDumpLock, NULL);

        slotIndex = CrashDump_FileHandleSlotFind(Context, fileObject, 2);
        if (slotIndex < 0)
        {
            status = STATUS_INVALID_HANDLE;
        }
        else
        {
            ringBufferSize = Context->CrashDumpDataSources[(ULONG)slotIndex].RingBufferSize;
            if (OutputBufferSize < ringBufferSize)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                captureBytes = 0;
                status = CrashDump_DataSourceCaptureInternal(
                    Context,
                    (ULONG)slotIndex,
                    (UCHAR*)OutputBuffer,
                    OutputBufferSize,
                    &captureBytes);
                if (NT_SUCCESS(status))
                {
                    *BytesReturned = captureBytes;
                }
            }
        }

        WdfWaitLockRelease(Context->CrashDumpLock);
        return status;
    }

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS
CrashDump_IoctlHandler(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    )
{
    return HandleCrashDumpRequest(
        Context,
        Request,
        IoControlCode,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutputBufferSize,
        BytesReturned);
}

static VOID
CrashDump_PrepareBugCheckSnapshot(
    _Inout_ PMICGLINK_CRASHDUMP_DATA_SOURCE* Source
    )
{
    ULONG validEntries;
    ULONG oldestIndex;
    ULONG index;

    if (Source == NULL)
    {
        return;
    }

    Source->BugCheckBufferPointer = NULL;
    if ((Source->BugCheckSnapshotBuffer == NULL)
        || (Source->RingBufferSize == 0)
        || (Source->EntriesCount == 0)
        || (Source->RingBufferSizeOfEachEntry == 0))
    {
        return;
    }

    RtlZeroMemory(Source->BugCheckSnapshotBuffer, Source->RingBufferSize);

    if ((Source->RingBufferData == NULL) || (Source->ValidEntryCount == 0))
    {
        Source->BugCheckBufferPointer = Source->BugCheckSnapshotBuffer;
        return;
    }

    validEntries = Source->ValidEntryCount;
    if (validEntries > Source->EntriesCount)
    {
        validEntries = Source->EntriesCount;
    }

    oldestIndex = (Source->CurrentRingBufferIndex + Source->EntriesCount - validEntries) % Source->EntriesCount;
    for (index = 0; index < validEntries; index++)
    {
        ULONG sourceIndex;

        sourceIndex = (oldestIndex + index) % Source->EntriesCount;
        RtlCopyMemory(
            Source->BugCheckSnapshotBuffer + ((SIZE_T)index * Source->RingBufferSizeOfEachEntry),
            Source->RingBufferData + ((SIZE_T)sourceIndex * Source->RingBufferSizeOfEachEntry),
            Source->RingBufferSizeOfEachEntry);
    }

    Source->BugCheckBufferPointer = Source->BugCheckSnapshotBuffer;
}

static NTSTATUS
CrashDump_RingBufferElementsFirstBufferGet(
    _In_opt_ PVOID DmfModule,
    _Out_writes_bytes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _Inout_opt_ PVOID CallbackContext
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;

    UNREFERENCED_PARAMETER(DmfModule);
    UNREFERENCED_PARAMETER(BufferSize);

    if ((Buffer == NULL) || (CallbackContext == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = (PMICGLINK_CRASHDUMP_DATA_SOURCE*)CallbackContext;
    source->BugCheckBufferPointer = Buffer;
    return STATUS_SUCCESS;
}

static NTSTATUS
CrashDump_RingBufferElementsXor(
    _In_opt_ PVOID DmfModule,
    _Inout_updates_bytes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _Inout_opt_ PVOID CallbackContext
    )
{
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONG index;
    ULONG keySize;

    UNREFERENCED_PARAMETER(DmfModule);

    if ((Buffer == NULL) || (CallbackContext == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    source = (PMICGLINK_CRASHDUMP_DATA_SOURCE*)CallbackContext;
    keySize = source->RingBufferEncryptionKeySize;
    if ((keySize == 0) || (keySize > 32))
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (index = source->RingBufferEnumerationOffset; index < BufferSize; index++)
    {
        Buffer[index] ^= (UCHAR)source->RingBufferEncryptionKey[index % keySize];
    }
    source->RingBufferEnumerationOffset = BufferSize;

    return STATUS_SUCCESS;
}

static VOID
CrashDump_BugCheckSecondaryDumpDataCallbackRingBuffer(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG* reason64;
    ULONG* reason32;
    ULONG bytesReported;
    UCHAR* xorBuffer;

    UNREFERENCED_PARAMETER(Reason);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < 44))
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    source = CrashDump_FindSourceFromCallbackRecord(context, Record);
    if (source == NULL)
    {
        source = CrashDump_FindActiveDataSource(context);
    }
    if (source == NULL)
    {
        WdfWaitLockRelease(context->CrashDumpLock);
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    reason32 = (ULONG*)ReasonSpecificData;

    if (reason64[4] != 0)
    {
        if (reason64[4] == reason64[0])
        {
            RtlCopyMemory(&reason64[2], &source->RingBufferGuid, sizeof(GUID));
            source->RingBufferEnumerationOffset = 0;
            xorBuffer = (source->BugCheckBufferPointer != NULL)
                ? source->BugCheckBufferPointer
                : source->RingBufferData;
            if (xorBuffer != NULL)
            {
                (VOID)CrashDump_RingBufferElementsXor(
                    NULL,
                    xorBuffer,
                    source->RingBufferSize,
                    source);
            }
            source->RingBufferEnumerationOffset = 0;
        }
    }
    else
    {
        CrashDump_PrepareBugCheckSnapshot(source);
        if (source->BugCheckBufferPointer != NULL)
        {
            (VOID)CrashDump_RingBufferElementsFirstBufferGet(
                NULL,
                source->BugCheckBufferPointer,
                source->RingBufferSize,
                source);
        }
    }

    if (source->BugCheckBufferPointer != NULL)
    {
        reason64[4] = (ULONGLONG)(ULONG_PTR)source->BugCheckBufferPointer;
        bytesReported = reason32[3];
        if (source->RingBufferSize < bytesReported)
        {
            bytesReported = source->RingBufferSize;
        }
    }
    else
    {
        reason64[4] = 0;
        bytesReported = 0;
    }

    reason32[10] = bytesReported;
    WdfWaitLockRelease(context->CrashDumpLock);
}

static VOID
CrashDump_BugCheckSecondaryDumpDataCallbackAdditional(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    PMICGLINK_CRASHDUMP_DATA_SOURCE* source;
    ULONGLONG* reason64;
    ULONG* reason32;
    ULONG bytesReported;

    UNREFERENCED_PARAMETER(Reason);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < 44))
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    source = CrashDump_FindSourceFromCallbackRecord(context, Record);
    if (source == NULL)
    {
        source = CrashDump_FindActiveDataSource(context);
    }
    if (source == NULL)
    {
        WdfWaitLockRelease(context->CrashDumpLock);
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    reason32 = (ULONG*)ReasonSpecificData;

    if (reason64[4] != 0)
    {
        if (reason64[4] == reason64[0])
        {
            RtlCopyMemory(&reason64[2], &source->RingBufferGuid, sizeof(GUID));
            bytesReported = reason32[3];
            if (source->RingBufferSize < bytesReported)
            {
                bytesReported = source->RingBufferSize;
            }
            reason32[10] = bytesReported;
        }
    }
    else
    {
        CrashDump_PrepareBugCheckSnapshot(source);
        if (source->BugCheckBufferPointer != NULL)
        {
            reason64[4] = (ULONGLONG)(ULONG_PTR)source->BugCheckBufferPointer;
            bytesReported = reason32[3];
            if (source->RingBufferSize < bytesReported)
            {
                bytesReported = source->RingBufferSize;
            }
        }
        else
        {
            reason64[4] = 0;
            bytesReported = 0;
        }
        reason32[10] = bytesReported;
    }

    WdfWaitLockRelease(context->CrashDumpLock);
}

static VOID
CrashDump_BugCheckTriageDumpDataCallback(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    _Inout_updates_bytes_opt_(ReasonSpecificDataLength) PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
    )
{
    PPMIC_GLINK_DEVICE_CONTEXT context;
    ULONGLONG* reason64;

    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(Record);

    if ((ReasonSpecificData == NULL) || (ReasonSpecificDataLength < sizeof(ULONGLONG) * 2))
    {
        return;
    }

    reason64 = (ULONGLONG*)ReasonSpecificData;
    if ((reason64[1] & 1ULL) == 0)
    {
        return;
    }

    context = gCrashDumpContext;
    if ((context == NULL) || (context->CrashDumpLock == NULL))
    {
        return;
    }

    WdfWaitLockAcquire(context->CrashDumpLock, NULL);
    CrashDump_PopulateTriageDataArray(context);
    if (context->CrashDumpTriageDataArray != NULL)
    {
        reason64[0] = (ULONGLONG)(ULONG_PTR)context->CrashDumpTriageDataArray;
    }
    else
    {
        reason64[0] = 0;
    }
    WdfWaitLockRelease(context->CrashDumpLock);
}

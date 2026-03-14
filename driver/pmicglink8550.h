#pragma once

#include <ntddk.h>
#include <wdf.h>

#define FILE_DEVICE_PMIC_GLINK 0x8033
#define FILE_DEVICE_BATT_MNGR  0x8009

#define IOCTL_PMICGLINK_GET_FEATURE_MASK       0x80332000u
#define IOCTL_PMICGLINK_UCSI_WRITE             0x80332004u
#define IOCTL_PMICGLINK_UCSI_READ              0x80332008u
#define IOCTL_PMICGLINK_PLATFORM_USBC_READ     0x8033200Cu
#define IOCTL_PMICGLINK_GET_OEM_MSG            0x80332010u
#define IOCTL_PMICGLINK_PLATFORM_USBC_NOTIFY   0x80332014u
#define IOCTL_PMICGLINK_TAD_GWS                0x80332018u
#define IOCTL_PMICGLINK_TAD_CWS                0x8033201Cu
#define IOCTL_PMICGLINK_TAD_STP                0x80332020u
#define IOCTL_PMICGLINK_TAD_STV                0x80332024u
#define IOCTL_PMICGLINK_TAD_TIP                0x80332028u
#define IOCTL_PMICGLINK_TAD_TIV                0x8033202Cu
#define IOCTL_PMICGLINK_OEM_READ_BUFFER        0x80332030u
#define IOCTL_PMICGLINK_OEM_WRITE_BUFFER       0x80332034u
#define IOCTL_PMICGLINK_GET_CHARGER_PORTS      0x80332038u
#define IOCTL_PMICGLINK_GET_USB_CHG_STATUS     0x8033203Cu
#define IOCTL_PMICGLINK_PRESHUTDOWN_CMD        0x80332040u
#define IOCTL_PMICGLINK_I2C_WRITE              0x80332044u
#define IOCTL_PMICGLINK_I2C_READ               0x80332048u
#define IOCTL_PMICGLINK_QCMB_GET_CHARGER_INFO  0x8033204Cu

#define IOCTL_PMICGLINK_ABD_UCSI_WRITE_LEGACY  0x80330FA0u
#define IOCTL_PMICGLINK_ABD_UCSI_READ_LEGACY   0x80330FA4u
#define IOCTL_PMICGLINK_ABD_USBC_READ_LEGACY   0x80330FA8u
#define IOCTL_PMICGLINK_ABD_GET_OEM_MSG_LEGACY 0x80330FACu
#define IOCTL_PMICGLINK_ABD_USBC_NOTIFY_LEGACY 0x80330FB0u

#define IOCTL_BATTMNGR_GET_CAPABILITIES            0x80092000u
#define IOCTL_BATTMNGR_GET_BATT_ID                 0x80092004u
#define IOCTL_BATTMNGR_GET_CHARGER_STATUS          0x80092008u
#define IOCTL_BATTMNGR_GET_BATT_INFO               0x8009200Cu
#define IOCTL_BATTMNGR_CONTROL_CHARGING            0x80092010u
#define IOCTL_BATTMNGR_SET_STATUS_CRITERIA         0x80092014u
#define IOCTL_BATTMNGR_GET_BATT_PRESENT            0x80092018u
#define IOCTL_BATTMNGR_SET_OPERATION_MODE          0x8009201Cu
#define IOCTL_BATTMNGR_SET_CHARGE_RATE             0x80092024u
#define IOCTL_BATTMNGR_NOTIFY_IFACE_FREE           0x80092028u
#define IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS  0x80092030u
#define IOCTL_BATTMNGR_GET_TEST_INFO               0x8009203Cu
#define IOCTL_BATTMNGR_GET_BATT_PRESENT_V1         0x80092040u
#define IOCTL_BATTMNGR_ENABLE_CHARGE_LIMIT         0x80092044u

#define IOCTL_BATTMNGR_GET_CHARGER_STATUS_V1       0x80090FA4u
#define IOCTL_BATTMNGR_GET_BATT_INFO_V1            0x80090FA8u
#define IOCTL_BATTMNGR_CONTROL_CHARGING_V1         0x80090FACu
#define IOCTL_BATTMNGR_SET_STATUS_CRITERIA_V1      0x80090FB0u
#define IOCTL_BATTMNGR_GET_BATT_PRESENT_V1_ALIAS0  0x80090FB4u
#define IOCTL_BATTMNGR_GET_BATT_PRESENT_V1_ALIAS1  0x80090FB8u
#define IOCTL_BATTMNGR_SET_OPERATION_MODE_V1       0x80090FBCu
#define IOCTL_BATTMNGR_SET_CHARGE_RATE_V1          0x80090FC4u
#define IOCTL_BATTMNGR_NOTIFY_IFACE_FREE_V1        0x80090FC8u
#define IOCTL_BATTMNGR_GET_BATTERY_PRESENT_STATUS_V1 0x80090FD0u
#define IOCTL_BATTMNGR_GET_TEST_INFO_V1            0x80090FDCu
#define IOCTL_BATTMNGR_GET_BATT_PRESENT_V2         0x80090FE0u

#define IOCTL_CRASHDUMP_DATA_SOURCE_READ           0x226010u
#define IOCTL_CRASHDUMP_DATA_SOURCE_QUERY          0x226014u
#define IOCTL_CRASHDUMP_DATA_SOURCE_CAPTURE        0x226018u
#define IOCTL_CRASHDUMP_DATA_SOURCE_CREATE         0x22A000u
#define IOCTL_CRASHDUMP_DATA_SOURCE_DESTROY        0x22A004u
#define IOCTL_CRASHDUMP_DATA_SOURCE_WRITE          0x22A008u

#define PMICGLINK_STATUS_DEVICE_NOT_READY ((NTSTATUS)0xC00000A3L)
#define PMICGLINK_STATUS_INVALID_ADDRESS  ((NTSTATUS)0xC0000141L)

#define PMICGLINK_MAX_PORTS 4u
#define PMICGLINK_CRASHDUMP_MAX_SOURCES 16u
#define PMICGLINK_UCSI_BUFFER_SIZE 48u
#define PMICGLINK_OEM_PROP_WORDS 64u
#define PMICGLINK_OEM_BUFFER_SIZE 64u
#define PMICGLINK_OEM_SEND_BUFFER_SIZE 66u
#define PMICGLINK_I2C_HEADER_SIZE 6u
#define PMICGLINK_I2C_DATA_SIZE 64u
#define PMICGLINK_BATTMNGR_BATT_INFO_OUT_SIZE 512u
#define PMICGLINK_BATTMNGR_TEST_INFO_INPUT_DWORDS 128u
#define PMICGLINK_BATTMNGR_TEST_INFO_OUTPUT_DWORDS 80u

typedef struct _PMICGLINK_UCSI_WRITE_DATA_BUF_TYPE
{
    UCHAR data[PMICGLINK_UCSI_BUFFER_SIZE];
} PMICGLINK_UCSI_WRITE_DATA_BUF_TYPE;

typedef struct _USBPD_DPM_USBC_PAN_ACK_DATA
{
    UCHAR port_index;
} USBPD_DPM_USBC_PAN_ACK_DATA;

typedef struct _USBPD_DPM_USBC_HOLDOFF_RELEASE_DATA
{
    ULONG port_index : 8;
} USBPD_DPM_USBC_HOLDOFF_RELEASE_DATA;

typedef union _USBPD_DPM_USBC_WRITE_BUFFER_CMD_PAYLOAD
{
    ULONG read_sel;
    USBPD_DPM_USBC_PAN_ACK_DATA pan_ack;
    USBPD_DPM_USBC_HOLDOFF_RELEASE_DATA hold_off_release;
} USBPD_DPM_USBC_WRITE_BUFFER_CMD_PAYLOAD;

typedef struct _USBPD_DPM_USBC_WRITE_BUFFER
{
    ULONG cmd_type;
    USBPD_DPM_USBC_WRITE_BUFFER_CMD_PAYLOAD cmd_payload;
} USBPD_DPM_USBC_WRITE_BUFFER;

typedef struct _USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_COMMON_DATA
{
    ULONG port_index : 8;
    ULONG orientation : 8;
    ULONG mux_ctrl : 8;
    ULONG reserved : 8;
    ULONG vid : 16;
    ULONG svid : 16;
} USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_COMMON_DATA;

typedef struct _USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DETAIL_DATA
{
    USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_COMMON_DATA common;
    UCHAR extended[8];
} USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DETAIL_DATA;

typedef union _USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DATA
{
    UCHAR AsUINT8[16];
    USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DETAIL_DATA detail;
} USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DATA;

typedef VOID (*PFN_PMICGLINK_UCSI_ALERT_CALLBACK)(
    _In_opt_ PVOID Context,
    _In_ ULONG EventCode,
    _In_ ULONG EventData
    );

typedef struct _PMICGLINK_DEVICE_DDIINTERFACE_TYPE
{
    INTERFACE InterfaceHeader;
    PFN_PMICGLINK_UCSI_ALERT_CALLBACK PmicGlinkUCSIAlertCallback;
} PMICGLINK_DEVICE_DDIINTERFACE_TYPE, *PPMICGLINK_DEVICE_DDIINTERFACE_TYPE;

typedef struct _QCMB_CHARGER_STATUS_INFO_DATA
{
    ULONG AsUINT32;
} QCMB_CHARGER_STATUS_INFO_DATA;

typedef struct _QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA
{
    ULONG currentChargerPowerUW;
    ULONG goodChargerThresholdUW;
    QCMB_CHARGER_STATUS_INFO_DATA chargerStatusInfo;
} QCMB_GET_ACTIVE_CHARGER_INFO_CMD_EXT_DATA;

typedef struct _PMICGLINK_PRESHUTDOWN_CMD_INBUF
{
    ULONG IoctlRev;
    ULONG CmdBitMask;
} PMICGLINK_PRESHUTDOWN_CMD_INBUF;

typedef struct _PMICGLINK_OEM_GET_PROP_INPUT
{
    ULONG property_id;
    ULONG data_size;
} PMICGLINK_OEM_GET_PROP_INPUT;

typedef struct _PMICGLINK_OEM_GET_PROP_OUTPUT
{
    ULONG data[PMICGLINK_OEM_PROP_WORDS];
} PMICGLINK_OEM_GET_PROP_OUTPUT;

typedef struct _PMICGLINK_OEM_SET_PROP_INPUT
{
    ULONG property_id;
    ULONG data_size;
    ULONG data[PMICGLINK_OEM_PROP_WORDS];
} PMICGLINK_OEM_SET_PROP_INPUT;

typedef struct _PMICGLINK_OEM_SET_DATA_BUF_TYPE
{
    UCHAR data[PMICGLINK_OEM_BUFFER_SIZE];
} PMICGLINK_OEM_SET_DATA_BUF_TYPE;

typedef struct _PMICGLINK_GET_CHARGER_PORTS_INFO_OUTBUF
{
    ULONG num_ports;
} PMICGLINK_GET_CHARGER_PORTS_INFO_OUTBUF;

typedef struct _BATT_MNGR_GET_BATT_ID_OUT
{
    ULONG batt_id;
} BATT_MNGR_GET_BATT_ID_OUT;

typedef struct _BATT_MNGR_CHG_STATUS_OUT
{
    ULONG power_state;
    ULONG capacity;
    ULONG voltage;
    LONG rate;
    ULONG charging_source;
} BATT_MNGR_CHG_STATUS_OUT;

typedef struct _BATT_MNGR_INFO_OUT
{
    ULONG capabilities;
    UCHAR technology;
    UCHAR reserved[3];
    UCHAR chemistry[4];
    ULONG designed_capacity;
    ULONG full_charged_capacity;
    ULONG default_alert1;
    ULONG default_alert2;
    ULONG critical_bias;
    ULONG cycle_count;
} BATT_MNGR_INFO_OUT;

typedef struct _BATT_MNGR_REPORTING_SCALE_OUT
{
    ULONG granularity;
    ULONG capacity;
} BATT_MNGR_REPORTING_SCALE_OUT;

typedef struct _BATT_MNGR_MANUFACTURE_DATE_OUT
{
    UCHAR day;
    UCHAR month;
    USHORT year;
} BATT_MNGR_MANUFACTURE_DATE_OUT;

typedef union _BATT_MNGR_GET_BATT_INFO_OUT
{
    BATT_MNGR_INFO_OUT batt_info;
    BATT_MNGR_REPORTING_SCALE_OUT batt_reporting_scale[4];
    ULONG batt_temperature;
    ULONG batt_estimated_time;
    WCHAR batt_device_name[256];
    BATT_MNGR_MANUFACTURE_DATE_OUT batt_manufacture_date;
    WCHAR batt_manufacture_name[256];
    WCHAR batt_unique_id[256];
    WCHAR batt_serial_num[256];
    UCHAR raw[PMICGLINK_BATTMNGR_BATT_INFO_OUT_SIZE];
} BATT_MNGR_GET_BATT_INFO_OUT;

typedef struct _BATT_MNGR_GET_BATT_INFO
{
    ULONG batt_id;
    ULONG batt_info_type;
    ULONG rate_of_drain;
} BATT_MNGR_GET_BATT_INFO;

typedef struct _BATT_MNGR_CONTROL_CHARGING
{
    ULONG batt_id;
    ULONG batt_command;
    ULONG batt_critical_bias_mw;
    ULONG batt_max_charge_current;
    GUID charger_guid;
    ULONG batt_charger_source_type;
    ULONG batt_charger_flags;
    ULONG batt_charger_voltage;
    ULONG batt_charger_port_type;
    ULONGLONG batt_charger_port_id;
} BATT_MNGR_CONTROL_CHARGING;

typedef struct _BATT_MNGR_NOTIFICATION_CRITERIA
{
    ULONG power_state;
    ULONG low_capacity;
    ULONG high_capacity;
} BATT_MNGR_NOTIFICATION_CRITERIA;

typedef struct _BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA
{
    ULONG batt_id;
    BATT_MNGR_NOTIFICATION_CRITERIA batt_notify_criteria;
} BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA;

typedef struct _BATT_MNGR_SET_OPERATIONAL_MODE
{
    ULONG mode_type;
} BATT_MNGR_SET_OPERATIONAL_MODE;

typedef struct _BATT_MNGR_SET_CHARGE_RATE
{
    ULONG charge_perc;
    UCHAR device_name;
    UCHAR reserved[3];
} BATT_MNGR_SET_CHARGE_RATE;

typedef struct _BATT_MNGR_GET_BATTERY_PRESENT_STATUS
{
    UCHAR IsBatteryPresent;
    UCHAR battPercentage;
    USHORT Reserved;
    ULONG capacity;
    LONG battTemperature;
    LONG battRate;
} BATT_MNGR_GET_BATTERY_PRESENT_STATUS;

typedef struct _PMICGLINK_PROP_CHARGER
{
    BOOLEAN ChargerSupported;
    ULONG ChargerCurrent;
    GUID ChargerGUID;
} PMICGLINK_PROP_CHARGER;

typedef struct _DATA_SOURCE_CREATE
{
    ULONG EntriesCount;
    ULONG EntrySize;
    GUID Guid;
} DATA_SOURCE_CREATE;

typedef struct _PMICGLINK_CRASHDUMP_DATA_SOURCE
{
    PUCHAR RingBufferData;
    WDFFILEOBJECT FileObject[3];
    GUID RingBufferGuid;
    CHAR RingBufferEncryptionKey[33];
    ULONG RingBufferEncryptionKeySize;
    ULONG RingBufferSize;
    ULONG RingBufferSizeOfEachEntry;
    ULONG EntriesCount;
    ULONG CurrentRingBufferIndex;
    ULONG ValidEntryCount;
    PUCHAR BugCheckBufferPointer;
    PUCHAR BugCheckSnapshotBuffer;
    ULONG RingBufferEnumerationOffset;
    KBUGCHECK_REASON_CALLBACK_RECORD BugCheckCallbackRecord;
    BOOLEAN BugCheckCallbackRegistered;
} PMICGLINK_CRASHDUMP_DATA_SOURCE;

typedef struct _BATT_MNGR_GET_CAPABILITIES_OUT
{
    ULONGLONG batt_mngr_capabilities;
} BATT_MNGR_GET_CAPABILITIES_OUT;

typedef struct _BATT_MNGR_GENERIC_TEST_INFO_INPUT_V1
{
    ULONG command;
    ULONG args[PMICGLINK_BATTMNGR_TEST_INFO_INPUT_DWORDS];
} BATT_MNGR_GENERIC_TEST_INFO_INPUT_V1;

typedef struct _BATT_MNGR_GENERIC_TEST_INFO_OUTPUT
{
    ULONG data[PMICGLINK_BATTMNGR_TEST_INFO_OUTPUT_DWORDS];
} BATT_MNGR_GENERIC_TEST_INFO_OUTPUT;

typedef struct _PMICGLINK_BATT_MNGR_GET_CHARGER_STATUS_INBUF
{
    ULONG port_index;
} PMICGLINK_BATT_MNGR_GET_CHARGER_STATUS_INBUF;

typedef struct _PMICGLINK_TAD_GWS_INBUF
{
    ULONG TimerId;
} PMICGLINK_TAD_GWS_INBUF;

typedef struct _PMICGLINK_TAD_CWS_INBUF
{
    ULONG TimerId;
} PMICGLINK_TAD_CWS_INBUF;

typedef struct _PMICGLINK_TAD_STP_INBUF
{
    ULONG TimerId;
    ULONG PolicyValue;
} PMICGLINK_TAD_STP_INBUF;

typedef struct _PMICGLINK_TAD_STV_INBUF
{
    ULONG TimerId;
    ULONG TimerValue;
} PMICGLINK_TAD_STV_INBUF;

typedef struct _PMICGLINK_TAD_TIP_INBUF
{
    ULONG TimerId;
} PMICGLINK_TAD_TIP_INBUF;

typedef struct _PMICGLINK_TAD_TIV_INBUF
{
    ULONG TimerId;
} PMICGLINK_TAD_TIV_INBUF;

typedef struct _PMICGLINK_TAD_GWS_OUTBUF
{
    ULONG AlarmStatus;
} PMICGLINK_TAD_GWS_OUTBUF;

typedef struct _PMICGLINK_TAD_CWS_OUTBUF
{
    ULONG Status;
} PMICGLINK_TAD_CWS_OUTBUF;

typedef struct _PMICGLINK_TAD_STP_OUTBUF
{
    ULONG Status;
} PMICGLINK_TAD_STP_OUTBUF;

typedef struct _PMICGLINK_TAD_STV_OUTBUF
{
    ULONG Status;
} PMICGLINK_TAD_STV_OUTBUF;

typedef struct _PMICGLINK_TAD_TIP_OUTBUF
{
    ULONG PolicySetting;
} PMICGLINK_TAD_TIP_OUTBUF;

typedef struct _PMICGLINK_TAD_TIV_OUTBUF
{
    ULONG TimerValueRemain;
} PMICGLINK_TAD_TIV_OUTBUF;

typedef struct _PMIC_GLINK_DEVICE_CONTEXT
{
    WDFDEVICE Device;

    BOOLEAN DeviceInterfacesRegistered;
    BOOLEAN AllReqIntfArrived;
    BOOLEAN GlinkDeviceLoaded;
    BOOLEAN GlinkChannelConnected;
    BOOLEAN GlinkChannelRestart;
    BOOLEAN GlinkChannelFirstConnect;
    BOOLEAN GlinkChannelUlogConnected;
    BOOLEAN GlinkChannelUlogRestart;
    BOOLEAN GlinkChannelUlogFirstConnect;
    BOOLEAN GlinkLinkStateUp;
    BOOLEAN RpeInitialized;
    BOOLEAN Hibernate;

    BOOLEAN Notify;
    BOOLEAN NotificationFlag;

    ULONG EventID;
    ULONG GlinkRxIntent;
    ULONG GlinkUlogRxIntent;

    ULONG NumPorts;
    LONG UsbinPower[PMICGLINK_MAX_PORTS];
    LARGE_INTEGER LastUsbBattMngrQueryTime;

    UCHAR UCSIDataBuffer[PMICGLINK_UCSI_BUFFER_SIZE];

    ULONG OemPropData[PMICGLINK_OEM_PROP_WORDS];
    UCHAR OemReceivedData[PMICGLINK_OEM_BUFFER_SIZE];
    UCHAR CachedOemSendBuffer[PMICGLINK_OEM_SEND_BUFFER_SIZE];

    UCHAR I2CHeader[PMICGLINK_I2C_HEADER_SIZE];
    UCHAR I2CData[PMICGLINK_I2C_DATA_SIZE];
    ULONG I2CDataLength;

    PMICGLINK_TAD_GWS_OUTBUF gws_out;
    PMICGLINK_TAD_CWS_OUTBUF cws_out;
    PMICGLINK_TAD_STP_OUTBUF stp_out;
    PMICGLINK_TAD_STV_OUTBUF stv_out;
    PMICGLINK_TAD_TIP_OUTBUF tip_out;
    PMICGLINK_TAD_TIV_OUTBUF tiv_out;

    BOOLEAN QcmbConnected;
    ULONG QcmbStatus;
    ULONG QcmbCurrentChargerPowerUW;
    ULONG QcmbGoodChargerThresholdUW;
    ULONG QcmbChargerStatusInfo;
    KEVENT QcmbNotifyEvent;
    PMICGLINK_PROP_CHARGER HvdcpCharger;
    PMICGLINK_PROP_CHARGER HvdcpV3Charger;
    PMICGLINK_PROP_CHARGER IWallCharger;
    ULONG MaxFlashCurrent;
    ULONG LastChargerVoltage;
    ULONG LastChargerPortType;

    BATT_MNGR_GET_BATT_ID_OUT LegacyBattId;
    BATT_MNGR_CHG_STATUS_OUT LegacyChargeStatus;
    BATT_MNGR_INFO_OUT LegacyBattInfo;
    BATT_MNGR_REPORTING_SCALE_OUT LegacyReportingScale[4];
    ULONG LegacyBattTemperature;
    ULONG LegacyBattEstimatedTime;
    WCHAR LegacyBattDeviceName[256];
    BATT_MNGR_MANUFACTURE_DATE_OUT LegacyBattManufactureDate;
    WCHAR LegacyBattManufactureName[256];
    WCHAR LegacyBattUniqueId[256];
    WCHAR LegacyBattSerialNumber[256];
    BATT_MNGR_CONTROL_CHARGING LegacyControlCharging;
    BATT_MNGR_SET_STATUS_NOTIFICATION_CRITERIA LegacyStatusCriteria;
    BATT_MNGR_SET_OPERATIONAL_MODE LegacyOperationalMode;
    BATT_MNGR_SET_CHARGE_RATE LegacyChargeRate;
    BATT_MNGR_GENERIC_TEST_INFO_OUTPUT LegacyTestInfo;

    UCHAR LegacyBattPercentage;
    BOOLEAN LegacyStatusNotificationPending;
    BOOLEAN LegacyStateChangePending;

    ULONGLONG LegacyLastBattIdQueryMsec;
    ULONGLONG LegacyLastChargeStatusQueryMsec;
    ULONGLONG LegacyLastBattInfoQueryMsec;
    ULONG LegacyLastTestInfoRequestType;
    ULONG LegacyReserved;

    ULONG ModernStandbyState;
    PCALLBACK_OBJECT ModernStandbyCallbackObject;
    PVOID ModernStandbyCallbackHandle;
    PVOID GlinkNotificationEntry;
    PVOID AbdNotificationEntry;
    PVOID BattMiniNotificationEntry;

    PMICGLINK_DEVICE_DDIINTERFACE_TYPE DdiInterface;
    WDFSPINLOCK StateLock;
    WDFWAITLOCK DdiInterfaceLock;
    WDFWAITLOCK CrashDumpLock;
    USHORT DdiInterfaceRefCount;
    USHORT DdiInterfacePadding;
    KBUGCHECK_REASON_CALLBACK_RECORD CrashDumpAdditionalCallbackRecord;
    BOOLEAN CrashDumpAdditionalCallbackRegistered;
    KBUGCHECK_REASON_CALLBACK_RECORD CrashDumpTriageCallbackRecord;
    BOOLEAN CrashDumpTriageCallbackRegistered;
    ULONG CrashDumpDataSourceCount;
    PMICGLINK_CRASHDUMP_DATA_SOURCE CrashDumpDataSources[PMICGLINK_CRASHDUMP_MAX_SOURCES];

    USBPD_DPM_USBC_WRITE_BUFFER LastUsbcWriteRequest;
    USBPD_DPM_USBC_PORT_PIN_ASSIGNMENT_DATA LastUsbcNotification;
    UCHAR PendingPan;
    UCHAR PlatformState;
    UCHAR Reserved2[2];
} PMIC_GLINK_DEVICE_CONTEXT, *PPMIC_GLINK_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PMIC_GLINK_DEVICE_CONTEXT, PmicGlinkGetDeviceContext)

EVT_WDF_DRIVER_DEVICE_ADD PmicGlinkEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PmicGlinkEvtIoDeviceControl;
EVT_WDF_DEVICE_PREPARE_HARDWARE PmicGlinkEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PmicGlinkEvtReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY PmicGlinkEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT PmicGlinkEvtD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT PmicGlinkEvtSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_RESTART PmicGlinkEvtSelfManagedIoRestart;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP PmicGlinkEvtSelfManagedIoCleanup;
EVT_WDF_DEVICE_QUERY_REMOVE PmicGlinkEvtQueryRemove;
EVT_WDF_DEVICE_QUERY_STOP PmicGlinkEvtQueryStop;

NTSTATUS
HandlePmicGlinkRequest(
    _In_ PPMIC_GLINK_DEVICE_CONTEXT Context,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_ SIZE_T* BytesReturned
    );

/*
 * Copyright (C) 2017 Red Hat, Inc.
 */

#include <ntddk.h>
#include <wdf.h>

#include "trace.h"

typedef struct _DEVICE_CONTEXT {

    // HW Resources.
    PVOID               IoBaseAddress;
    ULONG               IoRange;
    BOOLEAN             MappedPort;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

#define VMCOREINFO_DRIVER_MEMORY_TAG (ULONG)'npVP'

// Referenced in MSDN but not declared in SDK/WDK headers.
#define DUMP_TYPE_FULL 1

//
// WDFDRIVER events.
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD VMCoreInfoEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP VMCoreInfoEvtDriverContextCleanup;

EVT_WDF_DEVICE_PREPARE_HARDWARE VMCoreInfoEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE VMCoreInfoEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY VMCoreInfoEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT VMCoreInfoEvtDeviceD0Exit;

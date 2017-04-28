/*
 * Copyright (C) 2017 Red Hat, Inc.
 */

#include "vmcoreinfo.h"
#include "power.tmh"
#include <Acpiioct.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VMCoreInfoEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, VMCoreInfoEvtDeviceReleaseHardware)
#pragma alloc_text(PAGE, VMCoreInfoEvtDeviceD0Entry)
#pragma alloc_text(PAGE, VMCoreInfoEvtDeviceD0Exit)
#endif

NTSTATUS VMCoreInfoEvtDevicePrepareHardware(IN WDFDEVICE Device,
                                            IN WDFCMRESLIST Resources,
                                            IN WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Resources);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> %!FUNC! Device: %p",
        Device);

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- %!FUNC!");

    return STATUS_SUCCESS;
}

NTSTATUS VMCoreInfoEvtDeviceReleaseHardware(IN WDFDEVICE Device,
                                            IN WDFCMRESLIST ResourcesTranslated)
{
    PDEVICE_CONTEXT context = GetDeviceContext(Device);

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> %!FUNC!");

    PAGED_CODE();

    if (context->MappedPort && context->IoBaseAddress)
    {
        MmUnmapIoSpace(context->IoBaseAddress, context->IoRange);
        context->IoBaseAddress = NULL;
    }

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- %!FUNC!");

    return STATUS_SUCCESS;
}

static NTSTATUS VMCoreInfoGetAddr(IN WDFDEVICE Device, PACPI_EVAL_OUTPUT_BUFFER *Output)
{
    NTSTATUS status;
    WDFMEMORY inputMemory;
    WDF_MEMORY_DESCRIPTOR inputMemDesc;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    size_t inputBufferSize;
    size_t inputArgumentBufferSize;
    WDF_MEMORY_DESCRIPTOR outputMemDesc;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    size_t outputBufferSize;
    size_t outputArgumentBufferSize;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_REQUEST_SEND_OPTIONS sendOptions;

    PAGED_CODE();

    inputMemory = WDF_NO_HANDLE;
    outputBuffer = NULL;

    inputArgumentBufferSize = 0;

    inputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
        inputArgumentBufferSize;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    status = WdfMemoryCreate(&attributes,
        NonPagedPoolNx,
        0,
        inputBufferSize,
        &inputMemory,
        (PVOID*)&inputBuffer);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("WdfMemoryCreate failed for %Iu bytes - %x\n", inputBufferSize, status);
        goto Exit;
    }

    RtlZeroMemory(inputBuffer, inputBufferSize);

    inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer->Size = (ULONG)inputArgumentBufferSize;
    inputBuffer->ArgumentCount = 0;
    inputBuffer->MethodNameAsUlong = (ULONG)'RDDA';

    outputArgumentBufferSize = 2 * ACPI_METHOD_ARGUMENT_LENGTH(sizeof(PHYSICAL_ADDRESS));
    outputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
        outputArgumentBufferSize;

    outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)ExAllocatePoolWithTag(NonPagedPoolNx,
        outputBufferSize,
        VMCOREINFO_DRIVER_MEMORY_TAG);

    if (!outputBuffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DbgPrint("ExAllocatePoolWithTag failed for %Iu bytes\n", outputBufferSize);
        goto Exit;
    }

    RtlZeroMemory(outputBuffer, outputBufferSize);

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputMemDesc, inputMemory, NULL);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputMemDesc, outputBuffer, (ULONG)outputBufferSize);

    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_MS(1000));

    status = WdfIoTargetSendInternalIoctlSynchronously(
        WdfDeviceGetIoTarget(Device),
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputMemDesc,
        &outputMemDesc,
        &sendOptions,
        NULL);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("IOCTL_ACPI_EVAL_METHOD for ADDR failed - %x\n", status);
        goto Exit;
    }

    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        DbgPrint("ACPI_EVAL_OUTPUT_BUFFER signature is incorrect\n");
        status = STATUS_ACPI_INVALID_DATA;
        goto Exit;
    }

Exit:

    if (inputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(inputMemory);
    }

    if (!NT_SUCCESS(status) || !Output)
    {
        if (outputBuffer)
        {
            ExFreePoolWithTag(outputBuffer, VMCOREINFO_DRIVER_MEMORY_TAG);
        }
    } else
    {
        *Output = outputBuffer;
    }

    return status;
}

NTSTATUS VMCoreInfoEvtDeviceD0Entry(IN WDFDEVICE Device,
								    IN WDF_POWER_DEVICE_STATE PreviousState)
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER Output = NULL;

    UNREFERENCED_PARAMETER(PreviousState);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> %!FUNC! Device: %p",
		Device);

	PAGED_CODE();

    status = VMCoreInfoGetAddr(Device, &Output);
    DbgPrint("VMCoreInfoGetAddr returned %x, %p\n", status, Output);
    if (Output)
    {
        DbgPrint("ARG0: (%d) %x\n", Output->Argument[0].DataLength, *(unsigned *)Output->Argument[0].Data);
        DbgPrint("ARG1: (%d) %x\n", Output->Argument[1].DataLength, *(unsigned *)Output->Argument[1].Data);
        ExFreePoolWithTag(Output, VMCOREINFO_DRIVER_MEMORY_TAG);
    }

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- %!FUNC!");

	return STATUS_SUCCESS;
}

NTSTATUS VMCoreInfoEvtDeviceD0Exit(IN WDFDEVICE Device,
								   IN WDF_POWER_DEVICE_STATE TargetState)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> %!FUNC! Device: %p",
		Device);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- %!FUNC!");

	return STATUS_SUCCESS;
}

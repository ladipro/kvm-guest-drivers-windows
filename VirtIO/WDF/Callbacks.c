/**********************************************************************
 * Copyright (c) 2016  Red Hat, Inc.
 *
 * File: Callbacks.c
 *
 * Author(s):
 *  Ladi Prosek <lprosek@redhat.com>
 *
 * Implementation of virtio_system_ops VirtioLib callbacks
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * See the COPYING file in the top-level directory.
 *
**********************************************************************/
#include "osdep.h"
#include "virtio_pci.h"
#include "VirtioWDF.h"
#include "private.h"

static void *alloc_pages_exact(void *context, size_t size)
{
    PHYSICAL_ADDRESS HighestAcceptable;
    void *ret;

    UNREFERENCED_PARAMETER(context);

    HighestAcceptable.QuadPart = 0xFFFFFFFFFF;
    ret = MmAllocateContiguousMemory(size, HighestAcceptable);
    RtlZeroMemory(ret, size);
    return ret;
}

static void free_pages_exact(void *context, void *virt, size_t size)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(size);

    MmFreeContiguousMemory(virt);
}

static ULONGLONG virt_to_phys(void *context, void *address)
{
    PHYSICAL_ADDRESS pa;
    UNREFERENCED_PARAMETER(context);

    pa = MmGetPhysicalAddress(address);
    return pa.QuadPart;
}

static void *kmalloc(void *context, size_t size)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;

    return ExAllocatePoolWithTag(
        NonPagedPool,
        size,
        pWdfDriver->MemoryTag);
}

static void kfree(void *context, void *addr)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;

    ExFreePoolWithTag(
        addr,
        pWdfDriver->MemoryTag);
}

static int pci_read_config_byte(void *context, int where, u8 *bVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, bVal, sizeof(*bVal));
}

static int pci_read_config_word(void *context, int where, u16 *wVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, wVal, sizeof(*wVal));
}

static int pci_read_config_dword(void *context, int where, u32 *dwVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, dwVal, sizeof(*dwVal));
}

static PVIRTIO_WDF_BAR find_bar(void *context, int bar)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;
    PSINGLE_LIST_ENTRY iter = &pWdfDriver->PCIBars;
    
    while (iter->Next != NULL) {
        PVIRTIO_WDF_BAR pBar = CONTAINING_RECORD(iter->Next, VIRTIO_WDF_BAR, ListEntry);
        if (pBar->iBar == bar) {
            return pBar;
        }
        iter = iter->Next;
    }
    return NULL;
}

static size_t pci_resource_len(void *context, int bar)
{
    PVIRTIO_WDF_BAR pBar = find_bar(context, bar);
    return (pBar ? pBar->uLength : 0);
}

static u32 pci_resource_flags(void *context, int bar)
{
    PVIRTIO_WDF_BAR pBar = find_bar(context, bar);
    if (pBar) {
        return (pBar->bPortSpace ? IORESOURCE_IO : IORESOURCE_MEM);
    }
    return 0;
}

static void *pci_iomap_range(void *context, int bar, size_t offset, size_t maxlen)
{
    PVIRTIO_WDF_BAR pBar = find_bar(context, bar);
    if (pBar) {
        if (pBar->pBase == NULL) {
            ASSERT(!pBar->bPortSpace);
            pBar->pBase = MmMapIoSpace(pBar->BasePA, pBar->uLength, MmNonCached);
        }
        if (pBar->pBase != NULL && offset < pBar->uLength) {
            return (char *)pBar->pBase + offset;
        }
    }
    return NULL;
}

static void pci_iounmap(void *context, void *address)
{
    /* We map entire memory/IO regions on demand and unmap all of them on shutdown
     * so nothing to do here.
     */
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(address);
}

static u16 pci_get_msix_vector(void *context, int queue)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;
    WDF_INTERRUPT_INFO info;
    u16 vector;

    WDF_INTERRUPT_INFO_INIT(&info);
    if (queue >= 0) {
        /* queue interrupt */
        if (pWdfDriver->pQueueParams != NULL) {
            WdfInterruptGetInfo(pWdfDriver->pQueueParams[queue].Interrupt, &info);
        }
    }
    else if (pWdfDriver->ConfigInterrupt != NULL) {
        /* on-device-config-change interrupt */
        WdfInterruptGetInfo(pWdfDriver->ConfigInterrupt, &info);
    }
    if (info.MessageSignaled) {
        ASSERT(info.MessageNumber < MAXUSHORT);
        vector = (u16)info.MessageNumber;
    }
    else {
        vector = VIRTIO_MSI_NO_VECTOR;
    }

    return vector;
}

static void msleep(void *context, unsigned int msecs)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(context);

    if (KeGetCurrentIrql() <= APC_LEVEL) {
        LARGE_INTEGER delay;
        delay.QuadPart = Int32x32To64(msecs, -10000);
        status = KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    if (!NT_SUCCESS(status)) {
        /* fall back to busy wait if we're not allowed to sleep */
        KeStallExecutionProcessor(1000 * msecs);
    }
}

VirtIOSystemOps VirtIOWdfSystemOps = {
    alloc_pages_exact,
    free_pages_exact,
    virt_to_phys,
    kmalloc,
    kfree,
    pci_read_config_byte,
    pci_read_config_word,
    pci_read_config_dword,
    pci_resource_len,
    pci_resource_flags,
    pci_get_msix_vector,
    pci_iomap_range,
    pci_iounmap,
    msleep,
};

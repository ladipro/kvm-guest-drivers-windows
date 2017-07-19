/*
* Virtio PCI driver
*
* This module allows virtio devices to be used over a virtual PCI device.
* This can be used with QEMU based VMMs like KVM or Xen.
*
* Copyright IBM Corp. 2007
*
* Authors:
*  Anthony Liguori  <aliguori@us.ibm.com>
*
* This header is BSD licensed so anyone can use the definitions to implement
* compatible drivers/servers.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of IBM nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef _LINUX_VIRTIO_PCI_H
#define _LINUX_VIRTIO_PCI_H

#include <linux/types.h>

#ifndef VIRTIO_PCI_NO_LEGACY

/* A 32-bit r/o bitmask of the features supported by the host */
#define VIRTIO_PCI_HOST_FEATURES	0

/* A 32-bit r/w bitmask of features activated by the guest */
#define VIRTIO_PCI_GUEST_FEATURES	4

/* A 32-bit r/w PFN for the currently selected queue */
#define VIRTIO_PCI_QUEUE_PFN		8

/* A 16-bit r/o queue size for the currently selected queue */
#define VIRTIO_PCI_QUEUE_NUM		12

/* A 16-bit r/w queue selector */
#define VIRTIO_PCI_QUEUE_SEL		14

/* A 16-bit r/w queue notifier */
#define VIRTIO_PCI_QUEUE_NOTIFY		16

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS		18

/* An 8-bit r/o interrupt status register.  Reading the value will return the
* current contents of the ISR and will also clear it.  This is effectively
* a read-and-acknowledge. */
#define VIRTIO_PCI_ISR			19

/* MSI-X registers: only enabled if MSI-X is enabled. */
/* A 16-bit vector for configuration changes. */
#define VIRTIO_MSI_CONFIG_VECTOR        20
/* A 16-bit vector for selected queue notifications. */
#define VIRTIO_MSI_QUEUE_VECTOR         22

/* The remaining space is defined by each driver as the per-driver
* configuration space */
#define VIRTIO_PCI_CONFIG_OFF(msix_enabled)	((msix_enabled) ? 24 : 20)
/* Deprecated: please use VIRTIO_PCI_CONFIG_OFF instead */
#define VIRTIO_PCI_CONFIG(msix_enabled)	VIRTIO_PCI_CONFIG_OFF(msix_enabled)

/* Virtio ABI version, this must match exactly */
#define VIRTIO_PCI_ABI_VERSION		0

/* How many bits to shift physical queue address written to QUEUE_PFN.
* 12 is historical, and due to x86 page size. */
#define VIRTIO_PCI_QUEUE_ADDR_SHIFT	12

/* The alignment to use between consumer and producer parts of vring.
* x86 pagesize again. */
#define VIRTIO_PCI_VRING_ALIGN		4096

#endif /* VIRTIO_PCI_NO_LEGACY */

/* The bit of the ISR which indicates a device configuration change. */
#define VIRTIO_PCI_ISR_CONFIG		0x2
/* Vector value used to disable MSI for queue */
#define VIRTIO_MSI_NO_VECTOR            0xffff

#ifndef VIRTIO_PCI_NO_MODERN

/* IDs for different capabilities.  Must all exist. */

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG	1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
/* ISR access */
#define VIRTIO_PCI_CAP_ISR_CFG		3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG		5

/* This is the PCI capability header: */
struct virtio_pci_cap {
    __u8 cap_vndr;		/* Generic PCI field: PCI_CAP_ID_VNDR */
    __u8 cap_next;		/* Generic PCI field: next ptr. */
    __u8 cap_len;		/* Generic PCI field: capability length */
    __u8 cfg_type;		/* Identifies the structure. */
    __u8 bar;		/* Where to find it. */
    __u8 padding[3];	/* Pad to full dword. */
    __le32 offset;		/* Offset within bar. */
    __le32 length;		/* Length of the structure, in bytes. */
};

struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    __le32 notify_off_multiplier;	/* Multiplier for queue_notify_off. */
};

/* Fields in VIRTIO_PCI_CAP_COMMON_CFG: */
struct virtio_pci_common_cfg {
    /* About the whole device. */
    __le32 device_feature_select;	/* read-write */
    __le32 device_feature;		/* read-only */
    __le32 guest_feature_select;	/* read-write */
    __le32 guest_feature;		/* read-write */
    __le16 msix_config;		/* read-write */
    __le16 num_queues;		/* read-only */
    __u8 device_status;		/* read-write */
    __u8 config_generation;		/* read-only */

    /* About a specific virtqueue. */
    __le16 queue_select;		/* read-write */
    __le16 queue_size;		/* read-write, power of 2. */
    __le16 queue_msix_vector;	/* read-write */
    __le16 queue_enable;		/* read-write */
    __le16 queue_notify_off;	/* read-only */
    __le32 queue_desc_lo;		/* read-write */
    __le32 queue_desc_hi;		/* read-write */
    __le32 queue_avail_lo;		/* read-write */
    __le32 queue_avail_hi;		/* read-write */
    __le32 queue_used_lo;		/* read-write */
    __le32 queue_used_hi;		/* read-write */
};

/* Macro versions of offsets for the Old Timers! */
#define VIRTIO_PCI_CAP_VNDR		0
#define VIRTIO_PCI_CAP_NEXT		1
#define VIRTIO_PCI_CAP_LEN		2
#define VIRTIO_PCI_CAP_CFG_TYPE		3
#define VIRTIO_PCI_CAP_BAR		4
#define VIRTIO_PCI_CAP_OFFSET		8
#define VIRTIO_PCI_CAP_LENGTH		12

#define VIRTIO_PCI_NOTIFY_CAP_MULT	16

#define VIRTIO_PCI_COMMON_DFSELECT	0
#define VIRTIO_PCI_COMMON_DF		4
#define VIRTIO_PCI_COMMON_GFSELECT	8
#define VIRTIO_PCI_COMMON_GF		12
#define VIRTIO_PCI_COMMON_MSIX		16
#define VIRTIO_PCI_COMMON_NUMQ		18
#define VIRTIO_PCI_COMMON_STATUS	20
#define VIRTIO_PCI_COMMON_CFGGENERATION	21
#define VIRTIO_PCI_COMMON_Q_SELECT	22
#define VIRTIO_PCI_COMMON_Q_SIZE	24
#define VIRTIO_PCI_COMMON_Q_MSIX	26
#define VIRTIO_PCI_COMMON_Q_ENABLE	28
#define VIRTIO_PCI_COMMON_Q_NOFF	30
#define VIRTIO_PCI_COMMON_Q_DESCLO	32
#define VIRTIO_PCI_COMMON_Q_DESCHI	36
#define VIRTIO_PCI_COMMON_Q_AVAILLO	40
#define VIRTIO_PCI_COMMON_Q_AVAILHI	44
#define VIRTIO_PCI_COMMON_Q_USEDLO	48
#define VIRTIO_PCI_COMMON_Q_USEDHI	52

#endif /* VIRTIO_PCI_NO_MODERN */

#define MAX_QUEUES_PER_DEVICE_DEFAULT           8

typedef struct _tVirtIOPerQueueInfo
{
    /* the actual virtqueue */
    struct virtqueue *vq;
    /* the number of entries in the queue */
    int num;
    /* the index of the queue */
    int queue_index;
    /* the virtual address of the ring queue */
    void *queue;
    /* physical address of the ring queue */
    PHYSICAL_ADDRESS phys;
    /* owner per-queue context */
    void *pOwnerContext;
} tVirtIOPerQueueInfo, virtio_pci_vq_info;

typedef struct virtio_system_ops {
    // memory management
    void *(*alloc_contiguous_pages)(void *context, size_t size);
    void (*free_contiguous_pages)(void *context, void *virt, size_t size);
    ULONGLONG (*virt_to_phys)(void *context, void *address);
    void *(*kmalloc)(void *context, size_t size);
    void (*kfree)(void *context, void *addr);

    // PCI config space access
    int (*pci_read_config_byte)(void *context, int where, u8 *bVal);
    int (*pci_read_config_word)(void *context, int where, u16 *wVal);
    int (*pci_read_config_dword)(void *context, int where, u32 *dwVal);

    // PCI resource handling
    size_t (*pci_get_resource_len)(void *context, int bar);
    u32 (*pci_get_resource_flags)(void *context, int bar);
    u16 (*pci_get_msix_vector)(void *context, int queue);
    void *(*pci_iomap_range)(void *context, int bar, size_t offset, size_t maxlen);
    void (*pci_iounmap)(void *context, void *address);
} VirtIOSystemOps;

typedef struct TypeVirtIODevice
{
    ULONG_PTR addr;
    bool msix_used;

    const struct virtio_config_ops *config;
    const struct virtio_system_ops *system;
    void *DeviceContext;
    u8 *isr;
    u64 features;

    /* virtio 1.0 specific fields begin */
    struct virtio_pci_common_cfg *common;
    unsigned char *device;
    unsigned char *notify_base;
    int notify_map_cap;
    u32 notify_offset_multiplier;

    size_t device_len;
    size_t notify_len;
    /* virtio 1.0 specific fields end */

    struct virtqueue *(*setup_vq)(struct TypeVirtIODevice *vp_dev,
                                  tVirtIOPerQueueInfo *info,
                                  unsigned idx,
                                  void(*callback)(struct virtqueue *vq),
                                  const char *name,
                                  u16 msix_vec);
    void(*del_vq)(virtio_pci_vq_info *info);

    u16(*config_vector)(struct TypeVirtIODevice *vp_dev, u16 vector);

    ULONG maxQueues;
    tVirtIOPerQueueInfo info[MAX_QUEUES_PER_DEVICE_DEFAULT];
    /* do not add any members after info struct, it is extensible */
} VirtIODevice;


/***************************************************
shall be used only if VirtIODevice device storage is allocated
dynamically to provide support for more than 8 (MAX_QUEUES_PER_DEVICE_DEFAULT) queues.
return size in bytes to allocate for VirtIODevice structure.
***************************************************/
ULONG __inline VirtIODeviceSizeRequired(USHORT maxNumberOfQueues)
{
    ULONG size = sizeof(VirtIODevice);
    if (maxNumberOfQueues > MAX_QUEUES_PER_DEVICE_DEFAULT)
    {
        size += sizeof(tVirtIOPerQueueInfo) * (maxNumberOfQueues - MAX_QUEUES_PER_DEVICE_DEFAULT);
    }
    return size;
}

/***************************************************
addr - start of IO address space (usually 32 bytes)
allocatedSize - sizeof(VirtIODevice) if static or built-in allocation used

if allocated dynamically to provide support for more than MAX_QUEUES_PER_DEVICE_DEFAULT queues
allocatedSize should be at least VirtIODeviceSizeRequired(...) and pVirtIODevice should be aligned
at 8 bytes boundary (OS allocation does it automatically
***************************************************/
void VirtIODeviceInitialize(VirtIODevice * pVirtIODevice, ULONG_PTR addr, ULONG allocatedSize);
/***************************************************
shall be called if the device currently uses MSI-X feature
as soon as possible after initialization
before use VirtIODeviceGet or VirtIODeviceSet
***************************************************/
void VirtIODeviceSetMSIXUsed(VirtIODevice * pVirtIODevice, bool used);
void VirtIODeviceReset(VirtIODevice * pVirtIODevice);
void VirtIODeviceDumpRegisters(VirtIODevice * pVirtIODevice);

#define VirtIODeviceReadHostFeatures(pVirtIODevice) \
    ReadVirtIODeviceRegister((pVirtIODevice)->addr + VIRTIO_PCI_HOST_FEATURES)

#define VirtIODeviceWriteGuestFeatures(pVirtIODevice, u32Features) \
    WriteVirtIODeviceRegister((pVirtIODevice)->addr + VIRTIO_PCI_GUEST_FEATURES, (u32Features))

#define VirtIOIsFeatureEnabled(FeaturesList, Feature)   (!!((FeaturesList) & (1 << (Feature))))
#define VirtIOFeatureEnable(FeaturesList, Feature)      ((FeaturesList) |= (1 << (Feature)))
#define VirtIOFeatureDisable(FeaturesList, Feature)     ((FeaturesList) &= ~(1 << (Feature)))

void VirtIODeviceGet(VirtIODevice * pVirtIODevice,
                     unsigned offset,
                     void *buf,
                     unsigned len);
void VirtIODeviceSet(VirtIODevice * pVirtIODevice,
                     unsigned offset,
                     const void *buf,
                     unsigned len);
ULONG VirtIODeviceISR(VirtIODevice * pVirtIODevice);
void VirtIODeviceAddStatus(VirtIODevice * pVirtIODevice, u8 status);
void VirtIODeviceRemoveStatus(VirtIODevice * pVirtIODevice, u8 status);

void VirtIODeviceQueryQueueAllocation(VirtIODevice *vp_dev, unsigned index, unsigned long *pNumEntries, unsigned long *pAllocationSize);
struct virtqueue *VirtIODevicePrepareQueue(
                    VirtIODevice *vp_dev,
                    unsigned index,
                    PHYSICAL_ADDRESS pa,
                    void *va,
                    unsigned long size,
                    void *ownerContext,
                    BOOLEAN usePublishedIndices);
void VirtIODeviceDeleteQueue(struct virtqueue *vq, /* optional*/ void **pOwnerContext);
u32  VirtIODeviceGetQueueSize(struct virtqueue *vq);
void VirtIODeviceRenewQueue(struct virtqueue *vq);

unsigned long VirtIODeviceIndirectPageCapacity();

int virtio_device_initialize(VirtIODevice *pVirtIODevice,
                             const VirtIOSystemOps *pSystemOps,
                             PVOID DeviceContext,
                             ULONG allocatedSize);
void virtio_device_shutdown(VirtIODevice *pVirtIODevice);

int virtio_finalize_features(VirtIODevice *vdev);
u8 virtio_read_isr_status(VirtIODevice *vdev);

#ifdef NDIS50_MINIPORT
#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2
#define PCI_TYPE2_ADDRESSES             5

typedef struct _PCI_COMMON_HEADER {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    union {
        struct _PCI_HEADER_TYPE_0 /*{
            ULONG   BaseAddresses[PCI_TYPE0_ADDRESSES];
            ULONG   CIS;
            USHORT  SubVendorID;
            USHORT  SubSystemID;
            ULONG   ROMBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   Reserved2;
            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)
        } */ type0;



        //
        // PCI to PCI Bridge
        //

        struct _PCI_HEADER_TYPE_1 /*{
            ULONG   BaseAddresses[PCI_TYPE1_ADDRESSES];
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            UCHAR   IOBase;
            UCHAR   IOLimit;
            USHORT  SecondaryStatus;
            USHORT  MemoryBase;
            USHORT  MemoryLimit;
            USHORT  PrefetchBase;
            USHORT  PrefetchLimit;
            ULONG   PrefetchBaseUpper32;
            ULONG   PrefetchLimitUpper32;
            USHORT  IOBaseUpper16;
            USHORT  IOLimitUpper16;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   ROMBaseAddress;
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        }*/ type1;

        //
        // PCI to CARDBUS Bridge
        //

        struct _PCI_HEADER_TYPE_2 /*{
            ULONG   SocketRegistersBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved;
            USHORT  SecondaryStatus;
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            struct {
                ULONG   Base;
                ULONG   Limit;
            }       Range[PCI_TYPE2_ADDRESSES - 1];
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        }*/ type2;



    } u;

} PCI_COMMON_HEADER, *PPCI_COMMON_HEADER;
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
// IO space read\write functions
//
// ReadVirtIODeviceRegister
// WriteVirtIODeviceRegister
// ReadVirtIODeviceByte
// WriteVirtIODeviceByte
//
// Must be implemented in device specific module
//
/////////////////////////////////////////////////////////////////////////////////////
extern u32 ReadVirtIODeviceRegister(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceRegister(ULONG_PTR ulRegister, u32 ulValue);
extern u8 ReadVirtIODeviceByte(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceByte(ULONG_PTR ulRegister, u8 bValue);
extern u16 ReadVirtIODeviceWord(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceWord(ULONG_PTR ulRegister, u16 bValue);

#endif

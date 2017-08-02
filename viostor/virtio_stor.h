/**********************************************************************
 * Copyright (c) 2008-2016 Red Hat, Inc.
 *
 * File: virtio_stor.h
 *
 * Main include file
 * This file contains vrious routines and globals
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
**********************************************************************/

#ifndef ___VIOSTOR_H__
#define ___VIOSTOR_H__

#include <ntddk.h>
#ifdef USE_STORPORT
#define STOR_USE_SCSI_ALIASES
#include <storport.h>
#else
#include <scsi.h>
#endif

#include "osdep.h"
#include "virtio_pci.h"
#include "virtio_config.h"
#include "virtio.h"
#include "virtio_ring.h"

typedef struct VirtIOBufferDescriptor VIO_SG, *PVIO_SG;

#define VPD_SUPPORTED_PAGES         0x00
#define VPD_SERIAL_NUMBER           0x80
#define VPD_DEVICE_IDENTIFIERS      0x83
#define VPD_MEDIA_SERIAL_NUMBER     0x84
#define VPD_SOFTWARE_INTERFACE_IDENTIFIERS 0x84
#define VPD_NETWORK_MANAGEMENT_ADDRESSES 0x85
#define VPD_EXTENDED_INQUIRY_DATA   0x86
#define VPD_MODE_PAGE_POLICY        0x87
#define VPD_SCSI_PORTS              0x88

typedef struct _VPD_SUPPORTED_PAGES_PAGE {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
#if !defined(__midl)
    UCHAR SupportedPageList[0];
#endif
} VPD_SUPPORTED_PAGES_PAGE, *PVPD_SUPPORTED_PAGES_PAGE;

typedef struct _VPD_SERIAL_NUMBER_PAGE {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
#if !defined(__midl)
    UCHAR SerialNumber[0];
#endif
} VPD_SERIAL_NUMBER_PAGE, *PVPD_SERIAL_NUMBER_PAGE;

typedef struct _VPD_IDENTIFICATION_PAGE {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;


    //
    // The following field is actually a variable length array of identification
    // descriptors.  Unfortunately there's no C notation for an array of
    // variable length structures so we're forced to just pretend.
    //

#if !defined(__midl)
    // VPD_IDENTIFICATION_DESCRIPTOR Descriptors[0];
    UCHAR Descriptors[0];
#endif
} VPD_IDENTIFICATION_PAGE, *PVPD_IDENTIFICATION_PAGE;

typedef struct _VPD_IDENTIFICATION_DESCRIPTOR {
    UCHAR CodeSet : 4;          // VPD_CODE_SET
    UCHAR Reserved : 4;
    UCHAR IdentifierType : 4;   // VPD_IDENTIFIER_TYPE
    UCHAR Association : 2;
    UCHAR Reserved2 : 2;
    UCHAR Reserved3;
    UCHAR IdentifierLength;
#if !defined(__midl)
    UCHAR Identifier[0];
#endif
} VPD_IDENTIFICATION_DESCRIPTOR, *PVPD_IDENTIFICATION_DESCRIPTOR;

typedef enum _VPD_CODE_SET {
    VpdCodeSetReserved = 0,
    VpdCodeSetBinary = 1,
    VpdCodeSetAscii = 2,
    VpdCodeSetUTF8 = 3
} VPD_CODE_SET, *PVPD_CODE_SET;

typedef enum _VPD_IDENTIFIER_TYPE {
    VpdIdentifierTypeVendorSpecific = 0,
    VpdIdentifierTypeVendorId = 1,
    VpdIdentifierTypeEUI64 = 2,
    VpdIdentifierTypeFCPHName = 3,
    VpdIdentifierTypePortRelative = 4,
    VpdIdentifierTypeTargetPortGroup = 5,
    VpdIdentifierTypeLogicalUnitGroup = 6,
    VpdIdentifierTypeMD5LogicalUnitId = 7,
    VpdIdentifierTypeSCSINameString = 8
} VPD_IDENTIFIER_TYPE, *PVPD_IDENTIFIER_TYPE;


//
// Mode Sense/Select page constants.
//

#define MODE_PAGE_VENDOR_SPECIFIC       0x00
#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03 // disk
#define MODE_PAGE_MRW                   0x03 // cdrom
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05 // disk
#define MODE_PAGE_WRITE_PARAMETERS      0x05 // cdrom
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_PAGE_CD_AUDIO_CONTROL      0x0E
#define MODE_PAGE_DATA_COMPRESS         0x0F
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_XOR_CONTROL           0x10 // disk
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_ENCLOSURE_SERVICES_MANAGEMENT 0x14
#define MODE_PAGE_EXTENDED              0x15
#define MODE_PAGE_EXTENDED_DEVICE_SPECIFIC 0x16
#define MODE_PAGE_CDVD_FEATURE_SET      0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_LUN 0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_PORT 0x19
#define MODE_PAGE_POWER_CONDITION       0x1A
#define MODE_PAGE_LUN_MAPPING           0x1B
#define MODE_PAGE_FAULT_REPORTING       0x1C
#define MODE_PAGE_CDVD_INACTIVITY       0x1D // cdrom
#define MODE_PAGE_ELEMENT_ADDRESS       0x1D
#define MODE_PAGE_TRANSPORT_GEOMETRY    0x1E
#define MODE_PAGE_DEVICE_CAPABILITIES   0x1F
#define MODE_PAGE_CAPABILITIES          0x2A // cdrom

#define MODE_SENSE_RETURN_ALL           0x3f

#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0

typedef union _EIGHT_BYTE {

    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
        UCHAR Byte4;
        UCHAR Byte5;
        UCHAR Byte6;
        UCHAR Byte7;
    };

    ULONGLONG AsULongLong;
} EIGHT_BYTE, *PEIGHT_BYTE;

/* Feature bits */
#define VIRTIO_BLK_F_BARRIER    0       /* Does host support barriers? */
#define VIRTIO_BLK_F_SIZE_MAX   1       /* Indicates maximum segment size */
#define VIRTIO_BLK_F_SEG_MAX    2       /* Indicates maximum # of segments */
#define VIRTIO_BLK_F_GEOMETRY   4       /* Legacy geometry available  */
#define VIRTIO_BLK_F_RO         5       /* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE   6       /* Block size of disk is available*/
#define VIRTIO_BLK_F_SCSI       7       /* Supports scsi command passthru */
#define VIRTIO_BLK_F_WCACHE     9       /* write cache enabled */
#define VIRTIO_BLK_F_TOPOLOGY   10      /* Topology information is available */

/* These two define direction. */
#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1

#define VIRTIO_BLK_T_SCSI_CMD   2
#define VIRTIO_BLK_T_FLUSH      4
#define VIRTIO_BLK_T_GET_ID     8

#define VIRTIO_BLK_S_OK         0
#define VIRTIO_BLK_S_IOERR      1
#define VIRTIO_BLK_S_UNSUPP     2

#define SECTOR_SIZE             512
#define IO_PORT_LENGTH          0x40

#define VIRTIO_RING_F_INDIRECT_DESC     28

#define BLOCK_SERIAL_STRLEN     20

#ifdef INDIRECT_SUPPORTED
#define MAX_PHYS_SEGMENTS       64
#else
#define MAX_PHYS_SEGMENTS       16
#endif

#define VIRTIO_MAX_SG           (3+MAX_PHYS_SEGMENTS)

#pragma pack(1)
typedef struct virtio_blk_config {
    /* The capacity (in 512-byte sectors). */
    u64 capacity;
    /* The maximum segment size (if VIRTIO_BLK_F_SIZE_MAX) */
    u32 size_max;
    /* The maximum number of segments (if VIRTIO_BLK_F_SEG_MAX) */
    u32 seg_max;
    /* geometry the device (if VIRTIO_BLK_F_GEOMETRY) */
    struct virtio_blk_geometry {
        u16 cylinders;
        u8 heads;
        u8 sectors;
    } geometry;
    /* block size of device (if VIRTIO_BLK_F_BLK_SIZE) */
    u32 blk_size;
    u8  physical_block_exp;
    u8  alignment_offset;
    u16 min_io_size;
    u16 opt_io_size;
}blk_config, *pblk_config;
#pragma pack()

typedef struct virtio_blk_outhdr {
    /* VIRTIO_BLK_T* */
    u32 type;
    /* io priority. */
    u32 ioprio;
    /* Sector (ie. 512 byte offset) */
    u64 sector;
}blk_outhdr, *pblk_outhdr;

typedef struct virtio_blk_req {
    LIST_ENTRY list_entry;
    PVOID      req;
    blk_outhdr out_hdr;
    u8         status;
    VIO_SG     sg[VIRTIO_MAX_SG];
}blk_req, *pblk_req;

typedef struct virtio_bar {
    PHYSICAL_ADDRESS  BasePA;
    ULONG             uLength;
    PVOID             pBase;
    BOOLEAN           bPortSpace;
} VIRTIO_BAR, *PVIRTIO_BAR;

typedef struct _ADAPTER_EXTENSION {
    VirtIODevice          vdev;

    PVOID                 pageAllocationVa;
    ULONG                 pageAllocationSize;
    ULONG                 pageOffset;

    PVOID                 poolAllocationVa;
    ULONG                 poolAllocationSize;
    ULONG                 poolOffset;

    struct virtqueue *    vq;
    INQUIRYDATA           inquiry_data;
    blk_config            info;
    ULONG                 queue_depth;
    BOOLEAN               dump_mode;
    LIST_ENTRY            list_head;
    ULONG                 msix_vectors;
    BOOLEAN               msix_enabled;
    ULONGLONG             features;
    CHAR                  sn[BLOCK_SERIAL_STRLEN];
    BOOLEAN               sn_ok;
    blk_req               vbr;
    BOOLEAN               indirect;
    ULONGLONG             lastLBA;

    union {
        PCI_COMMON_HEADER pci_config;
        UCHAR             pci_config_buf[sizeof(PCI_COMMON_CONFIG)];
    };
    VIRTIO_BAR            pci_bars[PCI_TYPE0_ADDRESSES];
    ULONG                 system_io_bus_number;

#ifdef USE_STORPORT
    LIST_ENTRY            complete_list;
    STOR_DPC              completion_dpc;
    BOOLEAN               dpc_ok;
#endif
}ADAPTER_EXTENSION, *PADAPTER_EXTENSION;

typedef struct vring_desc_alias
{
    union
    {
        ULONGLONG data[2];
        UCHAR chars[SIZE_OF_SINGLE_INDIRECT_DESC];
    }u;
};

typedef struct _RHEL_SRB_EXTENSION {
    blk_req               vbr;
    ULONG                 out;
    ULONG                 in;
    ULONG                 Xfer;
    BOOLEAN               fua;
#ifndef USE_STORPORT
    BOOLEAN               call_next;
#endif
#if INDIRECT_SUPPORTED
    struct vring_desc_alias     desc[VIRTIO_MAX_SG];
#endif
}RHEL_SRB_EXTENSION, *PRHEL_SRB_EXTENSION;

BOOLEAN
VirtIoInterrupt(
    IN PVOID DeviceExtension
    );

#ifdef MSI_SUPPORTED
#ifndef PCIX_TABLE_POINTER
typedef struct {
  union {
    struct {
      ULONG BaseIndexRegister :3;
      ULONG Reserved          :29;
    };
    ULONG TableOffset;
  };
} PCIX_TABLE_POINTER, *PPCIX_TABLE_POINTER;
#endif

#ifndef PCI_MSIX_CAPABILITY
typedef struct {
  PCI_CAPABILITIES_HEADER Header;
  struct {
    USHORT TableSize      :11;
    USHORT Reserved       :3;
    USHORT FunctionMask   :1;
    USHORT MSIXEnable     :1;
  } MessageControl;
  PCIX_TABLE_POINTER      MessageTable;
  PCIX_TABLE_POINTER      PBATable;
} PCI_MSIX_CAPABILITY, *PPCI_MSIX_CAPABILITY;
#endif
#endif

#endif ___VIOSTOR__H__

#include "osdep.h"
#include "virtio_pci.h"
#include "virtio.h"
#include "kdebugprint.h"
#include "virtio_ring.h"

static inline u16 get_free_desc(struct virtqueue *vq)
{
    u16 idx = vq->first_free;

    ASSERT(vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT);
    vq->first_free = vq->vring.desc[idx].next;
    return idx;
}

static inline void put_free_desc(struct virtqueue *vq, u16 idx)
{
    vq->vring.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    vq->vring.desc[idx].next = vq->first_free;

    vq->first_free = idx;
}

static int virtqueue_add_buf_indirect(
    struct virtqueue *vq,
    struct scatterlist sg[],
    unsigned int out,
    unsigned int in,
    PVOID va,
    PHYSICAL_ADDRESS pa)
{
    struct vring_desc *desc = (struct vring_desc *)va;
    unsigned head;
    unsigned int i;

    for (i = 0; i < out + in; i++) {
        desc[i].flags = (i < out ? 0 : VIRTQ_DESC_F_WRITE);
        desc[i].flags |= VIRTQ_DESC_F_NEXT;
        desc[i].addr = sg->physAddr.QuadPart;
        desc[i].len = sg->length;
        desc[i].next = (u16)i + 1;
        sg++;
    }
    desc[i - 1].flags &= ~VIRTQ_DESC_F_NEXT;

    /* Use a single buffer which doesn't continue */
    head = get_free_desc(vq);
    vq->vring.desc[head].flags = VIRTQ_DESC_F_INDIRECT;
    vq->vring.desc[head].addr = pa.QuadPart;
    vq->vring.desc[head].len = i * sizeof(struct vring_desc);

    return head;
}

int virtqueue_add_buf(struct virtqueue *vq,
struct scatterlist sg[],
    unsigned int out,
    unsigned int in,
    void *opaque,
    void *va_indirect,
    ULONGLONG phys_indirect)
{
    struct vring *vring = &vq->vring;
    unsigned int i;
    SSIZE_T idx = -1;

    DbgPrint("virtqueue_add_buf: entry\n");

    if (va_indirect && (out + in) > 1 && vq->num_free > 0) {
        PHYSICAL_ADDRESS pa;
        pa.QuadPart = phys_indirect;
        idx = virtqueue_add_buf_indirect(vq, sg, out, in, va_indirect, pa);
        vq->num_free--;
    } else {
        if (out + in > vq->num_free) {
            DbgPrint("virtqueue_add_buf: error\n");
            return -ENOSPC;
        }

        // fill out vring descriptors
        SSIZE_T prev_desc_idx = -1;
        for (i = 0; i < out + in; i++) {
            SSIZE_T desc_idx = get_free_desc(vq);
            ASSERT(desc_idx >= 0);

            if (prev_desc_idx == -1) {
                vq->opaque[desc_idx] = opaque;
            } else {
                vring->desc[prev_desc_idx].flags |= VIRTQ_DESC_F_NEXT;
                vring->desc[prev_desc_idx].next = (u16)desc_idx;
            }
            prev_desc_idx = desc_idx;

            if (idx < 0) {
                idx = desc_idx;
            }
            vring->desc[desc_idx].addr = sg[i].physAddr.QuadPart;
            vring->desc[desc_idx].len = sg[i].length;
            vring->desc[desc_idx].flags = (i < out ? 0 : VIRTQ_DESC_F_WRITE);
            vring->desc[desc_idx].next = 0;

            vq->num_free--;
        }
    }

    vring->avail->ring[vring->avail->idx % vring->num] = (u16)idx;
    MemoryBarrier();
    vring->avail->idx = (vring->avail->idx + 1);
    vq->num_added++;

    DbgPrint("virtqueue_add_buf: exit\n");

    return 0;
}

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
    void *opaque;
    int idx;

    DbgPrint("virtqueue_get_buf: entry\n");

    if (vq->last_used == (int)vq->vring.used->idx) {
        // no buffers in the used ring
        DbgPrint("### no buffers in the used ring\n");
        return NULL;
    }

    KeMemoryBarrier();

    idx = vq->last_used % vq->vring.num;
    *len = vq->vring.used->ring[idx].len;

    // clear out the descriptors
    idx = vq->vring.used->ring[idx].id;
    opaque = vq->opaque[idx];
    do {
        u16 curr_idx = (u16)idx;
        vq->opaque[idx] = NULL;
        if (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT) {
            idx = vq->vring.desc[idx].next;
        } else {
            idx = -1;
        }
        put_free_desc(vq, curr_idx);
        vq->num_free++;
    } while (idx >= 0);

    vq->last_used++;
    if (!(vq->vring.avail->flags & VIRTQ_AVAIL_F_NO_INTERRUPT)) {
        vring_used_event(&vq->vring) = vq->last_used;
        MemoryBarrier();
    }

    DbgPrint("virtqueue_get_buf returning %p\n", opaque);

    return opaque;
}

BOOLEAN virtqueue_has_buf(struct virtqueue *vq)
{
    return (vq->last_used != vq->vring.used->idx);
}

/*
static int vring_add_indirect(struct vring_virtqueue *vq,
struct scatterlist sg[],
unsigned int out,
unsigned int in,
PVOID va,
ULONGLONG phys)
*/

bool virtqueue_kick_prepare(struct virtqueue *vq)
{
    KeMemoryBarrier();

    u16 old = (u16)(vq->vring.avail->idx - vq->num_added);
    u16 new = vq->vring.avail->idx;
    vq->num_added = 0;

    if (vq->event_suppression_enabled) {
        return (bool)vring_need_event(vring_avail_event(&vq->vring), new, old);
    } else {
        return !(vq->vring.used->flags & VRING_USED_F_NO_NOTIFY);
    }
}

void virtqueue_kick_always(struct virtqueue *vq)
{
    KeMemoryBarrier();
    vq->num_added = 0;
    virtqueue_notify(vq);
}

//static void detach_buf(struct vring_virtqueue *vq, unsigned int head)
//static inline bool more_used(const struct vring_virtqueue *vq)


bool virtqueue_enable_cb(struct virtqueue *vq)
{
    vq->vring.avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;

    vring_used_event(&vq->vring) = vq->last_used;
    MemoryBarrier();
    return (vq->last_used == vq->vring.used->idx);
}

bool virtqueue_enable_cb_delayed(struct virtqueue *vq)
{
    u16 bufs;

    vq->vring.avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;

    /* TODO: tune this threshold */
    bufs = (u16)(vq->vring.avail->idx - vq->last_used) * 3 / 4;
    vring_used_event(&vq->vring) = vq->last_used + bufs;
    MemoryBarrier();
    return ((u16)(vq->vring.used->idx - vq->last_used) <= bufs);
}

void virtqueue_disable_cb(struct virtqueue *vq)
{
    vq->vring.avail->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
}

BOOLEAN virtqueue_is_interrupt_enabled(struct virtqueue *vq)
{
    return !!(vq->vring.avail->flags & VIRTQ_AVAIL_F_NO_INTERRUPT);
}

struct virtqueue *vring_new_virtqueue(unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    VirtIODevice *vdev,
    void *pages,
    void (*notify)(struct virtqueue *),
    void *control)
{
    struct virtqueue *vq = (struct virtqueue *)control;

    if (num & (num - 1)) {
        DPrintf(0, ("Virtqueue length %u is not a power of 2\n", num));
        return NULL;
    }

    unsigned short i = (unsigned short)num;
    RtlZeroMemory(vq, sizeof(*vq) + num * sizeof(void *));

    vring_init(&vq->vring, num, pages, vring_align);
    vq->vdev = vdev;
    vq->notification_cb = notify;
    vq->index = index;
    vq->last_used = 0;
    vq->num_added = 0;
    //vq->avail_flags_shadow = 0;
    //vq->avail_idx_shadow = 0;

    // build a linked list of free descriptors
    vq->num_free = num;
    vq->first_free = 0;
    for (i = 0; i < num - 1; i++) {
        vq->vring.desc[i].flags = VIRTQ_DESC_F_NEXT;
        vq->vring.desc[i].next = i + 1;
    }
    return vq;
}

void virtqueue_shutdown(struct virtqueue *vq)
{
    unsigned int num = vq->vring.num;
    void *pages = vq->vring.desc;
    unsigned int vring_align = vq->vdev->addr ? PAGE_SIZE : SMP_CACHE_BYTES;

    RtlZeroMemory(pages, vring_size(num, vring_align));
    (void)vring_new_virtqueue(
        vq->index,
        vq->vring.num,
        vring_align,
        vq->vdev,
        pages,
        vq->notification_cb,
        vq);
}

void *virtqueue_detach_unused_buf(struct virtqueue *vq)
{
    int idx;
    void *opaque = NULL;

    for (idx = 0; idx < (int)vq->vring.num; idx++) {
        opaque = vq->opaque[idx];
        if (opaque) {
            break;
        }
    }
    if (opaque) {
        do {
            u16 curr_idx = (u16)idx;
            vq->opaque[idx] = NULL;
            if (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT) {
                idx = vq->vring.desc[idx].next;
            } else {
                idx = -1;
            }
            put_free_desc(vq, curr_idx);
            vq->vring.avail->idx--;
            vq->num_free++;
        } while (idx >= 0);
    }
    return opaque;
}

unsigned int vring_control_block_size()
{
    return sizeof(struct virtqueue);
}

void vring_transport_features(VirtIODevice *vdev, u64 *features)
{
    unsigned int i;

    for (i = VIRTIO_TRANSPORT_F_START; i < VIRTIO_TRANSPORT_F_END; i++) {
        if (i != VIRTIO_RING_F_INDIRECT_DESC &&
            i != VIRTIO_RING_F_EVENT_IDX &&
            i != VIRTIO_F_VERSION_1) {
            virtio_feature_disable(*features, i);
        }
    }
}

void virtio_set_queue_event_suppression(struct virtqueue *vq, bool enable)
{
    vq->event_suppression_enabled = enable;
}

u32 virtio_get_indirect_page_capacity()
{
    return PAGE_SIZE / sizeof(struct vring_desc);
}

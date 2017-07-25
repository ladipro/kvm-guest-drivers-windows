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

static inline u16 put_free_desc(struct virtqueue *vq, u16 idx)
{
    u16 next = vq->vring.desc[idx].next;
    vq->vring.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    vq->vring.desc[idx].next = vq->first_free;

    vq->opaque[idx] = NULL;

    vq->first_free = idx;
    return next;
}

int virtqueue_add_buf(
    struct virtqueue *vq,
    struct scatterlist sg[],
    unsigned int out,
    unsigned int in,
    void *opaque,
    void *va_indirect,
    ULONGLONG phys_indirect)
{
    struct vring *vring = &vq->vring;
    unsigned int i;
    u16 idx;

    DbgPrint("virtqueue_add_buf: entry\n");

    if (va_indirect && (out + in) > 1 && vq->num_free > 0) {
        /* Use one indirect descriptor */
        struct vring_desc *desc = (struct vring_desc *)va_indirect;

        for (i = 0; i < out + in; i++) {
            desc[i].flags = (i < out ? 0 : VIRTQ_DESC_F_WRITE);
            desc[i].flags |= VIRTQ_DESC_F_NEXT;
            desc[i].addr = sg->physAddr.QuadPart;
            desc[i].len = sg->length;
            desc[i].next = (u16)i + 1;
            sg++;
        }
        desc[i - 1].flags &= ~VIRTQ_DESC_F_NEXT;

        idx = get_free_desc(vq);
        vq->vring.desc[idx].flags = VIRTQ_DESC_F_INDIRECT;
        vq->vring.desc[idx].addr = phys_indirect;
        vq->vring.desc[idx].len = i * sizeof(struct vring_desc);

        vq->opaque[idx] = opaque;
        vq->num_free--;
    } else {
        u16 last_idx;

        /* Use out + in regular descriptors */
        if (out + in > vq->num_free) {
            DbgPrint("virtqueue_add_buf: error\n");
            return -ENOSPC;
        }

        /* First descriptor */
        idx = last_idx = get_free_desc(vq);
        vq->opaque[idx] = opaque;

        vring->desc[idx].addr = sg[0].physAddr.QuadPart;
        vring->desc[idx].len = sg[0].length;
        vring->desc[idx].flags = VIRTQ_DESC_F_NEXT;
        if (out == 0) {
            vring->desc[idx].flags |= VIRTQ_DESC_F_WRITE;
        }
        vring->desc[idx].next = vq->first_free;

        /* The rest of descriptors */
        for (i = 1; i < out + in; i++) {
            last_idx = get_free_desc(vq);

            vring->desc[last_idx].addr = sg[i].physAddr.QuadPart;
            vring->desc[last_idx].len = sg[i].length;
            vring->desc[last_idx].flags = VIRTQ_DESC_F_NEXT;
            if (i >= out) {
                vring->desc[last_idx].flags |= VIRTQ_DESC_F_WRITE;
            }
            vring->desc[last_idx].next = vq->first_free;
        }
        vring->desc[last_idx].flags &= ~VIRTQ_DESC_F_NEXT;
        vq->num_free -= (out + in);
    }

    /* Write the first descriptor into the available ring */
    vring->avail->ring[vq->master_vring_avail.idx & (vring->num - 1)] = idx;
    MemoryBarrier();
    vring->avail->idx = ++vq->master_vring_avail.idx;
    vq->num_added++;

    DbgPrint("virtqueue_add_buf: exit\n");

    return 0;
}

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
    void *opaque;
    u16 idx;

    DbgPrint("virtqueue_get_buf: entry\n");

    if (vq->last_used == (int)vq->vring.used->idx) {
        /* No descriptor index in the used ring */
        DbgPrint("### no buffers in the used ring\n");
        return NULL;
    }

    KeMemoryBarrier();

    idx = vq->last_used & (vq->vring.num - 1);
    *len = vq->vring.used->ring[idx].len;

    /* Get the first used descriptor */
    idx = (u16)vq->vring.used->ring[idx].id;
    opaque = vq->opaque[idx];

    /* Put all descriptors back to the free list */
    while (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT)
    {
        idx = put_free_desc(vq, idx);
        vq->num_free++;
    }
    put_free_desc(vq, idx);
    vq->num_free++;

    vq->last_used++;
    if (virtqueue_is_interrupt_enabled(vq)) {
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

bool virtqueue_kick_prepare(struct virtqueue *vq)
{
    KeMemoryBarrier();
    u16 old = (u16)(vq->master_vring_avail.idx - vq->num_added);
    u16 new = vq->master_vring_avail.idx;
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

bool virtqueue_enable_cb(struct virtqueue *vq)
{
    if (!virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        vq->vring.avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
    }

    vring_used_event(&vq->vring) = vq->last_used;
    MemoryBarrier();
    return (vq->last_used == vq->vring.used->idx);
}

bool virtqueue_enable_cb_delayed(struct virtqueue *vq)
{
    u16 bufs;

    if (!virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        vq->vring.avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
    }

    /* Note that this is an arbitrary threshold */
    bufs = (u16)(vq->master_vring_avail.idx - vq->last_used) * 3 / 4;
    vring_used_event(&vq->vring) = vq->last_used + bufs;
    MemoryBarrier();
    return ((vq->vring.used->idx - vq->last_used) <= bufs);
}

void virtqueue_disable_cb(struct virtqueue *vq)
{
    if (virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
        vq->vring.avail->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
    }
}

BOOLEAN virtqueue_is_interrupt_enabled(struct virtqueue *vq)
{
    return !(vq->master_vring_avail.flags & VIRTQ_AVAIL_F_NO_INTERRUPT);
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

    /* Build a linked list of free descriptors */
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
    u16 idx;
    void *opaque = NULL;

    for (idx = 0; idx < (u16)vq->vring.num; idx++) {
        opaque = vq->opaque[idx];
        if (opaque) {
            while (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT) {
                idx = put_free_desc(vq, idx);
                vq->num_free++;
            }
            put_free_desc(vq, idx);
            vq->num_free++;
            vq->vring.avail->idx = --vq->master_vring_avail.idx;
            break;
        }
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

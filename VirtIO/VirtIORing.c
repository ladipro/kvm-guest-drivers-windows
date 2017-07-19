#include "osdep.h"
#include "virtio_pci.h"
#include "virtio.h"
#include "kdebugprint.h"
#include "virtio_ring.h"

// TODO: This will be optimized of course
static int get_free_desc(struct vring *vring)
{
    unsigned int i;
    for (i = 0; i < vring->num; i++) {
        if (vring->desc[i].addr == 0) {
            return i;
        }
    }
    return -1;
}

int virtqueue_add_buf(struct virtqueue *vq,
struct scatterlist sg[],
    unsigned int out,
    unsigned int in,
    void *data,
    void *va_indirect,
    ULONGLONG phys_indirect)
{
    struct vring *vring = &vq->vring;
    unsigned int i;
    SSIZE_T idx = -1;

    DbgPrint("virtqueue_add_buf: entry\n");

    if (out + in > vq->num_free) {
        DbgPrint("virtqueue_add_buf: error\n");
        return -ENOSPC;
    }

    // fill out vring descriptors
    SSIZE_T prev_desc_idx = -1;
    for (i = 0; i < out + in; i++) {
        SSIZE_T desc_idx = get_free_desc(vring);
        ASSERT(desc_idx >= 0);

        vq->data[desc_idx] = data;

        if (prev_desc_idx >= 0) {
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

    vring->avail->ring[vring->avail->idx % vring->num] = (u16)idx;
    MemoryBarrier();
    DbgPrint("$$$ avail idx pa %I64x\n",
        vq->vdev->system->mem_get_physical_address(vq->vdev, &vring->avail->idx));
    vring->avail->idx = (vring->avail->idx + 1);// % vring->num;
    DbgPrint("$$$ idx %u\n", vring->avail->idx);

    DbgPrint("virtqueue_add_buf: exit\n");

    return 0;
}

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
    void *data;
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
    data = vq->data[idx];
    do {
        vq->vring.desc[idx].addr = 0;
        vq->data[idx] = NULL;
        if (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT) {
            idx = vq->vring.desc[idx].next;
        } else {
            idx = -1;
        }

        vq->num_free++;
    } while (idx >= 0);

    vq->last_used = vq->last_used + 1;
    DbgPrint("virtqueue_get_buf returning %p\n", data);

    return data;
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
    KeMemoryBarrier(); // ?
    return true;
}


void virtqueue_kick_always(struct virtqueue *vq)
{
    KeMemoryBarrier(); // ?
    virtqueue_notify(vq);
}

//static void detach_buf(struct vring_virtqueue *vq, unsigned int head)
//static inline bool more_used(const struct vring_virtqueue *vq)


bool virtqueue_enable_cb(struct virtqueue *_vq)
{
    // TODO
    return true;
}

bool virtqueue_enable_cb_delayed(struct virtqueue *_vq)
{
    // TODO
    return true;
}

void virtqueue_disable_cb(struct virtqueue *_vq)
{
    // TODO
    return;
}

BOOLEAN virtqueue_is_interrupt_enabled(struct virtqueue *_vq)
{
    return TRUE;
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
    memset(vq, 0, sizeof(*vq) + num * sizeof(void *));

    vring_init(&vq->vring, num, pages, vring_align);
    vq->vdev = vdev;
    vq->notification_cb = notify;
    vq->index = index;
    vq->last_used = 0;
    //vq->avail_flags_shadow = 0;
    //vq->avail_idx_shadow = 0;
    //vq->num_added = 0;

    /* Put everything in free lists. */
    vq->num_free = num;
/*
    vq//->free_head = 0;
    for (i = 0; i < num - 1; i++) {
        vq->vring.desc[i].next = i + 1;
        vq->data[i] = NULL;
    }
    vq->data[i] = NULL;
*/
    return vq;
}

void virtqueue_shutdown(struct virtqueue *vq)
{
    unsigned int num = vq->vring.num;
    void *pages = vq->vring.desc;
    unsigned int vring_align = vq->vdev->addr ? PAGE_SIZE : SMP_CACHE_BYTES;

    memset(pages, 0, vring_size(num, vring_align));
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
    unsigned int i;
    void *data = NULL;

    for (i = 0; i < vq->vring.num; i++) {
        data = vq->data[i];
        if (data) {
            vq->data[i] = NULL;
            break;
        }
    }
    return NULL;
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

void virtio_set_queue_event_suppression(struct virtqueue *_vq, bool enable)
{
    // TODO
}

u32 virtio_get_indirect_page_capacity()
{
    return PAGE_SIZE / sizeof(struct vring_desc);
}

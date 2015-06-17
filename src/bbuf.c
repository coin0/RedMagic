// buffer IOs of block devices

#include "common.h"
#include "device.h"
#include "heap.h"
#include "string.h"
#include "sched.h"
#include "klog.h"

#define B_BUSY   0x1		// buffer is locked by some process
#define B_VALID  0x2		// buffer has been read from disk
#define B_DIRTY  0x4		// buffer needs to be written to disk
#define B_UNUSED 0x8		// newly initialized buffer

// define this struct for the thread waiting on specific buffer
typedef struct {
	thread_t *threadp;
	list_head_t wq;
} bufq_thread_t;

static buf_cache_t *get_buffer(blk_dev_t * bdev, uint_t blkno);
static int release_buffer(buf_cache_t * buf);
static int wait_buffer(buf_cache_t * buf);
static int notify_buffer(buf_cache_t * buf);

int bdev_init_buffer_cache(blk_dev_t * bdev, size_t blks)
{
	buf_cache_t *buf;

	bdev->buf_num = blks;
	for (; blks > 0; blks--) {
		buf = (buf_cache_t *) kmalloc(sizeof(buf_cache_t));
		if (buf == NULL)
			return -1;

		buf->bdev = bdev;
		buf->blkno = 0;
		buf->flags = B_UNUSED;

		// link the buffer and zero IO & waiting list
		list_add_tail(&buf->list, &bdev->list);
		INIT_LIST_HEAD(&buf->io);
		INIT_LIST_HEAD(&buf->wq);

		bzero(buf->data, BLOCK_SIZE);
	}

	return OK;
}

size_t bdev_read_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data)
{
	buf_cache_t *buf;
	size_t size;

	buf = get_buffer(bdev, blkno);
	if (buf == NULL)
		PANIC("No buffers available");
	if (!(buf->flags & B_VALID))
		bdev->ops->read_block(buf, blkno);

	size = sizeof(buf->data);
	memcpy(data, buf->data, size);
	release_buffer(buf);

	return size;
}

size_t bdev_write_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data)
{
	// coin front
	return 0;
}

static buf_cache_t *get_buffer(blk_dev_t * bdev, uint_t blkno)
{
	buf_cache_t *buf;

      retry:

	mutex_lock(&bdev->lock);
	list_for_each_entry(buf, &bdev->list, list) {
		// skip newly initialized buffer
		if (buf->flags & B_UNUSED)
			continue;
		if (buf->bdev == bdev && buf->blkno == blkno) {
			if (!(buf->flags & B_BUSY) && list_empty(&buf->wq)) {
				buf->flags |= B_BUSY;
				mutex_unlock(&bdev->lock);
				return buf;
			} else {
				mutex_unlock(&bdev->lock);

				// will block, wait for the buf to be released
				wait_buffer(buf);
				goto retry;
			}
		}
	}
	mutex_unlock(&bdev->lock);

	// not found, then scan from tail to find a buffer that is available
	mutex_lock(&bdev->lock);
	list_for_each_entry_reverse(buf, &bdev->list, list) {
		if ((!(buf->flags & B_BUSY) && !(buf->flags & B_DIRTY))
		    || (buf->flags & B_UNUSED)) {
			buf->bdev = bdev;
			buf->blkno = blkno;
			buf->flags |= B_BUSY;
			buf->flags &= ~B_UNUSED;
			mutex_unlock(&bdev->lock);
			return buf;
		}
	}

	mutex_unlock(&bdev->lock);
	return NULL;
}

static int release_buffer(buf_cache_t * buf)
{
	blk_dev_t *bdev = buf->bdev;

	mutex_lock(&bdev->lock);
	list_move(&buf->list, &bdev->list);
	buf->flags &= ~B_BUSY;
	mutex_unlock(&bdev->lock);

	return notify_buffer(buf);
}

static int wait_buffer(buf_cache_t * buf)
{
	bufq_thread_t waiter;
	blk_dev_t *bdev = buf->bdev;

	mutex_lock(&bdev->lock);
	waiter.threadp = get_curr_thread();
	list_add_tail(&waiter.wq, &buf->wq);

	// this is a simple way but not graceful
	preempt_disable();
	make_sleep();
	mutex_unlock(&bdev->lock);
	preempt_enable();

	// now sleep
	schedule();

	return OK;
}

static int notify_buffer(buf_cache_t * buf)
{
	bufq_thread_t *waiter;
	blk_dev_t *bdev = buf->bdev;
	int rv;

	mutex_lock(&bdev->lock);
	if (!list_empty(&buf->wq)) {
		waiter = list_first_entry(&buf->wq, bufq_thread_t, wq);
		list_del(&waiter->wq);
		rv = wake_up(waiter->threadp);
	}
	mutex_unlock(&bdev->lock);

	return rv;
}

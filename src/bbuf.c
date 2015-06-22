/*
 *      Red Magic 1996 - 2015
 *
 *      bbuf.c - block buffer layer for the IOs of block devices
 *
 *      2015 Lin Coin - initial version
 */

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

#define MAX_BGET_RETRY 1

// define this struct for the thread waiting on specific buffer
typedef struct {
	thread_t *threadp;
	list_head_t wq;
} bufq_thread_t;

static buf_cache_t *get_buffer(blk_dev_t * bdev, uint_t blkno);
static int release_buffer(buf_cache_t * buf);
static int wait_buffer(buf_cache_t * buf);
static int notify_buffer(buf_cache_t * buf);
static int sync_buffer(blk_dev_t * bdev);
static void cache_buffer(buf_cache_t * buf);
static int update_buffer(buf_cache_t * buf);

static void queue_buffer(blk_dev_t * bdev, buf_cache_t * buf);
static void dequeue_buffer(blk_dev_t * bdev, buf_cache_t * buf);

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

int bdev_read_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data)
{
	buf_cache_t *buf;
	size_t size;
	int status = OK;

	buf = get_buffer(bdev, blkno);
	log_dbg(LOG_BLOCK "ReadBuf: dev 0x%08X, blkno 0x%08X\n", bdev, blkno);
	if (!(buf->flags & B_VALID)) {
		status = update_buffer(buf);
		if (status == OK)
			buf->flags |= B_VALID;
	}
	size = sizeof(buf->data);
	memcpy(data, buf->data, size);
	release_buffer(buf);

	return status;
}

int bdev_write_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data)
{
	buf_cache_t *buf;
	size_t size;
	int status = OK;

	buf = get_buffer(bdev, blkno);
	log_dbg(LOG_BLOCK "WriteBuf: dev 0x%08X, blkno 0x%08X\n", bdev, blkno);
	size = sizeof(buf->data);
	memcpy(buf->data, data, size);
	buf->flags |= B_DIRTY;

	cache_buffer(buf);
	release_buffer(buf);

	return status;
}

int bdev_sync_buffer(blk_dev_t * bdev)
{
	return sync_buffer(bdev);
}

static int update_buffer(buf_cache_t * buf)
{
	blk_dev_t *bdev = buf->bdev;
	int status = OK;

	mutex_lock(&bdev->lock);
	status = bdev->ops->read_block(buf);
	mutex_unlock(&bdev->lock);

	return status;
}

static void cache_buffer(buf_cache_t * buf)
{
	blk_dev_t *bdev = buf->bdev;

	mutex_lock(&bdev->lock);
	queue_buffer(bdev, buf);
	mutex_unlock(&bdev->lock);
}

// return >0 indicating num of blocks failed to flush
static int sync_buffer(blk_dev_t * bdev)
{
	buf_cache_t *buf, *tmp;
	int status, rv = 0;

	// find dirty blocks and flush to block device
	mutex_lock(&bdev->lock);
	list_for_each_entry_safe(buf, tmp, &bdev->io, io) {
		if (buf->flags & B_DIRTY) {
			status = bdev->ops->write_block(buf);
			if (status == OK) {
				dequeue_buffer(bdev, buf);
				buf->flags &= ~B_DIRTY;
			} else
				rv++;
		}
	}
	mutex_unlock(&bdev->lock);

	return rv;
}

// caller should hold device lock
static void queue_buffer(blk_dev_t * bdev, buf_cache_t * buf)
{
	buf_cache_t *tmp;

	// if buffer is already in queue, move, otherwise, add
	list_for_each_entry(tmp, &bdev->io, io) {
		if (tmp == buf) {
			list_move_tail(&buf->io, &bdev->io);
			return;
		}
	}
	list_add_tail(&buf->io, &bdev->io);
}

// caller should hold device lock
static void dequeue_buffer(blk_dev_t * bdev, buf_cache_t * buf)
{
	list_del(&buf->io);
}

static buf_cache_t *get_buffer(blk_dev_t * bdev, uint_t blkno)
{
	buf_cache_t *buf;
	int retry = 0;

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

	// not found, then scan from tail to find a buffer that is available
	list_for_each_entry_reverse(buf, &bdev->list, list) {
		if ((!(buf->flags & B_BUSY) && !(buf->flags & B_DIRTY))
		    || (buf->flags & B_UNUSED)) {
			buf->bdev = bdev;
			buf->blkno = blkno;

			// reset flag and set busy
			buf->flags = B_BUSY;
			mutex_unlock(&bdev->lock);
			return buf;
		}
	}

	mutex_unlock(&bdev->lock);

	// no available buffers, flush dirty buffers and try again
	if (retry++ < MAX_BGET_RETRY) {
		sync_buffer(bdev);
		goto retry;
	}
	PANIC("No buffers available");

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

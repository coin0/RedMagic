/*
 *      Red Magic 1996 - 2015
 *
 *      ide_pio.c - IDE driver for PIO mode
 *
 *      2015 Lin Coin - based on the IDE driver of xv6
 */

// Simple PIO-based (non-DMA) IDE driver code.

#include "common.h"
#include "debug.h"
#include "interrupt.h"
#include "device.h"

#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

//*/ static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;

static void ide_start(struct buf *);
static void ide_int_handler(registers_t * regs);

// Wait for IDE disk to become ready.
static int ide_wait(int checkerr)
{
	int r;

	while (((r = inb(0x1f7)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY) ;
	if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0)
		return -1;
	return 0;
}

int init_ide_master(dev_t * dev)
{
	return OK;
}

void ide_init(void)
{
	int i;

	//*/ initlock(&idelock, "ide");
	register_interrupt_handler(IRQ14, &ide_int_handler);
	//*/ ioapicenable(IRQ_IDE, ncpu - 1);
	ide_wait(0);

	// Check if disk 1 is present
	outb(0x1f6, 0xe0 | (1 << 4));
	for (i = 0; i < 1000; i++) {
		if (inb(0x1f7) != 0) {
			havedisk1 = 1;
			break;
		}
	}

	// Switch back to disk 0.
	outb(0x1f6, 0xe0 | (0 << 4));
}

// Start the request for b.  Caller must hold idelock.
static void ide_start(struct buf *b)
{
	if (b == 0)
		PANIC("ide_start");

	ide_wait(0);
	outb(0x3f6, 0);		// generate interrupt
	outb(0x1f2, 1);		// number of sectors
	outb(0x1f3, b->sector & 0xff);
	outb(0x1f4, (b->sector >> 8) & 0xff);
	outb(0x1f5, (b->sector >> 16) & 0xff);
	outb(0x1f6, 0xe0 | ((b->dev & 1) << 4) | ((b->sector >> 24) & 0x0f));
	if (b->flags & B_DIRTY) {
		outb(0x1f7, IDE_CMD_WRITE);
		outsl(0x1f0, b->data, 512 / 4);
	} else {
		outb(0x1f7, IDE_CMD_READ);
	}
}

// Interrupt handler.
static void ide_int_handler(registers_t * regs)
{
	struct buf *b;

	// First queued buffer is the active request.
	//*/ acquire(&idelock);
	if ((b = idequeue) == 0) {
		//*/ release(&idelock);
		// cprintf("spurious IDE interrupt\n");
		return;
	}
	idequeue = b->qnext;

	// Read data if needed.
	if (!(b->flags & B_DIRTY) && ide_wait(1) >= 0)
		insl(0x1f0, b->data, 512 / 4);

	// Wake process waiting for this buf.
	b->flags |= B_VALID;
	b->flags &= ~B_DIRTY;
	//*// wakeup(b);

	// Start disk on next buf in queue.
	if (idequeue != 0)
		ide_start(idequeue);

	//*/ release(&idelock);
}

//PAGEBREAK!
// Sync buf with disk. 
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void ide_rw(struct buf *b)
{
	struct buf **pp;

	if (!(b->flags & B_BUSY))
		PANIC("ide_rw: buf not busy");
	if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
		PANIC("ide_rw: nothing to do");
	if (b->dev != 0 && !havedisk1)
		PANIC("ide_rw: ide disk 1 not present");

	//*/ acquire(&idelock); //DOC:acquire-lock

	// Append b to idequeue.
	b->qnext = 0;
	for (pp = &idequeue; *pp; pp = &(*pp)->qnext)	//DOC:insert-queue
		;
	*pp = b;

	// Start disk if necessary.
	if (idequeue == b)
		ide_start(b);

	// Wait for request to finish.
	while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
		//*// sleep(b, &idelock);
	}

	//*/ release(&idelock);
}

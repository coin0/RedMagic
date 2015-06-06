#include "common.h"
#include "mp.h"
#include "paging.h"
#include "string.h"
#include "cpu.h"
#include "klog.h"

// global mp information
mp_t mpinfo;

/* migrated from xv6 */
static mpfp_t *__search_mpfp(uint_t pa, size_t len);
static int __zero_checksum(schar_t * startp, size_t len);
static mpfp_t *search_mpfp();
static mpconf_t *mpconfig(mpfp_t ** pmp);

/*
 *  calculate checksum for each bytes ranged from 'startp' to 'startp + len'
 *  and add up to 0
 */
static int __zero_checksum(schar_t * startp, size_t len)
{
	uint_t i;
	schar_t sum;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += startp[i];

	return sum;
}

/*
 *  find mp floating pointer structure
 */
static mpfp_t *__search_mpfp(uint_t pa, size_t len)
{
	addr_t va, cur;

	va = __phys_to_virt_lm(NULL, pa);
	for (cur = va; cur < va + len; cur += sizeof(mpfp_t)) {
		if (!memcmp((void *)cur, "_MP_", 4)) {
			printk("found signature _MP_ @0x%08X\n", (void *)cur);
			if (!__zero_checksum((schar_t *) cur, sizeof(mpfp_t)))
				return (mpfp_t *) cur;
			else
				printk("_MP_ checksum error !\n");
		}
	}
	return NULL;
}

/*
 * search for the MP Floating Pointer Structure
 * see Intel MP spec - 4. MP Configuration Table
 *
 * 1) in the first KB of the EBDA;
 * 2) in the last KB of system base memory;
 * 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
 *  
 * m from xv6
 */
static mpfp_t *search_mpfp()
{
	uchar_t *bda;
	addr_t p;
	mpfp_t *mp;

	bda = (uchar_t *) __phys_to_virt_lm(NULL, 0x400);
	if ((p = ((bda[0x0F] << 8) | bda[0x0E]) << 4)) {
		if ((mp = __search_mpfp(p, 1024)))
			return mp;
	} else {
		p = ((bda[0x14] << 8) | bda[0x13]) * 1024;
		if ((mp = __search_mpfp(p - 1024, 1024)))
			return mp;
	}

	return __search_mpfp(0xF0000, 0x10000);
}

static mpconf_t *mpconfig(mpfp_t ** pmp)
{
	mpconf_t *conf;
	mpfp_t *mp;

	if ((mp = search_mpfp()) == NULL || mp->physaddr == NULL)
		return NULL;

	conf = (mpconf_t *) __phys_to_virt_lm(NULL, (uint_t) mp->physaddr);
	if (memcmp(conf, "PCMP", 4) != 0)
		return NULL;
	if (conf->version != 1 && conf->version != 4)
		return NULL;
	if (__zero_checksum((schar_t *) conf, conf->length) != 0) {
		printk("MP configuration checksum error !\n");
		return NULL;
	}

	*pmp = mp;

	return conf;
}

int init_mp()
{
	uchar_t *p;
	uint_t ncpu = 0, ismp = 1;
	mpconf_t *conf;
	mpproc_t *proc;
	mpfp_t *mp;
	mpioapic_t *ioapic;

	if ((conf = mpconfig(&mp)) == NULL) {
		printk("could not find MP configuration\n");
		return 0;
	}

	lapic_regp = (uint_t *) conf->lapicaddr;
	for (p = (uchar_t *) (conf + 1); p < (uchar_t *) conf + conf->length;) {
		switch (*p) {
		case MPPROC:
			proc = (mpproc_t *) p;
			if (ncpu != proc->apicid) {
				printk(LOG_MP
				       "mpinit: ncpu=%d apicid=%d\n",
				       ncpu, proc->apicid);
				ismp = 0;
			}

			cpu_reset_state(&cpuset[ncpu]);
			if (proc->flags & MPBOOT) {
				cpu_set_val(&cpuset[ncpu], flag_bsp, 1);
				printk
				    ("found bootstrap processor #%d\n",
				     proc->apicid);
			} else
				printk
				    ("found application processor #%d\n",
				     proc->apicid);
			cpu_set_val(&cpuset[ncpu], proc_id, proc->apicid);

			// find next processor
			ncpu++;
			p += sizeof(mpproc_t);
			continue;
		case MPIOAPIC:
			ioapic = (mpioapic_t *) p;
			ioapic_id = ioapic->apicno;
			p += sizeof(mpioapic_t);
			continue;
		case MPBUS:
		case MPIOINTR:
		case MPLINTR:
			p += 8;
			continue;
		default:
			printk(LOG_MP "mpinit: unknown config type %x\n", *p);
			ismp = 0;
		}
	}

	if (!ismp) {
		// Didn't like what we found; fall back to no MP.
		mpinfo.ncpu = 1;
		mpinfo.ismp = 0;
		lapic_regp = NULL;
		ioapic_id = 0;
		return 0;
	}
	// Select IMCR,  Mask external interrupts
	if (mp->imcrp) {
		outb(0x22, 0x70);
		outb(0x23, inb(0x23) | 1);
	}

	if (ncpu > 1)
		mpinfo.ismp = 1;
	else
		mpinfo.ismp = 0;
	mpinfo.ncpu = ncpu;

	// only return AP num
	return ncpu - 1;
}

/*
 *  utilities to manage self or other processors
 */

void smp_halt_others()
{
	if (!mpinfo.ismp)
		return;

	lapic_send_ipi_mcast(IRQ_STOP_CPU);
}

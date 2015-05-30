#ifndef MPROC_H
#define MPROC_H

#include "common.h"

typedef struct mpfp {		// floating pointer
	uchar_t signature[4];	// "_MP_"
	void *physaddr;		// phys addr of MP config table
	uchar_t length;		// 1
	uchar_t specrev;	// [14]
	uchar_t checksum;	// all bytes must add up to 0
	uchar_t type;		// MP system config type
	uchar_t imcrp;
	uchar_t reserved[3];
} mpfp_t;

typedef struct mpconf {		// configuration table header
	uchar_t signature[4];	// "PCMP"
	ushort_t length;	// total table length
	uchar_t version;	// [14]
	uchar_t checksum;	// all bytes must add up to 0
	uchar_t product[20];	// product id
	uint_t *oemtable;	// OEM table pointer
	ushort_t oemlength;	// OEM table length
	ushort_t entry;		// entry count
	uint_t *lapicaddr;	// address of local APIC
	ushort_t xlength;	// extended table length
	uchar_t xchecksum;	// extended table checksum
	uchar_t reserved;
} mpconf_t;

typedef struct mpproc {		// processor table entry
	uchar_t type;		// entry type (0)
	uchar_t apicid;		// local APIC id
	uchar_t version;	// local APIC verison
	uchar_t flags;		// CPU flags
#define MPBOOT 0x02		// This proc is the bootstrap processor.
	uchar_t signature[4];	// CPU signature
	uint_t feature;		// feature flags from CPUID instruction
	uchar_t reserved[8];
} mpproc_t;

typedef struct mpioapic {	// I/O APIC table entry
	uchar_t type;		// entry type (2)
	uchar_t apicno;		// I/O APIC id
	uchar_t version;	// I/O APIC version
	uchar_t flags;		// I/O APIC flags
	uint_t *addr;		// I/O APIC address
} mpioapic_t;

// Table entry types
#define MPPROC    0x00		// One per processor
#define MPBUS     0x01		// One per bus
#define MPIOAPIC  0x02		// One per I/O APIC
#define MPIOINTR  0x03		// One per bus interrupt source
#define MPLINTR   0x04		// One per system interrupt source

typedef struct {
	int ismp;
	int ncpu;
} mp_t;

extern mp_t mpinfo;

#endif

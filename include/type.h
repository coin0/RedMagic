#ifndef TYPE_H
#define TYPE_H

#ifdef ARCH_X86_32
// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86 under gcc/m32.
typedef unsigned long long _u64;
typedef long long _s64;
typedef unsigned int _u32;
typedef int _s32;
typedef unsigned short _u16;
typedef short _s16;
typedef unsigned char _u8;
typedef signed char _s8;
typedef unsigned long ulong_t;

typedef _u32 size_t;
typedef _s32 ssize_t;
typedef _u64 ull_t;
typedef _s64 sll_t;
typedef _u32 uint_t;
typedef _u16 ushort_t;
typedef _u8 uchar_t;
typedef _s8 schar_t;

// platform irrelevant
typedef _u32 addr_t;
#endif

typedef struct {
	addr_t base;
	size_t length;
} addr_vec_t;

#define NULL ((void *)0)
#define OK 0

#endif

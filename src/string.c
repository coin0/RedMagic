/*
 *      Red Magic 1996 - 2015
 *
 *      string.c - handle memory & string in byte
 *
 *      2015 Lin Coin - initial version, some functions are referred to
 *                      xv6 and hurlex tutorial
 */

#include "common.h"

void memcpy(char *dest, const char *src, _u32 len)
{
	for (; len != 0; len--) {
		*dest++ = *src++;
	}
}

void memset(void *dest, char val, _u32 len)
{
	char *dst = (char *)dest;

	for (; len != 0; len--) {
		*dst++ = val;
	}
}

int memcmp(const void *v1, const void *v2, size_t n)
{
	const char *s1, *s2;

	s1 = v1;
	s2 = v2;
	while (n-- > 0) {
		if (*s1 != *s2)
			return *s1 - *s2;
		s1++, s2++;
	}

	return 0;
}

void *memmove(void *dst, const void *src, size_t n)
{
	const char *s;
	char *d;

	s = src;
	d = dst;
	if (s < d && s + n > d) {
		s += n;
		d += n;
		while (n-- > 0)
			*--d = *--s;
	} else
		while (n-- > 0)
			*d++ = *s++;

	return dst;
}

void bzero(void *dest, _u32 len)
{
	memset(dest, 0, len);
}

int strcmp(const char *str1, const char *str2)
{
	while (*str1 && *str2 && (*str1++ == *str2++)) ;

	if (*str1 == '\0' && *str2 == '\0') {
		return 0;
	}

	if (*str1 == '\0') {
		return -1;
	}

	return 1;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while (*src) {
		*dest++ = *src++;
	}

	*dest = '\0';

	return tmp;
}

char *strcat(char *dest, const char *src)
{
	char *cp = dest;

	while (*cp) {
		cp++;
	}

	while ((*cp++ = *src++)) ;

	return dest;
}

int strlen(const char *src)
{
	const char *eos = src;

	while (*eos++) ;

	return (eos - src - 1);
}

#define is_digit(c)     ((c) >= '0' && (c) <= '9')

int atoi(const char *str)
{
	const char *p;
	int sign = 1;
	int max, sum;

	for (p = str; *p == ' '; p++) ;
	// get max positive interger like 0100 0000 ... 0000
	max = ~(1 << (8 * sizeof(int) - 1));
	if (*p == '-')
		sign = -1;
	for (sum = 0; sum <= max && *p != '\0'; p++)
		if (is_digit(*p))
			sum = sum * 10 + (*p - '0');

	return (int)(sign * sum);
}

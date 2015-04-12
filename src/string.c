#include "common.h"

void memcpy(_u8 * dest, const _u8 * src, _u32 len)
{
	for (; len != 0; len--) {
		*dest++ = *src++;
	}
}

void memset(void *dest, _u8 val, _u32 len)
{
	_u8 *dst = (_u8 *) dest;

	for (; len != 0; len--) {
		*dst++ = val;
	}
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

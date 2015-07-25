#ifndef KEYVAL_H
#define KEYVAL_H

#include "common.h"

// key-value list type
typedef struct kv_list {
	char *key;
	char *value;
	struct kv_list *next;
} kv_list_t;

// key-value option type
typedef struct {
	uint_t opt_len;
	kv_list_t *opt_list;
} kv_option_t;

extern int kv_option_set(kv_option_t * options, char *key, char *value);
extern char *kv_option_get(kv_option_t * options, char *key);
extern int kv_option_unset(kv_option_t * options, char *key);
extern int kv_option_unset_all(kv_option_t * options);
extern int atoi(const char *str);

#endif

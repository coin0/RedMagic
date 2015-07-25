/*
 *      Red Magic 1996 - 2015
 *
 *      keyval.c - key-value model based utilities
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "keyval.h"
#include "klog.h"
#include "heap.h"
#include "string.h"

int kv_option_set(kv_option_t * options, char *key, char *value)
{
	kv_list_t *p, *new_opt;

	new_opt = kmalloc(sizeof(kv_list_t));
	if (new_opt == NULL)
		return -1;
	new_opt->key = key;
	new_opt->value = value;
	new_opt->next = NULL;

	if (options->opt_list != NULL) {
		p = options->opt_list;
		while (p->next != NULL)
			p = p->next;
		p->next = new_opt;
	} else {
		ASSERT(options->opt_len == 0);
		options->opt_list = new_opt;
	}
	options->opt_len++;

	return OK;
}

char *kv_option_get(kv_option_t * options, char *key)
{
	kv_list_t *p = options->opt_list;

	while (p != NULL) {
		if (strcmp(p->key, key) == 0)
			return p->value;
		p = p->next;
	}

	return NULL;
}

int kv_option_unset(kv_option_t * options, char *key)
{
	kv_list_t *prev, *p = options->opt_list;

	while (p != NULL) {
		prev = p;
		if (strcmp(p->key, key) == 0) {
			// handle option at head or in the middle of list
			if (p == options->opt_list)
				options->opt_list = p->next;
			else
				prev->next = p->next;
			kfree(p);
			return OK;
		}
		p = p->next;
	}

	return -1;
}

int kv_option_unset_all(kv_option_t * options)
{
	kv_list_t *prev, *p = options->opt_list;

	// delete from head, for safety, will protect the rest
	// of list from mem-leaking
	while (p != NULL) {
		prev = p;
		if (kfree(prev) != OK) {
			options->opt_list = prev;
			return -1;
		}
		p = p->next;
	}

	return OK;
}

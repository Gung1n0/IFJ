#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

typedef struct ht_table ht_table;

ht_table *ht_init(void);

bool ht_insert(ht_table *t, char *k, void *data);

void ht_dispose(ht_table *t);

bool ht_search(ht_table *t, char *k);

void *ht_get_data(ht_table *t, char *k);

#endif
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

typedef struct
{
    ht_data *arr;
    int capacity;
    int itemsStored;

} ht_table;

typedef struct
{
    char *key;
} ht_key;

typedef struct
{
    void *ptr;
} ht_data;

ht_table *ht_init();

bool ht_insert(ht_table *t, ht_key k, ht_data data);

void ht_dispose(ht_table *t);

bool ht_search(ht_table *t, ht_key k);

ht_data ht_getData(ht_table *t, ht_key k);

#endif
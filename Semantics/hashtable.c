#include <hashtable.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// generic prime number, with resonable size
#define HASHTABLE_SIZE 307

ht_table *ht_init()
{
    ht_table *t = calloc(1, sizeof(ht_table));

    if (t == NULL)
    {
        perror("Error: Couldn't allocate space for hash table");
        return NULL;
    }

    t->capacity = HASHTABLE_SIZE;
    t->arr = calloc(t->capacity, sizeof(ht_data));

    if (t->arr == NULL)
    {
        perror("Error: Couldn't allocate space for hash table");
        free(t);
        return NULL;
    }

    t->itemsStored = 0;
    return t;
}

void ht_dispose(ht_table *t)
{
    free(t->arr);
    free(t);
}
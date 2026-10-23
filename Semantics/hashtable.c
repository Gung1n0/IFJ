#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// generic prime number, with resonable size
#define HASHTABLE_SIZE 307
#define STEP 1
#define FILLED_RATIO 0.7

typedef struct
{
    char *key;
    void *value;
} ht_data;

struct ht_table
{
    ht_data *arr;
    int capacity;
    int items_stored;
};

ht_table *ht_init(void)
{
    ht_table *t = calloc(1, sizeof(ht_table));

    if (t == NULL)
    {
        perror("Error: Couldn't allocate space for hash table");
        return NULL;
    }

    t->capacity = HASHTABLE_SIZE;
    t->arr = (ht_data *)calloc(t->capacity, sizeof(ht_data));

    if (t->arr == NULL)
    {
        perror("Error: Couldn't allocate space for hash table");
        free(t);
        return NULL;
    }

    t->items_stored = 0;
    return t;
}

void ht_dispose(ht_table *t)
{
    if (t != NULL)
    {
        for (int i = 0; i < t->capacity; i++)
        {
            free(t->arr[i].key);
            // free(t->arr[i].value);
        }
        free(t->arr);
    }
    free(t);

    t = NULL;
}

int hashing_function(char *string)
{
    // basic hashing function, changes required after the presentation on the 27.10.2026
    int sum = 0;
    for (int i = 0; i < strlen(string); i++)
    {
        sum += string[i];
    }
    return sum % HASHTABLE_SIZE;
}

bool exceeded_capacity_limit(int capacity, int number_of_items)
{
    int max_items = (float)capacity * FILLED_RATIO;
    return max_items <= number_of_items;
}

bool ht_search(ht_table *t, char *k)
{
    if (t == NULL || k == NULL)
        return false;

    int hashed_key = hashing_function(k);

    for (int i = 0; i < t->capacity; i++)
    {
        int index = (hashed_key + i * STEP) % t->capacity;
        if (t->arr[index].key == NULL)
            return false;

        if (!strcmp(k, t->arr[index].key))
            return true;
    }
    return false;
}

bool ht_insert(ht_table *t, char *k, void *data)
{
    if (t == NULL || k == NULL)
        return false;

    int hashed_key = hashing_function(k);

    for (int i = 0; i < t->capacity; i++)
    {
        int index = (hashed_key + i * STEP) % t->capacity;

        if (t->arr[index].key == NULL)
        {
            if (exceeded_capacity_limit(t->capacity, t->items_stored))
                return false;

            // allocate space for key
            t->arr[index].key = calloc(strlen(k) + 1, sizeof(char));

            if (t->arr[index].key == NULL)
            {
                perror("Error: Couldn't allocate space for key");
                return false;
            }

            strcpy(t->arr[index].key, k);

            t->arr[index].value = data;
            t->items_stored++;

            return true;
        }

        if (!strcmp(t->arr[index].key, k))
        {
            t->arr[index].value = data;
            return true;
        }
    }
    return false;
}
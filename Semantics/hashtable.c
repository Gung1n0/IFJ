/*
 * implementation of a compiler for the language IFJ26
 * hashtable.c
 * implementation of the hash table adt, uses DJB for hashing
 * xzombot00 Tomáš Zomborský
 */

#include "hashtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// generic prime number, with resonable size
#define HASHTABLE_SIZE 307

// when a spot in the array of the table is already filled in, move by this
// number of indicies
#define STEP 1

// how filled does the array have to be before we refuse to accept more items
// (in this case 70%)
#define FILLED_RATIO 0.7

typedef struct {
  char *key;
  void *value;
} ht_data;

struct ht_table {
  ht_data *arr;
  int capacity;
  int items_stored;
};

/// @brief Inits ht_table data structures
ht_table *ht_init(void) {
  ht_table *t = calloc(1, sizeof(ht_table));

  if (t == NULL)
    return NULL;

  t->capacity = HASHTABLE_SIZE;
  t->arr = (ht_data *)calloc(t->capacity, sizeof(ht_data));

  if (t->arr == NULL) {
    free(t);
    return NULL;
  }

  t->items_stored = 0;
  return t;
}

/// @brief Frees the content of ht_table
void ht_dispose(ht_table *t) {
  if (t != NULL) {
    for (int i = 0; i < t->capacity; i++)
      free(t->arr[i].key);
    free(t->arr);
  }
  free(t);
}

/// @brief Mapping function that translates a string into a index
/// @param string String to map to index
/// @return Returns "unique" index for that specific string
static int mapping_function(const char *string) {
  // DJB mapping function
  unsigned long hash = 5381;
  int c;

  while ((c = *string++))
    hash = ((hash << 5) + hash) + c;

  return hash % HASHTABLE_SIZE;
}

/// @brief Checks, if number of items exceeds "artifical" capacity (in this case
/// 70% of the real capacity)
/// @param capacity Max capacity
/// @param number_of_items Number of items
/// @return Returns true if number of items is above "artificial" capacity, else
/// false
static bool exceeded_capacity_limit(int capacity, int number_of_items) {
  int max_items = (float)capacity * FILLED_RATIO;
  return max_items <= number_of_items;
}

/// @brief Checks if item exists in the table
bool ht_search(ht_table *t, const char *k) {
  if (t == NULL || k == NULL)
    return false;

  int hashed_key = mapping_function(k);

  for (int i = 0; i < t->capacity; i++) {
    int index = (hashed_key + i * STEP) % t->capacity;
    if (t->arr[index].key == NULL)
      return false;

    if (!strcmp(k, t->arr[index].key))
      return true;
  }
  return false;
}

/// @brief Attemps to insert item into the table
bool ht_insert(ht_table *t, const char *k, void *data) {
  if (t == NULL || k == NULL)
    return false;

  int hashed_key = mapping_function(k);

  for (int i = 0; i < t->capacity; i++) {
    int index = (hashed_key + i * STEP) % t->capacity;

    if (t->arr[index].key == NULL) {
      if (exceeded_capacity_limit(t->capacity, t->items_stored))
        return false;

      t->arr[index].key = calloc(strlen(k) + 1, sizeof(char));
      if (t->arr[index].key == NULL)
        return false;

      strcpy(t->arr[index].key, k);

      t->arr[index].value = data;
      t->items_stored++;

      return true;
    }

    if (!strcmp(t->arr[index].key, k)) {
      t->arr[index].value = data;
      return true;
    }
  }
  return false;
}

/// @brief Attemps to get data of item, given it's key
void *ht_get_data(ht_table *t, const char *k) {
  if (t == NULL || k == NULL)
    return NULL;

  if (!ht_search(t, k))
    return NULL;

  int hashed_key = mapping_function(k);

  for (int i = 0; i < t->capacity; i++) {
    int index = (hashed_key + i * STEP) % t->capacity;
    if (!strcmp(t->arr[index].key, k))
      return t->arr[index].value;
  }
  return NULL;
}
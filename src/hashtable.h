/*
 * implementation of a compiler for the language IFJ26
 * hashtable.h
 * xzombot00 Tomáš Zomborský
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

typedef struct ht_table ht_table;

/// @brief Inits ht_table data structures
/// @param (void) Nothing
/// @return Returns ht_table pointer, if allocation fails, returns NULL
ht_table *ht_init(void);

/// @brief Attemps to insert item into the table
/// @param t Table, to be insterted into
/// @param k Key of the item
/// @param data Data of the item
/// @return Returns true if item was successfully inserted, false if allocation
/// fails, capacity limit is exceeded or if some elements are NULL
bool ht_insert(ht_table *t, const char *k, void *data);

/// @brief Frees the content of ht_table
/// @param t Table to free
void ht_dispose(ht_table *t);

/// @brief Checks if item exists in the table
/// @param t Table, that holds
/// @param k String, which we are trying to find
/// @return Returns true, if item already exists, else false
bool ht_search(ht_table *t, const char *k);

/// @brief Attemps to get data of item, given it's key
/// @param t Table, where item resides
/// @param k Key of the value we want to get
/// @return Returns NULL if item doesn't exist, or some elements elements are
/// NULL, if successful returns item with key "k"
void *ht_get_data(ht_table *t, const char *k);

#endif
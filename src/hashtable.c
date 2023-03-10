#include "parameters.h"
#include "hashtable.h"
#include "macros.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define FNV_PRIME        0x00000100000001B3ULL
#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL

typedef struct hashtable_element {
    int key[VALUE_COUNT];
    int value;
    bool exists;
} hashtable_element;

typedef struct hashtable {
    size_t elementCount;
    size_t size;
    hashtable_element data[];
} hashtable;

static inline uint64_t hash_func(const char *data, size_t length);
static inline void hashtable_insert_no_resize(hashtable **tablePointer, const int *key, int value);

hashtable *hashtable_create(void) {
    hashtable *table = calloc(1, sizeof *table + HASHTABLE_BASE_SIZE*sizeof *table->data);
    CHECK_ALLOC(table, "hash table");

    table->size = HASHTABLE_BASE_SIZE;

    return table;
}

void hashtable_destroy(hashtable *table) {
    free(table);
}

// Check if the table needs to be extended and extend it if yes
static inline void hashtable_check_extend(hashtable **tablePointer) {
    hashtable *oldTable = *tablePointer;

    if (oldTable->elementCount * 100 / oldTable->size < HASHTABLE_MAX_LOAD_PERCENTAGE)
        return;

    size_t newSize = 2 * oldTable->size;

    hashtable *newTable = calloc(1, sizeof *newTable + newSize*sizeof *newTable->data);
    CHECK_ALLOC(newTable, "hash table");

    newTable->size = newSize;

    for (size_t i=0; i < oldTable->size; ++i) {
        hashtable_element element = oldTable->data[i];
        if (element.exists)
            hashtable_insert_no_resize(&newTable, element.key, element.value);
    }

    free(oldTable);

    *tablePointer = newTable;
}

// Update the value of the key if the value given is higher than its current one.
// Returns whether the value was updated.
bool hashtable_insert_if_higher(hashtable **tablePointer, const int *key, int value) {
    hashtable *table = *tablePointer;
    
    size_t index = hash_func((char *)key, VALUE_COUNT * sizeof *key) % table->size;
    hashtable_element *element;
    while ((element = table->data + index)->exists && memcmp(key, element->key, VALUE_COUNT * sizeof *key)) // Linear probing
        index = (index + 1) % table->size;

    if (element->exists && value <= element->value)
        return false;

    *element = (hashtable_element){
        .exists = true,
        .value = value
    };

    table->elementCount++;

    memcpy(element->key, key, VALUE_COUNT * sizeof *key);
    hashtable_check_extend(tablePointer);

    return true;
}

// Update the value at key without checking whether the table is big enough
// Used when the table resizes
static inline void hashtable_insert_no_resize(hashtable **tablePointer, const int *key, int value) {
    hashtable *table = *tablePointer;

    size_t index = hash_func((char *)key, VALUE_COUNT * sizeof *key) % table->size;
    hashtable_element *element;
    while ((element = table->data + index)->exists && memcmp(key, element->key, VALUE_COUNT * sizeof *key)) // Linear probing
        index = (index + 1) % table->size;

    *element = (hashtable_element){
        .exists = true,
        .value = value
    };

    memcpy(element->key, key, VALUE_COUNT * sizeof *key);
    table->elementCount++;
}

// FNV-1a hash
static inline uint64_t hash_func(const char *data, size_t length) {
    uint64_t hash = FNV_OFFSET_BASIS;

    for (size_t i=0; i < length; ++i) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

#include "hash_map.h"

#include <malloc.h>
#include <memory.h>

// a 32 bit prime number
#define MAGIC_PRIME 2748002342
#define MIN_CAPACITY    32

bool hash_map_init(struct hash_map* hash_map, int capacity) {
    if (capacity < MIN_CAPACITY) {
        capacity = MIN_CAPACITY;
    }

    capacity *= 2;

    hash_map->entries = malloc(sizeof(struct hash_map_entry) * capacity);

    if (!hash_map->entries) {
        return false;
    }

    hash_map->capacity = capacity;
    hash_map->count = 0;

    memset(hash_map->entries, 0, sizeof(struct hash_map_entry) * capacity);

    return true;
}

void hash_map_destroy(struct hash_map* hash_map) {
    free(hash_map->entries);
    hash_map->entries = NULL;
    hash_map->capacity = 0;
    hash_map->count = 0;
}

struct hash_map_entry* hash_map_find_entry(struct hash_map_entry* entries, int capacity, int key) {
    int mask = capacity - 1;

    int index = (key * MAGIC_PRIME) & mask;

    for (int i = 0; i < capacity; i += 1) {
        struct hash_map_entry* entry = &entries[index];

        if (entry->key == key || entry->key == 0) {
            return entry;
        }

        index = (index + 1) & mask;
    }
    
    return NULL;
}

bool hash_map_resize(struct hash_map* hash_map) {
    int new_capacity = hash_map->capacity * 2;
    struct hash_map_entry* new_entries = malloc(sizeof(struct hash_map_entry) * new_capacity);

    if (!new_entries) {
        return false;
    }

    memset(new_entries, 0, sizeof(struct hash_map_entry) * new_capacity);

    for (int i = 0; i < hash_map->capacity; i += 1) {
        struct hash_map_entry* prev_entry = &hash_map->entries[i];

        struct hash_map_entry* new_entry = hash_map_find_entry(new_entries, new_capacity, prev_entry->key);
        new_entry->key = prev_entry->key;
        new_entry->value = prev_entry->value;
    }

    free(hash_map->entries);
    hash_map->entries = new_entries;
    hash_map->capacity = new_capacity;

    return true;
}

void* hash_map_get(struct hash_map* hash_map, int key) {
    struct hash_map_entry* result = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);

    if (result) {
        return result->value;
    }

    return NULL;
}

bool hash_map_set(struct hash_map* hash_map, int key, void* value) {
    struct hash_map_entry* result = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);

    // check if the hash map should be grown
    if (result->key != key && (hash_map->count << 1) >= hash_map->capacity) {
        if (!hash_map_resize(hash_map)) {
            return false;
        }

        result = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);
    }

    if (result->key == key && result->value) {
        result->value = value;
    } else {
        result->key = key;
        result->value = value;
        hash_map->count += 1;
    }

    return true;
}

void hash_map_delete(struct hash_map* hash_map, int key) {
    struct hash_map_entry* entry = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);

    if (entry && entry->key == key) {
        hash_map->count -= 1;
        entry->key = 0;
        entry->value = 0;
    }
}

struct hash_map_entry* hash_map_next(struct hash_map* hash_map, struct hash_map_entry* curr) {
    if (!hash_map->entries || !hash_map->count) {
        return NULL;
    }

    if (!curr) {
        curr = hash_map->entries - 1;
    }

    struct hash_map_entry* end = hash_map->entries + hash_map->capacity;

    do {
        ++curr;

        if (curr == end) {
            return NULL;
        }

        if (curr->value) {
            return curr;
        }
    } while (curr < end);

    return NULL;
}
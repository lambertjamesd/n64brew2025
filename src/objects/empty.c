#include "empty.h"

#include "../util/hash_map.h"

static hash_map_t active_empties;

void empty_init(struct empty* empty, struct empty_definition* definition, entity_id entity_id) {
    empty->position = definition->position;
    empty->rotation = definition->rotation;
    hash_map_set(&active_empties, entity_id, empty);
}

void empty_destroy(struct empty* empty, struct empty_definition* definition) {
    hash_map_delete(&active_empties, empty->entity_id);
}

void empty_common_init() {
    hash_map_init(&active_empties, 8);
}

void empty_common_destroy() {
    hash_map_destroy(&active_empties);
}

empty_t* empty_find(entity_id entity_id) {
    return hash_map_get(&active_empties, entity_id);
}
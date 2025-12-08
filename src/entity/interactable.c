#include "interactable.h"

#include <stddef.h>
#include "../util/hash_map.h"

static struct hash_map interactable_entity_mapping;

static float interaction_range[] = {
    [INTERACT_TYPE_NONE] = 0.0f,
    [INTERACT_TYPE_TALK] = 2.0f,
    [INTERACT_TYPE_READ] = 2.0f,
    [INTERACT_TYPE_PICKUP] = 0.85f,
    [INTERACT_TYPE_DROP] = 0.0f,
    [INTERACT_TYPE_OPEN] = 0.85f,
    [INTERACT_TYPE_CAST] = 0.0f,
};

static const char* interaction_names[] = {
    [INTERACT_TYPE_NONE] = NULL,
    [INTERACT_TYPE_TALK] = "Talk",
    [INTERACT_TYPE_READ] = "Read",
    [INTERACT_TYPE_PICKUP] = "Lift",
    [INTERACT_TYPE_DROP] = "Drop",
    [INTERACT_TYPE_OPEN] = "Open",
    [INTERACT_TYPE_CAST] = "Cast",
};

void interactable_reset() {
    hash_map_destroy(&interactable_entity_mapping);
    hash_map_init(&interactable_entity_mapping, 32);
}

void interactable_init(struct interactable* interactable, entity_id id, interact_type_t interact_type, interaction_callback callback, void* data) {
    interactable->id = id;
    interactable->interact_type = interact_type;
    interactable->callback = callback;
    interactable->data = data;
    hash_map_set(&interactable_entity_mapping, id, interactable);
    
    interactable->flags.target_straight_on = false;
}

void interactable_destroy(struct interactable* interactable) {
    hash_map_delete(&interactable_entity_mapping, interactable->id);
}

bool interactable_is_in_range(struct interactable* interactable, float distance_sqrd) {
    return distance_sqrd < interaction_range[interactable->interact_type] * interaction_range[interactable->interact_type];
}

struct interactable* interactable_get(entity_id id) {
    return hash_map_get(&interactable_entity_mapping, id);
}

const char* interact_type_to_name(interact_type_t type) {
    return interaction_names[type];
}
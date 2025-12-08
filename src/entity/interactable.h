#ifndef __ENTITY_INTERACTABLE_H__
#define __ENTITY_INTERACTABLE_H__

#include "entity_id.h"
#include <stdint.h>
#include <stdbool.h>

enum __attribute__ ((__packed__)) interact_type {
    INTERACT_TYPE_NONE,
    INTERACT_TYPE_TALK,
    INTERACT_TYPE_READ,
    INTERACT_TYPE_PICKUP,
    INTERACT_TYPE_DROP,
    INTERACT_TYPE_OPEN,
    INTERACT_TYPE_CAST,
};

#define MAX_INTERACT_RANGE  2.0f

typedef enum interact_type interact_type_t;

struct interactable;

typedef void (*interaction_callback)(struct interactable* interactable, entity_id from);

struct interactable_flags {
    uint16_t target_straight_on: 1;
};

struct interactable {
    entity_id id;
    interact_type_t interact_type;
    struct interactable_flags flags;
    interaction_callback callback;
    void* data;
};

typedef struct interactable interactable_t;

void interactable_reset();

void interactable_init(interactable_t* interactable, entity_id id, interact_type_t interact_type, interaction_callback callback, void* data);
void interactable_destroy(interactable_t* interactable);
bool interactable_is_in_range(interactable_t* interactable, float distance_sqrd);

static inline void interactable_set_type(interactable_t* interactable, interact_type_t type) {
    interactable->interact_type = type;
}
static inline interact_type_t interactable_get_type(interactable_t* interactable) {
    return interactable->interact_type;
}

interactable_t* interactable_get(entity_id id);

const char* interact_type_to_name(interact_type_t type);

#endif
#ifndef __ENTITY_DAMAGE_H__
#define __ENTITY_DAMAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include "../math/vector3.h"
#include "./entity_id.h"

enum damage_type {
    DAMAGE_TYPE_PROJECTILE = (1 << 0),
    DAMAGE_TYPE_FIRE = (1 << 1),
    DAMAGE_TYPE_ICE = (1 << 2),
    DAMAGE_TYPE_LIGHTING = (1 << 3),
    DAMAGE_TYPE_WATER = (1 << 4),
    DAMAGE_TYPE_BASH = (1 << 5),
    DAMAGE_TYPE_STEAL = (1 << 6),
    DAMAGE_TYPE_KNOCKBACK = (1 << 7),
};

struct damage_source {
    float amount;
    enum damage_type type;
    float knockback_strength;
};

typedef struct damage_source damage_source_t;

struct damage_info {
    float amount;
    enum damage_type type;
    entity_id source;
    float knockback_strength;
    // may not be normalized
    struct Vector3 direction;
};

typedef struct damage_info damage_info_t;

#define MAX_DAMAGED_SET_SIZE    8

struct damaged_set {
    uint8_t damaged_entities[MAX_DAMAGED_SET_SIZE];
};

void damaged_set_reset(struct damaged_set* set);
bool damaged_set_check(struct damaged_set* set, entity_id id);

#endif
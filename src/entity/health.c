#include "health.h"

#include "../math/mathf.h"
#include "../util/hash_map.h"
#include "../time/time.h"
#include "../collision/dynamic_object.h"
#include "../collision/collision_scene.h"
#include <stddef.h>

static struct hash_map health_entity_mapping;

void health_reset() {
    hash_map_destroy(&health_entity_mapping);
    hash_map_init(&health_entity_mapping, 32);
}

void health_update(void *data) {
    struct health* health = (struct health*)data;

    if (health->status_timer) {
        health->status_timer -= fixed_time_step;

        if (health->status_timer < 0.0f) {
            health->status_timer = 0.0f;
        }
    }
}

void health_init(struct health* health, entity_id id, float max_health) {
    health->entity_id = id;
    health->max_health = max_health;
    health->current_health = max_health;
    health->current_status = 0;
    health->status_timer = 0.0f;

    hash_map_set(&health_entity_mapping, id, health);

    update_add(health, health_update, 0, UPDATE_LAYER_WORLD);

    health->callback = NULL;
    health->callback_data = NULL;
    health->health_shield = NULL;
}

void health_set_callback(struct health* health, health_damage_callback callback, void* data) {
    health->callback = callback;
    health->callback_data = data;
}

void health_destroy(struct health* health) {
    hash_map_delete(&health_entity_mapping, health->entity_id);
    update_remove(health);
}

float health_damage(struct health* health, struct damage_info* damage) {
    if (health->health_shield && health_shield_does_block(health->health_shield, damage)) {
        return 0.0f;
    }

    float amount = damage->amount;

    if (health->callback) {
        amount = health->callback(health->callback_data, damage);

        if (amount <= 0.0f) {
            return 0.0f;
        }
    }

    float result = 0.0f;

    if (health->max_health) {
        if (health->current_health >= amount) {
            result = amount;
            health->current_health -= amount;
        } else {
            result = health->current_health;
            health->current_health = 0.0f;
        }
    }

    if (damage->type & DAMAGE_TYPE_FIRE) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_FIRE;
    } else if (damage->type & DAMAGE_TYPE_ICE) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_ICE;
    } else if (damage->type & DAMAGE_TYPE_LIGHTING) {
        health->status_timer = 3;
        health->current_status = DAMAGE_TYPE_LIGHTING;
    } else if (damage->type & DAMAGE_TYPE_WATER) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_WATER;
    }

    if (damage->type & DAMAGE_TYPE_KNOCKBACK) {
        struct dynamic_object* object = collision_scene_find_object(health->entity_id);

        if (object) {
            struct Vector3 direction = damage->direction;
            direction.y = 0.0f;
            vector3Normalize(&direction, &direction);
            vector3Scale(&direction, &direction, -damage->knockback_strength);
            direction.y = damage->knockback_strength;
            DYNAMIC_OBJECT_MARK_JUMPING(object);

            vector3Add(&object->velocity, &direction, &object->velocity);
        }
    }

    return result;
}

void health_damage_all(struct health* health) {
    health->current_health = 0;
}

float health_damage_id(entity_id target, struct damage_info* damage, struct damaged_set* set) {
    struct health* health = health_get(target);

    if (!health) {
        return 0.0f;
    }

    if (!damaged_set_check(set, target)) {
        return 0.0f;
    }

    return health_damage(health, damage);
}

void health_heal(struct health* health, float amount) {
    health->current_health = minf(health->max_health, health->current_health + amount);
}

bool health_apply_contact_damage(contact_t* first_contact, struct damage_source* source, struct damaged_set* set) {
    struct contact* curr = first_contact;

    struct damage_info damage;
    damage.amount = source->amount;
    damage.type = source->type;
    damage.knockback_strength = source->knockback_strength;

    bool did_hit = false;

    while (curr) {
        struct health* target_health = health_get(curr->other_object);

        if (!target_health || !damaged_set_check(set, curr->other_object)) {
            curr = curr->next;
            continue;
        }

        damage.source = curr->other_object;
        damage.direction = curr->normal;

        health_damage(target_health, &damage);
        did_hit = true;
        curr = curr->next;
    }

    return did_hit;
}

struct health* health_get(entity_id id) {
    return hash_map_get(&health_entity_mapping, id);
}

bool health_is_burning(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_FIRE;
}

bool health_is_frozen(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_ICE;
}

bool health_is_shocked(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_LIGHTING;
}

bool health_has_status(struct health* health, enum damage_type damage_type) {
    return health->status_timer > 0.0f && (health->current_status & damage_type) != 0;
}

bool health_is_alive(struct health* health) {
    return health->current_health > 0.0f;
}

void health_clear_status(struct health* health) {
    health->current_health = 0.0f;
    health->current_status = 0;
}

void health_add_shield(struct health* health, struct health_shield* health_shield) {
    health->health_shield = health_shield;
}

void health_remove_shield(struct health* health, struct health_shield* health_shield) {
    if (health->health_shield == health_shield) {
        health->health_shield = NULL;
    }
}
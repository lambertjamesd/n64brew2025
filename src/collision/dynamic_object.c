#include "dynamic_object.h"

#include "../time/time.h"
#include "../math/minmax.h"
#include <math.h>
#include <stddef.h>
#include <libdragon.h>

void dynamic_object_init(
    entity_id entity_id,
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    uint16_t collision_layers,
    struct Vector3* position, 
    struct Vector2* rotation
) {
    object->entity_id = entity_id;
    object->type = type;
    object->position = position;
    object->rotation = rotation;
    object->pitch = 0;
    object->velocity = gZeroVec;
    object->center = gZeroVec;
    object->time_scalar = 1.0f;
    object->has_gravity = 1;
    object->trigger_type = TRIGGER_TYPE_NONE;
    object->is_fixed = 0;
    object->is_pushed = 0;
    object->is_jumping = 0;
    object->under_water = 0;
    object->density_class = DYNAMIC_DENSITY_LIGHT;
    object->weight_class = WEIGHT_CLASS_LIGHT;
    object->has_ice_dash = 0;
    object->collision_layers = collision_layers;
    object->collision_group = 0;
    object->active_contacts = NULL;
    object->shadow_contact = NULL;
    object->scale = 1.0f;
    dynamic_object_recalc_bb(object);
}

void dynamic_object_set_type(struct dynamic_object* object, struct dynamic_object_type* type) {
    object->type = type;
    dynamic_object_recalc_bb(object);
}

void dynamic_object_update(struct dynamic_object* object) {
    if (object->trigger_type != TRIGGER_TYPE_NONE || object->is_fixed) {
        return;
    }

    if (object->has_gravity) {
        object->velocity.y += fixed_time_step * object->time_scalar * GRAVITY_CONSTANT;
    }

    vector3AddScaled(object->position, &object->velocity, fixed_time_step * object->time_scalar, object->position);
}

struct contact* dynamic_object_nearest_contact(struct contact* first_contact, struct Vector3* position) {
    struct contact* nearest_target = NULL;
    struct contact* current = first_contact;
    float distance = 0.0f;

    while (current) {
        float check = vector3DistSqrd(&current->point, position);
        if (!nearest_target || check < distance) {
            distance = check;
            nearest_target = current;
        }

        current = current->next;
    }

    return nearest_target;
}

struct contact* dynamic_object_find_contact(struct dynamic_object* object, entity_id id) {
    struct contact* current = object->active_contacts;

    while (current) {
        if (current->other_object == id) {
            return current;
        }
            
        current = current->next;
    }

    return NULL;
}

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct dynamic_object* object = (struct dynamic_object*)data;

    struct Vector3 rotated_dir;

    if (!object->rotation && !object->pitch) {
        object->type->minkowsi_sum(&object->type->data, direction, output);
        vector3Add(output, object->position, output);
        vector3Add(output, &object->center, output);
        return;
    }

    struct Vector3 pitched_dir;

    if (object->pitch) {
        pitched_dir.x = direction->x;
        pitched_dir.y = direction->y * object->pitch->x - direction->z * object->pitch->y;
        pitched_dir.z = direction->z * object->pitch->x + direction->y * object->pitch->y;
    } else {
        pitched_dir = *direction;
    }

    struct Vector2 inv_rotation;
    vector2ComplexConj(object->rotation, &inv_rotation);
    vector3RotateWith2(&pitched_dir, &inv_rotation, &rotated_dir);
    struct Vector3 unrotated_out;
    
    object->type->minkowsi_sum(&object->type->data, &rotated_dir, &unrotated_out);
    vector3Add(&unrotated_out, &object->center, &unrotated_out);

    struct Vector3 unpitched_out;
    vector3RotateWith2(&unrotated_out, object->rotation, &unpitched_out);

    if (object->pitch) {
        output->x = unpitched_out.x;
        output->y = unpitched_out.y * object->pitch->x + unpitched_out.z * object->pitch->y;
        output->z = unpitched_out.z * object->pitch->x - unpitched_out.y * object->pitch->y;
    } else {
        *output = unpitched_out;
    }

    if (object->scale != 1.0f) {
        vector3Scale(output, output, object->scale);
    }

    vector3Add(output, object->position, output);
}

void dynamic_object_recalc_bb(struct dynamic_object* object) {
    object->type->bounding_box(&object->type->data, object->rotation, &object->bounding_box);
    struct Vector3 offset;
    if (object->scale != 1.0f) {
        vector3Scale(&object->bounding_box.min, &object->bounding_box.min, object->scale);
        vector3Scale(&object->bounding_box.max, &object->bounding_box.max, object->scale);
        vector3Scale(&object->center, &offset, object->scale);
    } else {
        offset = object->center;
    }

    struct Vector3 rotatedOffset;

    if (object->rotation) {
        rotatedOffset.x = offset.x * object->rotation->x - offset.z * object->rotation->y;
        rotatedOffset.y = offset.y;
        rotatedOffset.z = offset.z * object->rotation->x + offset.x * object->rotation->y;
    } else {
        rotatedOffset = offset;
    }

    vector3Add(&rotatedOffset, object->position, &rotatedOffset);

    vector3Add(&object->bounding_box.min, &rotatedOffset, &object->bounding_box.min);
    vector3Add(&object->bounding_box.max, &rotatedOffset, &object->bounding_box.max);
}

bool dynamic_object_should_slide(float max_stable_slope, float normal_y, enum surface_type surface_type) {
    return (surface_type != SURFACE_TYPE_STICKY && normal_y <= 1.0f - max_stable_slope) || fabsf(normal_y) < 0.001f;
}

bool dynamic_object_is_grounded(struct dynamic_object* object) {
    struct contact* curr = object->active_contacts;

    while (curr) {
        if (curr->normal.y > 0.3f) {
            return true;
        }
        
        curr = curr->next;
    }

    return false;
}

#define SHADOW_AS_GROUND_TOLERNACE  0.15f

int8_t surface_ground_priorities[SURFACE_TYPE_COUNT] = {
    [SURFACE_TYPE_STICKY] = 1,
};

struct contact* dynamic_object_get_ground(struct dynamic_object* object) {
    struct contact* contact = object->active_contacts;

    struct contact* result = NULL;
    int ground_priority = 0;

    while (contact) {
        int priority = surface_ground_priorities[(int)contact->surface_type];

        if (contact->normal.y > 0.001f && (
            !result || 
            priority > ground_priority || 
            (priority == ground_priority && contact->normal.y > result->normal.y)
        )) {
            result = contact;
            ground_priority = priority;
        }

        contact = contact->next;
    }

    if (!result && object->shadow_contact && object->shadow_contact->point.y + SHADOW_AS_GROUND_TOLERNACE > object->bounding_box.min.y) {
        return object->shadow_contact;
    }

    return result;
}

void dynamic_object_set_scale(struct dynamic_object* object, float scale) {
    object->scale = scale;
    dynamic_object_recalc_bb(object);
}
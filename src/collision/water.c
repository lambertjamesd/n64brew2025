#include "water.h"

#include <stddef.h>
#include "../time/time.h"
#include "../math/mathf.h"
#include "../collision/collide.h"

#define MAX_MOVE_AMOUNT     10.0f

static float density_level[] = {
    [DYNAMIC_DENSITY_LIGHT] = GRAVITY_CONSTANT / 0.3f,
    [DYNAMIC_DENSITY_MEDIUM] = GRAVITY_CONSTANT / 0.6f,
    [DYNAMIC_DENSITY_NEUTRAL] = GRAVITY_CONSTANT / 1.0f,
    [DYNAMIC_DENSITY_HEAVY] = GRAVITY_CONSTANT / 1.4f,
};

void water_apply(struct dynamic_object* object) {
    if (!object->shadow_contact || object->shadow_contact->surface_type != SURFACE_TYPE_WATER) {
        return;
    }

    float water_top = object->shadow_contact->point.y;

    if (object->bounding_box.min.y >= water_top) {
        return;
    }

    if (object->has_ice_dash) {
        struct contact* other = object->shadow_contact;
        object->shadow_contact = NULL;

        other->next = object->active_contacts;
        other->normal = gUp;
        other->other_object = 0;
        other->point = *object->position;
        other->point.y = water_top;
        other->surface_type = SURFACE_TYPE_WATER;

        object->active_contacts = other;

        float target_pos = water_top + object->position->y - object->bounding_box.min.y;

        object->position->y = mathfMoveTowards(object->position->y, target_pos, MAX_MOVE_AMOUNT * fixed_time_step);
        correct_velocity(&object->velocity, &gUp, -1.0f, 0.01f, 0.0f);

        return;   
    }

    float underwater_ratio = object->bounding_box.max.y <= water_top ? 
        1.0f : 
        (water_top - object->bounding_box.min.y) / (object->bounding_box.max.y - object->bounding_box.min.y);

    vector3Scale(&object->velocity, &object->velocity, mathfLerp(1.0f, 0.9f, underwater_ratio));
    object->velocity.y -= underwater_ratio * density_level[object->density_class] * fixed_time_step;

    if (underwater_ratio > 0.5f) {
        DYNAMIC_OBJECT_MARK_UNDER_WATER(object);
    }
}
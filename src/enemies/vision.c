#include "vision.h"

#include <stddef.h>
#include "../collision/collision_scene.h"

struct dynamic_object* vision_update_current_target(entity_id* current_target_ptr, struct spatial_trigger* trigger, float sight_range, struct Vector3* offset) {
    entity_id current_target = *current_target_ptr;

    if (!current_target) {
        struct contact* nearest_contact = dynamic_object_nearest_contact(trigger->active_contacts, &trigger->transform->position);

        if (!nearest_contact || !nearest_contact->other_object) {
            return NULL;
        }

        current_target = nearest_contact->other_object;
        *current_target_ptr = nearest_contact->other_object;
    }

    struct dynamic_object* target_object = collision_scene_find_object(current_target);

    if (!target_object) {
        *current_target_ptr = 0;
        return NULL;
    }

    vector3Sub(target_object->position, &trigger->transform->position, offset);

    if (vector3MagSqrd2D(offset) > sight_range * sight_range) {
        *current_target_ptr = 0;
        return NULL;
    }

    return target_object;
}
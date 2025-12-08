#include "collide.h"

#include "epa.h"

#include "collision_scene.h"
#include "../util/flags.h"
#include <stdio.h>
#include <libdragon.h>

int surface_type_collision_layers[] = {
    COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
    COLLISION_LAYER_TANGIBLE,
    0,
    COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
};

bool correct_velocity(struct Vector3* velocity, struct Vector3* normal, float ratio, float friction, float bounce) {
    float velocityDot = vector3Dot(velocity, normal);

    
    if ((velocityDot < 0) == (ratio < 0)) {
        struct Vector3 tangentVelocity;
        
        vector3AddScaled(velocity, normal, -velocityDot, &tangentVelocity);
        vector3Scale(&tangentVelocity, &tangentVelocity, 1.0f - friction * fabsf(normal->y));
        vector3AddScaled(&tangentVelocity, normal, velocityDot * -bounce, velocity);
        return true;
    }
    return false;
}

void correct_overlap_no_slide(
    struct dynamic_object* object, 
    float penetration, 
    float slope_amount, 
    float friction, 
    float bounce
) {
    float offset = penetration / slope_amount;
    object->position->y -= offset;

    if (object->velocity.y < 0.0f) {
        object->velocity.y = -object->velocity.y * object->type->bounce;
    }

    if (object->type->friction) {
        float scalar = 1.0f - object->type->friction;
        object->velocity.x *= scalar;
        object->velocity.z *= scalar;
    }
}

void correct_overlap(struct dynamic_object* object, struct EpaResult* result, float ratio, float friction, float bounce, enum surface_type surface_type) {
    if (object->is_fixed) {
        return;
    }

    float slope_amount = ratio > 0.0f ? -result->normal.y : result->normal.y;

    if (dynamic_object_should_slide(object->type->max_stable_slope, slope_amount, surface_type)) {
        vector3AddScaled(object->position, &result->normal, result->penetration * ratio, object->position);
        correct_velocity(&object->velocity, &result->normal, ratio, friction, bounce);
    } else {
        correct_overlap_no_slide(
            object,
            result->penetration,
            slope_amount,
            friction,
            bounce
        );
    }

}

struct object_mesh_collide_data {
    struct mesh_collider* mesh;
    struct dynamic_object* object;
    struct mesh_triangle triangle;
};

bool collide_object_check_triangle_bounding_box(struct object_mesh_collide_data* data) {
    struct Vector3* points[3];

    for (int i = 0; i < 3; i += 1) {
        points[i] = &data->triangle.vertices[data->triangle.triangle.indices[i]];
    }

    float* min = VECTOR3_AS_ARRAY(&data->object->bounding_box.min);
    float* max = VECTOR3_AS_ARRAY(&data->object->bounding_box.max);

    for (int axis = 0; axis < 3; axis += 1) {
        bool is_below = true;
        bool is_above = true;

        for (int point_index = 0; point_index < 3; point_index += 1) {
            float pos = VECTOR3_AS_ARRAY(points[point_index])[axis]; 
            if (*max > pos) {
                is_below = false;
            }
            if (*min < pos) {
                is_above = false;
            }
        }

        if (is_below || is_above) {
            return false;
        }

        min += 1;
        max += 1;
    }

    return true;
}

bool collide_object_to_triangle(void* data, int triangle_index, int collision_layers) {
    struct object_mesh_collide_data* collide_data = (struct object_mesh_collide_data*)data;

    collide_data->triangle.triangle = collide_data->mesh->index.indices[triangle_index];
    
    if (!(surface_type_collision_layers[collide_data->triangle.triangle.surface_type] & collision_layers)) {
        return false;
    }

    if (!collide_object_check_triangle_bounding_box(collide_data)) {
        return false;
    }

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, &collide_data->triangle, mesh_triangle_minkowski_sum, collide_data->object, dynamic_object_minkowski_sum, &gRight)) {
        return false;
    }

    struct EpaResult result;

    if (epaSolve(
        &simplex, 
        &collide_data->triangle, 
        mesh_triangle_minkowski_sum, 
        collide_data->object, 
        dynamic_object_minkowski_sum, 
        &result)) {
        enum surface_type surface_type = collide_data->triangle.triangle.surface_type;
        correct_overlap(
            collide_data->object, 
            &result, 
            -1.0f, 
            collide_data->object->disable_friction ? 0.0f : collide_data->object->type->friction, 
            collide_data->object->type->bounce,
            surface_type
        );
        collide_add_contact(collide_data->object, &result, surface_type);
        return true;
    }

    return false;
}

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh) {   
    if (object->trigger_type != TRIGGER_TYPE_NONE) {
        return;
    }

    struct object_mesh_collide_data collide_data;
    collide_data.mesh = mesh;
    collide_data.object = object;
    collide_data.triangle.vertices = mesh->index.vertices;
    mesh_collider_lookup_triangle_indices(mesh, &object->bounding_box, collide_object_to_triangle, &collide_data, object->collision_layers);
}

void collide_adjust_collision(struct dynamic_object* object, struct EpaResult* result, struct Vector3* momentum_center, struct Vector3* relative_velocity, float factor) {
    if (!object->is_fixed) {
        vector3AddScaled(object->position, &result->normal, result->penetration * factor, object->position);

        if (relative_velocity) {
            vector3AddScaled(momentum_center, relative_velocity, factor, &object->velocity);
        }
    }
}

void collide_object_to_object(struct dynamic_object* a, struct dynamic_object* b) {
    if (!(a->collision_layers & b->collision_layers)) {
        return;
    }

    if (a->collision_group && a->collision_group == b->collision_group) {
        return;
    }

    if (a->trigger_type != 0 && b->trigger_type != 0) {
        return;
    }

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &gRight)) {
        return;
    }

    bool needs_overlap = DYNAMIC_OBJECT_NEEDS_OVERLAP(a) && DYNAMIC_OBJECT_NEEDS_OVERLAP(b);

    if (!needs_overlap) {
        struct contact* contact = collision_scene_new_contact();

        if (!contact) {
            return;
        }

        if (b->trigger_type == TRIGGER_TYPE_BASIC) {
            contact->normal = gZeroVec;
            contact->point = *a->position;
            contact->other_object = a->entity_id;
            contact->collision_layers = a->collision_layers;
            contact->surface_type = a->type->surface_type;

            contact->next = b->active_contacts;
            b->active_contacts = contact;
        } else {
            contact->normal = gZeroVec;
            contact->point = *b->position;
            contact->other_object = b->entity_id;
            contact->collision_layers = b->collision_layers;
            contact->surface_type = b->type->surface_type;

            contact->next = a->active_contacts;
            a->active_contacts = contact;
        }

        return;
    }

    struct EpaResult result;

    if (!epaSolve(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &result)) {
        return;
    }

    bool should_push = DYNAMIC_OBJECT_SHOULD_PUSH(a) && DYNAMIC_OBJECT_SHOULD_PUSH(b);

    if (should_push) {
        float friction = a->type->friction < b->type->friction ? a->type->friction : b->type->friction;
        float bounce = a->type->bounce > b->type->bounce ? a->type->bounce : b->type->bounce;

        struct Vector3 relative_velocity;
        vector3Sub(&a->velocity, &b->velocity, &relative_velocity);
        bool should_correct_velocity = correct_velocity(&relative_velocity, &result.normal, 1.0f, friction, bounce);
        struct Vector3* relative_velocity_ptr = should_correct_velocity ? &relative_velocity : NULL;

        if (a->weight_class == b->weight_class) {
            struct Vector3 momentum_center;
            vector3Add(&a->velocity, &b->velocity, &momentum_center);
            vector3Scale(&momentum_center, &momentum_center, 0.5f);

            collide_adjust_collision(b, &result, &momentum_center, relative_velocity_ptr, -0.5f);
            collide_adjust_collision(a, &result, &momentum_center, relative_velocity_ptr, 0.5f);
        } else if (a->weight_class < b->weight_class) {
            collide_adjust_collision(a, &result, &b->velocity, relative_velocity_ptr, 1.0f);
        } else {
            collide_adjust_collision(b, &result, &a->velocity, relative_velocity_ptr, -1.0f);
        }
    }


    if (DYNAMIC_OBJECT_NEEDS_OVERLAP(b)) {
        struct contact* contact = collision_scene_new_contact();
    
        if (!contact) {
            return;
        }
    
        contact->normal = result.normal;
        contact->point = result.contactA;
        contact->other_object = a->entity_id;
        contact->surface_type = a->type->surface_type;
        contact->collision_layers = a->collision_layers;
    
        contact->next = b->active_contacts;
        b->active_contacts = contact;
    }
    
    if (DYNAMIC_OBJECT_NEEDS_OVERLAP(a)) {
        struct contact* contact = collision_scene_new_contact();
    
        if (!contact) {
            return;
        }
        
        vector3Negate(&result.normal, &contact->normal);
        contact->point = result.contactB;
        contact->other_object = b->entity_id;
        contact->surface_type = b->type->surface_type;
        contact->collision_layers = b->collision_layers;
    
        contact->next = a->active_contacts;
        a->active_contacts = contact;
    }
}

void collide_object_to_trigger(struct dynamic_object* obj, struct spatial_trigger* trigger) {
    if (!(obj->collision_layers & trigger->collision_layers)) {
        return;
    }

    if (trigger->collision_group && trigger->collision_group == obj->collision_group) {
        return;
    }

    if (!spatial_trigger_does_contain_point(trigger, obj->position)) {
        return;
    }

    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }
    
    contact->normal = gZeroVec;
    contact->point = *obj->position;
    contact->other_object = obj->entity_id;
    contact->surface_type = obj->type->surface_type;
    contact->collision_layers = obj->collision_layers;

    contact->next = trigger->active_contacts;
    trigger->active_contacts = contact;
}

void collide_add_contact(struct dynamic_object* object, struct EpaResult* result, enum surface_type surface_type) {
    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }

    contact->normal = result->normal;
    contact->point = result->contactA;
    contact->other_object = 0;
    contact->surface_type = surface_type;
    // TODO add collision layers
    contact->collision_layers = 0;

    contact->next = object->active_contacts;
    object->active_contacts = contact;
}
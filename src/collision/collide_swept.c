#include "collide_swept.h"

#include <math.h>
#include "collision_scene.h"
#include "collide.h"
#include "gjk.h"
#include "epa.h"
#include <stdio.h>

struct swept_dynamic_object {
    struct dynamic_object* object;
    struct Vector3 offset;
};

void swept_dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct swept_dynamic_object* obj = (struct swept_dynamic_object*)data;

    dynamic_object_minkowski_sum(obj->object, direction, output);

    if (vector3Dot(&obj->offset, direction) > 0.0f) {
        vector3Add(output, &obj->offset, output);
    }
}

void object_mesh_collide_data_init(
    struct object_mesh_collide_data* data,
    struct Vector3* prev_pos,
    struct mesh_collider* mesh,
    struct dynamic_object* object
) {
    data->prev_pos = prev_pos;
    data->mesh = mesh;
    data->object = object;
}

bool collide_object_swept_to_triangle(void* data, int triangle_index, int collision_layers) {
    struct object_mesh_collide_data* collide_data = (struct object_mesh_collide_data*)data;

    struct mesh_triangle triangle;
    triangle.vertices = collide_data->mesh->index.vertices;
    triangle.triangle = collide_data->mesh->index.indices[triangle_index];

    if (!(surface_type_collision_layers[triangle.triangle.surface_type] & collision_layers)) {
        return false;
    }

    struct swept_dynamic_object swept;
    swept.object = collide_data->object;
    vector3Sub(collide_data->prev_pos, collide_data->object->position, &swept.offset);

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, &triangle, mesh_triangle_minkowski_sum, &swept, swept_dynamic_object_minkowski_sum, &gRight)) {
        return false;
    }

    struct EpaResult result;

    if (epaSolveSwept(
        &simplex, 
        &triangle, 
        mesh_triangle_minkowski_sum, 
        &swept, 
        swept_dynamic_object_minkowski_sum, 
        collide_data->prev_pos,
        collide_data->object->position,
        &result
    )) {
        if (!mesh_triangle_filter_edge_contacts(
            &triangle.triangle, 
            collide_data->mesh->index.vertices, 
            &result.normal)) {
            return false;
        }

        collide_data->hit_result = result;
        collide_data->surface_type = triangle.triangle.surface_type;
        return true;
    }

    struct Vector3 final_pos = *collide_data->object->position;
    *collide_data->object->position = *collide_data->prev_pos;

    if (epaSolve(
        &simplex,
        &triangle,
        mesh_triangle_minkowski_sum,
        collide_data->object,
        dynamic_object_minkowski_sum,
        &result
    )) {
        if (!mesh_triangle_filter_edge_contacts(
            &triangle.triangle, 
            collide_data->mesh->index.vertices, 
            &result.normal)) {
            return false;
        }
        
        collide_data->hit_result = result;
        collide_data->surface_type = triangle.triangle.surface_type;
        return true;
    }

    *collide_data->object->position = final_pos;

    return false;
}

#define FACE_MARGIN 0.001f

void collide_object_swept_bounce(
    struct dynamic_object* object, 
    struct object_mesh_collide_data* collide_data,
    struct Vector3* start_pos
) {
    // this is the new prev position when iterating
    // over mulitple swept collisions
    *collide_data->prev_pos = *object->position;

    struct Vector3 move_amount;
    vector3Sub(start_pos, object->position, &move_amount);

    struct Vector3 move_amount_normal;
    vector3Project(&move_amount, &collide_data->hit_result.normal, &move_amount_normal);
    struct Vector3 move_amount_tangent;
    vector3Sub(&move_amount, &move_amount_normal, &move_amount_tangent);

    vector3Scale(&move_amount_normal, &move_amount_normal, -object->type->bounce);
    
    vector3Add(object->position, &move_amount_normal, object->position);
    vector3Add(object->position, &move_amount_tangent, object->position);
    vector3AddScaled(object->position, &collide_data->hit_result.normal, -collide_data->hit_result.penetration + FACE_MARGIN, object->position);

    // don't include friction on a bounce
    correct_velocity(&object->velocity, &collide_data->hit_result.normal, -1.0f, 0.0f, object->type->bounce);

    vector3Sub(object->position, start_pos, &move_amount);
    vector3Add(&move_amount, &object->bounding_box.min, &object->bounding_box.min);
    vector3Add(&move_amount, &object->bounding_box.max, &object->bounding_box.max);

    collide_add_contact(object, &collide_data->hit_result, collide_data->surface_type);
}

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos) {
    if (object->trigger_type != TRIGGER_TYPE_NONE) {
        return false;
    }

    struct object_mesh_collide_data collide_data;
    object_mesh_collide_data_init(&collide_data, prev_pos, mesh, object);

    struct Vector3 start_pos = *object->position;

    if (!mesh_collider_lookup_triangle_indices(
        mesh, 
        &object->bounding_box, 
        collide_object_swept_to_triangle, 
        &collide_data,
        object->collision_layers
    )) {
        return false;
    }

    collide_object_swept_bounce(object, &collide_data, &start_pos);

    return true;
}

bool collide_object_to_multiple_mesh_swept(struct dynamic_object* object, struct mesh_collider** meshes, int mesh_count, struct Vector3* prev_pos) {
    if (object->trigger_type != TRIGGER_TYPE_NONE) {
        return false;
    }

    struct object_mesh_collide_data collide_data;
    object_mesh_collide_data_init(&collide_data, prev_pos, NULL, object);

    struct Vector3 start_pos = *object->position;

    bool did_hit = false;

    for (int i = 0; i < mesh_count; i += 1) {
        collide_data.mesh = meshes[i];
        if (mesh_collider_lookup_triangle_indices(
            meshes[i], 
            &object->bounding_box, 
            collide_object_swept_to_triangle, 
            &collide_data,
            object->collision_layers
        )) {
            did_hit = true;
        }
    }

    if (!did_hit) {
        return false;
    }

    collide_object_swept_bounce(object, &collide_data, &start_pos);

    return true;
}

void collide_object_to_object_swept(struct dynamic_object* a, struct dynamic_object* b, struct Vector3* prev_a, struct Vector3* prev_b) {
    if (!(a->collision_layers & b->collision_layers)) {
        return;
    }

    if (a->collision_group && a->collision_group == b->collision_group) {
        return;
    }

    if (a->trigger_type != 0 && b->trigger_type != 0) {
        return;
    }

    struct Vector3 relative_offset;
    struct Vector3 prev_pos_b;
    vector3Sub(a->position, prev_a, &relative_offset);
    vector3Add(&relative_offset, prev_b, &prev_pos_b);
    
    struct swept_dynamic_object swept;
    swept.object = b;
    vector3Sub(&prev_pos_b, b->position, &swept.offset);

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, a, dynamic_object_minkowski_sum, &swept, swept_dynamic_object_minkowski_sum, &gRight)) {
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

    struct Vector3 b_original_position = *b->position;

    if (!epaSolveSwept(
        &simplex, 
        a, 
        dynamic_object_minkowski_sum, 
        &swept, 
        swept_dynamic_object_minkowski_sum, 
        &prev_pos_b,
        b->position,
        &result
    )) {
        return;
    }
    
    bool should_push = DYNAMIC_OBJECT_SHOULD_PUSH(a) && DYNAMIC_OBJECT_SHOULD_PUSH(b);
    
    struct Vector3 relative_movement;
    vector3Sub(&b_original_position, &prev_pos_b, &relative_movement);
    struct Vector3 moved_amount;
    vector3Sub(b->position, &prev_pos_b, &moved_amount);

    float lerp = vector3Dot(&relative_movement, &moved_amount) / vector3MagSqrd(&relative_movement);
    vector3AddScaled(&result.contactA, &relative_offset, -lerp, &result.contactA);
    vector3AddScaled(&result.contactB, &relative_offset, -lerp, &result.contactB);

    if (should_push) {
        vector3Lerp(prev_b, &b_original_position, lerp, b->position);
        vector3Lerp(prev_a, a->position, lerp, a->position);
        vector3ProjectPlane(&a->velocity, &result.normal, &a->velocity);
        vector3ProjectPlane(&b->velocity, &result.normal, &b->velocity);
    } else {
        *b->position = b_original_position;
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
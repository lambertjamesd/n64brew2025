#include "collision_scene.h"

#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "mesh_collider.h"
#include "collide.h"
#include "collide_swept.h"
#include "contact.h"
#include "../util/hash_map.h"
#include <memory.h>
#include "../math/constants.h"
#include "water.h"
#include "../entity/entity_spawner.h"

#define MAX_SNAP_TO_GROUND_ANGLE    35.0f
#define KILL_PLANE                  -10.0f

struct collision_scene g_scene;
static struct Vector2 max_ground_snap_angle;

struct Box3D* collision_scene_element_bounding_box(struct collision_scene_element* element) {
    if (element->type == COLLISION_ELEMENT_TYPE_DYNAMIC) {
        return &((struct dynamic_object*)element->object)->bounding_box;
    }

    if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
        return &((struct spatial_trigger*)element->object)->bounding_box;
    }

    return NULL;
}

void collision_scene_reset() {
    free(g_scene.elements);
    free(g_scene.all_contacts);
    hash_map_destroy(&g_scene.entity_mapping);
    hash_map_destroy(&g_scene.trigger_mapping);
    memset(&g_scene, 0, sizeof(g_scene));

    hash_map_init(&g_scene.entity_mapping, MIN_DYNAMIC_OBJECTS);
    hash_map_init(&g_scene.trigger_mapping, MIN_DYNAMIC_OBJECTS);

    g_scene.elements = malloc(sizeof(struct collision_scene_element) * MIN_DYNAMIC_OBJECTS);
    g_scene.capacity = MIN_DYNAMIC_OBJECTS;
    g_scene.count = 0;
    g_scene.all_contacts = malloc(sizeof(struct contact) * MAX_ACTIVE_CONTACTS);
    g_scene.next_free_contact = &g_scene.all_contacts[0];
    g_scene.kill_plane = KILL_PLANE;

    for (int i = 0; i + 1 < MAX_ACTIVE_CONTACTS; ++i) {
        g_scene.all_contacts[i].next = &g_scene.all_contacts[i + 1];
    }

    g_scene.all_contacts[MAX_ACTIVE_CONTACTS - 1].next = NULL;

    vector2ComplexFromAngle(MAX_SNAP_TO_GROUND_ANGLE * M_DEG_2_RAD, &max_ground_snap_angle);
}

void collision_scene_add_with_type(void* object, enum collision_element_type type) {
    if (g_scene.count >= g_scene.capacity) {
        g_scene.capacity *= 2;
        g_scene.elements = realloc(g_scene.elements, sizeof(struct collision_scene_element) * g_scene.capacity);
    }

    struct collision_scene_element* next = &g_scene.elements[g_scene.count];

    next->object = object;
    next->type = type;

    g_scene.count += 1;
}

void collision_scene_add(struct dynamic_object* object) {
    collision_scene_add_with_type(object, COLLISION_ELEMENT_TYPE_DYNAMIC);
    hash_map_set(&g_scene.entity_mapping, object->entity_id, object);
}

struct dynamic_object* collision_scene_find_object(entity_id id) {
    if (!id) {
        return 0;
    }

    return hash_map_get(&g_scene.entity_mapping, id);
}

spatial_trigger_t* collision_scene_find_trigger(entity_id id) {
    if (!id) {
        return 0;
    }

    return hash_map_get(&g_scene.trigger_mapping, id);
}

void collision_scene_return_contacts(struct contact* active_contacts) {
    struct contact* last_contact = active_contacts;

    while (last_contact && last_contact->next) {
        last_contact = last_contact->next;
    }

    if (last_contact) {
        last_contact->next = g_scene.next_free_contact;
        g_scene.next_free_contact = active_contacts;
    }
}

void collision_scene_remove_any(void* object) {
    for (int i = 0; i < g_scene.count; ++i) {
        if (object == g_scene.elements[i].object) {
            if (g_scene.elements[i].type == COLLISION_ELEMENT_TYPE_DYNAMIC) {
                struct dynamic_object* obj = g_scene.elements[i].object;
                collision_scene_return_contacts(obj->active_contacts);
                obj->active_contacts = NULL;
            } else {
                struct spatial_trigger* trigger = g_scene.elements[i].object;
                collision_scene_return_contacts(trigger->active_contacts);
                trigger->active_contacts = NULL;
            }

            g_scene.count -= 1;
            g_scene.elements[i] = g_scene.elements[g_scene.count];
            return;
        }
    }
}

void collision_scene_remove(struct dynamic_object* object) {
    collision_scene_remove_any(object);
    hash_map_delete(&g_scene.entity_mapping, object->entity_id);
}

void collision_scene_add_trigger(struct spatial_trigger* trigger) {
    collision_scene_add_with_type(trigger, COLLISION_ELEMENT_TYPE_TRIGGER);
    hash_map_set(&g_scene.trigger_mapping, trigger->entity_id, trigger);
}

void collision_scene_remove_trigger(struct spatial_trigger* trigger) {
    collision_scene_remove_any(trigger);
    hash_map_delete(&g_scene.trigger_mapping, trigger->entity_id);
}

void collision_scene_add_static_mesh(struct mesh_collider* collider) {
    float kill_plane = collider->index.min.y + KILL_PLANE;

    if (kill_plane < g_scene.kill_plane) {
        g_scene.kill_plane = kill_plane;
    }

    for (int i = 0; i < MAX_STATIC_MESHES; i += 1) {
        if (g_scene.mesh_colliders[i] == NULL) {
            g_scene.mesh_colliders[i] = collider;
            g_scene.mesh_collider_count += 1;
            return;
        }
    }
    assert(false);
}

void collision_scene_remove_static_mesh(struct mesh_collider* collider) {
    g_scene.kill_plane = KILL_PLANE;

    int write_index = 0;
    for (int read_index = 0; read_index < MAX_STATIC_MESHES; read_index += 1) {
        if (write_index != read_index) {
            g_scene.mesh_colliders[write_index] = g_scene.mesh_colliders[read_index];
        }
        
        if (g_scene.mesh_colliders[read_index] != collider) {
            write_index += 1;

            if (g_scene.mesh_colliders[read_index]) {
                float kill_plane = g_scene.mesh_colliders[read_index]->index.min.y + KILL_PLANE;
    
                if (kill_plane < g_scene.kill_plane) {
                    g_scene.kill_plane = kill_plane;
                }
            }
        } else {
            g_scene.mesh_collider_count -= 1;
        }
    }

    while (write_index < MAX_STATIC_MESHES) {
        g_scene.mesh_colliders[write_index] = NULL;
        write_index += 1;
    }
}

struct collide_edge {
    uint16_t is_start_edge: 1;
    uint16_t object_index: 15;
    short x;
};

int collide_edge_compare(struct collide_edge a, struct collide_edge b) {
    if (a.x == b.x) {
        return b.is_start_edge - a.is_start_edge;
    }

    return a.x - b.x;
}

void collide_edge_sort(struct collide_edge* edges, struct collide_edge* tmp, int start, int end) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    collide_edge_sort(edges, tmp, start, mid);
    collide_edge_sort(edges, tmp, mid, end);

    int a = start;
    int b = mid;
    int output = start;

    while (a < mid || b < end) {
        if (b >= end || (a < mid && collide_edge_compare(edges[a], edges[b]) < 0)) {
            tmp[output] = edges[a];
            ++output;
            ++a;
        } else {
            tmp[output] = edges[b];
            ++output;
            ++b;
        }
    }

    for (int i = start; i < end; ++i) {
        edges[i] = tmp[i];
    }
}

void collision_scene_collide_dynamic(struct Vector3* prev_positions) {
    int edge_count = g_scene.count * 2;

    struct collide_edge collide_edges[edge_count];

    struct collide_edge* curr_edge = &collide_edges[0];

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        struct Box3D* bb = collision_scene_element_bounding_box(element);

        curr_edge->is_start_edge = 1;
        curr_edge->object_index = i;
        curr_edge->x = (short)floorf(bb->min.x * 8.0f);

        curr_edge += 1;

        curr_edge->is_start_edge = 0;
        curr_edge->object_index = i;
        curr_edge->x = (short)ceilf(bb->max.x * 8.0f);

        curr_edge += 1;
    }

    struct collide_edge tmp[edge_count];
    collide_edge_sort(collide_edges, tmp, 0, edge_count);

    uint16_t active_objects[g_scene.count];
    int active_object_count = 0;

    for (int edge_index = 0; edge_index < edge_count; edge_index += 1) {
        struct collide_edge edge = collide_edges[edge_index];

        if (edge.is_start_edge) {
            struct collision_scene_element* a = &g_scene.elements[edge.object_index];

            for (int active_index = 0; active_index < active_object_count; active_index += 1) {
                struct collision_scene_element* b = &g_scene.elements[active_objects[active_index]];

                if (a->type == COLLISION_ELEMENT_TYPE_DYNAMIC && b->type == COLLISION_ELEMENT_TYPE_DYNAMIC) {
                    struct dynamic_object* a_obj = a->object;
                    struct dynamic_object* b_obj = b->object;
                    if (box3DHasOverlap(&a_obj->bounding_box, &b_obj->bounding_box)) {
                        if (a_obj->should_sweep_collide || b_obj->should_sweep_collide) {
                            collide_object_to_object_swept(
                                a_obj, 
                                b_obj, 
                                &prev_positions[edge.object_index], 
                                &prev_positions[active_objects[active_index]]
                            );
                        } else {
                            collide_object_to_object(a_obj, b_obj);
                        }
                    }
                } else if (a->type == COLLISION_ELEMENT_TYPE_DYNAMIC && b->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
                    collide_object_to_trigger(a->object, b->object);
                } else if (a->type == COLLISION_ELEMENT_TYPE_TRIGGER && b->type == COLLISION_ELEMENT_TYPE_DYNAMIC) {
                    collide_object_to_trigger(b->object, a->object);
                }
            }

            active_objects[active_object_count] = edge.object_index;
            active_object_count += 1;
            
        } else {
            int found_index = -1;

            for (int active_index = 0; active_index < active_object_count; active_index += 1) {
                if (active_objects[active_index] == edge.object_index) {
                    found_index = active_index;
                    break;
                }
            }

            assert(found_index != -1);

            // remove item by replacing it with the last one
            active_objects[found_index] = active_objects[active_object_count - 1];
            active_object_count -= 1;
        }
    }
}

bool collision_scene_should_sweep(dynamic_object_t* object, struct Vector3* prev_pos) {
    struct Vector3 offset;
    vector3Sub(object->position, prev_pos, &offset);
    struct Vector3 bbSize;
    vector3Sub(&object->bounding_box.max, &object->bounding_box.min, &bbSize);
    vector3Scale(&bbSize, &bbSize, 0.5f);

    return fabs(offset.x) > bbSize.x ||
        fabs(offset.y) > bbSize.y ||
        fabs(offset.z) > bbSize.z;
}

#define MAX_SWEPT_ITERATIONS    5

void collision_scene_collide_single(struct dynamic_object* object, struct Vector3* prev_pos) {
    for (int i = 0; i < MAX_SWEPT_ITERATIONS; i += 1) {
        if (collision_scene_should_sweep(object, prev_pos)) {
            bool did_hit = collide_object_to_multiple_mesh_swept(object, g_scene.mesh_colliders, g_scene.mesh_collider_count, prev_pos);
            if (!did_hit) {
                return;
            }
        } else {
            for (int collider_index = 0; collider_index < g_scene.mesh_collider_count; collider_index += 1) {
                struct mesh_collider* mesh = g_scene.mesh_colliders[collider_index];
                collide_object_to_mesh(object, mesh);
            }
            return;
        }
    }

    // too many swept iterations
    // to prevent tunneling just move 
    // the object back to the previous known
    // valid location
    *object->position = *prev_pos;
    // slow the the object so it can start using non swept collision
    vector3Scale(&object->velocity, &object->velocity, 0.9f);
}

void collision_scene_snap_to_ground(struct dynamic_object* object, struct Vector3* prev_pos) {
    if (!object->shadow_contact || 
        object->shadow_contact->surface_type == SURFACE_TYPE_WATER ||
        object->shadow_contact->point.y > object->position->y) {
        return;
    }

    struct Vector3 offset;
    vector3Sub(object->position, prev_pos, &offset);
    float horz_offset = offset.x * offset.x + offset.z * offset.z;

    float vert_offset = object->position->y - object->shadow_contact->point.y;

    horz_offset *= max_ground_snap_angle.y * max_ground_snap_angle.y;
    vert_offset *= max_ground_snap_angle.x;
    vert_offset *= vert_offset;

    if (vert_offset > horz_offset) {
        return;
    }

    object->position->y = object->shadow_contact->point.y;
    object->shadow_contact->next = object->active_contacts;
    object->active_contacts = object->shadow_contact;
    object->shadow_contact = NULL;
}

void collision_scene_collide() {
    struct Vector3 prev_pos[g_scene.count];
    bool prev_was_grounded[g_scene.count];

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];
        if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
            struct spatial_trigger* trigger = element->object;
            spatial_trigger_recalc_bb(trigger);
            collision_scene_return_contacts(trigger->active_contacts);
            trigger->active_contacts = NULL;
            continue;
        }

        struct dynamic_object* object = (struct dynamic_object*)element->object;

        prev_pos[i] = *object->position;
        prev_was_grounded[i] = dynamic_object_get_ground(object) != NULL;

        collision_scene_return_contacts(object->active_contacts);
        collision_scene_return_contacts(object->shadow_contact);
        object->active_contacts = NULL;
        object->shadow_contact = NULL;

        if (object->is_pushed) {
            --object->is_pushed;
        }

        if (object->is_jumping) {
            --object->is_jumping;
        }

        if (object->disable_friction) {
            --object->disable_friction;
        }

        if (object->under_water) {
            --object->under_water;
        }

        if (object->has_ice_dash) {
            --object->has_ice_dash;
        }

        dynamic_object_update(object);
        object->should_sweep_collide = collision_scene_should_sweep(object, &prev_pos[i]);

        dynamic_object_recalc_bb(object);

        if (object->should_sweep_collide) {
            struct Vector3 move_amount;
            vector3Sub(&prev_pos[i], object->position, &move_amount);
            box3DExtendDirection(&object->bounding_box, &move_amount, &object->bounding_box);
        }
    }

    collision_scene_collide_dynamic(prev_pos);

    entity_id kill_entity = 0;

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
            continue;
        }

        struct dynamic_object* object = (struct dynamic_object*)element->object;

        if (object->is_fixed) {
            continue;
        }

        collision_scene_collide_single(element->object, &prev_pos[i]);

        bool is_grounded = dynamic_object_get_ground(object) != NULL;

        struct mesh_shadow_cast_result shadow;
        if (collision_scene_shadow_cast(object->position, &shadow)) {
            struct contact* contact = collision_scene_new_contact();

            if (contact) {
                contact->next = NULL;
                vector3Normalize(&shadow.normal, &contact->normal);
                contact->other_object = 0;
                contact->point = *object->position;
                contact->point.y = shadow.y;
                contact->surface_type = shadow.surface_type;
                contact->collision_layers = 0;
                object->shadow_contact = contact;
            }

            water_apply(object);
        }
        
        if (!is_grounded && prev_was_grounded[i] && !object->is_jumping) {
            collision_scene_snap_to_ground(object, &prev_pos[i]);
        }

        if (object->position->y < g_scene.kill_plane) {
            kill_entity = object->entity_id;
            object->position->y = g_scene.kill_plane;
            object->velocity.y = 0.0f;
            object->hit_kill_plane = 1;
        }
    }

    // kill the entity outide the loop since despawing an entity
    // could mess with the element list
    if (kill_entity) {
        entity_despawn(kill_entity);
    }
}

struct contact* collision_scene_new_contact() {
    if (!g_scene.next_free_contact) {
        return NULL;
    }

    struct contact* result = g_scene.next_free_contact;
    g_scene.next_free_contact = result->next;
    return result;
}

struct positioned_shape {
    struct dynamic_object_type* type;
    struct Vector3* center;
};

void positioned_shape_mink_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct positioned_shape* shape = (struct positioned_shape*)data;
    shape->type->minkowsi_sum(&shape->type->data, direction, output);
    vector3Add(output, shape->center, output);
}

void collision_scene_query(struct dynamic_object_type* shape, struct Vector3* center, int collision_layers, collision_scene_query_callback callback, void* callback_data) {
    struct Box3D bounding_box;
    shape->bounding_box(&shape->data, NULL, &bounding_box);
    vector3Add(&bounding_box.min, center, &bounding_box.min);
    vector3Add(&bounding_box.max, center, &bounding_box.max);

    struct positioned_shape positioned_shape;

    positioned_shape.type = shape;
    positioned_shape.center = center;

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
            continue;
        }

        struct dynamic_object* object = element->object;

        if (!(object->collision_layers & collision_layers)) {
            continue;
        }

        if (!box3DHasOverlap(&bounding_box, &object->bounding_box)) {
            continue;
        }

        struct Simplex simplex;

        struct Vector3 first_dir;
        vector3Sub(center, object->position, &first_dir);

        if (!gjkCheckForOverlap(&simplex, &positioned_shape, positioned_shape_mink_sum, object, dynamic_object_minkowski_sum, &first_dir)) {
            continue;;
        }

        callback(callback_data, object);
    }
}

void collision_scene_query_trigger(
    struct spatial_trigger_type* shape, 
    struct TransformSingleAxis* transform, 
    int collision_layers, 
    collision_scene_query_callback callback, 
    void* callback_data) {

    struct spatial_trigger trigger = {
        .type = shape,
        .transform = transform,
    };

    spatial_trigger_recalc_bb(&trigger);

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
            continue;
        }

        struct dynamic_object* object = element->object;

        if (!(object->collision_layers & collision_layers)) {
            continue;
        }

        if (!spatial_trigger_does_contain_point(&trigger, object->position)) {
            continue;
        }

        callback(callback_data, object);
    }
}

bool collision_scene_shadow_cast(struct Vector3* starting_point, struct mesh_shadow_cast_result* result) {
    bool did_hit = false;

    for (int collider_index = 0; collider_index < g_scene.mesh_collider_count; collider_index += 1) {
        struct mesh_shadow_cast_result mesh_result;
        if (mesh_collider_shadow_cast(g_scene.mesh_colliders[collider_index], starting_point, &mesh_result)) {
            if (!did_hit || result->y > mesh_result.y) {
                *result = mesh_result;
                did_hit = true;
            }
        }
    }

    return did_hit;
}

int collision_scene_get_count() {
    return g_scene.count;
}

struct collision_scene_element* collision_scene_get_element(int index) {
    return &g_scene.elements[index];
}
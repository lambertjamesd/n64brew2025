#ifndef __COLLISION_COLLISION_SCENE_H__
#define __COLLISION_COLLISION_SCENE_H__

#include "dynamic_object.h"
#include "spatial_trigger.h"
#include "../collision/mesh_collider.h"
#include "../util/hash_map.h"
#include "contact.h"

typedef int collision_id;

#define MIN_DYNAMIC_OBJECTS 64
#define MAX_ACTIVE_CONTACTS 128

#define MAX_STATIC_MESHES   8

enum collision_element_type {
    COLLISION_ELEMENT_TYPE_DYNAMIC,
    COLLISION_ELEMENT_TYPE_TRIGGER,
};

struct collision_scene_element {
    void* object;
    enum collision_element_type type;
};

struct Box3D* collision_scene_element_bounding_box(struct collision_scene_element* element);

struct collision_scene {
    struct collision_scene_element* elements;
    struct contact* next_free_contact;
    struct contact* all_contacts;
    struct hash_map entity_mapping;
    struct hash_map trigger_mapping;
    uint16_t count;
    uint16_t capacity;
    uint16_t mesh_collider_count;

    float kill_plane;

    struct mesh_collider* mesh_colliders[8];
};

void collision_scene_reset();
void collision_scene_add(struct dynamic_object* object);
void collision_scene_remove(struct dynamic_object* object);

void collision_scene_add_trigger(struct spatial_trigger* trigger);
void collision_scene_remove_trigger(struct spatial_trigger* trigger);

struct dynamic_object* collision_scene_find_object(entity_id id);
spatial_trigger_t* collision_scene_find_trigger(entity_id id);

void collision_scene_add_static_mesh(struct mesh_collider* collider);
void collision_scene_remove_static_mesh(struct mesh_collider* collider);

void collision_scene_collide();

struct contact* collision_scene_new_contact();

typedef void (*collision_scene_query_callback)(void* data, struct dynamic_object* overlaps);

void collision_scene_query(struct dynamic_object_type* shape, struct Vector3* center, int collision_layers, collision_scene_query_callback callback, void* callback_data);
void collision_scene_query_trigger(struct spatial_trigger_type* shape, struct TransformSingleAxis* transform, int collision_layers, collision_scene_query_callback callback, void* callback_data);
bool collision_scene_shadow_cast(struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

int collision_scene_get_count();

struct collision_scene_element* collision_scene_get_element(int index);

void collision_scene_return_contacts(struct contact* active_contacts);

#endif
#ifndef __COLLISION_COLLIDE_SWEPT_H__
#define __COLLISION_COLLIDE_SWEPT_H__

#include "dynamic_object.h"
#include "mesh_collider.h"
#include "epa.h"

struct object_mesh_collide_data {
    struct Vector3* prev_pos;
    struct mesh_collider* mesh;
    struct dynamic_object* object;
    struct EpaResult hit_result;
    enum surface_type surface_type;
};

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos);
bool collide_object_to_multiple_mesh_swept(struct dynamic_object* object, struct mesh_collider** meshes, int mesh_count, struct Vector3* prev_pos);
void collide_object_to_object_swept(struct dynamic_object* a, struct dynamic_object* b, struct Vector3* prev_a, struct Vector3* prev_b);

#endif
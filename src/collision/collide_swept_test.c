#include "collide_swept.h"
#include "../test/framework_test.h"
#include "./shapes/box.h"
#include "collision_scene.h"
#include <stddef.h>

void object_mesh_collide_data_init(
    struct object_mesh_collide_data* data,
    struct Vector3* prev_pos,
    struct mesh_collider* mesh,
    struct dynamic_object* object
);

bool collide_object_swept_to_triangle(void* data, int triangle_index, int collision_layers);

static kd_tree_leaf_t single_triangle_leaf = {
    .node_type = KD_TREE_LEAF_NODE,
    .triangle_count = 1,
    .triangle_offset = 0,
};

static struct mesh_collider single_traingle_mesh = {
    .index = {
        .min = {0.0f, 0.0f, 0.0f},
        .size_inv = {1.0f, 1.0f, 1.0f},
        .vertices = (struct Vector3[]){
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
        },
        .indices = (struct mesh_triangle_indices[]){
            {{0, 1, 2}},
        },
        .nodes = &single_triangle_leaf,
    },
};

static struct dynamic_object_type simple_cube_object = {
    .minkowsi_sum = box_minkowski_sum,
    .bounding_box = box_bounding_box,
    .data = {
        .box = {
            .half_size = {0.05f, 0.05f, 0.05f},
        }
    },
    .bounce = 0.5f,
    .friction = 0.0f,
};

void test_collide_object_swept_to_triangle(struct test_context* t) {
    struct object_mesh_collide_data collide_data;
    struct Vector3 prev_pos = {0.5f, 0.25f, -1.0f};
    struct Vector3 position = {0.5f, 0.25f, 1.0f};

    struct dynamic_object object;
    dynamic_object_init(1, &object, &simple_cube_object, ~0, &position, NULL);

    object_mesh_collide_data_init(&collide_data, &prev_pos, &single_traingle_mesh, &object);

    bool did_hit = collide_object_swept_to_triangle(&collide_data, 0, COLLISION_LAYER_TANGIBLE);

    test_eqi(t, true, did_hit);
    test_ltf(t, position.z, -simple_cube_object.data.box.half_size.z);
    test_near_equalf(t, 0.0f, collide_data.hit_result.normal.x);
    test_near_equalf(t, 0.0f, collide_data.hit_result.normal.y);
    test_near_equalf(t, -1.0f, collide_data.hit_result.normal.z);

    prev_pos = (struct Vector3){0.5f, 0.75f, -1.0f};
    position = (struct Vector3){0.5f, 0.75f, 1.0f};

    did_hit = collide_object_swept_to_triangle(&collide_data, 0, COLLISION_LAYER_TANGIBLE);
    test_eqi(t, false, did_hit);
}

void test_collide_object_to_mesh_swept(struct test_context* t) {
    struct Vector3 prev_pos = {0.5f, 0.25f, -1.0f};
    struct Vector3 position = {0.5f, 0.25f, 1.0f};

    struct dynamic_object object;
    dynamic_object_init(1, &object, &simple_cube_object, ~0, &position, NULL);

    object.velocity = (struct Vector3){0.0f, 0.0f, 1.0f};

    bool did_hit = collide_object_to_mesh_swept(&object, &single_traingle_mesh, &prev_pos);

    test_eqi(t, true, did_hit);
    test_ltf(t, position.z, prev_pos.z * simple_cube_object.bounce);
    test_near_equalf(t, -0.5f, object.velocity.z);

    test_neqi(t, (int)NULL, (int)object.active_contacts);

    test_near_equalf(t, 0.0f, object.active_contacts->normal.x);
    test_near_equalf(t, 0.0f, object.active_contacts->normal.y);
    test_near_equalf(t, -1.0f, object.active_contacts->normal.z);

    collision_scene_return_contacts(object.active_contacts);
    object.active_contacts = NULL;
    test_eqi(t, (int)NULL, (int)object.active_contacts);

    prev_pos = (struct Vector3){0.5f, 0.75f, -1.0f};
    position = (struct Vector3){0.5f, 0.75f, 1.0f};

    did_hit = collide_object_to_mesh_swept(&object, &single_traingle_mesh, &prev_pos);
    test_eqi(t, false, did_hit);
    test_eqi(t, (int)NULL, (int)object.active_contacts);
}
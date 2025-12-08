#include "collision_scene.h"
#include "../test/framework_test.h"
#include "./shapes/box.h"

#include <stddef.h>
#include <malloc.h>
#include "../time/time.h"

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

void test_remove_and_free_object(void* object) {
    collision_scene_remove(object);
    free(object);
}

void collision_scene_collide_single(struct dynamic_object* object, struct Vector3* prev_pos);

void test_collision_scene_collide_single(struct test_context* t) {
    test_load_scene("rom:/scenes/testing.scene");

    struct Vector3 position = {40.0f, 30.0f, 0.0f};
    struct Vector3 prev_position = {0.0f, 30.0f, 0.0f};

    struct dynamic_object object;
    dynamic_object_init(1, &object, &simple_cube_object, ~0, &position, NULL);

    object.velocity = (struct Vector3){40.0f / fixed_time_step, 0.0f, 0.0f};

    collision_scene_collide_single(&object, &prev_position);

    test_ltf(t, position.x, 24.0f);
    test_ltf(t, object.velocity.x, 0.0f);
}

void test_collision_scene_collide(struct test_context* t) {
    struct Vector3 position = {0.0f, 30.0f, 0.0f};

    struct dynamic_object* object = malloc(sizeof(struct dynamic_object));
    dynamic_object_init(1, object, &simple_cube_object, ~0, &position, NULL);
    collision_scene_add(object);
    test_defer_call(t, test_remove_and_free_object, object);

    simple_cube_object.bounce = 0.5f;
    object->velocity = (struct Vector3){40.0f / fixed_time_step, 0.0f, 0.0f};

    collision_scene_collide();

    test_ltf(t, position.x, 24.0f);
    test_ltf(t, object->velocity.x, 0.0f);

    simple_cube_object.bounce = 0.0f;

    *object->position = (struct Vector3){0.0f, 40.0f, 15.0f};
    object->velocity = (struct Vector3){
        80.0f / fixed_time_step,
        -80.0f / fixed_time_step,
        1.0f / fixed_time_step
    };
    dynamic_object_recalc_bb(object);
    collision_scene_collide();
    test_ltf(t, position.x, 24.0f);
    test_gtf(t, position.y, 0.0f);
    test_near_equalf(t, 0.0f, object->velocity.x);
    test_near_equalf(t, 0.0f, object->velocity.y);
    test_gtf(t, object->velocity.z, 0);
}
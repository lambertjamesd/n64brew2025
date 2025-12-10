#include "motorcycle.h"

#include "../render/tmesh.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"

#define HOVER_SAG_AMOUNT        0.5f
#define HOVER_SPRING_STRENGTH   (-GRAVITY_CONSTANT / (CAST_POINT_COUNT * HOVER_SAG_AMOUNT))

struct motorcyle_assets {
    tmesh_t* mesh;
};

typedef struct motorcyle_assets motorcyle_assets_t;

static motorcyle_assets_t assets;

static dynamic_object_type_t collider_type = {
    BOX_COLLIDER(0.311f, 0.41f, 1.9f),
    .max_stable_slope = 0.5f,
    .surface_type = SURFACE_TYPE_DEFAULT,
    .friction = 0.4f,
    .bounce = 0.1f,
    .center = {0.0f, 0.35f, 0.0f},
};

static vector3_t local_cast_points[] = {
    {0.0f, 0.0f, 1.5f},
    {0.0f, 0.0f, -1.5f},
};

void motorcycle_common_init() {
    assets.mesh = tmesh_cache_load("rom:/meshes/vehicles/bike.tmesh");
}

void motorcycle_common_destroy() {
    tmesh_cache_release(assets.mesh);
    assets.mesh = NULL;
}

void motorcycle_ride(struct interactable* interactable, entity_id from) {
    motorcycle_t* motorcycle = (motorcycle_t*)interactable->data;

    debugf("here!\n");
}

float motorcycle_hover_height(motorcycle_t* motorcycle) {
    // this function will eventually do logic
    return 1.0f;
}

void motorcycle_update(void* data) {
    motorcycle_t* motorcycle = (motorcycle_t*)data;

    float target_height = motorcycle_hover_height(motorcycle) + HOVER_SAG_AMOUNT;

    bool needs_damping = true;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        cast_point_t* cast_point = &motorcycle->cast_points[i];

        if (cast_point->surface_type != SURFACE_TYPE_NONE) {
            float actual_height = cast_point->pos.y - cast_point->y;

            if (actual_height < target_height) {
                if (needs_damping) {
                    motorcycle->collider.velocity.y *= 0.8f;
                    needs_damping = false;
                }

                motorcycle->collider.velocity.y += (target_height - actual_height) * fixed_time_step * HOVER_SPRING_STRENGTH;
            }
        }

        vector3_t pos;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &pos);
        cast_point_set_pos(cast_point, &pos);
    }
}

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id) {
    transformSaInit(&motorcycle->transform, &definition->position, &definition->rotation, 1.0f);
    render_scene_init_add_renderable(&motorcycle->renderable, &motorcycle->transform, assets.mesh, 2.0f);
    dynamic_object_init(entity_id, &motorcycle->collider, &collider_type, COLLISION_LAYER_TANGIBLE, &motorcycle->transform.position, &motorcycle->transform.rotation);
    motorcycle->has_rider = false;
    collision_scene_add(&motorcycle->collider);
    update_add(motorcycle, motorcycle_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);

    interactable_init(&motorcycle->interactable, entity_id, INTERACT_TYPE_TALK, motorcycle_ride, motorcycle);

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        vector3_t cast_point;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &cast_point);
        collision_scene_add_cast_point(&motorcycle->cast_points[i], &cast_point);
    }
}

void motorcycle_destroy(motorcycle_t* motorcycle) {
    render_scene_remove_renderable(&motorcycle->renderable);
    collision_scene_remove(&motorcycle->collider);
    interactable_destroy(&motorcycle->interactable);
    update_remove(motorcycle);
}
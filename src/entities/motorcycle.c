#include "motorcycle.h"

#include "../render/tmesh.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"

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
};

void motorcycle_common_init() {
    assets.mesh = tmesh_cache_load("rom:/meshes/vehicles/bike.tmesh");
}

void motorcycle_common_destroy() {
    tmesh_cache_release(assets.mesh);
    assets.mesh = NULL;
}

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id) {
    transformSaInit(&motorcycle->transform, &definition->position, &definition->rotation, 1.0f);
    render_scene_init_add_renderable(&motorcycle->renderable, &motorcycle->transform, assets.mesh, 2.0f);
    dynamic_object_init(entity_id, &motorcycle->collider, &collider_type, COLLISION_LAYER_TANGIBLE, &motorcycle->transform.position, &motorcycle->transform.rotation);
    motorcycle->collider.center.y = 0.9f;
    collision_scene_add(&motorcycle->collider);
}

void motorcycle_destroy(motorcycle_t* motorcycle) {
    render_scene_remove_renderable(&motorcycle->renderable);
    collision_scene_remove(&motorcycle->collider);
}
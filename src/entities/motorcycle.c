#include "motorcycle.h"

#include "../render/tmesh.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../math/mathf.h"

#define HOVER_SAG_AMOUNT        0.25f
#define HOVER_SPRING_STRENGTH   (-GRAVITY_CONSTANT / HOVER_SAG_AMOUNT)

#define ACCEL_RATE              20.0f
#define BACKUP_SPEED            -5.0f
#define DRIVE_SPEED               50.0f
#define BOOST_SPEED             80.0f
#define MAX_TURN_RATE           1.0f

#define STILL_HOVER_HEIGHT      0.25f
#define RIDE_HOVER_HEIGHT       0.5f
#define FAST_HOVER_HEIGHT       1.0f

struct motorcyle_assets {
    tmesh_t* mesh;
};

typedef struct motorcyle_assets motorcyle_assets_t;

static motorcyle_assets_t assets;

static dynamic_object_type_t collider_type = {
    BOX_COLLIDER(0.3f, 0.4f, 0.8f),
    .max_stable_slope = 0.5f,
    .surface_type = SURFACE_TYPE_DEFAULT,
    .friction = 0.0f,
    .bounce = 0.1f,
    .center = {0.0f, 0.6f, 0.0f},
};

static vehicle_camera_target_t boost_positions[VEHICLE_CAM_COUNT] = {
    {
        .position = {0.0f, 1.75f, -3.0f},
        .look_at = {0.0f, 1.75f, 2.0f},
    },
    {
        .position = {0.0f, 6.5f, -5.0f},
        .look_at = {0.0f, 1.5f, 8.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
    {
        .position = {0.0f, 1.5f, -4.0f},
        .look_at = {0.0f, 1.5f, 16.0f},
    },
};

static vehicle_definiton_t vehicle_def = {
    .local_player_position = {
        .x = 0.0f,
        .y = 0.5f,
        .z = 0.5f,
    },
    .exit_position = {-1.0f, 0.0f, 0.0f},
    .camera_positions = {
        {
            .position = {0.0f, 2.0f, -6.0f},
            .look_at = {0.0f, 2.0f, 2.0f},
        },
        {
            .position = {0.0f, 15.5f, -8.0f},
            .look_at = {0.0f, 1.5f, 2.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
        {
            .position = {0.0f, 1.5f, -4.0f},
            .look_at = {0.0f, 1.5f, 16.0f},
        },
    },
    .boost_camera_positions = boost_positions,
};

static vector3_t local_cast_points[] = {
    {0.31f, 0.3f, 1.5f},
    {0.31f, 0.3f, -1.5f},
    {-0.31f, 0.3f, 1.5f},
    {-0.31f, 0.3f, -1.5f},
};

void motorcycle_common_init() {
    assets.mesh = tmesh_cache_load("rom:/meshes/vehicles/bike.tmesh");
}

void motorcycle_common_destroy() {
    tmesh_cache_release(assets.mesh);
    assets.mesh = NULL;
}

void motorcycle_ride(struct interactable* interactable, entity_id from) {

}

float motorcycle_hover_height(motorcycle_t* motorcycle, float speed) {
    if (!motorcycle->vehicle.driver) {
        return STILL_HOVER_HEIGHT;
    }
    
    float lerp = speed * (1.0f / DRIVE_SPEED);

    if (lerp > 1.0f) {
        return FAST_HOVER_HEIGHT;
    }

    return mathfLerp(RIDE_HOVER_HEIGHT, FAST_HOVER_HEIGHT, lerp);
}

void motorcycle_update(void* data) {
    motorcycle_t* motorcycle = (motorcycle_t*)data;

    float current_speed = sqrtf(vector3MagSqrd(&motorcycle->collider.velocity));

    float target_height = motorcycle_hover_height(motorcycle, current_speed) + HOVER_SAG_AMOUNT;

    vector3_t forward;

    vector2ToLookDir(&motorcycle->transform.rotation, &forward);

    float target_speed = 0.0f;
    joypad_inputs_t input = joypad_get_inputs(0);
    motorcycle->vehicle.is_boosting = input.btn.z && input.btn.a;

    if (motorcycle->vehicle.driver) {
        if (motorcycle->vehicle.is_boosting) {
            target_speed = BOOST_SPEED;
        } else if (input.btn.a) {
            target_speed = DRIVE_SPEED;
        } else if (input.btn.b) {
            target_speed = input.stick_y > -20 ? 0.0f : BACKUP_SPEED;
        } else {
            target_speed = 0.99f * sqrtf(vector3MagSqrd2D(&motorcycle->collider.velocity));
        }

        vector2_t new_rot;
        vector2_t rotation_amount;
        vector2ComplexFromAngle(fixed_time_step * MAX_TURN_RATE * input.stick_x * (1.0f / 80.0f), &rotation_amount);
        vector2ComplexMul(&motorcycle->transform.rotation, &rotation_amount, &new_rot);
        
        vector2ToLookDir(&new_rot, &forward);
        motorcycle->transform.rotation = new_rot;
    } else {
        target_speed = 0.0f;
    }

    current_speed = mathfMoveTowards(current_speed, target_speed, fixed_time_step * ACCEL_RATE);

    vector3_t target_vel;
    vector3Scale(&forward, &target_vel, target_speed);

    motorcycle->vehicle.is_stopped = vector3MagSqrd2D(&motorcycle->collider.velocity) < 0.01f && input.stick_y > -20;

    vector3_t ground_normal = (vector3_t){};
    float min_height_offset = target_height;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        cast_point_t* cast_point = &motorcycle->cast_points[i];

        if (cast_point->surface_type != SURFACE_TYPE_NONE) {
            float actual_height = cast_point->pos.y - cast_point->y;

            if (actual_height < min_height_offset) {
                min_height_offset = actual_height;
            }

            if (actual_height < target_height) {
                vector3Add(&ground_normal, &cast_point->normal, &ground_normal);
            }
        }

        vector3_t pos;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &pos);
        cast_point_set_pos(cast_point, &pos);
    }

    if (min_height_offset < target_height) {
        vector3_t* vel = &motorcycle->collider.velocity;
        vector2ToLookDir(&motorcycle->transform.rotation, vel);
        vector3Scale(vel, vel, current_speed);
        vector3Normalize(&ground_normal, &ground_normal);
        vector3ProjectPlane(vel, &ground_normal, vel);

        vel->y = vel->y * 0.8 + (target_height - min_height_offset) * fixed_time_step * HOVER_SPRING_STRENGTH;
    }
}

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id) {
    transformSaInit(&motorcycle->transform, &definition->position, &definition->rotation, 1.0f);
    render_scene_init_add_renderable(&motorcycle->renderable, &motorcycle->transform, assets.mesh, 2.0f);
    dynamic_object_init(entity_id, &motorcycle->collider, &collider_type, COLLISION_LAYER_TANGIBLE, &motorcycle->transform.position, &motorcycle->transform.rotation);
    vehicle_init(&motorcycle->vehicle, &motorcycle->transform, &vehicle_def, entity_id);
    collision_scene_add(&motorcycle->collider);
    update_add(motorcycle, motorcycle_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);

    interactable_init(&motorcycle->interactable, entity_id, INTERACT_TYPE_RIDE, motorcycle_ride, motorcycle);

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
    vehicle_destroy(&motorcycle->vehicle);
    
    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        collision_scene_remove_cast_point(&motorcycle->cast_points[i]);
    }
}
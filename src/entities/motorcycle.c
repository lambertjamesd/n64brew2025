#include "motorcycle.h"

#include "../render/tmesh.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../math/mathf.h"
#include "../config.h"
#include "../scene/scene.h"
#include "../player/inventory.h"
#include "../resource/animation_cache.h"

#define HOVER_SAG_AMOUNT        0.25f
#define HOVER_SPRING_STRENGTH   (-GRAVITY_CONSTANT / HOVER_SAG_AMOUNT)
#define STOPPED_SPEED_THESHOLD  8.0f

#define ACCEL_RATE              20.0f
#define BOOST_ACCEL_RATE        50.0f
#define DRIVE_SPEED             35.0f
#define BOOST_SPEED             60.0f
#define MAX_TURN_RATE           2.0f

#define MAX_BOOST_TURN_ACCEL    50.0f
#define MAX_TURN_ACCEL          30.0f
#define DRIFT_ACCEL             20.0f

#define TURN_BOOST_SLOW_THESHOLD    (MAX_BOOST_TURN_ACCEL / MAX_TURN_RATE)
#define TURN_SLOW_THRESHOLD         (MAX_TURN_ACCEL / MAX_TURN_RATE)

#define STILL_HOVER_HEIGHT      0.25f
#define RIDE_HOVER_HEIGHT       0.5f
#define FAST_HOVER_HEIGHT       1.0f
#define BOB_HEIGHT              0.1f
#define BOB_TIME                2.0f

#define BOOST_TIME              2.5f

struct motorcyle_assets {
    tmesh_t* mesh;
};

typedef struct motorcyle_assets motorcyle_assets_t;

static motorcyle_assets_t assets;

static dynamic_object_type_t collider_type = {
    BOX_COLLIDER(0.3f, 0.4f, 0.8f),
    .max_stable_slope = 0.0f,
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
        .y = 0.0f,
        .z = 0.0f,
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
    float bob_height = BOB_HEIGHT * cosf(game_time * 2.0f * PI_F * (1.0f / BOB_TIME));

    if (!motorcycle->vehicle.driver) {
        return STILL_HOVER_HEIGHT + bob_height;
    }
    
    float lerp = speed * (1.0f / DRIVE_SPEED);

    if (lerp > 1.0f) {
        return FAST_HOVER_HEIGHT;
    }

    return mathfLerp(RIDE_HOVER_HEIGHT + bob_height, FAST_HOVER_HEIGHT, lerp);
}

float motorcycle_target_speed(motorcycle_t* motorcycle, joypad_inputs_t input) {
    if (input.btn.a) {
        return motorcycle->boost_timer > 0.0f ? BOOST_SPEED : DRIVE_SPEED;
    } else if (input.btn.b) {
        return 0.0f;
    } else {
        return 0.99f * sqrtf(vector3MagSqrd2D(&motorcycle->collider.velocity));
    }
}

float motorycle_get_ground_height(motorcycle_t* motorcycle, float target_height, vector3_t* ground_normal) {
    float min_height_offset = target_height;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        cast_point_t* cast_point = &motorcycle->cast_points[i];

        if (cast_point->surface_type != SURFACE_TYPE_NONE) {
            float actual_height = cast_point->pos.y - cast_point->y;

            if (actual_height < min_height_offset) {
                min_height_offset = actual_height;
            }

            if (actual_height < target_height) {
                vector3Add(ground_normal, &cast_point->normal, ground_normal);
            }
        }

        vector3_t pos;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &pos);
        cast_point_set_pos(cast_point, &pos);
    }

    return min_height_offset;
}

#define MOTORCYCLE_CULL_DISTANCE    70.0f

bool motorcycle_check_active(motorcycle_t* motorcycle) {
    bool is_active = vector3DistSqrd(&motorcycle->transform.position, player_get_position(&current_scene->player)) < MOTORCYCLE_CULL_DISTANCE * MOTORCYCLE_CULL_DISTANCE;

    if (is_active && !motorcycle->is_active) {
        render_scene_init_add_renderable(&motorcycle->renderable, &motorcycle->transform, assets.mesh, 2.0f);
        collision_scene_add(&motorcycle->collider);
    } else if (!is_active && motorcycle->is_active) {
        render_scene_remove_renderable(&motorcycle->renderable);
        collision_scene_remove(&motorcycle->collider);
    }

    motorcycle->is_active = is_active;

    return is_active;
}

void motorcycle_check_for_mount(motorcycle_t* motorcycle) {
    if (inventory_has_item(ITEM_RIDING_MOTORCYCLE) && current_scene->player.state != PLAYER_IN_VEHICLE) {
        player_enter_vehicle(&current_scene->player, ENTITY_ID_MOTORCYLE);
    }
}

void motorcycle_update(void* data) {
    motorcycle_t* motorcycle = (motorcycle_t*)data;

    if (!motorcycle_check_active(motorcycle)) {
        return;
    }

    animator_update(&motorcycle->animator, &motorcycle->renderable.armature, scaled_time_step);

    float current_speed = sqrtf(vector3MagSqrd(&motorcycle->collider.velocity));

    float target_height = motorcycle_hover_height(motorcycle, current_speed) + HOVER_SAG_AMOUNT;

    vector3_t forward;

    vector2ToLookDir(&motorcycle->transform.rotation, &forward);

    joypad_inputs_t input = joypad_get_inputs(0);
    
    if (motorcycle->vehicle.hit_boost_pad) {
        motorcycle->boost_timer = BOOST_TIME;
        motorcycle->vehicle.hit_boost_pad = false;
    }

    motorcycle->vehicle.is_boosting = false;

    if (motorcycle->boost_timer > 0.0f) {
        motorcycle->boost_timer -= fixed_time_step;
        motorcycle->vehicle.is_boosting = input.btn.a;

        if (!input.btn.a) {
            motorcycle->boost_timer = 0.0f;
        }
    }

#if ENABLE_CHEATS
    if (input.btn.z) {
        motorcycle->vehicle.is_boosting = true;
    }
#endif

    if (motorcycle->vehicle.driver) {
        float accel = motorcycle->vehicle.is_boosting ? BOOST_ACCEL_RATE : ACCEL_RATE;

        float target_speed = motorcycle_target_speed(motorcycle, input);
        current_speed = mathfMoveTowards(current_speed, target_speed, fixed_time_step * ACCEL_RATE);

        float turn_rate = MAX_TURN_RATE;

        float slow_threshold = motorcycle->vehicle.is_boosting ? TURN_BOOST_SLOW_THESHOLD : TURN_SLOW_THRESHOLD;
        float turn_accel = motorcycle->vehicle.is_boosting ? MAX_BOOST_TURN_ACCEL : MAX_TURN_ACCEL;

        if (current_speed > slow_threshold) {
            turn_rate = turn_accel / current_speed;
        }

        vector2_t new_rot;
        vector2_t rotation_amount;

        vector2ComplexFromAngle(fixed_time_step * turn_rate * input.stick_x * (1.0f / 80.0f), &rotation_amount);
        vector2ComplexMul(&motorcycle->transform.rotation, &rotation_amount, &new_rot);
        
        vector2ToLookDir(&new_rot, &forward);
        motorcycle->transform.rotation = new_rot;
    } else {
        current_speed = mathfMoveTowards(current_speed, 0.0f, fixed_time_step * ACCEL_RATE);
    }

    motorcycle->vehicle.is_stopped = vector3MagSqrd2D(&motorcycle->collider.velocity) < STOPPED_SPEED_THESHOLD * STOPPED_SPEED_THESHOLD;

    vector3_t ground_normal = (vector3_t){};
    float min_height_offset = motorycle_get_ground_height(motorcycle, target_height, &ground_normal);

    if (motorcycle->collider.hit_kill_plane) {
        motorcycle->collider.velocity = gZeroVec;
        motorcycle->transform.position = motorcycle->last_ground_location;
        motorcycle->collider.hit_kill_plane = 0;
        motorcycle_check_for_mount(motorcycle);
    }

    if (min_height_offset < target_height) {
        vector3_t* vel = &motorcycle->collider.velocity;

        float prev_y = vel->y;

        vector3_t target_vel;
        vector2ToLookDir(&motorcycle->transform.rotation, &target_vel);
        vector3Normalize(&ground_normal, &ground_normal);
        vector3ProjectPlane(&target_vel, &ground_normal, &target_vel);
        vector3Normalize(&target_vel, &target_vel);
        vector3Scale(&target_vel, &target_vel, current_speed);
        motorcycle->last_ground_location = motorcycle->transform.position;

        float max_accel = motorcycle->has_traction ? MAX_TURN_ACCEL : DRIFT_ACCEL;

        vector3_t accel;
        vector3Sub(&target_vel, vel, &accel);

        motorcycle->has_traction = vector3MoveTowards(vel, &target_vel, 2.0f * max_accel * scaled_time_step, vel);
        
        if (vector3Dot(&ground_normal, vel) < 0.0f) {
            vel->y = vel->y * 0.8 + (target_height - min_height_offset) * fixed_time_step * HOVER_SPRING_STRENGTH;
        } else {
            vel->y = prev_y;
        }
    }
}

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id) {
    transformSaInit(&motorcycle->transform, &definition->position, &definition->rotation, 1.0f);
    render_scene_init_add_renderable(&motorcycle->renderable, &motorcycle->transform, assets.mesh, 2.0f);
    dynamic_object_init(entity_id, &motorcycle->collider, &collider_type, COLLISION_LAYER_TANGIBLE, &motorcycle->transform.position, &motorcycle->transform.rotation);
    animator_init(&motorcycle->animator, motorcycle->renderable.armature.bone_count);
    motorcycle->animations = animation_cache_load("rom:/meshes/vehicles/bike.anim");
    vehicle_init(&motorcycle->vehicle, &motorcycle->transform, &vehicle_def, entity_id);
    collision_scene_add(&motorcycle->collider);
    update_add(motorcycle, motorcycle_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    
    interactable_init(&motorcycle->interactable, entity_id, INTERACT_TYPE_RIDE, motorcycle_ride, motorcycle);
    
    motorcycle->has_traction = true;
    motorcycle->is_active = true;
    motorcycle->boost_timer = 0.0f;
    motorcycle->last_ground_location = definition->position;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        vector3_t cast_point;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &cast_point);
        collision_scene_add_cast_point(&motorcycle->cast_points[i], &cast_point);
    }

    motorcycle_check_for_mount(motorcycle);
    animator_run_clip(&motorcycle->animator, animation_set_find_clip(motorcycle->animations, "idle"), 0.0f, true);
    drop_shadow_init(&motorcycle->drop_shadow, &motorcycle->collider, "rom:/meshes/effects/bike-shadow.tmesh");
}

void motorcycle_destroy(motorcycle_t* motorcycle) {
    if (motorcycle->is_active) {
        render_scene_remove_renderable(&motorcycle->renderable);
        collision_scene_remove(&motorcycle->collider);
    }
    interactable_destroy(&motorcycle->interactable);
    update_remove(motorcycle);
    vehicle_destroy(&motorcycle->vehicle);
    animator_destroy(&motorcycle->animator);
    animation_cache_release(motorcycle->animations);
    drop_shadow_destroy(&motorcycle->drop_shadow);
    
    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        collision_scene_remove_cast_point(&motorcycle->cast_points[i]);
    }
}
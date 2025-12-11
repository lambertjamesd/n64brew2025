#include "camera_controller.h"

#include "../time/time.h"
#include "../math/transform_single_axis.h"
#include "../physics/move_towards.h"
#include "../math/mathf.h"
#include "../math/constants.h"
#include "../render/defs.h"
#include "../entity/interactable.h"
#include <math.h>

static struct move_towards_parameters camera_move_parameters = {
    .max_speed = 40.0f,
    .max_accel = 30.0f,
};

#define ASPECT_RATIO            (4.0f/3.0f)
#define OVER_SHOULDER_DISTANCE  1.1f
#define EXTEND_SPEED            2.0f

#define MIN_TWO_TARGET_DISTANCE 

void camera_cached_calcuations_check(struct camera_cached_calcuations* cache, struct Camera* camera) {
    if (camera->fov == cache->fov) {
        return;
    }

    float height = tanf(camera->fov * 0.5f);
    float width = ASPECT_RATIO * height;

    cache->fov_horz = atanf(width) * 2.0f;
    cache->cos_1_3_fov_horz = cosf(cache->fov_horz * 0.33333f);
    cache->sin_1_3_fov_horz = sinf(cache->fov_horz * 0.33333f);
}

void camera_look_at_from_rotation(struct camera_controller* controller) {
    quatMultVector(&controller->camera->transform.rotation, &gForward, &controller->looking_at);
    vector3AddScaled(&controller->stable_position, &controller->looking_at, -CAMERA_FOLLOW_DISTANCE, &controller->looking_at);
}

#define MAX_SIN_ANGLE DEFAULT_CAMERA_COS_FOV_2
#define MAX_COS_ANGLE DEFAULT_CAMERA_SIN_FOV_2

void camera_controller_direct_target(struct camera_controller* controller, struct Vector3* target) {
    struct Vector3* player_pos = &controller->player->cutscene_actor.transform.position;
    struct Vector3 offset;
    vector3Sub(player_pos, target, &offset);

    offset.y = 0.0f;
    vector3Normalize(&offset, &offset);

    vector3AddScaled(player_pos, &offset, CAMERA_FOLLOW_DISTANCE, &controller->target);
    controller->target.y += CAMERA_FOLLOW_HEIGHT;

    move_towards(&controller->looking_at, &controller->looking_at_speed, target, &camera_move_parameters);
}

void camera_controller_watch_target(struct camera_controller* controller, struct Vector3* target) {
    struct Vector3* camera_pos = &controller->stable_position;
    struct Vector3* player_pos = player_get_position(controller->player);

    struct Vector2 player_to_target = {
        .x = target->x - player_pos->x,
        .y = target->z - player_pos->z,
    };

    float target_distance = sqrtf(vector2MagSqr(&player_to_target));
    float target_distance_inv = 1.0f / target_distance;

    struct Vector2 rotation_amount = {
        .y = DEFAULT_CAMERA_SIN_TRI_CORNER * CAMERA_FOLLOW_DISTANCE * target_distance_inv
    };

    float follow_distance = CAMERA_FOLLOW_DISTANCE;

    if (rotation_amount.y > MAX_SIN_ANGLE) {
        rotation_amount.x = MAX_COS_ANGLE;
        rotation_amount.y = MAX_SIN_ANGLE;
        // follow_distance = target_distance * (MAX_SIN_ANGLE / DEFAULT_CAMERA_SIN_TRI_CORNER);
    } else {
        rotation_amount.x = sqrtf(1.0f - rotation_amount.y * rotation_amount.y);
    }
    rotation_amount.y = -rotation_amount.y;

    struct Vector2 player_to_cam = {
        .x = camera_pos->x - player_pos->x,
        .y = camera_pos->z - player_pos->z,
    };

    player_to_target.x *= target_distance_inv;
    player_to_target.y *= target_distance_inv;

    struct Vector2 camera_offset = {
        .x = DEFAULT_CAMERA_SIN_FOV_6 * follow_distance,
        .y = DEFAULT_CAMERA_COS_FOV_6 * follow_distance,
    };

    if (vector2Cross(&player_to_cam, &player_to_target) > 0.0f) {
        rotation_amount.y = -rotation_amount.y;
        camera_offset.x = -camera_offset.x;
    }

    struct Vector2 camera_rotation;
    vector2ComplexMul(&player_to_target, &rotation_amount, &camera_rotation);
    vector2Rotate90(&camera_rotation, &camera_rotation);


    struct Vector2 rotated_camera_offset;
    vector2ComplexMul(&camera_rotation, &camera_offset, &rotated_camera_offset);

    controller->target = (struct Vector3){
        .x = player_pos->x + rotated_camera_offset.x,
        .y = player_pos->y + CAMERA_FOLLOW_HEIGHT,
        .z = player_pos->z + rotated_camera_offset.y,
    };
    struct Vector3 looking_at = {
        .x = controller->target.x + camera_rotation.y * CAMERA_FOLLOW_DISTANCE,
        .y = player_pos->y + CAMERA_FOLLOW_HEIGHT,
        .z = controller->target.z - camera_rotation.x * CAMERA_FOLLOW_DISTANCE,
    };

    if (target->y < player_pos->y || target->y > player_pos->y + CAMERA_FOLLOW_HEIGHT) {        
        struct Vector3 offset;
        vector3Sub(target, player_pos, &offset);

        if (target->y > player_pos->y) {
            offset.y -= CAMERA_FOLLOW_HEIGHT;
        }

        float distance = sqrtf(vector3MagSqrd2D(&offset));

        if (distance > 0.001f) {
            float slope = offset.y / distance;
            controller->target.y -= slope * CAMERA_FOLLOW_DISTANCE;

            vector3Sub(&controller->target, player_pos, &offset);
            offset.y -= CAMERA_FOLLOW_HEIGHT;

            vector3Normalize(&offset, &offset);

            float followDistance = offset.y < 0.0 ? mathfLerp(CAMERA_FOLLOW_DISTANCE, OVER_SHOULDER_DISTANCE, -offset.y) : CAMERA_FOLLOW_DISTANCE;

            vector3AddScaled(player_pos, &offset, followDistance, &controller->target);
            controller->target.y += CAMERA_FOLLOW_HEIGHT;
        }
    }

    move_towards(&controller->looking_at, &controller->looking_at_speed, &looking_at, &camera_move_parameters);
}

void camera_controller_determine_player_move_target(struct camera_controller* controller, struct Vector3* result, bool behind_player) {
    struct Vector3 offset;
    struct Vector3* player_pos = player_get_position(controller->player);

    if (behind_player) {
        struct Quaternion quat;
        quatAxisComplex(&gUp, &controller->player->cutscene_actor.transform.rotation, &quat);
        quatMultVector(&quat, &gForward, &offset);
    } else {
        vector3Sub(player_pos, &controller->stable_position, &offset);

        offset.y = 0.0f;
        vector3Normalize(&offset, &offset);

        if (vector3MagSqrd(&offset) < 0.1f) {
            offset = gForward;
        }
    }

    float clamped_distance = controller->wall_checker.actual_distance + EXTEND_SPEED * fixed_time_step;

    if (clamped_distance < CAMERA_FOLLOW_DISTANCE) {
        vector3AddScaled(player_pos, &offset, -clamped_distance, result);
    } else {
        vector3AddScaled(player_pos, &offset, -CAMERA_FOLLOW_DISTANCE, result);
    }

    result->y += CAMERA_FOLLOW_HEIGHT;
    struct Vector3 looking_at = *player_pos;
    looking_at.y += CAMERA_FOLLOW_HEIGHT;
    move_towards(&controller->looking_at, &controller->looking_at_speed, &looking_at, &camera_move_parameters);
}

void camera_controller_update_position(struct camera_controller* controller, struct TransformSingleAxis* target) {
    move_towards(&controller->stable_position, &controller->speed, &controller->target, &camera_move_parameters);

    struct Vector3 offset;
    vector3Sub(&controller->looking_at, &controller->stable_position, &offset);
    quatLook(&offset, &gUp, &controller->camera->transform.rotation);
}

void camera_controller_return_target(struct camera_controller* controller, struct Vector3* target) {
    struct Vector3 offset;
    struct Transform* cam_transform = &controller->camera->transform;

    quatMultVector(&cam_transform->rotation, &gForward, &offset);
    offset.y = 0.0f;
    vector3Scale(&offset, &offset, -1.0f);
    vector3Normalize(&offset, &offset);
    vector3AddScaled(player_get_position(controller->player), &offset, -CAMERA_FOLLOW_DISTANCE, target);
    target->y += CAMERA_FOLLOW_HEIGHT;

    move_towards(&controller->stable_position, &controller->speed, &controller->target, &camera_move_parameters);
    camera_look_at_from_rotation(controller);

    controller->camera->fov = mathfMoveTowards(controller->camera->fov, 70.0f, 20.0f * fixed_time_step);

    if (vector3DistSqrd(target, &controller->stable_position) < 0.0001f) {
        controller->state = CAMERA_STATE_FOLLOW;
        controller->camera->fov = 70.0f;
    }
}

void camera_controller_move_to(struct camera_controller* controller) {
    if (controller->state_data.move_to.moving_look_at) {
        if (move_towards(&controller->looking_at, &controller->looking_at_speed, &controller->look_target, &camera_move_parameters)) {
            controller->state_data.move_to.moving_look_at = false;
        }
    }

    if (controller->state_data.move_to.moving_position) {
        if (move_towards(&controller->stable_position, &controller->speed, &controller->target, &camera_move_parameters)) {
            controller->state_data.move_to.moving_position = false;
        }
    }
    
    struct Vector3 offset;
    vector3Sub(&controller->looking_at, &controller->stable_position, &offset);
    quatLook(&offset, &gUp, &controller->camera->transform.rotation);
}

static struct camera_animation_frame __attribute__((aligned(16))) anim_frame_buffer;

void camera_controller_update_animation(struct camera_controller* controller) {
    struct camera_animation* animation = controller->state_data.animate.animation;

    if (!animation || controller->state_data.animate.current_frame >= animation->frame_count) {
        return;
    }

    data_cache_hit_invalidate((void*)&anim_frame_buffer, 16);
    dma_read(
        &anim_frame_buffer, 
        animation->rom_offset + sizeof(struct camera_animation_frame) * controller->state_data.animate.current_frame, 
        sizeof(struct camera_animation_frame)
    );

    controller->stable_position = anim_frame_buffer.position;
    controller->camera->transform.rotation.x = anim_frame_buffer.rotation[0];
    controller->camera->transform.rotation.y = anim_frame_buffer.rotation[1];
    controller->camera->transform.rotation.z = anim_frame_buffer.rotation[2];

    float neg_w = controller->camera->transform.rotation.x * controller->camera->transform.rotation.x 
        + controller->camera->transform.rotation.y * controller->camera->transform.rotation.y 
        + controller->camera->transform.rotation.z * controller->camera->transform.rotation.z;

    if (neg_w > 1.0f) {
        neg_w = 1.0f;
    }

    controller->camera->transform.rotation.w = sqrtf(1.0f - neg_w);
    controller->camera->fov = (180.0f / M_PI) * anim_frame_buffer.fov;
    controller->state_data.animate.current_frame += 1;
    camera_look_at_from_rotation(controller);
}

#define MAX_NEAR_PLANE  1.2f
#define MIN_NEAR_PLANE  0.3f
#define PLAYER_CLIP_RADIUS 0.25f

void camera_controller_determine_near_plane(struct camera_controller* controller) {
    vector3_t offset;
    vector3Sub(player_get_position(controller->player), &controller->camera->transform.position, &offset);

    float target_near_plane = sqrtf(vector3MagSqrd2D(&offset)) - PLAYER_CLIP_RADIUS;

    camera_set_near(controller->camera, clampf(target_near_plane, MIN_NEAR_PLANE, MAX_NEAR_PLANE));
}

void camera_controller_update(struct camera_controller* controller) {
    switch (controller->state) {
        case CAMERA_STATE_FOLLOW: {
            camera_controller_determine_player_move_target(controller, &controller->target, joypad_get_buttons_held(0).z);
            camera_controller_update_position(controller, &controller->player->cutscene_actor.transform);
            break;
        }
        case CAMERA_STATE_LOOK_AT_WITH_PLAYER:
            camera_controller_watch_target(controller, &controller->look_target);
            camera_controller_update_position(controller, &controller->player->cutscene_actor.transform);
            break;
        case CAMERA_STATE_ANIMATE:
            camera_controller_update_animation(controller);
            break;
        case CAMERA_STATE_RETURN_TO_PLAYER:
            camera_controller_return_target(controller, &controller->target);
            break;
        case CAMERA_STATE_FIXED:
            // empty
            break;
        case CAMERA_STATE_MOVE_TO:
            camera_controller_move_to(controller);
            break;
    }

    camera_wall_checker_update(&controller->wall_checker, &controller->looking_at, &controller->target);

    vector3AddScaled(&controller->shake_velocity, &controller->shake_offset, -50.0f, &controller->shake_velocity);
    vector3AddScaled(&controller->shake_offset, &controller->shake_velocity, fixed_time_step, &controller->shake_offset);
    vector3Scale(&controller->shake_velocity, &controller->shake_velocity, 0.5f);
    vector3Add(&controller->stable_position, &controller->shake_offset, &controller->camera->transform.position);

    camera_controller_determine_near_plane(controller);
}

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player) {
    controller->camera = camera;
    controller->player = player;
    controller->state = CAMERA_STATE_FOLLOW;

    update_add(controller, (update_callback)camera_controller_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    controller->speed = 0.0f;
    controller->looking_at = player->cutscene_actor.transform.position;
    controller->looking_at_speed = 0.0f;
    controller->look_target = gZeroVec;
    camera_controller_determine_player_move_target(controller, &controller->target, true);
    controller->follow_distace = 3.0f;
    controller->shake_offset = gZeroVec;
    controller->shake_velocity = gZeroVec;

    controller->camera->transform.position = controller->target;
    controller->stable_position = controller->target;
    controller->camera->transform.scale = gOneVec;
    controller->_cache_calcluations.fov = 0.0f;
    quatAxisAngle(&gRight, 0.0f, &controller->camera->transform.rotation);

    camera_controller_update_position(controller, &player->cutscene_actor.transform);

    controller->state_data.animate.animation = NULL;
    controller->state_data.animate.current_frame = 0;

    camera_wall_checker_init(&controller->wall_checker);
}

void camera_controller_destroy(struct camera_controller* controller) {
    update_remove(controller);
    camera_wall_checker_destroy(&controller->wall_checker);
}

void camera_look_at(struct camera_controller* controller, struct Vector3* target) {
    controller->look_target = *target;
    controller->state = CAMERA_STATE_LOOK_AT_WITH_PLAYER;
    controller->camera->fov = 70.0f;
}

void camera_follow_player(struct camera_controller* controller) {
    controller->state = CAMERA_STATE_FOLLOW;
    controller->camera->fov = 70.0f;
}

void camera_return(struct camera_controller* controller) {
    controller->state = CAMERA_STATE_RETURN_TO_PLAYER;
    controller->speed = 0.0f;
}

void camera_play_animation(struct camera_controller* controller, struct camera_animation* animation) {
    controller->state = CAMERA_STATE_ANIMATE;
    controller->state_data.animate.animation = animation;
    controller->state_data.animate.current_frame = 0;
}

void camera_move_to(struct camera_controller* controller, struct Vector3* position, bool instant, bool move_target) {
    controller->state = CAMERA_STATE_MOVE_TO;
    if (move_target) {
        controller->look_target = *position;
        controller->state_data.move_to.moving_look_at = !instant;
        if (instant) {
            controller->looking_at = *position;
        }
    } else {
        controller->target = *position;
        controller->state_data.move_to.moving_position = !instant;
        if (instant) {
            controller->stable_position = *position;
        }
    }
}

void camera_set_fixed(struct camera_controller* controller, struct Vector3* position, struct Quaternion* rotation, float fov) {
    controller->state = CAMERA_STATE_FIXED;
    controller->camera->transform.position = *position;
    controller->stable_position = *position;
    controller->camera->transform.rotation = *rotation;
    controller->camera->fov = fov;
}

bool camera_is_animating(struct camera_controller* controller) {
    switch (controller->state) {
        case CAMERA_STATE_ANIMATE:
            return controller->state_data.animate.animation != 0 && 
                controller->state_data.animate.current_frame < controller->state_data.animate.animation->frame_count;
        case CAMERA_STATE_MOVE_TO:
            return controller->state_data.move_to.moving_look_at || controller->state_data.move_to.moving_position;
        default:
            return false;
    }
}

void camera_shake(struct camera_controller* controller, float strength) {
    controller->shake_offset.x += randomInRangef(-strength, strength);
    controller->shake_offset.y += randomInRangef(-strength, strength);
    controller->shake_offset.z += randomInRangef(-strength, strength);
}
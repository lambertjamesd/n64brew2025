#ifndef __SCENE_CAMERA_CONTROLLER_H__
#define __SCENE_CAMERA_CONTROLLER_H__

#include "../render/camera.h"
#include "../player/player.h"
#include "camera_animation.h"
#include "camera_wall_checker.h"

#define CAMERA_FOLLOW_DISTANCE  3.4f
#define CAMERA_FOLLOW_HEIGHT    1.6f

enum camera_controller_state {
    CAMERA_STATE_FOLLOW,
    CAMERA_STATE_LOOK_AT_WITH_PLAYER,
    CAMERA_STATE_ANIMATE,
    CAMERA_STATE_RETURN_TO_PLAYER,
    CAMERA_STATE_FIXED,
    CAMERA_STATE_MOVE_TO,
};

struct camera_cached_calcuations {
    float fov;
    float fov_horz;
    float cos_1_3_fov_horz;
    float sin_1_3_fov_horz;
};

union camera_controller_state_data {
    struct {
        struct camera_animation* animation;
        uint16_t current_frame;
    } animate;
    struct {
        bool moving_position;
        bool moving_look_at;
    } move_to;
};

struct camera_controller {
    struct Camera* camera;
    struct Vector3 stable_position;
    struct player* player;
    struct camera_cached_calcuations _cache_calcluations;
    float follow_distace;
    struct Vector3 target;
    float speed;
    struct Vector3 looking_at;
    float looking_at_speed;
    struct Vector3 look_target;
    struct Vector3 shake_offset;
    struct Vector3 shake_velocity;
    camera_wall_checker_t wall_checker;
    enum camera_controller_state state;
    union camera_controller_state_data state_data;
};

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player);

void camera_controller_destroy(struct camera_controller* controller);

void camera_look_at(struct camera_controller* controller, struct Vector3* target);
void camera_follow_player(struct camera_controller* controller);
void camera_return(struct camera_controller* controller);
void camera_play_animation(struct camera_controller* controller, struct camera_animation* animation);
void camera_move_to(struct camera_controller* controller, struct Vector3* position, bool instant, bool move_target);
void camera_set_fixed(struct camera_controller* controller, struct Vector3* position, struct Quaternion* rotation, float fov);

bool camera_is_animating(struct camera_controller* controller);

void camera_shake(struct camera_controller* controller, float strength);

#endif
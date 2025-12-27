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
    CAMERA_FOLLOW_VEHICLE,
};

struct camera_cached_calcuations {
    float fov;
    float fov_horz;
    float cos_1_3_fov_horz;
    float sin_1_3_fov_horz;
};

/**
 * stores variables only needed for a single state
 * the data will be lost when changing states so
 * if you need some data to persist after changing
 * states just store it directly on the camera_controller
 * struct
 */
union camera_controller_state_data {
    struct {
        float horizontal_velocity;
        float vertical_angle;
        float vertical_angle_vel;
    } follow;
    struct {
        struct camera_animation* animation;
        uint16_t current_frame;
    } animate;
    struct {
        bool moving_position;
        bool moving_look_at;
    } move_to;
    struct {
        vector3_t last_vehicle_pos;
        uint8_t shake_timer;
    } follow_vehicle;
};

/**
 * The camera controller can be in a few different states
 * based on the enum camera_controller_state. Each state
 * will give different camera behavior.
 * 
 * The state is responsible for setting the the following
 * variables as part of controlling the camera
 * stable_position - set the position of the camera pre camera shake
 * camera->transform.rotation - sets the rotation of the camera
 * camera->fov - sets the field of view of the camera
 * 
 * The common code in the update function will automatically apply
 * camera shake based on the stable position
 * 
 * there are some variables that may be useful
 * target - used to store where the camera wants to move towards
 * looking_at - used to indicate what the camera will look at
 * looking_at_target - used to indicate where the camera wants to look at
 * speed - how fast the camera is currently moving towards target
 * 
 * calling camera_controller_update_position will move the camera position
 * towards target and rotate the camera to look directly at looking_at
 */
struct camera_controller {
    struct Camera* camera;
    struct Vector3 stable_position;
    struct player* player;
    float follow_distace;
    struct Vector3 target;
    float speed;
    struct Vector3 looking_at;
    float looking_at_speed;
    struct Vector3 look_target;
    struct Vector3 shake_offset;
    struct Vector3 shake_velocity;
    // checks if something is between the player and the camera
    camera_wall_checker_t wall_checker;
    enum camera_controller_state state;
    union camera_controller_state_data state_data;
};

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player);

void camera_controller_destroy(struct camera_controller* controller);

void camera_look_at(struct camera_controller* controller, struct Vector3* target);
void camera_follow_player(struct camera_controller* controller);
void camera_return(struct camera_controller* controller);
void camera_follow_vehicle(struct camera_controller* controller);
void camera_play_animation(struct camera_controller* controller, struct camera_animation* animation);
void camera_move_to(struct camera_controller* controller, struct Vector3* position, bool instant, bool move_target);
void camera_set_fixed(struct camera_controller* controller, struct Vector3* position, struct Quaternion* rotation, float fov);

bool camera_is_animating(struct camera_controller* controller);

void camera_shake(struct camera_controller* controller, float strength);

#endif
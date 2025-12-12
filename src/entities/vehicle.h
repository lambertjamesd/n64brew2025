#ifndef __ENTITIES_VEHICLE_H__
#define __ENTITIES_VEHICLE_H__

#include <libdragon.h>
#include "../math/transform_single_axis.h"
#include "../entity/entity_id.h"

struct vehicle_camera_target {
    vector3_t position;
    vector3_t look_at;
};

typedef struct vehicle_camera_target vehicle_camera_target_t;

enum vehicle_cam {
    VEHICLE_CAM_NEUTRAL,
    VEHICLE_CAM_U,
    VEHICLE_CAM_UR,
    VEHICLE_CAM_R,
    VEHICLE_CAM_DR,
    VEHICLE_CAM_D,
    VEHICLE_CAM_DL,
    VEHICLE_CAM_L,
    VEHICLE_CAM_UL,
    
    VEHICLE_CAM_COUNT,
};

typedef enum vehicle_cam vehicle_cam_t;

struct vehicle_definiton {
    vector3_t local_player_position;
    vector3_t exit_position;

    vehicle_camera_target_t camera_positions[VEHICLE_CAM_COUNT];
    vehicle_camera_target_t* boost_camera_positions;
};

typedef struct vehicle_definiton vehicle_definiton_t;

struct vehicle {
    transform_sa_t* transform;
    vehicle_definiton_t* def;
    entity_id id;
    entity_id driver;

    vector3_t last_input_direction;
    bool is_stopped: 1;
    bool is_boosting: 1;
};

typedef struct vehicle vehicle_t;

void vehicle_global_reset();
void vehicle_init(vehicle_t* vehicle, transform_sa_t* transform, vehicle_definiton_t* def, entity_id id);
void vehicle_destroy(vehicle_t* vehicle);

void vehicle_enter(vehicle_t* vehicle, entity_id driver);
void vehicle_steer(vehicle_t* vehicle, vector3_t* input_direction);
void vehicle_exit(vehicle_t* vehicle);

void vehicle_apply_driver_transform(vehicle_t* vehicle, transform_sa_t* driver_transform);
void vehicle_apply_exit_transform(vehicle_t* vehicle, transform_sa_t* driver_transform);

void vehicle_get_camera_target(vehicle_t* vehicle, joypad_buttons_t held, vehicle_camera_target_t* result); 

vehicle_t* vehicle_get(entity_id id);

#endif
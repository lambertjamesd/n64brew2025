#include "vehicle.h"

#include "../util/hash_map.h"
static struct hash_map entity_mapping;

void vehicle_global_reset() {
    hash_map_destroy(&entity_mapping);
    hash_map_init(&entity_mapping, 4);
}

void vehicle_init(vehicle_t* vehicle, transform_sa_t* transform, vehicle_definiton_t* def, entity_id id) {
    vehicle->transform = transform;
    vehicle->def = def;
    vehicle->id = id;
    vehicle->last_input_direction = gZeroVec;
    vehicle->is_stopped = true;
    vehicle->is_boosting = false;
    vehicle->driver = 0;

    hash_map_set(&entity_mapping, id, vehicle);
}

void vehicle_destroy(vehicle_t* vehicle) {
    hash_map_delete(&entity_mapping, vehicle->id);
}

void vehicle_enter(vehicle_t* vehicle, entity_id driver) {
    vehicle->driver = driver;
}

void vehicle_steer(vehicle_t* vehicle, vector3_t* input_direction) {
    vehicle->last_input_direction = *input_direction;
}

void vehicle_exit(vehicle_t* vehicle) {
    vehicle->driver = 0;
    vehicle->last_input_direction = gZeroVec;
}

void vehicle_apply_driver_transform(vehicle_t* vehicle, transform_sa_t* driver_transform) {
    transformSaTransformPoint(vehicle->transform, &vehicle->def->local_player_position, &driver_transform->position);
    driver_transform->rotation = vehicle->transform->rotation;
}

void vehicle_apply_exit_transform(vehicle_t* vehicle, transform_sa_t* driver_transform) {
    transformSaTransformPoint(vehicle->transform, &vehicle->def->exit_position, &driver_transform->position);
}

vehicle_t* vehicle_get(entity_id id) {
    return hash_map_get(&entity_mapping, id);
}

#define PACK_BUTTONS(u, r, d, l)    (((u) << 3) | ((r) << 2) | ((d) << 1) | (l))

static vehicle_cam_t button_to_pos[] = {
    [PACK_BUTTONS(0, 0, 0, 0)] = VEHICLE_CAM_NEUTRAL,
    [PACK_BUTTONS(0, 0, 0, 1)] = VEHICLE_CAM_L,
    [PACK_BUTTONS(0, 0, 1, 0)] = VEHICLE_CAM_D,
    [PACK_BUTTONS(0, 0, 1, 1)] = VEHICLE_CAM_DL,
    [PACK_BUTTONS(0, 1, 0, 0)] = VEHICLE_CAM_R,
    [PACK_BUTTONS(0, 1, 0, 1)] = VEHICLE_CAM_NEUTRAL,
    [PACK_BUTTONS(0, 1, 1, 0)] = VEHICLE_CAM_DR,
    [PACK_BUTTONS(0, 1, 1, 1)] = VEHICLE_CAM_D,
    [PACK_BUTTONS(1, 0, 0, 0)] = VEHICLE_CAM_U,
    [PACK_BUTTONS(1, 0, 0, 1)] = VEHICLE_CAM_UL,
    [PACK_BUTTONS(1, 0, 1, 0)] = VEHICLE_CAM_NEUTRAL,
    [PACK_BUTTONS(1, 0, 1, 1)] = VEHICLE_CAM_L,
    [PACK_BUTTONS(1, 1, 0, 0)] = VEHICLE_CAM_UR,
    [PACK_BUTTONS(1, 1, 0, 1)] = VEHICLE_CAM_U,
    [PACK_BUTTONS(1, 1, 1, 0)] = VEHICLE_CAM_R,
    [PACK_BUTTONS(1, 1, 1, 1)] = VEHICLE_CAM_NEUTRAL,
};

void vehicle_get_camera_target(vehicle_t* vehicle, joypad_buttons_t held, vehicle_camera_target_t* result) {
    int index = button_to_pos[PACK_BUTTONS(held.c_up, held.c_right, held.c_down, held.c_left)];
    vehicle_camera_target_t* local_pos = NULL;


    if (vehicle->is_boosting && vehicle->def->boost_camera_positions) {
        local_pos = &vehicle->def->boost_camera_positions[index];
    } else {
        local_pos = &vehicle->def->camera_positions[index];
    }

    transformSaTransformPoint(vehicle->transform, &local_pos->look_at, &result->look_at);
    transformSaTransformPoint(vehicle->transform, &local_pos->position, &result->position);
}
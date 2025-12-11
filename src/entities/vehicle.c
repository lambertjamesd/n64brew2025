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
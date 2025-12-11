#ifndef __ENTITIES_VEHICLE_H__
#define __ENTITIES_VEHICLE_H__

#include "../math/transform_single_axis.h"
#include "../entity/entity_id.h"

struct vehicle_definiton {
    vector3_t local_player_position;
    vector3_t exit_position;
};

typedef struct vehicle_definiton vehicle_definiton_t;

struct vehicle {
    transform_sa_t* transform;
    vehicle_definiton_t* def;
    entity_id id;
    entity_id driver;

    vector3_t last_input_direction;
    bool is_stopped;
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

vehicle_t* vehicle_get(entity_id id);

#endif
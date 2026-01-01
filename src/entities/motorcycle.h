#ifndef __ENTITIES_MOTORCYCLE_H__
#define __ENTITIES_MOTORCYCLE_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"
#include "../collision/dynamic_object.h"
#include "../entity/interactable.h"
#include "../collision/cast_point.h"
#include "vehicle.h"

#define CAST_POINT_COUNT 4

struct motorcycle {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    interactable_t interactable; 
    vehicle_t vehicle;
    bool has_traction;

    cast_point_t cast_points[CAST_POINT_COUNT];
};

typedef struct motorcycle motorcycle_t;

void motorcycle_common_init();
void motorcycle_common_destroy();

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id);
void motorcycle_destroy(motorcycle_t* motorcycle);


#endif
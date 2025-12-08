#ifndef __OBJECTS_TRIGGER_CUBE_H__
#define __OBJECTS_TRIGGER_CUBE_H__

#include "../math/transform_single_axis.h"
#include "../collision/spatial_trigger.h"
#include "../scene/scene_definition.h"

struct trigger_cube {
    transform_sa_t transform;
    spatial_trigger_type_t type;
    spatial_trigger_t trigger;
};

typedef struct trigger_cube trigger_cube_t;

void trigger_cube_common_init();
void trigger_cube_common_destroy();

void trigger_cube_init(trigger_cube_t* cube, struct trigger_cube_definition* definition, entity_id entity_id);
void trigger_cube_destroy(trigger_cube_t* cube);

#endif
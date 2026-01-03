#ifndef __ENTITIES_BOOST_PAD_H__
#define __ENTITIES_BOOST_PAD_H__

#include "../math/vector3.h"
#include "../math/transform.h"
#include "../math/transform_single_axis.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../entity/interactable.h"

struct boost_pad {
    transform_t transform;
    transform_sa_t transform_sa;
    dynamic_object_t collider;
    spatial_trigger_t boost_trigger;
    interactable_t interactable;
    bool is_on;
};

typedef struct boost_pad boost_pad_t;

void boost_pad_init(boost_pad_t* boost_pad, struct boost_pad_definition* definition, entity_id entity_id);
void boost_pad_destroy(boost_pad_t* boost_pad, struct boost_pad_definition* definition);
void boost_pad_common_init();
void boost_pad_common_destroy();

#endif
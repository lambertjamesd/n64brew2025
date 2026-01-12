#ifndef __OBJECTS_SIGN_H__
#define __OBJECTS_SIGN_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../entity/interactable.h"
#include "../collision/dynamic_object.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_reference.h"

struct sign {
    struct TransformSingleAxis transform;
    renderable_t renderable; 
    interactable_t interactable;
    dynamic_object_t dynamic_object;
    cutscene_ref_t cutscene;
    entity_id entity_id;
};

typedef struct sign sign_t;

void sign_init(sign_t* sign, struct sign_definition* def, entity_id entity_id);
void sign_destroy(sign_t* sign);
void sign_common_init();
void sign_common_destroy();

#endif
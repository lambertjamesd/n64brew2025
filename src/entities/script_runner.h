#ifndef __ENTITY_SCRIPT_RUNNER_H__
#define __ENTITY_SCRIPT_RUNNER_H__

#include "../scene/scene_definition.h"
#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../cutscene/cutscene_reference.h"

struct script_runner {
    struct Vector3 position;
    cutscene_ref_t cutscene;
    bool loop;
    bool should_run;
};

void script_runner_init(struct script_runner* script_runner, struct script_runner_definition* definiton, entity_id id);
void script_runner_destroy(struct script_runner* script_runner);
void script_runner_common_init();
void script_runner_common_destroy();

#endif
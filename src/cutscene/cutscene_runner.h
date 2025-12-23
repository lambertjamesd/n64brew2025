#ifndef __CUTSCENE_CUTSCENE_RUNNER_H__
#define __CUTSCENE_CUTSCENE_RUNNER_H__

#include "cutscene.h"
#include "../scene/scene_definition.h"
#include <stdbool.h>
#include "../entity/entity_id.h"

typedef void (*cutscene_finish_callback)(struct cutscene* cutscene, void* data);

void cutscene_runner_init();

void cutscene_runner_run(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject);
bool cutscene_runner_is_running();

void cutscene_runner_cancel(struct cutscene* cutscene);

// this is only a function that returns a callback to help balance out the pairing_checker
cutscene_finish_callback cutscene_runner_free_on_finish();

#endif
#include "script_runner.h"

#include "../time/time.h"

void script_runner_finish(struct cutscene* cutscene, void* data) {
    struct script_runner* script_runner = (struct script_runner*)data;

    if (script_runner->loop) {
        script_runner->should_run = true;
    }
}

void script_runner_update(void* data) {
    struct script_runner* script_runner = (struct script_runner*)data;
    if (script_runner->should_run) {
        script_runner->should_run = false;
        cutscene_ref_run_then_callback(&script_runner->cutscene, script_runner_finish, script_runner, 0);
    }
}

void script_runner_init(struct script_runner* script_runner, struct script_runner_definition* definiton, entity_id id) {
    script_runner->position = definiton->position;
    cutscene_ref_init(&script_runner->cutscene, definiton->target);
    script_runner->loop = definiton->loop;
    script_runner->should_run = true;
    update_add(script_runner, script_runner_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void script_runner_destroy(struct script_runner* script_runner) {
    cutscene_ref_destroy(&script_runner->cutscene);
    update_remove(script_runner);
}

void script_runner_common_init() {

}

void script_runner_common_destroy() {

}
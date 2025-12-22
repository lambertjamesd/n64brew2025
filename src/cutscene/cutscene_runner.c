#include "cutscene_runner.h"

#include "../time/time.h"
#include "../time/game_mode.h"
#include "../math/mathf.h"
#include "../menu/menu_rendering.h"
#include "../objects/empty.h"
#include "../util/flags.h"
#include "../scene/scene.h"
#include "evaluation_context.h"
#include "expression_evaluate.h"
#include "../savefile/savefile.h"
#include "../effects/fade_effect.h"
#include "../effects/area_title.h"
#include "expression_evaluate.h"
#include "../menu/dialog_box.h"
#include "../collision/collision_scene.h"
#include <assert.h>

#define MAX_QUEUED_CUTSCENES   4
#define MAX_CUTSCENE_CALL_DEPTH 6

static color_t fade_colors[] = {
    {0, 0, 0, 0},
    {0, 0, 0, 255},
    {255, 255, 255, 255},
};

extern struct scene* current_scene;

union cutscene_runner_data {
    struct { bool has_shown; } dialog;
    struct { float time; } delay;
};

struct cutscene_queue_entry {
    cutscene_t* cutscene;
    cutscene_finish_callback finish_callback;
    void* data;
    entity_id subject;
    uint16_t function_index;
};

struct cutscene_active_entry {
    cutscene_t* cutscene;
    cutscene_function_t* function;
    cutscene_finish_callback finish_callback;
    void* data;
    entity_id subject;
    struct evaluation_context context;
    int16_t current_instruction;
};

struct cutscene_runner {
    uint16_t next_cutscene;
    uint16_t last_cutscene;

    struct cutscene_queue_entry queue[MAX_QUEUED_CUTSCENES];

    struct cutscene_active_entry active_cutscenes[MAX_CUTSCENE_CALL_DEPTH];
    int16_t current_cutscene;

    union cutscene_runner_data active_step_data;
};

static struct cutscene_runner cutscene_runner;

void cuscene_runner_start(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject);

cutscene_actor_t* cutscene_runner_lookup_actor(struct cutscene_active_entry* cutscene, entity_id input) {
    if (input == ENTITY_ID_SUBJECT) {
        return cutscene_actor_find(cutscene->subject);
    }

    return cutscene_actor_find(input);
}

vector3_t* cutscene_runner_lookup_pos(struct cutscene_active_entry* cutscene, entity_id id) {
    if (id == ENTITY_ID_SUBJECT) {
        id = cutscene->subject;
    }

    cutscene_actor_t* actor = cutscene_actor_find(id);

    if (actor != NULL) {
        return cutscene_actor_get_pos(actor);
    }

    empty_t* empty = empty_find(id);

    if (empty != NULL) {
        return &empty->position;
    }

    return NULL;
}

void cutscene_runner_init_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_DIALOG:
            cutscene_runner.active_step_data.dialog.has_shown = false;
            break;
        case CUTSCENE_STEP_SHOW_ITEM:
            break;
        case CUTSCENE_STEP_PAUSE:
            if (step->data.pause.should_pause) {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
                }
                update_pause_layers(step->data.pause.layers);
            } else {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_3D;
                }
                update_unpause_layers(step->data.pause.layers);
            }
            break;
        case CUTSCENE_STEP_EXPRESSION:
            expression_evaluate(&cutscene->context, &step->data.expression.expression);
            break;
        case CUTSCENE_STEP_JUMP_IF_NOT:
        case CUTSCENE_STEP_JUMP:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_SET_LOCAL: {
            struct evaluation_context* context = &cutscene->context;

            evaluation_context_save(
                context->local_varaibles,
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_SET_SCENE: {
            struct evaluation_context* context = &cutscene->context;

            evaluation_context_save(
                expression_get_scene_variables(),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_SET_GLOBAL: {
            struct evaluation_context* context = &cutscene->context;

            evaluation_context_save(
                savefile_get_globals(),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_DELAY: {
            cutscene_runner.active_step_data.delay.time = step->data.delay.duration;
            break;   
        }
        case CUTSCENE_STEP_INTERACT_WITH_NPC: {
            int entities[2];
            evaluation_context_popn(&cutscene->context, entities, 2);

            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, entities[0]);
            if (!subject) {
                break;
            }

            vector3_t* target = cutscene_runner_lookup_pos(cutscene, entities[1]);
            if (!target) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_npc.type,
                target
            );

            break;
        }
        case CUTSCENE_STEP_IDLE_NPC: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }
            cutscene_actor_idle(subject);
            break;
        }
        case CUTSCENE_STEP_CAMERA_LOOK_AT_NPC: {
            struct cutscene_actor* target = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!target) {
                break;
            }

            camera_look_at(&current_scene->camera_controller, &target->transform.position);
            break;
        }
        case CUTSCENE_STEP_CAMERA_FOLLOW: {
            camera_follow_player(&current_scene->camera_controller);
            break;
        }
        case CUTSCENE_STEP_CAMERA_RETURN: {
            camera_return(&current_scene->camera_controller);
            break;
        }
        case CUTSCENE_STEP_CAMERA_ANIMATE: {
            camera_play_animation(
                &current_scene->camera_controller, 
                camera_animation_lookup(&current_scene->camera_animations, step->data.camera_animate.animation_name)
            );
            break;
        }
        case CUTSCENE_STEP_CAMERA_MOVE_TO: {
            camera_move_to(&current_scene->camera_controller, &step->data.camera_move_to.target, step->data.camera_move_to.args.instant, step->data.camera_move_to.args.move_target);
            break;
        }
        case CUTSCENE_STEP_CAMERA_WAIT:
            break;
        case CUTSCENE_STEP_INTERACT_WITH_LOCATION: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }

            struct named_location* target = scene_find_location(step->data.interact_with_location.location_name);
            if (!target) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_location.type,
                &target->position
            );
            break;
        }
        case CUTSCENE_STEP_FADE:
            fade_effect_set(fade_colors[step->data.fade.color], step->data.fade.duration);
            break;
        case CUTSCENE_STEP_INTERACT_WITH_POSITION: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_location.type,
                &step->data.interact_with_position.position
            );
            break;
        }
        case CUTSCENE_STEP_NPC_WAIT: {
            break;
        }
        case CUTSCENE_STEP_NPC_SET_SPEED: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }
            cutscene_actor_set_speed(subject, step->data.npc_set_speed.speed);
            break;
        }
        case CUTSCENE_STEP_SHOW_TITLE: {
            area_title_show(step->data.show_title.message);
            break;
        }
        case CUTSCENE_STEP_LOOK_AT_SUBJECT: {
            struct dynamic_object* subject = collision_scene_find_object(cutscene->subject);
            if (!subject) {
                break;
            }
            cutscene_actor_interact_with(&current_scene->player.cutscene_actor, INTERACTION_LOOK_SPACE, subject->position);
            break;
        }
        case CUTSCENE_STEP_NPC_ANIMATE: {
            entity_id subject_id = evaluation_context_pop(&cutscene->context);
            cutscene_actor_t* subject = cutscene_runner_lookup_actor(cutscene, subject_id);
            if (!subject) {
                break;
            }
            cutscene_actor_run_animation(subject, step->data.npc_animate.animation_name, step->data.npc_animate.loop);
            break;
        }
        case CUTSCENE_STEP_PRINT: {
            int args[step->data.print.message.nargs];
            evaluation_context_popn(&cutscene->context, args, step->data.print.message.nargs);
            char message[128];
            dialog_box_format_string(message, step->data.print.message.template, args);
            debugf("%s\n", message);
            break;
        }
        case CUTSCENE_STEP_SPAWN: {
            int spawner = evaluation_context_pop(&cutscene->context);
            scene_spawn_entity(current_scene, spawner >> 16, spawner & 0xFFFF);
            break;
        }
        case CUTSCENE_STEP_CALLBACK: {
            step->data.callback.callback(step->data.callback.data);
            break;
        }
        case CUTSCENE_STEP_SHOW_BOSS_HEALTH:
            break;
        case CUTSCENE_STEP_LOAD_SCENE: {
            scene_queue_next(step->data.load_scene.scene);
            break;
        }
    }
}

bool cutscene_runner_update_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_DIALOG:
            if (!cutscene_runner.active_step_data.dialog.has_shown && !dialog_box_is_active()) {
                int args[step->data.dialog.message.nargs];
                evaluation_context_popn(&cutscene->context, args, step->data.dialog.message.nargs);
                dialog_box_show(step->data.dialog.message.template, args, NULL, NULL);
                cutscene_runner.active_step_data.dialog.has_shown = true;
            } else if (cutscene_runner.active_step_data.dialog.has_shown) {
                return !dialog_box_is_active();
            }
            return false;
        case CUTSCENE_STEP_SHOW_ITEM:
            return true;
        case CUTSCENE_STEP_JUMP_IF_NOT:
            if (!evaluation_context_pop(&cutscene->context)) {
                cutscene->current_instruction += step->data.jump.offset;
            }
            return true;
        case CUTSCENE_STEP_JUMP:
            cutscene->current_instruction += step->data.jump.offset;
            return true;
        case CUTSCENE_STEP_DELAY:
            cutscene_runner.active_step_data.delay.time -= fixed_time_step;
            return cutscene_runner.active_step_data.delay.time <= 0.0f;
        case CUTSCENE_STEP_CAMERA_WAIT:
            return !camera_is_animating(&current_scene->camera_controller);
        case CUTSCENE_STEP_NPC_WAIT: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            return !subject || !cutscene_actor_is_moving(subject);
        }
        default:
            return true;
    }
}

void cuscene_runner_start(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    if (function_index < 0 || function_index >= cutscene->function_count) {
        finish_callback(cutscene, data);
        return;
    }

    cutscene_function_t* fn = &cutscene->functions[function_index];

    if (cutscene->step_count == 0) {
        finish_callback(cutscene, data);
        return;
    }

    cutscene_runner.current_cutscene += 1;
    assert(cutscene_runner.current_cutscene < MAX_CUTSCENE_CALL_DEPTH);

    struct cutscene_active_entry* next = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];
    next->cutscene = cutscene;
    next->function = fn;
    next->finish_callback = finish_callback;
    next->data = data;
    next->subject = subject;
    next->current_instruction = 0;
    evaluation_context_init(&next->context, cutscene->locals_size);

    cutscene_runner_init_step(next, &fn->steps[0]);
}

void cutscene_runner_update(void* data) {
    if (!cutscene_runner_is_running()) {
        return;
    }

    struct cutscene_active_entry* active_cutscene = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];

    struct cutscene_step* step = &active_cutscene->function->steps[active_cutscene->current_instruction];

    if (!cutscene_runner_update_step(active_cutscene, step)) {
        return;
    }

    active_cutscene->current_instruction += 1;

    while (cutscene_runner.current_cutscene >= 0) {
        active_cutscene = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];
        step = &active_cutscene->function->steps[active_cutscene->current_instruction];

        if (active_cutscene->current_instruction < active_cutscene->function->step_count) {
            cutscene_runner_init_step(active_cutscene, step);
            return;
        }

        if (active_cutscene->finish_callback) {
            evaluation_context_destroy(&active_cutscene->context);
            active_cutscene->finish_callback(active_cutscene->cutscene, active_cutscene->data);
        }

        cutscene_runner.current_cutscene -= 1;
    }

    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[cutscene_runner.next_cutscene];

    if (!queue_entry->cutscene) {
        return;
    }

    cutscene_runner.next_cutscene += 1;

    if (cutscene_runner.next_cutscene == MAX_QUEUED_CUTSCENES) {
        cutscene_runner.next_cutscene = 0;
    }

    cuscene_runner_start(queue_entry->cutscene, queue_entry->function_index, queue_entry->finish_callback, queue_entry->data, queue_entry->subject);
    queue_entry->cutscene = NULL;
}

void cutscene_runner_render(void* data) {

}

void cutscene_runner_init() {
    cutscene_runner.next_cutscene = 0;
    cutscene_runner.last_cutscene = 0;

    cutscene_runner.current_cutscene = -1;

    // update_remove() is never called
    update_add(&cutscene_runner, cutscene_runner_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_PAUSE_MENU);
    menu_add_callback(cutscene_runner_render, &cutscene_runner, MENU_PRIORITY_HUD);

    for (int i = 0; i < MAX_QUEUED_CUTSCENES; i += 1) {
        cutscene_runner.queue[i].cutscene = NULL;
    }
}

void cutscene_runner_run(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    if (cutscene_runner.current_cutscene == -1) {
        cuscene_runner_start(cutscene, function_index, finish_callback, data, subject);
        return;
    }

    int next_cutscene = cutscene_runner.last_cutscene;

    cutscene_runner.last_cutscene += 1;

    if (cutscene_runner.last_cutscene == MAX_QUEUED_CUTSCENES) {
        cutscene_runner.last_cutscene = 0;
    }

    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[next_cutscene];

    assert(!queue_entry->cutscene);

    queue_entry->cutscene = cutscene;
    queue_entry->function_index = function_index;
    queue_entry->finish_callback = finish_callback;
    queue_entry->data = data;
    queue_entry->subject = subject;
}

bool cutscene_runner_is_running() {
    return cutscene_runner.current_cutscene != -1;
}

void _cutscene_runner_free_on_finish(struct cutscene* cutscene, void* data) {
    cutscene_free(cutscene);
}

// used to free a cutscene created by cutscene_new() or cutscene_load()
cutscene_finish_callback cutscene_runner_free_on_finish() {
    return _cutscene_runner_free_on_finish;
}
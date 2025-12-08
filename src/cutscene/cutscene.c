#include "cutscene.h"

#include <libdragon.h>
#include <malloc.h>
#include <assert.h>

#include "../time/time.h"
#include "evaluation_context.h"

#define EXPECTED_HEADER 0x4354534E

char* cutscene_load_string(FILE* file) {
    uint8_t lower_length;
    fread(&lower_length, 1, 1, file);

    uint16_t length = 0;

    if (lower_length & 0x80) {
        uint8_t upper_length;
        fread(&upper_length, 1, 1, file);

        length = ((uint16_t)upper_length << 7) | (lower_length & 0x7F);
    } else {
        length = lower_length;
    }

    char* result = malloc(length + 1);
    fread(result, 1, length, file);
    result[length] = '\0';
    return result;
}

void cutscene_load_template_string(struct templated_string* string, FILE* file) {
    uint8_t nargs;
    fread(&nargs, 1, 1, file);
    string->nargs = nargs;
    string->template = cutscene_load_string(file);
}

void cutscene_destroy_template_string(struct templated_string* string) {
    free(string->template);
}

char* string_load(FILE* file) {
    uint8_t result_length;
    fread(&result_length, 1, 1, file);

    if (result_length == 0) {
        return NULL;
    }

    char* result = malloc(result_length + 1);
    fread(result, 1, result_length, file);
    result[result_length] = '\0';
    return result;
}

// release with cutscene_free()
struct cutscene* cutscene_load(char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    uint16_t step_count;
    fread(&step_count, 2, 1, file);

    uint16_t function_count;
    fread(&function_count, 2, 1, file);

    uint16_t locals_size;
    fread(&locals_size, 2, 1, file);

    // release with cutscene_free()
    struct cutscene* result = cutscene_new(step_count, locals_size, function_count);

    if (locals_size) {
        fread(result->locals, 1, locals_size, file);
    }

    for (int i = 0; i < step_count; i += 1) {
        struct cutscene_step* step = &result->steps[i];

        uint8_t step_type;
        fread(&step_type, 1, 1, file);

        step->type = step_type;

        switch (step_type) {
            case CUTSCENE_STEP_DIALOG:
                cutscene_load_template_string(&step->data.dialog.message, file);
                break;
            case CUTSCENE_STEP_PAUSE:
                fread(&step->data.pause, 1, 2, file);
                step->data.pause.layers = UPDATE_LAYER_WORLD;
                break;
            case CUTSCENE_STEP_EXPRESSION:
                expression_load(&step->data.expression.expression, file);
                break;
            case CUTSCENE_STEP_JUMP_IF_NOT:
            case CUTSCENE_STEP_JUMP: {
                int16_t offset;
                fread(&offset, 2, 1, file);
                step->data.jump.offset = offset;
                break;
            }
            case CUTSCENE_STEP_SET_LOCAL:
            case CUTSCENE_STEP_SET_SCENE:
            case CUTSCENE_STEP_SET_GLOBAL:
                fread(&step->data.store_variable, 4, 1, file);
                break;
            case CUTSCENE_STEP_DELAY:
                fread(&step->data.delay.duration, 4, 1, file);
                break;
            case CUTSCENE_STEP_INTERACT_WITH_NPC:
                fread(&step->data.interact_with_npc.type, 4, 1, file);
                break;
            case CUTSCENE_STEP_CAMERA_ANIMATE:
                step->data.camera_animate.animation_name = string_load(file);
                break;
            case CUTSCENE_STEP_INTERACT_WITH_LOCATION: {
                fread(&step->data.interact_with_location.type, 4, 1, file);
                step->data.interact_with_location.location_name = string_load(file);
                break;
            }    
            case CUTSCENE_STEP_FADE:
                fread(&step->data.fade.color, 4, 1, file);
                fread(&step->data.fade.duration, 4, 1, file);
                break;
            case CUTSCENE_STEP_INTERACT_WITH_POSITION: {
                fread(&step->data.interact_with_position.type, 4, 1, file);
                fread(&step->data.interact_with_position.position, 12, 1, file);
                break;
            }
            case CUTSCENE_STEP_NPC_SET_SPEED: {
                fread(&step->data.npc_set_speed.speed, 4, 1, file);
                break;
            }
            case CUTSCENE_STEP_SHOW_TITLE: {
                step->data.show_title.message = string_load(file);
                break;
            }
            case CUTSCENE_STEP_NPC_ANIMATE: {
                step->data.npc_animate.animation_name = string_load(file);
                fread(&step->data.npc_animate.loop, 1, 1, file);
                break;
            }
            case CUTSCENE_STEP_PRINT:
                cutscene_load_template_string(&step->data.print.message, file);
                break;
        }
    }
    
    cutscene_step_t* function_steps = result->steps;
    for (int i = 0; i < function_count; i += 1) {
        uint16_t function_size;
        fread(&function_size, 2, 1, file);

        result->functions[i].steps = function_steps;
        result->functions[i].step_count = function_size;
        result->functions[i].name = string_load(file);

        function_steps += function_size;
    }

    fclose(file);
    
    return result;
}

void cutscene_init(struct cutscene* cutscene, int capacity, int locals_size, int function_count) {
    cutscene->steps = malloc(sizeof(struct cutscene_step) * capacity);
    cutscene->functions = malloc(sizeof(cutscene_function_t) * function_count);
    cutscene->step_count = capacity;
    cutscene->locals_size = locals_size;
    cutscene->function_count = function_count;
    if (locals_size) {
        cutscene->locals = malloc(locals_size);
    } else {
        cutscene->locals = NULL;
    }
}

void cutscene_destroy(struct cutscene* cutscene) {
    for (int i = 0; i < cutscene->step_count; i += 1) {
        struct cutscene_step* step = &cutscene->steps[i];

        switch (step->type) {
            case CUTSCENE_STEP_DIALOG:
                cutscene_destroy_template_string(&step->data.dialog.message);
                break;
            case CUTSCENE_STEP_EXPRESSION:
                expression_destroy(&step->data.expression.expression);
                break;
            case CUTSCENE_STEP_CAMERA_ANIMATE:
                free(step->data.camera_animate.animation_name);
                break;
            case CUTSCENE_STEP_INTERACT_WITH_LOCATION:
                free(step->data.interact_with_location.location_name);
                break;
            case CUTSCENE_STEP_SHOW_TITLE:
                free(step->data.show_title.message);
                break;
            case CUTSCENE_STEP_PRINT:
                cutscene_destroy_template_string(&step->data.print.message);
                break;
            default:
                break;
        }
    }

    free(cutscene->steps);
    free(cutscene->locals);
    free(cutscene->functions);
}

struct cutscene* cutscene_new(int capacity, int locals_capacity, int function_count) {
    struct cutscene* result = malloc(sizeof(struct cutscene));
    cutscene_init(result, capacity, locals_capacity, function_count);
    return result;
}

void cutscene_free(struct cutscene* cutscene) {
    if (!cutscene) {
        return;
    }
    cutscene_destroy(cutscene);
    free(cutscene);
}

void cutscene_builder_init(struct cutscene_builder* builder) {
    builder->step_count = 0;
}

struct cutscene_step* cutscene_builder_next_step(struct cutscene_builder* builder) {
    assert(builder->step_count < MAX_BUILDER_STEP_COUNT);

    struct cutscene_step* result = &builder->steps[builder->step_count];
    builder->step_count += 1;
    return result;
}

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode, int layers) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_PAUSE,
        .data = {
            .pause = {
                .should_pause = should_pause,
                .should_change_game_mode = should_change_game_mode,
                .layers = layers,
            },
        },
    };
}

void cutscene_builder_dialog(struct cutscene_builder* builder, const char* message) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    char* message_copy = malloc(strlen(message) + 1);
    // free(message_copy) is done in cutscene_load_template_string
    strcpy(message_copy, message);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_DIALOG,
        .data = {
            .dialog = {
                .message = {
                    .template = message_copy,
                    .nargs = 0,
                },
            },
        },
    };
}

void cutscene_builder_show_item(struct cutscene_builder* builder, enum inventory_item_type item, bool should_show) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_SHOW_ITEM,
        .data = {
            .show_item = {
                .item = item,
                .should_show = should_show,
            },
        },
    };
}

void cutscene_builder_delay(struct cutscene_builder* builder, float delay) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_DELAY,
        .data = {
            .delay = {
                .duration = delay,
            },
        },
    };
}

void cutscene_builder_interact_npc(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    entity_id target
) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    expression_builder_load_literal(&expr, target);
    cutscene_builder_expression(builder, &expr);

    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_INTERACT_WITH_NPC,
        .data = {
            .interact_with_npc = {
                .type = type,
            },
        },
    };
}

void cutscene_builder_npc_set_speed(struct cutscene_builder* builder, entity_id subject, float speed) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    cutscene_builder_expression(builder, &expr);

    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_NPC_SET_SPEED,
        .data = {
            .npc_set_speed = {
                .speed = speed,
            },
        },
    };
}

void cutscene_builder_interact_position(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    struct Vector3* position
) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    cutscene_builder_expression(builder, &expr);

    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_INTERACT_WITH_POSITION,
        .data = {
            .interact_with_position = {
                .type = type,
                .position = *position,
            },
        },
    };
}

void cutscene_builder_npc_wait(
    struct cutscene_builder* builder,
    entity_id subject
) {    
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    cutscene_builder_expression(builder, &expr);

    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_NPC_WAIT,
    };
}

void cutscene_builder_camera_wait(struct cutscene_builder* builder) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CAMERA_WAIT,
    };
}

void cutscene_builder_camera_follow(struct cutscene_builder* builder) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CAMERA_FOLLOW,
    };
}

void cutscene_builder_camera_return(struct cutscene_builder* builder) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CAMERA_RETURN,
    };
}

void cutscene_builder_camera_move_to(struct cutscene_builder* builder, struct Vector3* position, camera_move_to_args_t* args) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CAMERA_MOVE_TO,
        .data.camera_move_to = {
            .target = {position->x, position->y, position->z},
            .args = *args,
        },
    };
}

void cutscene_builder_set_boolean(struct cutscene_builder* builder, boolean_variable variable, bool value) {
    struct cutscene_step* expression = cutscene_builder_next_step(builder);
    struct cutscene_step* set = cutscene_builder_next_step(builder);

    *expression = (struct cutscene_step){
        .type = CUTSCENE_STEP_EXPRESSION,
    };
    expression_load_literal(&expression->data.expression.expression, value);

    if (SCENE_VARIABLE_FLAG & variable) {
        *set = (struct cutscene_step){
            .type = CUTSCENE_STEP_SET_SCENE,
            .data = {
                .store_variable = {
                    .data_type = DATA_TYPE_BOOL,
                    .word_offset = variable ^ SCENE_VARIABLE_FLAG,
                },
            },
        };
    } else {
        *set = (struct cutscene_step){
            .type = CUTSCENE_STEP_SET_GLOBAL,
            .data = {
                .store_variable = {
                    .data_type = DATA_TYPE_BOOL,
                    .word_offset = variable,
                },
            },
        };
    }
}

void cutscene_builder_callback(struct cutscene_builder* builder, cutscene_step_callback callback, void* data) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CALLBACK,
        .data.callback = {
            .callback = callback,
            .data = data,
        },
    };
}

void cutscene_builder_expression(struct cutscene_builder* builder, expression_builder_t* expression) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_EXPRESSION,
    };
    expression_builder_finish(expression, &step->data.expression.expression);
}

// release with cutscene_free()
struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder) {
    // release with cutscene_free()
    struct cutscene* result = cutscene_new(builder->step_count, 0, 1);
    memcpy(result->steps, builder->steps, sizeof(struct cutscene_step) * builder->step_count);
    result->functions[0] = (cutscene_function_t){
        .name = NULL,
        .step_count = builder->step_count,
        .steps = result->steps,
    };
    return result;
}

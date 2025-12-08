#include "expression_evaluate.h"

#include "../savefile/savefile.h"
#include "expression_fn.h"

static void* scene_variables;

void expression_set_scene_variables(void* variables) {
    scene_variables = variables;
}

void* expression_get_scene_variables() {
    return scene_variables;
}

bool expression_get_bool(boolean_variable variable) {
    if (variable == VARIABLE_DISCONNECTED) {
        return false;
    }

    if (SCENE_VARIABLE_FLAG & variable) {
        return evaluation_context_load(scene_variables, DATA_TYPE_BOOL, variable ^ SCENE_VARIABLE_FLAG);
    } else {
        return evaluation_context_load(savefile_get_globals(), DATA_TYPE_BOOL, variable);
    }
}

void expression_set_bool(boolean_variable variable, bool value) {
    if (variable == VARIABLE_DISCONNECTED) {
        return;
    }

    if (SCENE_VARIABLE_FLAG & variable) {
        evaluation_context_save(scene_variables, DATA_TYPE_BOOL, variable ^ SCENE_VARIABLE_FLAG, value);
    } else {
        evaluation_context_save(savefile_get_globals(), DATA_TYPE_BOOL, variable, value);
    }
}

int expression_get_integer(integer_variable variable) {
    if (variable == VARIABLE_DISCONNECTED) {
        return 0;
    }

    data_type_t type = GET_INT_VAR_SIZE(variable);

    if (SCENE_VARIABLE_FLAG & variable) {
        return evaluation_context_load(scene_variables, type, variable & INT_OFFSET_MASK);
    } else {
        return evaluation_context_load(savefile_get_globals(), type, variable & INT_OFFSET_MASK);
    }
}

void expression_set_integer(integer_variable variable, int value) {
    if (variable == VARIABLE_DISCONNECTED) {
        return;
    }

    data_type_t type = GET_INT_VAR_SIZE(variable);
    
    if (SCENE_VARIABLE_FLAG & variable) {
        evaluation_context_save(scene_variables, type, variable & INT_OFFSET_MASK, value);
    } else {
        evaluation_context_save(savefile_get_globals(), type, variable & INT_OFFSET_MASK, value);
    }
}

void expression_evaluate(struct evaluation_context* context, struct expression* expression) {
    uint8_t* current = expression->expression_program;

    while (*current != EXPRESSION_TYPE_END) {
        int instruction = *current;
        ++current;

        union expression_data data;

        switch (instruction) {
            case EXPRESSION_TYPE_LOAD_LOCAL:
                // this avoids alignment issues
                memcpy(&data, current, sizeof(union expression_data));
                
                evaluation_context_push(context, evaluation_context_load(context->local_varaibles, data.load_variable.data_type, data.load_variable.word_offset));
                current += sizeof(union expression_data);
                break;
            case EXPRESSION_TYPE_LOAD_SCENE_VAR:
                // this avoids alignment issues
                memcpy(&data, current, sizeof(union expression_data));

                evaluation_context_push(context, evaluation_context_load(scene_variables, data.load_variable.data_type, data.load_variable.word_offset));
                current += sizeof(union expression_data);
                break;
            case EXPRESSION_TYPE_LOAD_GLOBAL:
                // this avoids alignment issues
                memcpy(&data, current, sizeof(union expression_data));

                evaluation_context_push(context, evaluation_context_load(savefile_get_globals(), data.load_variable.data_type, data.load_variable.word_offset));
                current += sizeof(union expression_data);
                break;
            case EXPRESSION_TYPE_LOAD_LITERAL:
                // this avoids alignment issues
                memcpy(&data, current, sizeof(union expression_data));

                evaluation_context_push(context, data.literal);
                current += sizeof(union expression_data);
                break;
            case EXPRESSION_TYPE_AND: {
                int a = evaluation_context_pop(context);
                int b = evaluation_context_pop(context);
                evaluation_context_push(context, a && b);
                break;
            }
            case EXPRESSION_TYPE_OR: {
                int a = evaluation_context_pop(context);
                int b = evaluation_context_pop(context);
                evaluation_context_push(context, a || b);
                break;
            }
            case EXPRESSION_TYPE_NOT:
                evaluation_context_push(
                    context,
                    !evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_EQ:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) == evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_NEQ:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) != evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_GT:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) > evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_GTE:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) >= evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_ADD:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) + evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_SUB:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) - evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_MUL:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) * evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_DIV:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) / evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_NEGATE:
                evaluation_context_push(
                    context,
                    -evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_BUILT_IN_FN:
                // this avoids alignment issues
                memcpy(&data, current, sizeof(union expression_data));
                current += sizeof(union expression_data);
                
                int starting_size = evaluation_context_stack_size(context);
                expression_built_in_fn fn = expression_lookup_fn(data.fn_call.fn);
                assert(fn);

                int actual_result_count = fn(context, data.fn_call.arg_count);
                assert(evaluation_context_stack_size(context) == starting_size - data.fn_call.arg_count + actual_result_count);

                while (actual_result_count < data.fn_call.result_count) {
                    evaluation_context_push(context, 0);
                    ++actual_result_count;
                }

                if (actual_result_count > data.fn_call.result_count) {
                    evaluation_context_popn(context, NULL, actual_result_count - data.fn_call.result_count);
                }
                break;
        }
    }
}
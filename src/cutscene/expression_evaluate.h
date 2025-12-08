#ifndef __CUTSCENE_EXPRESSION_EVALUATE_H__
#define __CUTSCENE_EXPRESSION_EVALUATE_H__

#include "expression.h"
#include "evaluation_context.h"
#include "../scene/scene_definition.h"

// the evaluation result is left on the top of th context stack
void expression_evaluate(struct evaluation_context* context, struct expression* expression);

void expression_set_scene_variables(void* variables);
void* expression_get_scene_variables();

bool expression_get_bool(boolean_variable variable);
void expression_set_bool(boolean_variable variable, bool value);

int expression_get_integer(integer_variable variable);
void expression_set_integer(integer_variable variable, int value);

#endif
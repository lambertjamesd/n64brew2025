#include "expression_fn.h"

#include <stddef.h>
#include "../collision/collision_scene.h"

int expression_are_touching(struct evaluation_context* context, int arg_count) {
    if (arg_count != 2) {
        evaluation_context_popn(context, NULL, arg_count);
        return 0;
    }

    int args[2];
    evaluation_context_popn(context, args, 2);

    spatial_trigger_t* a_trigger = collision_scene_find_trigger(args[0]);

    if (a_trigger) {
        evaluation_context_push(context, contacts_are_touching(a_trigger->active_contacts, args[1]));
        return 1;
    }

    dynamic_object_t* a_obj = collision_scene_find_object(args[0]);

    if (!a_obj) {
        return 0;
    }

    if (contacts_are_touching(a_obj->active_contacts, args[1])) {
        evaluation_context_push(context, true);
        return 1;
    }

    spatial_trigger_t* b_trigger = collision_scene_find_trigger(args[1]);

    if (b_trigger) {
        evaluation_context_push(context, contacts_are_touching(b_trigger->active_contacts, args[0]));
        return 1;
    }

    return 0;
}

static expression_built_in_fn fn_array[EXPRESSION_BUILT_IN_COUNT] = {
    [EXPRESSION_BUILT_IN_ARE_TOUCHING] = expression_are_touching,
};

expression_built_in_fn expression_lookup_fn(enum expression_built_in_type type) {
    if (type < EXPRESSION_BUILT_IN_COUNT) {
        return fn_array[type];
    }

    return NULL;
}
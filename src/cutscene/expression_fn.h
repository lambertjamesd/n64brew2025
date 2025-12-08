#ifndef __CUTSCENE_EXPRESSION_FN_H__
#define __CUTSCENE_EXPRESSION_FN_H__

#include "evaluation_context.h"

typedef int (*expression_built_in_fn)(struct evaluation_context* context, int arg_count);

enum expression_built_in_type {
    EXPRESSION_BUILT_IN_ARE_TOUCHING,

    EXPRESSION_BUILT_IN_COUNT,
};

expression_built_in_fn expression_lookup_fn(enum expression_built_in_type type);

#endif
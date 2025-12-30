#include "inventory.h"

#include "../cutscene/expression_evaluate.h"
#include "../savefile/savefile.h"

extern struct global_location inventory_item_locations[ITEM_TYPE_COUNT];


bool inventory_has_item(enum inventory_item_type item) {
    if (item >= ITEM_TYPE_COUNT) {
        return false;
    }

    struct global_location* global = &inventory_item_locations[item];

    assert(global->data_type);

    return evaluation_context_load(savefile_get_globals(), global->data_type, global->word_offset);
}
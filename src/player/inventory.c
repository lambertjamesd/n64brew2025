#include "inventory.h"

#include "../cutscene/expression_evaluate.h"
#include "../savefile/savefile.h"

extern struct global_location inventory_item_locations[ITEM_TYPE_COUNT];


bool inventory_has_item(enum inventory_item_type item) {
    if (item >= ITEM_TYPE_COUNT || item == ITEM_TYPE_NONE) {
        return false;
    }

    struct global_location* global = &inventory_item_locations[item];

    if (!global->data_type) {
        debugf("item %d has no mapping\n", item);
    }

    assert(global->data_type);

    return evaluation_context_load(savefile_get_globals(), global->data_type, global->word_offset);
}

void inventory_set_has_item(enum inventory_item_type item, bool value) {
    if (item >= ITEM_TYPE_COUNT || item == ITEM_TYPE_NONE) {
        return;
    }

    struct global_location* global = &inventory_item_locations[item];

    if (!global->data_type) {
        debugf("item %d has no mapping\n", item);
    }

    assert(global->data_type);

    evaluation_context_save(savefile_get_globals(), global->data_type, global->word_offset, true);
}
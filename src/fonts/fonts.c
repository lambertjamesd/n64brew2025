#include "fonts.h"

#include <stdint.h>
#include "assert.h"

rdpq_font_t* font_types[FONT_COUNT];
static uint8_t font_ref_count[FONT_COUNT];
static const char* font_sources[FONT_COUNT] = {
    [FONT_DIALOG] = "rom:/fonts/DeltaBlock.font64",
};

void font_type_use(font_type_t type) {
    assert(type > FONT_NONE && type < FONT_COUNT);
    assert(font_ref_count[type] < 255);

    if (font_ref_count[type] == 0) {
        font_types[type] = rdpq_font_load(font_sources[type]);
        rdpq_text_register_font(type, font_types[type]);
    }
    ++font_ref_count[type];
}

void font_type_release(font_type_t type) {
    assert(type > FONT_NONE && type < FONT_COUNT);
    assert(font_ref_count[type] > 0);

    --font_ref_count[type];
    if (font_ref_count[type] == 0) {
        rdpq_text_unregister_font(type);
        rdpq_font_free(font_types[type]);
        font_types[type] = NULL;
    }
}
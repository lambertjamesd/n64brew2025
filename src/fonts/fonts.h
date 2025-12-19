#ifndef __FONT_FONTS_H__
#define __FONT_FONTS_H__

#include <libdragon.h>
enum font_type {
    FONT_NONE,
    FONT_DIALOG,

    FONT_COUNT,
};

extern rdpq_font_t* font_types[FONT_COUNT];

typedef enum font_type font_type_t;

void font_type_use(font_type_t type);
void font_type_release(font_type_t type);

static inline rdpq_font_t* font_get(font_type_t type) {
    return font_types[type];
}

#endif
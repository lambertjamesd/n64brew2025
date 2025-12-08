#ifndef __EFFECTS_FADE_EFFECT_H__
#define __EFFECTS_FADE_EFFECT_H__

#include "../render/coloru8.h"

void fade_effect_set(color_t color, float time);

void fade_effect_flash(color_t color);

#endif
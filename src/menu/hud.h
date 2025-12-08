#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../render/material.h"
#include "../player/player.h"
#include "../render/material.h"

struct hud {
    struct player* player;
    rdpq_font_t* action_font;
};

void hud_init(struct hud* hud, struct player* player);
void hud_destroy(struct hud* hud);

#endif
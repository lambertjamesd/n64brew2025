#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "../cutscene/cutscene_runner.h"
#include "../render/coloru8.h"
#include "../resource/material_cache.h"
#include "../resource/font_cache.h"
#include "../time/time.h"

void hud_render(void *data) {

}

void hud_init(struct hud* hud, struct player* player) {
    menu_add_callback(hud_render, hud, MENU_PRIORITY_HUD);
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
}
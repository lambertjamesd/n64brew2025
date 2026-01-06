#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../render/material.h"
#include "../player/player.h"
#include "../render/material.h"
#include "../render/camera.h"

struct hud_assets {
    material_t* overlay_material;
    material_t* icon_material;
    material_t* compass_arrow;
    sprite_t* compass_border;
};

typedef struct hud_assets hud_assets_t;

struct hud {
    struct player* player;
    camera_t* camera;
    hud_assets_t assets;
};

typedef struct hud hud_t;

void hud_init(struct hud* hud, struct player* player, camera_t* camera);
void hud_destroy(struct hud* hud);

#endif
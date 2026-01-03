#include <stdio.h>

#include <t3d/t3d.h>

#include "resource/material_cache.h"
#include "resource/sprite_cache.h"
#include "render/camera.h"
#include "math/transform.h"
#include "savefile/savefile.h"
#include "time/time.h"
#include "collision/collision_scene.h"
#include "menu/menu_rendering.h"
#include "render/render_scene.h"

#include "render/render_batch.h"
#include "scene/scene_loader.h"
#include "time/game_mode.h"
#include "render/tmesh.h"
#include "util/init.h"
#include "util/screen_debug.h"
#include "effects/area_title.h"
#include "effects/fade_effect.h"
#include "audio/audio.h"
#include "repair/repair_scene.h"

#include <libdragon.h>
#include <n64sys.h>

#define RDPQ_VALIDATE_DETACH_ADDR    0x00800000

void load_scene_or_repair(const char* filename) {
    if (strncmp("rom:/scenes/", filename, strlen("rom:/scenes/")) == 0) {
        current_scene = scene_load(filename);
    } else if (strncmp("rom:/repair/", filename, strlen("rom:/repair/")) == 0) {
        current_repair_scene = repair_scene_load(filename);
    } else {
        debugf("invalid scene %s", filename);
        assert(false);
    }
}


void setup() {
    debug_init_isviewer();
    // rdpq_debug_start();
    savefile_new();
    init_engine();
    interactable_reset();

    fade_effect_init();
    fade_effect_set((color_t){0, 0, 0, 255}, 0.0f);
    fade_effect_set((color_t){0, 0, 0, 0}, 3.0f);

    // scene_queue_next("rom:/scenes/overworld_accuracy_test.scene");
    scene_queue_next("rom:/scenes/overworld.scene#exit_repair_well");
    // scene_queue_next("rom:/repair/motorycle_engine.repair");
    // scene_queue_next("rom:/scenes/garage.scene");

    load_scene_or_repair(scene_get_next());

    scene_clear_next();
    screen_debug_init();
}

void reset() {
    area_title_hide();
}

static struct frame_memory_pool frame_memory_pools[2];
static uint8_t next_frame_memory_pool;

void render_3d() {
    uint8_t colorAmbient[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    t3d_frame_start();

	t3d_screen_clear_color(RGBA32(180, 220, 255, 0));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient); // one global ambient light, always active
    t3d_light_set_count(0);
    
    struct frame_memory_pool* pool = &frame_memory_pools[next_frame_memory_pool];
    frame_pool_reset(pool);

    T3DViewport* viewport = frame_malloc(pool, sizeof(T3DViewport));
    *viewport = t3d_viewport_create();

    if (current_scene) {
        render_scene_render(&current_scene->camera, viewport, &frame_memory_pools[next_frame_memory_pool]);
    } else if (current_repair_scene) {
        repair_scene_render(current_repair_scene, viewport, pool);
    }
    
    next_frame_memory_pool ^= 1;
}

void render_menu() {
    rdpq_sync_pipe();
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    menu_render();
    screen_debug_render();
}

void render(surface_t* zbuffer) {
    update_render_time();

    if (current_game_mode == GAME_MODE_3D || current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
        render_3d();
    } else if (current_game_mode == GAME_MODE_MENU) {
        static surface_t background;
        background = surface_make_linear(zbuffer->buffer, FMT_RGBA16, zbuffer->width, zbuffer->height);
        rdpq_sync_pipe();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_tex_blit(&background, 0, 0, NULL);
    }
    render_menu();
}

#define VI_PER_FRAME 2
volatile static uint8_t vi_delay;

void on_vi_interrupt() {
    if (vi_delay > 0) {
        vi_delay -= 1;
    }
}

bool check_scene_load() {
    static uint8_t frame_wait = 0;

    if (!scene_has_next()) {
        return false;
    }

    if (frame_wait == 0) {
        frame_wait = 2;
        return true;
    } 

    --frame_wait;

    if (frame_wait > 0) {
        return true;
    }

    if (current_scene) {
        scene_release(current_scene);
        current_scene = NULL;
    }
    if (current_repair_scene) {
        repair_scene_destroy(current_repair_scene);
        current_repair_scene = NULL;
    }
    reset();
    load_scene_or_repair(scene_get_next());
    scene_clear_next();

    return false;
}

#define DEBUG_CONNECT_DELAY     TICKS_FROM_MS(1500)

int main(void)
{
	resolution_t custom_res = {320, 240, false};

	if (get_tv_type() == 0) //TEMP: if PAL, adjust vertical res
	{
		custom_res.height = 288;
	}

    display_init(custom_res, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
	// *(volatile uint32_t*)0xA4400000 |= 0x300; //disables resampling on the VI
	rdpq_init();
    t3d_init((T3DInitParams){});
    tpx_init((TPXInitParams){});
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();

    surface_t zbuffer = surface_alloc(FMT_RGBA16, custom_res.width, custom_res.height);
	
    debug_init_usblog();
    console_set_debug(true);

    // give time for the debugger to connect
    long long start_time = timer_ticks();
    while (timer_ticks() - start_time < DEBUG_CONNECT_DELAY);

    setup();

    register_VI_handler(on_vi_interrupt);

    while(1) {
        while (vi_delay > 0) {
            // TODO process low priority tasks
        }
        vi_delay = VI_PER_FRAME;

        if (check_scene_load()) {
            continue;
        }

        if (current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
            surface_t* fb = display_get();

            rdpq_attach(fb, &zbuffer);

            render_3d();
            rdpq_sync_pipe();

            // copy the frame buffer into the z buffer
            // to be used as the background while the game
            // is paused
            rdpq_set_color_image_raw(
                0, 
                PhysicalAddr(zbuffer.buffer), 
                FMT_RGBA16, 
                zbuffer.width, 
                zbuffer.height, 
                zbuffer.stride
            );
            
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            rdpq_tex_blit(fb, 0, 0, NULL);

            rdpq_sync_pipe();
            rdpq_set_color_image(fb);

            render_menu();

            rdpq_detach_show();

            current_game_mode = GAME_MODE_MENU;
        } else {
            surface_t* fb = display_try_get();

            if (fb) {
                rdpq_attach(fb, &zbuffer);

                render(&zbuffer);

                rdpq_detach_show();
            } 
        }

        joypad_poll();
        struct Vector3 right;
        if (current_scene) {
            quatMultVector(&current_scene->camera.transform.rotation, &gRight, &right);
            audio_update_listener(&current_scene->camera.transform.position, &right, &gZeroVec);
        } else {
            audio_update_listener(&gZeroVec, &gRight, &gZeroVec);
        }
        audio_player_update();
        if (update_has_layer(UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE)) {
            // uint64_t start_time = get_ticks_us();
            collision_scene_collide();
            // debugf("collision_time = %f\n", (get_ticks_us() - start_time) / 1000.0f);
        }
        update_dispatch();
    }
}
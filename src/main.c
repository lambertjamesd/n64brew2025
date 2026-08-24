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
#include "config.h"
#include "menu/main_menu.h"
#include "overworld/overworld_load.h"
#include "render/z_clear.h"
#include "render/defs.h"
#include "debug/rewind.h"
#include "profile/profile.h"
#include "intro/logo.h"

#include <libdragon.h>
#include <n64sys.h>

#define RDPQ_VALIDATE_DETACH_ADDR    0x00800000

void load_scene_or_repair(const char* filename) {
    if (main_menu_enable_scene_saving) {
        savefile_set_last_scene(filename, scene_get_next_entry());
    }

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
#if DEBUG_ENABLED
    debug_init_isviewer();
#endif
    // rdpq_debug_start();
    savefile_init();
    savefile_check_for_data();
    init_engine();
    interactable_reset();
    z_clear_init();

    fade_effect_init();
    fade_effect_set((color_t){0, 0, 0, 255}, 0.0f);
    fade_effect_set((color_t){0, 0, 0, 0}, 3.0f);

#if ENABLE_CHEATS
    scene_queue_next("rom:/scenes/level_select.scene#default");
#else
    scene_queue_next("rom:/scenes/overworld.scene#main_menu");
#endif

#if ENABLE_WORLD_SCALE
    scene_queue_next("rom:/scenes/world_scale_test.scene");
#endif
#if ENABLE_REWIND
    scene_queue_next("rom:/scenes/overworld.scene#test");
#endif
    // scene_queue_next("rom:/scenes/settlement_house2_inside.scene#default");
    // scene_queue_next("rom:/scenes/overworld.scene#default");
    // scene_queue_next("rom:/repair/motorycle_engine.repair");
    // scene_queue_next("rom:/scenes/inside_boat.scene");
    // scene_queue_next("rom:/scenes/inside_house.scene#defualt");

    load_scene_or_repair(scene_get_next());

    scene_clear_next();
    screen_debug_init();
}

void reset() {
    area_title_hide();
}

static struct frame_memory_pool frame_memory_pools[2];
static uint8_t next_frame_memory_pool;

void render_3d(surface_t* col, surface_t* z_buffer) {
    uint8_t colorAmbient[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    if (current_scene && current_scene->overworld) {
        z_clear_dithered(z_buffer);
    } else {
        t3d_screen_clear_depth();
    }
    rdpq_set_color_image(col);
    rdpq_set_z_image(z_buffer);
    if ((current_scene && !current_scene->overworld) || ENABLE_LOD_RENDER_DEBUG) {
        t3d_screen_clear_color(RGBA32(0, 0, 0, 0));
    } /*else {
        t3d_screen_clear_color(RGBA32(0xc0, 0xf8, 0xf8, 0));
    }*/
    
    t3d_frame_start();

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
#if ENABLE_BIG_SCREEN_SHOT
    if (camera_is_showing_fov()) {
        return;
    }
#endif

    rdpq_sync_pipe();
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    menu_render();
    screen_debug_render();
}

void render(surface_t* col, surface_t* zbuffer) {
    update_render_time();

    if (current_game_mode == GAME_MODE_3D || current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
        render_3d(col, zbuffer);
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

#define MAX_FRAME_CATCHUP   2
#define VI_PER_FRAME 2
volatile static int8_t vi_delay;

void on_vi_interrupt() {
    if (vi_delay > -MAX_FRAME_CATCHUP * VI_PER_FRAME) {
        vi_delay -= 1;
    }
}

bool check_scene_load() {
    if (!scene_has_next()) {
        return false;
    }

    rspq_wait();

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

void step_simulation() {
    rewind_update();

    if (update_has_layer(UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE)) {
        SC_PROFILE_START(main);
        collision_scene_collide();
        SC_PROFILE_END(main, collision_scene_collide);
    }
    SC_PROFILE_START(main);
    update_dispatch();
    SC_PROFILE_END(main, update_dispatch);
}

int main(void)
{
	rdpq_init();
    t3d_init((T3DInitParams){});
    tpx_init((TPXInitParams){});
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();
    audio_player_init();

    logo_libdragon();
    logo_t3d();

	resolution_t custom_res = {SCREEN_WD, SCREEN_HT, false};

    display_init(custom_res, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    // display_set_fps_limit(30.0f);
	// *(volatile uint32_t*)0xA4400000 |= 0x300; //disables resampling on the VI

    surface_t zbuffer = surface_alloc(FMT_RGBA16, custom_res.width, custom_res.height);

#if DEBUG_ENABLED
    debug_init_usblog();
    console_set_debug(true);

    // give time for the debugger to connect
    long long start_time = timer_ticks();
    while (timer_ticks() - start_time < DEBUG_CONNECT_DELAY);
#endif

    setup();
    
    register_VI_handler(on_vi_interrupt);

#if DEBUG_ENABLED
    debugf("game started\n");
#endif

    while(1) {
        savefile_check_autosave();
        if (current_scene && current_scene->overworld) {
            SC_PROFILE_START(main);
            overworld_check_unload_queue(current_scene->overworld);
            SC_PROFILE_END(main, overworld_check_unload_queue);
        }

        while (vi_delay > 0) {}

        int update_count = -vi_delay / VI_PER_FRAME + 1;

        if (check_scene_load()) {
            continue;
        }
        
        mixer_try_play();

        SC_PROFILE_START(main);

        if (current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
            surface_t* fb = display_get();

            rdpq_attach(fb, &zbuffer);

            render_3d(fb, &zbuffer);
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

                render(fb, &zbuffer);

                rdpq_detach_show();
            } 
        }
        
        SC_PROFILE_END(main, render);

        for (int it = 0; it < update_count; it += 1) {
            SC_PROFILE_START(main);
            joypad_poll();
            struct Vector3 right;
            if (current_scene) {
                quatMultVector(&current_scene->camera.transform.rotation, &gRight, &right);
                audio_update_listener(&current_scene->camera.transform.position, &right);
            } else {
                audio_update_listener(&gZeroVec, &gRight);
            }
            audio_player_update();
    #if ENABLE_BIG_SCREEN_SHOT
            if (joypad_get_buttons_pressed(0).l) {
                camera_next_sub_fov();
            }
    
            if (!camera_is_showing_fov()) {
                step_simulation();
            }
    #else
            step_simulation();
    #endif
            
            vi_delay += VI_PER_FRAME;

            SC_PROFILE_END(main, update);
        }
    }
}
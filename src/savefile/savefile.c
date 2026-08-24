#include "savefile.h"
#include "../time/time.h"

#include <libdragon.h>
#include <malloc.h>
#include <stdint.h>
#include "../menu/map_menu.h"
#include "../config.h"

#define AUTOSAVE_INTERVAL       180.0f
#define DIRTY_AUTOSAVE_INTERVAL 3.0f
#define MIN_SAVE_INTERVAL       30.0f

static void* current_savefile;
static bool needs_save;
static float auto_save_time = AUTOSAVE_INTERVAL;
static float last_save_time = 0.0f;

#define HEADER_NAME 0x6A756E6B72756E6ELL

#define SRAM_ADDRESS    0x08000000

struct savefile_header {
    uint64_t header;
    uint16_t globals_size;
    char last_scene[64];
};

#define ALIGN_BLOCK(number)    (((number) + 15) & ~15)

#define MAP_BLOCK_SIZE      (128 * 128)

static struct savefile_header savefile;

void savefile_init() {
    sram_init();
}

void savefile_schedule_save(float offset) {
    float time = game_time + offset;

    float min_save_time = last_save_time + MIN_SAVE_INTERVAL;

    if (time < min_save_time) {
        time = min_save_time;
    }

    if (time < auto_save_time) {
        auto_save_time = time;
    }
}

void savefile_unload() {
    free(current_savefile); 
    current_savefile = NULL;
}

void savefile_check_for_data() {
    savefile_unload();

    sram_read(&savefile, 0, sizeof(struct savefile_header));

    if (savefile.header != HEADER_NAME) {
        savefile_new();
        savefile.header = 0;
        return;
    }

    FILE* file = asset_fopen("rom:/scripts/globals.dat", NULL);
    uint16_t size;
    fread(&size, 2, 1, file);
    fclose(file);

    if (size != savefile.globals_size) {
        savefile_new();
        savefile.header = 0;
        return;
    }

    current_savefile = malloc(ALIGN_BLOCK(size));
    sram_read(&savefile, ALIGN_BLOCK(sizeof(struct savefile_header)), ALIGN_BLOCK(size));

    uint8_t* map_revealed = map_get_revealed();
    sram_read(map_revealed, ALIGN_BLOCK(sizeof(struct savefile_header)) + ALIGN_BLOCK(size), MAP_BLOCK_SIZE);
}

bool savefile_save() {
#if ENABLE_REWIND
    return false;
#endif

    if (!savefile_has_save()) {
        return false;
    }

    sram_write(&savefile, 0, sizeof(struct savefile_header));    
    sram_write(&savefile, ALIGN_BLOCK(sizeof(struct savefile_header)), ALIGN_BLOCK(savefile.globals_size));
    
    uint8_t* map_revealed = map_get_revealed();
    sram_write(&savefile, ALIGN_BLOCK(sizeof(struct savefile_header)) + ALIGN_BLOCK(savefile.globals_size), MAP_BLOCK_SIZE);

    last_save_time = game_time;

    return true;
}

bool savefile_is_autosaving() {
    return (auto_save_time - game_time) < 2.0f && update_has_layer(UPDATE_LAYER_WORLD);
}

void savefile_new() {
    savefile_unload();

    FILE* file = asset_fopen("rom:/scripts/globals.dat", NULL);

    uint16_t size;
    fread(&size, 2, 1, file);
    current_savefile = malloc(ALIGN_BLOCK(size));
    fread(current_savefile, 1, size, file);

    savefile.header = HEADER_NAME;
    savefile.globals_size = size;
    savefile.last_scene[0] = '\0';

    memset(map_get_revealed(), 0, MAP_BLOCK_SIZE);

    fclose(file);
}

bool savefile_has_save() {
    return savefile.header == HEADER_NAME && current_savefile != NULL;
}

void savefile_set_last_scene(const char* name, const char* entry) {
    savefile_schedule_save(DIRTY_AUTOSAVE_INTERVAL);
    strcpy(savefile.last_scene, name);

    if (*entry) {
        int len = strlen(name);
        savefile.last_scene[len] = '#';
        strcpy(savefile.last_scene + len + 1, entry);
    }
}

const char* savefile_get_last_scene() {
    return savefile.last_scene;
}

void* savefile_get_globals(global_access_mode_t mode) {
    if (mode == GLOBAL_ACCESS_MODE_WRITE) {
        savefile_schedule_save(DIRTY_AUTOSAVE_INTERVAL);
    }
    return current_savefile;
}

void savefile_check_autosave() {
    if (game_time > auto_save_time) {
        savefile_save();
        auto_save_time = game_time + AUTOSAVE_INTERVAL;
    }
}
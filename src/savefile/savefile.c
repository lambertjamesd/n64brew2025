#include "savefile.h"

#include <libdragon.h>
#include <malloc.h>

static void* current_savefile;
static bool needs_save;
static uint16_t globals_size;

void savefile_unload() {
    free(current_savefile); 
    current_savefile = NULL;
}

void savefile_new() {
    savefile_unload();

    FILE* file = asset_fopen("rom:/scripts/globals.dat", NULL);

    uint16_t size;
    fread(&size, 2, 1, file);
    current_savefile = malloc(size);
    fread(current_savefile, 1, size, file);

    fclose(file);
}

void* savefile_get_globals(global_access_mode_t mode) {
    if (mode == GLOBAL_ACCESS_MODE_WRITE) {
        needs_save = true;
    }
    return current_savefile;
}
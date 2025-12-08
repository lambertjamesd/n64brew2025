#include "savefile.h"

#include <libdragon.h>
#include <malloc.h>

static void* current_savefile;

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

void* savefile_get_globals() {
    return current_savefile;
}
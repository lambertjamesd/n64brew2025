#ifndef __SAVEFILE_SAVEFILE_H__
#define __SAVEFILE_SAVEFILE_H__

#include <stdbool.h>

void savefile_init();

void savefile_check_for_data();
void savefile_new();
bool savefile_has_save();
bool savefile_save();
bool savefile_is_autosaving();
void savefile_set_last_scene(const char* name, const char* entry);
const char* savefile_get_last_scene();
void savefile_unload();

void savefile_check_autosave();

enum global_access_mode {
    GLOBAL_ACCESS_MODE_READ,
    GLOBAL_ACCESS_MODE_WRITE,
};

typedef enum global_access_mode global_access_mode_t;

void* savefile_get_globals(global_access_mode_t mode);

#endif
#ifndef __SAVEFILE_SAVEFILE_H__
#define __SAVEFILE_SAVEFILE_H__

void savefile_new();

enum global_access_mode {
    GLOBAL_ACCESS_MODE_READ,
    GLOBAL_ACCESS_MODE_WRITE,
};

typedef enum global_access_mode global_access_mode_t;

void* savefile_get_globals(global_access_mode_t mode);

#endif
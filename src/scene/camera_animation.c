#include "camera_animation.h"

#include <stddef.h>
#include <string.h>
#include <malloc.h>

void camera_animation_list_init(struct camera_animation_list* list, int count, uint32_t rom_location) {
    list->animations = malloc(sizeof(struct camera_animation) * count);
    list->animation_count = count;
    list->rom_location = rom_location;

    for (int i = 0; i < count; ++i) {
        list->animations[i].name = NULL;
    }
}

void camera_animation_list_destroy(struct camera_animation_list* list) {
    for (int i = 0; i < list->animation_count; ++i) {
        free(list->animations[i].name);
    }
    free(list->animations);
}

struct camera_animation* camera_animation_lookup(struct camera_animation_list* list, const char* name) {
    struct camera_animation* end = list->animations + list->animation_count;

    for (struct camera_animation* curr = list->animations; curr < end; ++curr) {
        if (strcmp(name, curr->name) == 0) {
            return curr;
        }
    }

    return NULL;
}
#ifndef __TIME_TIME_H__
#define __TIME_TIME_H__

#include <stdbool.h>

typedef void (*update_callback)(void* data);

typedef int update_id;

#define UPDATE_LAYER_WORLD          (1 << 0)
#define UPDATE_LAYER_PLAYER         (1 << 1)
#define UPDATE_LAYER_DIALOG         (1 << 2)
#define UPDATE_LAYER_PAUSE_MENU     (1 << 3)
#define UPDATE_LAYER_CUTSCENE       (1 << 4)

#define UPDATE_PRIORITY_PHYICS  -1
#define UPDATE_PRIORITY_PLAYER  0
#define UPDATE_PRIORITY_SPELLS  1
#define UPDATE_PRIORITY_CAMERA  2
#define UPDATE_PRIORITY_EFFECTS 3

void update_reset();
void update_add(void* data, update_callback callback, int priority, int mask);
void update_remove(void* data);
void update_remove_with_data(void* data, update_callback callback);

void update_set_layers(void* data, int mask);

void update_pause_layers(int mask);
void update_unpause_layers(int mask);
bool update_has_layer(int mask);

void update_render_time();

void update_dispatch();

#define DEFAULT_TIME_STEP   (1.0f / 30.0f)

extern float fixed_time_step;
extern float scaled_time_step;
extern float scaled_time_step_inv;
extern float total_time;
extern float game_time;
extern float global_time_scale;
extern float render_time_step;

#endif
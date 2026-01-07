#ifndef __CUTSCENE_TIMER_H__
#define __CUTSCENE_TIMER_H__

#include <stdbool.h>

void cutscene_timer_start(float time, const char* on_end);
void cutscene_timer_cancel();
bool cutscene_timer_is_active();
float cutscene_timer_remaining();

#endif
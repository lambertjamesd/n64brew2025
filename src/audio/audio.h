#ifndef __AUDIO_AUDIO_H__

#include <stdint.h>
#include <libdragon.h>
#include "../math/vector3.h"

typedef uint16_t audio_id;

void audio_player_init();
void audio_player_update();

audio_id audio_play_2d(wav64_t* wav, float volume, float pan, float pitch_shift, int16_t priority);
audio_id audio_play_3d(wav64_t* wav, float volume, struct Vector3* pos, struct Vector3* vel, float pitch_shift, int16_t priority);

void audio_update_position(audio_id id, struct Vector3* pos, struct Vector3* vel);
void audio_update_listener(struct Vector3* pos, struct Vector3* right, struct Vector3* velocity);

#endif
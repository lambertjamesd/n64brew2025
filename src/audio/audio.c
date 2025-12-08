#include "audio.h"

#include <stdbool.h>
#include "../math/mathf.h"

#define MAX_ACTIVE_SOUNDS       16
#define SPEED_OF_SOUND          343
#define FULL_VOLUME_DISTANCE    3.0f
#define OUTPUT_SAMPLE_RATE      44100

#define BUFFERS_PER_SECOND      25
#define MAX_ECHO_DURATION       0.5

// about half a second
#define DELAY_BUFFERS           13
#define ACTUAL_BUFFER_DELAY     4

#define TOTAL_AUDIO_BUFFERS     (DELAY_BUFFERS + ACTUAL_BUFFER_DELAY)

struct active_sound {
    wav64_t* wav;
    bool is_3d;
    int16_t priority;
    float volume;
    float frequency;
    struct Vector3 position;
    struct Vector3 velocity;
};

typedef struct active_sound active_sound_t;

struct audio_listener {
    struct Vector3 position;
    struct Vector3 right;
    struct Vector3 velocity;
};

static audio_id active_sound_ids[MAX_ACTIVE_SOUNDS];
static active_sound_t active_sounds[MAX_ACTIVE_SOUNDS];
static audio_id next_id = MAX_ACTIVE_SOUNDS;
static struct audio_listener listener = {
    .position = {0.0f, 0.0f, 0.0f},
    .right = {1.0f, 0.0f, 0.0f},
};

static inline int audio_channel_from_id(audio_id id) {
    return id & (MAX_ACTIVE_SOUNDS - 1);
}

audio_id audio_next_id(int channel) {
    audio_id result = next_id + channel;

    next_id += MAX_ACTIVE_SOUNDS;
    if (next_id == 0) {
        next_id = MAX_ACTIVE_SOUNDS;
    }

    return result;
}

void audio_active_sound_init(active_sound_t* active_sound) {
    active_sound->wav = NULL;
    active_sound->is_3d = false;
    active_sound->priority = 0;
    active_sound->volume = 0;
    active_sound->position = gZeroVec;
    active_sound->velocity = gZeroVec;
    active_sound->frequency = 0.0f;
}

void audio_player_init() {
    audio_init(OUTPUT_SAMPLE_RATE, TOTAL_AUDIO_BUFFERS);
    mixer_init(MAX_ACTIVE_SOUNDS);

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        active_sound_ids[i] = 0;
        audio_active_sound_init(&active_sounds[i]);
    }
}

int audio_time_left(int ch) {
    active_sound_t* curr = &active_sounds[ch];

    return curr->wav->wave.len - (int)mixer_ch_get_pos(ch);
}

audio_id audio_find_available_sound(int16_t priority) {
    for (uint16_t* curr = &active_sound_ids[0]; curr < &active_sound_ids[MAX_ACTIVE_SOUNDS]; curr += 1) {
        if (*curr == 0) {
            return audio_next_id(curr - active_sound_ids);
        }
    }

    active_sound_t* best = NULL;
    int best_index = -1;

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        active_sound_t* curr = &active_sounds[i];

        if (curr->priority > priority) {
            continue;
        }

        if (best == NULL || 
            curr->priority < best->priority || 
            audio_time_left(i) < audio_time_left(best_index)
        ) {
            best = curr;
            best_index = i;
        }
    }

    if (best_index == -1) {
        return 0;
    }

    return audio_next_id(best_index);
}

void audio_set_pan_volume(int channel, float volume, float pan) {
    mixer_ch_set_vol(
        channel,
        volume * clampf(1 - pan, 0.0f, 1.0f),
        volume * clampf(pan + 1, 0.0f, 1.0f)
    );
}

void audio_process_3d(active_sound_t* sound, int channel) {
    struct Vector3 offset;
    vector3Sub(&sound->position, &listener.position, &offset);

    float mag_sqrd = vector3MagSqrd(&offset);

    if (mag_sqrd < 0.00001f) {
        mixer_ch_set_vol(channel, 1.0f, 1.0f);
        return;
    }

    float volume = (FULL_VOLUME_DISTANCE * FULL_VOLUME_DISTANCE) * sound->volume / mag_sqrd;

    if (volume < 0.0001f) {
        mixer_ch_set_vol(channel, 0.0f, 0.0f);
        return;
    }

    vector3Scale(&offset, &offset, 1.0f / sqrtf(mag_sqrd));

    float pan = vector3Dot(&offset, &listener.right);

    audio_set_pan_volume(channel, volume, pan);

    float sound_direction = vector3Dot(&sound->velocity, &offset);
    float listener_direction = vector3Dot(&listener.velocity, &offset);

    float factor = (sound_direction - listener_direction) > 0.0f ? 1.0f : -1.0f;

    float denominator = SPEED_OF_SOUND + sound_direction * factor;

    if (denominator > SPEED_OF_SOUND - 1.0f) {
        denominator = SPEED_OF_SOUND - 1.0f;
    }

    mixer_ch_set_freq(channel, sound->frequency * (SPEED_OF_SOUND - listener_direction * factor) / denominator);
}

audio_id audio_play_2d(wav64_t* wav, float volume, float pan, float pitch_shift, int16_t priority) {
    audio_id audio_id = audio_find_available_sound(priority);

    if (!audio_id) {
        return 0;
    }

    int channel = audio_channel_from_id(audio_id);
    active_sound_ids[channel] = audio_id;

    active_sound_t* sound = &active_sounds[channel];
    sound->priority = priority;
    sound->is_3d = false;
    sound->wav = wav;

    mixer_ch_play(channel, &wav->wave);
    audio_set_pan_volume(channel, volume, pan);
    mixer_ch_set_freq(channel, wav->wave.frequency * pitch_shift);

    return audio_id;
}

audio_id audio_play_3d(wav64_t* wav, float volume, struct Vector3* pos, struct Vector3* vel, float pitch_shift, int16_t priority) {
    audio_id audio_id = audio_find_available_sound(priority);

    if (!audio_id) {
        return 0;
    }
    
    int channel = audio_channel_from_id(audio_id);
    active_sound_ids[channel] = audio_id;

    active_sound_t* sound = &active_sounds[channel];
    sound->priority = priority;
    sound->is_3d = true;
    sound->wav = wav;
    sound->position = *pos;
    sound->velocity = *vel;
    sound->volume = volume;
    sound->frequency = pitch_shift * wav->wave.frequency;

    mixer_ch_play(channel, &wav->wave);
    audio_process_3d(sound, channel);
    
    return audio_id;
}

static short* prev_buffers[TOTAL_AUDIO_BUFFERS];
static uint16_t prev_buffer;

struct audio_sample {
    short l, r;
};

typedef struct audio_sample audio_sample_t;

static float echo_delay = 0.2f;
static int32_t echo_decay = 0x2000; 
static int32_t echo_low_pass = 0xF800;
audio_sample_t prev_lowpass_output;

short audio_sample_reverb(short current_output, short input, short* prev_lowpass) {
    int low_passed = ((int)input * (0x10000 - echo_low_pass) + (int)*prev_lowpass * echo_low_pass) >> 16;
    *prev_lowpass = low_passed;
    
    int result = (int)current_output + (((int)low_passed * echo_decay) >> 16);

    if (result > 0x7fff) {
        return 0x7fff;
    }

    if (result < -0x7fff) {
        return -0x7fff;
    }

    return (short)result;
}

void audio_apply_echo(short* buffer, int buffer_length) {
    if (!echo_decay || !buffer) {
        return;
    }

    int sample_offset = (int)(echo_delay * OUTPUT_SAMPLE_RATE);
    int buffer_offset = (sample_offset + buffer_length - 1) / buffer_length;

    int read_buffer = (prev_buffer + TOTAL_AUDIO_BUFFERS - buffer_offset) % TOTAL_AUDIO_BUFFERS;

    if (!prev_buffers[read_buffer]) {
        return;
    }

    int read_buffer_start = (buffer_length - sample_offset) % buffer_length;

    if (read_buffer_start < 0) {
        read_buffer_start += buffer_length;
    }

    audio_sample_t* output = (audio_sample_t*)buffer;
    audio_sample_t* input = ((audio_sample_t*)prev_buffers[read_buffer]) + read_buffer_start;

    audio_sample_t* input_end = ((audio_sample_t*)prev_buffers[read_buffer]) + buffer_length;

    for (int i = 0; i < buffer_length; i += 1) {
        output->l = audio_sample_reverb(output->l, input->r, &prev_lowpass_output.l);
        output->r = audio_sample_reverb(output->r, input->l, &prev_lowpass_output.r);

        ++output;
        ++input;
        
        if (input == input_end) {
            input = (audio_sample_t*)prev_buffers[(read_buffer + 1) % TOTAL_AUDIO_BUFFERS];
        }
    }
}

void audio_player_update() {
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        if (!active_sound_ids[i]) {
            continue;
        }

        if (!mixer_ch_playing(i)) {
            active_sound_ids[i] = 0;
            continue;
        }
        
        active_sound_t* sound = &active_sounds[i];

        if (sound->is_3d) {
            audio_process_3d(sound, i);
        }
    }

    if (audio_can_write()) {
        short *buf = audio_write_begin();
        prev_buffers[prev_buffer] = buf;

        short *write_buffer = prev_buffers[(prev_buffer + TOTAL_AUDIO_BUFFERS - DELAY_BUFFERS) % TOTAL_AUDIO_BUFFERS];

        if (write_buffer) {
            mixer_poll(write_buffer, audio_get_buffer_length());
            audio_apply_echo(write_buffer, audio_get_buffer_length());
        }
        
        prev_buffer += 1;
        if (prev_buffer == TOTAL_AUDIO_BUFFERS) {
            prev_buffer = 0;
        }

        audio_write_end();
    }
}

void audio_update_position(audio_id id, struct Vector3* pos, struct Vector3* vel) {
    if (!id) {
        return;
    }

    int channel = audio_channel_from_id(id);

    if (active_sound_ids[channel] != id) {
        return;
    }

    
    active_sound_t* sound = &active_sounds[channel];

    if (!sound->is_3d) {
        return;
    }

    sound->position = *pos;
    sound->velocity = *vel;
}

void audio_update_listener(struct Vector3* pos, struct Vector3* right, struct Vector3* velocity) {
    listener.position = *pos;
    listener.right = *right;
    listener.velocity = *velocity;
}
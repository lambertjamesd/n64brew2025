#ifndef __TIME_GAME_MODE_H__
#define __TIME_GAME_MODE_H__

enum game_mode {
    GAME_MODE_3D,
    GAME_MODE_TRANSITION_TO_MENU,
    GAME_MODE_MENU,
};

extern enum game_mode current_game_mode;

#endif
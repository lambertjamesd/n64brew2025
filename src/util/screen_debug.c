#include "screen_debug.h"

#include <libdragon.h>
#include <string.h>

#define N_ROWS  10
#define N_COLS  80

static char screen_debug_grid[N_ROWS][N_COLS+1];
static uint8_t screen_debug_line = 0;

void screen_debug_init() {}

void screen_debug_print_line(const char* text) {
    strncpy(screen_debug_grid[screen_debug_line], text, N_COLS+1);
    screen_debug_grid[screen_debug_line][N_COLS] = '\0';
    ++screen_debug_line;

    if (screen_debug_line == N_ROWS) {
        screen_debug_line = 0;
    }
}

void screen_debug_render() {
    // rdpq_sync_pipe();

    // int line = screen_debug_line;
    // for (int i = 0; i < N_ROWS; i += 1) {
    //     if (line == 0) {
    //         line = N_ROWS - 1;
    //     } else {
    //         line -= 1;
    //     }

    //     rdpq_text_printn(&(rdpq_textparms_t){
    //             // .line_spacing = -3,
    //             .align = ALIGN_LEFT,
    //             .valign = VALIGN_TOP,
    //             .width = 320,
    //             .height = 240,
    //             .wrap = WRAP_NONE,
    //         }, 
    //         1, 
    //         20, i * 20, 
    //         screen_debug_grid[line],
    //         strlen(screen_debug_grid[line])
    //     );
    // }
}
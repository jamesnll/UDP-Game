#ifndef UDP_GAME_DISPLAY_H
#define UDP_GAME_DISPLAY_H

#include "../include/structs.h"
#include <ncurses.h>

void setup_window(WINDOW *w, const struct coordinates *coordinates, const char *player);

#endif    // UDP_GAME_DISPLAY_H

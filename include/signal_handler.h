#ifndef UDP_GAME_SIGNAL_HANDLER_H
#define UDP_GAME_SIGNAL_HANDLER_H

#include <p101_c/p101_string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile sig_atomic_t exit_flag;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void setup_signal_handler(void);
void sigint_handler(int signum);

#endif    // UDP_GAME_SIGNAL_HANDLER_H

#include "../include/display.h"
#include <ncurses.h>
#include <stdlib.h>

int main(void)
{
    initscr();
    addstr("-----------------\n| codedrome.com |\n| ncurses Demo  |\n-----------------\n\n");
    refresh();

    addstr("\npress any key to exit...");
    refresh();

    getch();

    endwin();
    display("Hello, World");

    return EXIT_SUCCESS;
}

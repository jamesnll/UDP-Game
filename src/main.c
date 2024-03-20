#include "../include/display.h"
#include <ncurses.h>
#include <stdlib.h>

int main(void)
{
    int ch;
    initscr();
    cbreak();                // Disable line buffering
    noecho();                // Don't echo user input to the screen
    keypad(stdscr, TRUE);    // Enable keypad mode to capture arrow keys
    // Print a prompt
    printw("Press an arrow key (or q to quit): ");
    refresh();    // Refresh the screen to display the prompt

    while((ch = getch()) != 'q')
    {
        // Handle arrow keys
        switch(ch)
        {
            case KEY_UP:
                mvprintw(1, 0, "You pressed: Up arrow");
                break;
            case KEY_DOWN:
                mvprintw(1, 0, "You pressed: Down arrow");
                break;
            case KEY_LEFT:
                mvprintw(1, 0, "You pressed: Left arrow");
                break;
            case KEY_RIGHT:
                mvprintw(1, 0, "You pressed: Right arrow");
                break;
            default:
                mvprintw(1, 0, "You pressed: %c", ch);
                break;
        }
        refresh();    // Refresh the screen to display the input
    }

    // End ncurses
    endwin();

    return EXIT_SUCCESS;
}

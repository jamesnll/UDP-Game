#include "../include/display.h"
#include "../include/structs.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_Y 5
#define INITIAL_X 7
#define WINDOW_Y_LENGTH 50
#define WINDOW_X_LENGTH 100

int main(void)
{
    WINDOW            *w;
    const char        *player = ".";
    int                ch;
    struct coordinates coordinates;

    coordinates.x = INITIAL_X;
    coordinates.y = INITIAL_Y;

    initscr();                                             // initialize Ncurses
    w = newwin(WINDOW_Y_LENGTH, WINDOW_X_LENGTH, 1, 1);    // create a new window
    setup_window(w, &coordinates, player);

    while((ch = wgetch(w)) != 'q')    // get the input
    {
        // use a variable to increment or decrement the value based on the input.
        switch(ch)
        {
            case KEY_UP:
                coordinates.y--;
                mvwprintw(w, (int)coordinates.y + 1, (int)coordinates.x, "%s", " ");    // replace old character position with space
                break;
            case KEY_DOWN:
                coordinates.y++;
                mvwprintw(w, (int)coordinates.y - 1, (int)coordinates.x, "%s", " ");    // replace old character position with space
                break;
            case KEY_LEFT:
                coordinates.x--;
                mvwprintw(w, (int)coordinates.y, (int)coordinates.x + 1, "%s", " ");    // replace old character position with space
                break;
            case KEY_RIGHT:
                coordinates.x++;
                mvwprintw(w, (int)coordinates.y, (int)coordinates.x - 1, "%s", " ");    // replace old character position with space
                break;
            default:
                break;
        }
        mvwprintw(w, (int)coordinates.y, (int)coordinates.x, "%s", player);    // update the characters position
    }
    delwin(w);
    endwin();

    return EXIT_SUCCESS;
}

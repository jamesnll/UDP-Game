#include "../include/display.h"

void setup_window(WINDOW *w, const struct coordinates *coordinates, const char *player)
{
    box(w, 0, 0);       // sets default borders for the window
    noecho();           // disable echoing of characters on the screen
    keypad(w, TRUE);    // enable keyboard input for the window.
    curs_set(0);        // hide the default screen cursor.

    mvwprintw(w, (int)coordinates->y, (int)coordinates->x, "%s", player);    // Set the position of the characte to (7,5)
    wrefresh(w);                                                             // update the terminal screen
}

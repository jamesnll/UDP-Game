#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_Y 5
#define INITIAL_X 7
#define WINDOW_Y_LENGTH 50
#define WINDOW_X_LENGTH 100

int main(void)
{
    WINDOW     *w;
    const char *character = ".";
    int         ch;
    int         x;
    int         y;

    x = INITIAL_X;
    y = INITIAL_Y;

    initscr();                                             // initialize Ncurses
    w = newwin(WINDOW_Y_LENGTH, WINDOW_X_LENGTH, 1, 1);    // create a new window
    box(w, 0, 0);                                          // sets default borders for the window
    mvwprintw(w, y, x, "%s", character);                   // Set the position of the characte to (7,5)
    wrefresh(w);                                           // update the terminal screen
    noecho();                                              // disable echoing of characters on the screen
    keypad(w, TRUE);                                       // enable keyboard input for the window.
    curs_set(0);                                           // hide the default screen cursor.

    while((ch = wgetch(w)) != 'q')    // get the input
    {
        // use a variable to increment or decrement the value based on the input.
        switch(ch)
        {
            case KEY_UP:
                y--;
                mvwprintw(w, y + 1, x, "%s", " ");    // replace old character position with space
                break;
            case KEY_DOWN:
                y++;
                mvwprintw(w, y - 1, x, "%s", " ");    // replace old character position with space
                break;
            case KEY_LEFT:
                x--;
                mvwprintw(w, y, x + 1, "%s", " ");    // replace old character position with space
                break;
            case KEY_RIGHT:
                x++;
                mvwprintw(w, y, x - 1, "%s", " ");    // replace old character position with space
                break;
            default:
                break;
        }
        mvwprintw(w, y, x, "%s", character);    // update the characters position
    }
    delwin(w);
    endwin();

    return EXIT_SUCCESS;
}

#include "../include/arguments.h"
#include "../include/display.h"
#include "../include/network.h"
#include <ncurses.h>
#include <p101_c/p101_string.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_Y 5
#define INITIAL_X 7
#define WINDOW_Y_LENGTH 50
#define WINDOW_X_LENGTH 100

int main(int argc, char *argv[])
{
    WINDOW            *w;
    const char        *player = ".";
    int                ch;
    int                ret_val;
    struct p101_env   *env;
    struct p101_error *error;
    struct arguments   arguments;
    struct context     context;
    struct coordinates coordinates;

    error = p101_error_create(false);
    if(error == NULL)
    {
        ret_val = EXIT_FAILURE;
        goto done;
    }

    env = p101_env_create(error, true, NULL);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_error;
    }

    p101_memset(env, &arguments, 0, sizeof(arguments));    // Set memory of arguments to 0
    p101_memset(env, &context, 0, sizeof(context));        // Set memory of context to 0
    context.arguments       = &arguments;
    context.arguments->argc = argc;
    context.arguments->argv = argv;

    parse_arguments(env, error, &context);
    check_arguments(env, error, &context);
    parse_in_port_t(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }
    convert_address(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }

    socket_create(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }

    socket_bind(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto close_socket;
    }

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
                if(coordinates.y != 1)
                {
                    coordinates.y--;
                    mvwprintw(w, (int)coordinates.y + 1, (int)coordinates.x, "%s", " ");    // replace old character position with space
                }
                break;
            case KEY_DOWN:
                if(coordinates.y != WINDOW_Y_LENGTH - 2)
                {
                    coordinates.y++;
                    mvwprintw(w, (int)coordinates.y - 1, (int)coordinates.x, "%s", " ");    // replace old character position with space
                }
                break;
            case KEY_LEFT:
                if(coordinates.x != 1)
                {
                    coordinates.x--;
                    mvwprintw(w, (int)coordinates.y, (int)coordinates.x + 1, "%s", " ");    // replace old character position with space
                }
                break;
            case KEY_RIGHT:
                if(coordinates.x != WINDOW_X_LENGTH - 2)
                {
                    coordinates.x++;
                    mvwprintw(w, (int)coordinates.y, (int)coordinates.x - 1, "%s", " ");    // replace old character position with space
                }
                break;
            default:
                break;
        }
        mvwprintw(w, (int)coordinates.y, (int)coordinates.x, "%s", player);    // update the characters position
    }
    delwin(w);
    endwin();

    ret_val = EXIT_SUCCESS;

close_socket:
    socket_close(env, error, &context);

free_env:
    free(context.exit_message);
    free(env);

free_error:
    if(p101_error_has_error(error))
    {
        fprintf(stderr, "Error: %s\n", p101_error_get_message(error));
    }
    p101_error_reset(error);
    free(error);

done:
    printf("Exit code: %d\n", ret_val);
    return ret_val;
}

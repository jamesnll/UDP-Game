#include "../include/convert.h"
#include "../include/display.h"
#include "../include/network.h"
#include <ncurses.h>
#include <p101_c/p101_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#define INITIAL_Y 5
#define INITIAL_X 7
#define WINDOW_Y_LENGTH 50
#define WINDOW_X_LENGTH 100

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define REQUIRED_ARGS_NUM 9

static void           parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static void           check_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static _Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context);

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
    struct coordinates coordinates      = {0};
    struct coordinates read_coordinates = {0};
    uint8_t            buffer[sizeof(coordinates.old_x) + sizeof(coordinates.old_y) + sizeof(coordinates.new_x) + sizeof(coordinates.new_y)];
    fd_set             readfds;
    int                maxfd;

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
    convert_client_args(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }

    socket_create(env, error, &context.settings.sockfd, context.settings.src_addr.ss_family);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }

    socket_bind(env, error, context.settings.sockfd, context.settings.src_port, &context.settings.src_addr);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto close_socket;
    }

    coordinates.old_x = INITIAL_X;
    coordinates.old_y = INITIAL_Y;
    coordinates.new_x = INITIAL_X;
    coordinates.new_y = INITIAL_Y;

    initscr();                                             // initialize Ncurses
    w = newwin(WINDOW_Y_LENGTH, WINDOW_X_LENGTH, 1, 1);    // create a new window
    setup_window(w, &coordinates, player);
    while(1)    // get the input
    {
        struct timeval timeout;

        // Clear the socket set
        FD_ZERO(&readfds);

        FD_SET(STDIN_FILENO, &readfds);               // Add standard input to the set
        FD_SET(context.settings.sockfd, &readfds);    // Add socket file descriptor to the set

        timeout.tv_sec  = 0;    // Set seconds to 0 for immediate return
        timeout.tv_usec = 0;    // Set microseconds to 0 for immediate return

        maxfd = (STDIN_FILENO > context.settings.sockfd) ? STDIN_FILENO : context.settings.sockfd;

        // Wait for activity on either standard input or socket
        if(select(maxfd + 1, &readfds, NULL, NULL, &timeout) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // If activity is on the socket (reading)
        if(FD_ISSET(context.settings.sockfd, &readfds))
        {
            ssize_t bytes_read;
            bytes_read = socket_read_full(env, context.settings.sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&context.settings.src_addr, context.settings.dest_addr_len);
            if(bytes_read > 0)
            {
                deserialize_position_from_buffer(env, &read_coordinates, buffer);
                mvwprintw(w, (int)read_coordinates.old_y, (int)read_coordinates.old_x, "%s", " ");
                if(read_coordinates.new_x == EXIT_COORDINATE && read_coordinates.new_y == EXIT_COORDINATE)
                {
                    mvwprintw(w, (int)read_coordinates.new_y, (int)read_coordinates.new_x, "%s", " ");
                }
                else
                {
                    mvwprintw(w, (int)read_coordinates.new_y, (int)read_coordinates.new_x, "%s", player);
                }
                wrefresh(w);
                memset(buffer, 0, sizeof(buffer));
            }
        }

        // Check if there is input from the user
        if(FD_ISSET(STDIN_FILENO, &readfds))
        {
            ch = wgetch(w);
            if(ch == 'q')
            {
                break;    // Exit loop if 'q' is pressed
            }
            // use a variable to increment or decrement the value based on the input.
            switch(ch)
            {
                case KEY_UP:
                    if(coordinates.new_y != 1)
                    {
                        coordinates.old_x = coordinates.new_x;
                        coordinates.old_y = coordinates.new_y;
                        coordinates.new_y--;
                        mvwprintw(w, (int)coordinates.new_y + 1, (int)coordinates.new_x, "%s", " ");    // replace old character position with space
                    }
                    break;
                case KEY_DOWN:
                    if(coordinates.new_y != WINDOW_Y_LENGTH - 2)
                    {
                        coordinates.old_x = coordinates.new_x;
                        coordinates.old_y = coordinates.new_y;
                        coordinates.new_y++;
                        mvwprintw(w, (int)coordinates.new_y - 1, (int)coordinates.new_x, "%s", " ");    // replace old character position with space
                    }
                    break;
                case KEY_LEFT:
                    if(coordinates.new_x != 1)
                    {
                        coordinates.old_x = coordinates.new_x;
                        coordinates.old_y = coordinates.new_y;
                        coordinates.new_x--;
                        mvwprintw(w, (int)coordinates.new_y, (int)coordinates.new_x + 1, "%s", " ");    // replace old character position with space
                    }
                    break;
                case KEY_RIGHT:
                    if(coordinates.new_x != WINDOW_X_LENGTH - 2)
                    {
                        coordinates.old_x = coordinates.new_x;
                        coordinates.old_y = coordinates.new_y;
                        coordinates.new_x++;
                        mvwprintw(w, (int)coordinates.new_y, (int)coordinates.new_x - 1, "%s", " ");    // replace old character position with space
                    }
                    break;
                default:
                    break;
            }
            mvwprintw(w, (int)coordinates.new_y, (int)coordinates.new_x, "%s", player);                                                                                 // update the characters position
            wrefresh(w);                                                                                                                                                // update the terminal screen
            serialize_position_to_buffer(env, &coordinates, buffer);                                                                                                    // Serialize the coordinates struct
            socket_write_full(env, context.settings.sockfd, buffer, sizeof(buffer), (struct sockaddr *)&context.settings.dest_addr, context.settings.dest_addr_len);    // Send updated coordinates to server
            memset(buffer, 0, sizeof(buffer));
        }
    }
    delwin(w);
    endwin();

    // write the exit coords to server
    coordinates.new_x = EXIT_COORDINATE;
    coordinates.new_y = EXIT_COORDINATE;
    serialize_position_to_buffer(env, &coordinates, buffer);
    socket_write_full(env, context.settings.sockfd, buffer, sizeof(buffer), (struct sockaddr *)&context.settings.dest_addr, context.settings.dest_addr_len);

    ret_val = EXIT_SUCCESS;

close_socket:
    socket_close(env, error, &context);

free_env:
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

static void parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context)
{
    int opt;

    P101_TRACE(env);

    context->arguments->program_name = context->arguments->argv[0];
    opterr                           = 0;

    while((opt = getopt(context->arguments->argc, context->arguments->argv, "hA:P:a:p:")) != -1)
    {
        switch(opt)
        {
            case 'a':    // Source IP address argument
            {
                context->arguments->src_ip_address = optarg;
                break;
            }
            case 'p':    // Source port argument
            {
                context->arguments->src_port_str = optarg;
                break;
            }
            case 'A':    // Destination IP address argument
            {
                context->arguments->dest_ip_address = optarg;
                break;
            }
            case 'P':    // Destination port argument
            {
                context->arguments->dest_port_str = optarg;
                printf("dest port: %s\n", optarg);
                break;
            }
            case 'h':    // Help argument
            {
                goto usage;
            }
            case '?':
            {
                char message[UNKNOWN_OPTION_MESSAGE_LEN];

                snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                context->exit_message = p101_strdup(env, err, message);
                goto usage;
            }
            default:
            {
                context->exit_message = p101_strdup(env, err, "Unknown error with getopt.");
                goto usage;
            }
        }
    }

    if(optind > REQUIRED_ARGS_NUM)
    {
        context->exit_message = p101_strdup(env, err, "Too many arguments.");
        goto usage;
    }

    return;

usage:
    usage(env, err, context);
}

static void check_arguments(struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    if(context->arguments->src_ip_address == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<source ip_address> must be passed.");
        goto usage;
    }

    if(context->arguments->src_port_str == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<source port> must be passed.");
        goto usage;
    }

    if(context->arguments->dest_ip_address == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<destination ip address> must be passed.");
        goto usage;
    }

    if(context->arguments->dest_port_str == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<destination port> must be passed.");
        goto usage;
    }

    context->settings.src_ip_address  = context->arguments->src_ip_address;
    context->settings.dest_ip_address = context->arguments->dest_ip_address;
    return;

usage:
    usage(env, err, context);
}

static _Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    context->exit_code = EXIT_FAILURE;

    if(context->exit_message != NULL)
    {
        fprintf(stderr, "%s\n", context->exit_message);
    }

    fprintf(stderr, "Usage: %s [-h] -a <source ip_address> -p <source port> -A <destination ip address> -P <destination port>\n", context->arguments->program_name);
    fputs("Options:\n", stderr);
    fputs("  -h Display this help message\n", stderr);
    fputs("  -a <source ip_address>  Option 'a' (required) with an IP Address.\n", stderr);
    fputs("  -p <source port>        Option 'p' (required) with a port.\n", stderr);
    fputs("  -a <destination ip_address>  Option 'A' (required) with an IP Address.\n", stderr);
    fputs("  -p <destination port>        Option 'P' (required) with a port.\n", stderr);

    free(context->exit_message);
    free(env);
    p101_error_reset(err);
    free(err);

    printf("Exit code: %d\n", context->exit_code);
    exit(context->exit_code);
}

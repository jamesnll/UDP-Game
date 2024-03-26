#include "../include/convert.h"
#include "../include/network.h"
#include "../include/signal_handler.h"
#include <p101_c/p101_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define REQUIRED_ARGS_NUM 5

static void           parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static void           check_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static _Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context);

int main(int argc, char *argv[])
{
    int                ret_val;
    struct p101_error *error;
    struct p101_env   *env;
    struct arguments   arguments;
    struct context     context;

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
    convert_server_args(env, error, &context);
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

    setup_signal_handler();
    while(!exit_flag)
    {
        struct sockaddr_in client_addr;
        char               client_ip[INET6_ADDRSTRLEN];
        socklen_t          client_addr_len;
        struct coordinates coordinates;
        ssize_t            bytes_read;
        uint8_t            buffer[sizeof(coordinates.x) + sizeof(coordinates.y)];

        client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        coordinates.x = 0;
        coordinates.y = 0;

        bytes_read = socket_read_full(env, context.settings.sockfd, buffer, sizeof(buffer), (struct sockaddr *)&client_addr, client_addr_len);
        deserialize_position_from_buffer(env, &coordinates, buffer);
        printf("Bytes read: %zu\n X: %d\nY: %d\n", (size_t)bytes_read, (int)coordinates.x, (int)coordinates.y);
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client IP address: %s\n", client_ip);
        printf("Client port: %d\n", ntohs(client_addr.sin_port));
    }

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

static void parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context)
{
    int opt;

    P101_TRACE(env);

    context->arguments->program_name = context->arguments->argv[0];
    opterr                           = 0;

    while((opt = getopt(context->arguments->argc, context->arguments->argv, "ha:p:")) != -1)
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
        context->exit_message = p101_strdup(env, err, "<ip_address> must be passed.");
        goto usage;
    }

    if(context->arguments->src_port_str == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<port> must be passed.");
        goto usage;
    }

    context->settings.src_ip_address = context->arguments->src_ip_address;
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

    fprintf(stderr, "Usage: %s [-h] -a <ip_address> -p <port>\n", context->arguments->program_name);
    fputs("Options:\n", stderr);
    fputs("  -h Display this help message\n", stderr);
    fputs("  -a <ip_address>  Option 'a' (required) with an IP Address.\n", stderr);
    fputs("  -p <port>        Option 'p' (required) with a port.\n", stderr);

    free(context->exit_message);
    free(env);
    p101_error_reset(err);
    free(err);

    printf("Exit code: %d\n", context->exit_code);
    exit(context->exit_code);
}

/*
 * TODO: Storing client IP addresses to broadcast
 * have an array of string ip addresses
 * dynamic memory
 * reallocs for each connection
 * check if ip address is in the array
 * if found, skip
 * if not found, add to the array
 * frees when disconnected
 */

/*
 * TODO: Broadcasting messages
 * receive message from client
 * store client IP address into array (from steps above) if needed
 * iterate through the IP address array
 * if the recv msg ip address matches the index in the array, continue
 * if the recv msg ip address doesn't match the index in the array, send the recv msg to that IP address
 */

/*
 * TODO: Removing a client IP address from the array
 * Client disconnects, send a special set of coords? (something out of bounds)
 * Server receives the coords
 * If the coords match the "exit coords"
 * Free that index from the array
 * Reorder array
 */

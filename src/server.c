#include "../include/convert.h"
#include "../include/network.h"
#include "../include/signal_handler.h"
#include <p101_c/p101_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define REQUIRED_ARGS_NUM 5
#define MAX_CLIENTS 10
#define PORT_SIZE 5

static void           parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static void           check_arguments(struct p101_env *env, struct p101_error *err, struct context *context);
static _Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context);
static int            check_existing_client_address(const struct p101_env *env, char *client_addresses[], const char *ip_address);
static void           add_client_address(const struct p101_env *env, char *client_addresses[], const char *ip_address, char *client_ports[], in_port_t port);

// static void           broadcast_coordinates(const struct p101_env *env, struct p101_error *err, char *client_addresses[], int address_index, struct coordinates *coordinates);

int main(int argc, char *argv[])
{
    int                ret_val;
    struct p101_error *error;
    struct p101_env   *env;
    struct arguments   arguments;
    struct context     context;
    char              *client_addresses[MAX_CLIENTS] = {0};
    char              *client_ports[MAX_CLIENTS]     = {0};

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
        int                address_index;

        client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        coordinates.x = 0;
        coordinates.y = 0;

        bytes_read = socket_read_full(env, context.settings.sockfd, buffer, sizeof(buffer), (struct sockaddr *)&client_addr, client_addr_len);
        if(bytes_read == -1)
        {
            break;
        }
        deserialize_position_from_buffer(env, &coordinates, buffer);
        printf("Bytes read: %zd\n X: %d\nY: %d\n", bytes_read, (int)coordinates.x, (int)coordinates.y);
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        address_index = check_existing_client_address(env, client_addresses, client_ip);
        if(address_index == -1)
        {
            add_client_address(env, client_addresses, client_ip, client_ports, client_addr.sin_port);
        }

        // Remove if exit coords
        if((coordinates.x == EXIT_COORDINATE && coordinates.y == EXIT_COORDINATE) && address_index != 1)
        {
            printf("Removed client address %s\n", client_addresses[address_index]);
            free(client_addresses[address_index]);
            free(client_ports[address_index]);
            client_ports[address_index]     = NULL;
            client_addresses[address_index] = NULL;
        }

        // broadcast
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(client_addresses[i] != NULL)
        {
            free(client_addresses[i]);
        }
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

static int check_existing_client_address(const struct p101_env *env, char *client_addresses[], const char *ip_address)    // cppcheck-suppress constParameter
{
    P101_TRACE(env);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(client_addresses[i] != NULL && strcmp(client_addresses[i], ip_address) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void add_client_address(const struct p101_env *env, char *client_addresses[], const char *ip_address, char *client_ports[], in_port_t port)
{
    P101_TRACE(env);

    // iterate through the loop
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        char port_str[PORT_SIZE];

        if(client_addresses[i] == NULL && client_ports[i] == NULL)
        {
            client_addresses[i] = strdup(ip_address);
            printf("Client address %s stored at index %d\n", ip_address, i);

            printf("Port: %hu\n", ntohs(port));
            snprintf(port_str, PORT_SIZE, "%hu", ntohs(port));
            client_ports[i] = strdup(port_str);
            printf("Client port %s stored at index %d\n", port_str, i);

            break;
        }
    }
}

// static void broadcast_coordinates(const struct p101_env *env, struct p101_error *err, char *client_addresses[], int address_index, struct coordinates *coordinates)
//{
//     uint8_t                 buffer[sizeof(coordinates.x) + sizeof(coordinates.y)];
//     struct sockaddr_storage dest_addr;
//     socklen_t               src_addr_len;
//     in_port_t               port;
//
//     P101_TRACE(env);
// }

/*
 * TODO: Broadcasting messages
 * receive message from client
 * store client IP address into array (from steps above) if needed
 * iterate through the IP address array
 * if the recv msg ip address matches the index in the array, continue
 * if the recv msg ip address doesn't match the index in the array, send the recv msg to that IP address
 */

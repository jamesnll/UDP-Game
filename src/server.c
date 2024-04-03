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
static int            check_existing_client_address(const struct p101_env *env, struct client_info *clients, const char *ip_address);
static void           add_client(const struct p101_env *env, struct client_info *clients, const char *ip_address, in_port_t port, struct coordinates *coordinates);
static void           update_client(const struct p101_env *env, struct client_info *clients, struct coordinates *coordinates, int client_index);
static void           broadcast_coordinates(const struct p101_env *env, struct p101_error *err, int sockfd, struct client_info *clients, const char *address, int client_index);

int main(int argc, char *argv[])
{
    int                ret_val;
    struct p101_error *error;
    struct p101_env   *env;
    struct arguments   arguments;
    struct context     context;
    struct client_info clients[MAX_CLIENTS] = {0};

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
        uint8_t            buffer[sizeof(coordinates.old_x) + sizeof(coordinates.old_y) + sizeof(coordinates.new_x) + sizeof(coordinates.new_y)];
        int                client_index;

        client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        bytes_read = socket_read_full(env, context.settings.sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, client_addr_len);
        if(bytes_read == -1)
        {
            break;
        }
        deserialize_position_from_buffer(env, &coordinates, buffer);
        printf("Bytes read: %zd\nold X: %d\nold Y: %d\nnew x: %d\nnew y: %d\n", bytes_read, (int)coordinates.old_x, (int)coordinates.old_y, (int)coordinates.new_x, (int)coordinates.new_y);
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client ip: %s\n", client_ip);
        client_index = check_existing_client_address(env, clients, client_ip);
        printf("client index: %d\n", client_index);
        if(client_index == -1)
        {
            add_client(env, clients, client_ip, client_addr.sin_port, &coordinates);
        }
        else
        {
            update_client(env, clients, &coordinates, client_index);
            // broadcast
            broadcast_coordinates(env, error, context.settings.sockfd, clients, client_ip, client_index);
        }

        // Remove if exit coords
        if((coordinates.new_x == EXIT_COORDINATE && coordinates.new_y == EXIT_COORDINATE) && client_index != 1)
        {
            printf("Removed client address %s\n", clients[client_index].client_ip);
            clients[client_index].client_ip[0] = '\0';
            clients[client_index].client_port  = 0;
            memset(&clients[client_index].coordinates, 0, sizeof(struct coordinates));
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

static int check_existing_client_address(const struct p101_env *env, struct client_info *clients, const char *ip_address)    // cppcheck-suppress constParameterPointer
{
    P101_TRACE(env);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(strcmp(clients[i].client_ip, ip_address) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void add_client(const struct p101_env *env, struct client_info *clients, const char *ip_address, in_port_t port, struct coordinates *coordinates)    // cppcheck-suppress constParameterPointer
{
    P101_TRACE(env);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].client_ip[0] == '\0')
        {
            strncpy(clients[i].client_ip, ip_address, INET6_ADDRSTRLEN);
            clients[i].client_ip[INET6_ADDRSTRLEN - 1] = '\0';
            clients[i].client_port                     = htons(port);
            clients[i].coordinates                     = *coordinates;
            break;
        }
    }
}

static void update_client(const struct p101_env *env, struct client_info *clients, struct coordinates *coordinates, int client_index)    // cppcheck-suppress constParameterPointer
{
    P101_TRACE(env);

    clients[client_index].coordinates = *coordinates;
}

static void broadcast_coordinates(const struct p101_env *env, struct p101_error *err, int sockfd, struct client_info *clients, const char *address, int client_index)    // cppcheck-suppress constParameterPointer
{
    struct coordinates      coordinates;
    uint8_t                 buffer[sizeof(coordinates.old_x) + sizeof(coordinates.old_y) + sizeof(coordinates.new_x) + sizeof(coordinates.new_y)];
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    ssize_t                 bytes_written;

    P101_TRACE(env);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].client_ip[0] != '\0')
        {
            printf("%s %s\n", address, clients[i].client_ip);
            // Skip if the received address is the index address
            if(strcmp(address, clients[i].client_ip) == 0)
            {
                continue;
            }

            convert_address(env, err, clients[i].client_ip, &addr, &addr_len);
            get_address_to_server(env, err, &addr, clients[i].client_port);

            serialize_position_to_buffer(env, &clients[client_index].coordinates, buffer);
            bytes_written = socket_write_full(env, sockfd, buffer, sizeof(buffer), (struct sockaddr *)&addr, addr_len);
            printf("%zd bytes sent to client %s\n", bytes_written, clients[i].client_ip);
        }
    }
}

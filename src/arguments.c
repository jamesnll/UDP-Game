#include "../include/arguments.h"

void parse_arguments(struct p101_env *env, struct p101_error *err, struct context *context)
{
    int opt;

    P101_TRACE(env);

    context->arguments->program_name = context->arguments->argv[0];
    opterr                           = 0;

    while((opt = getopt(context->arguments->argc, context->arguments->argv, "ha:p:")) != -1)
    {
        switch(opt)
        {
            case 'a':    // IP address argument
            {
                context->arguments->ip_address = optarg;
                break;
            }
            case 'p':    // Port argument
            {
                context->arguments->port_str = optarg;
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

void check_arguments(struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    if(context->arguments->ip_address == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<ip_address> must be passed.");
        goto usage;
    }

    if(context->arguments->port_str == NULL)
    {
        context->exit_message = p101_strdup(env, err, "<port> must be passed.");
        goto usage;
    }

    context->settings.ip_address = context->arguments->ip_address;
    return;

usage:
    usage(env, err, context);
}

void parse_in_port_t(struct p101_env *env, struct p101_error *err, struct context *context)
{
    char     *endptr;
    uintmax_t parsed_value;

    P101_TRACE(env);

    errno        = 0;
    parsed_value = strtoumax(context->arguments->port_str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        P101_ERROR_RAISE_USER(err, "Couldn't parse in_port_t", EXIT_FAILURE);
        goto done;
    }

    if(*endptr != '\0')
    {
        context->exit_message = p101_strdup(env, err, "Invalid characters in input.");
        goto usage;
    }

    if(parsed_value > UINT16_MAX)
    {
        context->exit_message = p101_strdup(env, err, "in_port_t value out of range.");
        goto usage;
    }

    context->settings.port = (in_port_t)parsed_value;
    goto done;

usage:
    usage(env, err, context);

done:
    return;
}

void convert_address(const struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    p101_memset(env, &context->settings.addr, 0, sizeof(context->settings.addr));

    if(inet_pton(AF_INET, context->settings.ip_address, &(((struct sockaddr_in *)&context->settings.addr)->sin_addr)) == 1)
    {
        context->settings.addr.ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, context->settings.ip_address, &(((struct sockaddr_in *)&context->settings.addr)->sin_addr)) == 1)
    {
        context->settings.addr.ss_family = AF_INET6;
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "Address is not an IPv4 or an IPv6 address", EXIT_FAILURE);
    }
}

_Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context)
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

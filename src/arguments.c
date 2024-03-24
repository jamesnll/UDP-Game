#include "../include/arguments.h"

in_port_t parse_in_port_t(const struct p101_env *env, struct p101_error *err, const char *port_str)
{
    char     *endptr;
    uintmax_t parsed_value;

    P101_TRACE(env);

    errno        = 0;
    parsed_value = strtoumax(port_str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        P101_ERROR_RAISE_USER(err, "Couldn't parse in_port_t", EXIT_FAILURE);
        parsed_value = 0;
        goto done;
    }

    if(*endptr != '\0')
    {
        P101_ERROR_RAISE_USER(err, "Invalid characters in input", EXIT_FAILURE);
        parsed_value = 0;
        goto done;
    }

    if(parsed_value > UINT16_MAX)
    {
        P101_ERROR_RAISE_USER(err, "in_port_t value out of range.", EXIT_FAILURE);
        parsed_value = 0;
        goto done;
    }

done:
    return (in_port_t)parsed_value;
}

void convert_address(const struct p101_env *env, struct p101_error *err, const char *ip_address, struct sockaddr_storage *addr)
{
    P101_TRACE(env);

    p101_memset(env, addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, ip_address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, ip_address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "Address is not an IPv4 or an IPv6 address", EXIT_FAILURE);
    }
}

//_Noreturn void usage(struct p101_env *env, struct p101_error *err, struct context *context)
//{
//    P101_TRACE(env);
//
//    context->exit_code = EXIT_FAILURE;
//
//    if(context->exit_message != NULL)
//    {
//        fprintf(stderr, "%s\n", context->exit_message);
//    }
//
//    fprintf(stderr, "Usage: %s [-h] -a <ip_address> -p <port>\n", context->arguments->program_name);
//    fputs("Options:\n", stderr);
//    fputs("  -h Display this help message\n", stderr);
//    fputs("  -a <ip_address>  Option 'a' (required) with an IP Address.\n", stderr);
//    fputs("  -p <port>        Option 'p' (required) with a port.\n", stderr);
//
//    free(context->exit_message);
//    free(env);
//    p101_error_reset(err);
//    free(err);
//
//    printf("Exit code: %d\n", context->exit_code);
//    exit(context->exit_code);
//}

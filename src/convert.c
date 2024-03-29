#include "../include/convert.h"

void convert_client_args(const struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    context->settings.src_port = parse_in_port_t(env, err, context->arguments->src_port_str);
    if(p101_error_has_error(err))
    {
        goto done;
    }

    context->settings.dest_port = parse_in_port_t(env, err, context->arguments->dest_port_str);
    if(p101_error_has_error(err))
    {
        goto done;
    }

    convert_address(env, err, context->settings.src_ip_address, &context->settings.src_addr, &context->settings.src_addr_len);
    if(p101_error_has_error(err))
    {
        goto done;
    }

    convert_address(env, err, context->settings.dest_ip_address, &context->settings.dest_addr, &context->settings.dest_addr_len);
    if(p101_error_has_error(err))
    {
        goto done;
    }

    get_address_to_server(env, err, &context->settings.dest_addr, context->settings.dest_port);
    if(p101_error_has_error(err))
    {
        goto done;
    }

done:
    return;
}

void convert_server_args(const struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    context->settings.src_port = parse_in_port_t(env, err, context->arguments->src_port_str);
    if(p101_error_has_error(err))
    {
        goto done;
    }

    convert_address(env, err, context->settings.src_ip_address, &context->settings.src_addr, &context->settings.src_addr_len);
    if(p101_error_has_error(err))
    {
        goto done;
    }

done:
    return;
}

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

void convert_address(const struct p101_env *env, struct p101_error *err, const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len)
{
    P101_TRACE(env);

    p101_memset(env, addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, ip_address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
        *addr_len       = sizeof(struct sockaddr_in);
    }
    else if(inet_pton(AF_INET6, ip_address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
        *addr_len       = sizeof(struct sockaddr_in6);
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "Address is not an IPv4 or an IPv6 address", EXIT_FAILURE);
    }
}

void get_address_to_server(const struct p101_env *env, struct p101_error *err, struct sockaddr_storage *addr, in_port_t port)
{
    P101_TRACE(env);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr             = (struct sockaddr_in *)addr;
        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port   = htons(port);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr              = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port   = htons(port);
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "Server address couldn't be found", EXIT_FAILURE);
    }
}

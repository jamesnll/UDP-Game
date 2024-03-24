#include "../include/network.h"

void socket_create(const struct p101_env *env, struct p101_error *err, struct context *context)
{
    P101_TRACE(env);

    context->settings.sockfd = socket(context->settings.addr.ss_family, SOCK_DGRAM, 0);

    if(context->settings.sockfd == -1)
    {
        P101_ERROR_RAISE_USER(err, "socket creation failed", EXIT_FAILURE);
    }
}

void socket_bind(const struct p101_env *env, struct p101_error *err, struct context *context)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    P101_TRACE(env);
    net_port = htons(context->settings.port);

    if(context->settings.addr.ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)&context->settings.addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)&context->settings.addr)->sin_addr);
    }
    else if(context->settings.addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)&context->settings.addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)&context->settings.addr)->sin6_addr);
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "addr->ss_family must be AF_INET or AF_INET6", EXIT_FAILURE);
        goto done;
    }

    if(inet_ntop(context->settings.addr.ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        P101_ERROR_RAISE_USER(err, "inet_ntop failed", EXIT_FAILURE);
        goto done;
    }

    printf("Binding to: %s:%u\n", addr_str, context->settings.port);

    if(bind(context->settings.sockfd, (struct sockaddr *)&context->settings.addr, addr_len) == -1)
    {
        P101_ERROR_RAISE_USER(err, "Binding failed", EXIT_FAILURE);
        goto done;
    }

    printf("Bound to socket: %s:%u\n", addr_str, context->settings.port);

done:
    return;
}

void serialize_position_to_buffer(const struct p101_env *env, const struct coordinates *coordinates, uint8_t *buffer)
{
    uint32_t net_x;
    uint32_t net_y;

    P101_TRACE(env);

    net_x = htonl(coordinates->x);
    net_y = htonl(coordinates->y);

    memcpy(buffer, &net_x, sizeof(net_x));
    memcpy(buffer + sizeof(net_x), &net_y, sizeof(net_y));
}

void deserialize_position_from_buffer(const struct p101_env *env, struct coordinates *coordinates, const uint8_t *buffer)
{
    uint32_t net_x;
    uint32_t net_y;

    P101_TRACE(env);

    memcpy(&net_x, buffer, sizeof(net_x));
    memcpy(&net_y, buffer + sizeof(net_x), sizeof(net_y));

    coordinates->x = ntohl(net_x);
    coordinates->y = ntohl(net_y);
}

void socket_close(const struct p101_env *env, struct p101_error *err, const struct context *context)
{
    P101_TRACE(env);

    if(close(context->settings.sockfd) == -1)
    {
        P101_ERROR_RAISE_USER(err, "socket close failed", EXIT_FAILURE);
    }
}

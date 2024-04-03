#include "../include/network.h"

void socket_create(const struct p101_env *env, struct p101_error *err, int *sockfd, int domain)
{
    P101_TRACE(env);

    *sockfd = socket(domain, SOCK_DGRAM | SOCK_CLOEXEC, 0);

    if(*sockfd == -1)
    {
        P101_ERROR_RAISE_USER(err, "socket creation failed", EXIT_FAILURE);
    }
}

void socket_bind(const struct p101_env *env, struct p101_error *err, int sockfd, in_port_t port, struct sockaddr_storage *addr)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    P101_TRACE(env);
    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        P101_ERROR_RAISE_USER(err, "addr->ss_family must be AF_INET or AF_INET6", EXIT_FAILURE);
        goto done;
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        P101_ERROR_RAISE_USER(err, "inet_ntop failed", EXIT_FAILURE);
        goto done;
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        P101_ERROR_RAISE_USER(err, "Binding failed", EXIT_FAILURE);
        goto done;
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);

done:
    return;
}

void serialize_position_to_buffer(const struct p101_env *env, const struct coordinates *coordinates, uint8_t *buffer)
{
    uint32_t net_old_x;
    uint32_t net_old_y;
    uint32_t net_new_x;
    uint32_t net_new_y;

    P101_TRACE(env);

    net_old_x = htonl(coordinates->old_x);
    net_old_y = htonl(coordinates->old_y);
    net_new_x = htonl(coordinates->new_x);
    net_new_y = htonl(coordinates->new_y);

    memcpy(buffer, &net_old_x, sizeof(net_old_x));
    memcpy(buffer + sizeof(net_old_x), &net_old_y, sizeof(net_old_y));
    memcpy(buffer + sizeof(net_old_x) + sizeof(net_old_y), &net_new_x, sizeof(net_new_x));
    memcpy(buffer + sizeof(net_old_x) + sizeof(net_old_y) + sizeof(net_new_x), &net_new_y, sizeof(net_new_y));
}

void deserialize_position_from_buffer(const struct p101_env *env, struct coordinates *coordinates, const uint8_t *buffer)
{
    uint32_t net_old_x;
    uint32_t net_old_y;
    uint32_t net_new_x;
    uint32_t net_new_y;

    P101_TRACE(env);

    memcpy(&net_old_x, buffer, sizeof(net_old_x));
    memcpy(&net_old_y, buffer + sizeof(net_old_x), sizeof(net_old_y));
    memcpy(&net_new_x, buffer + sizeof(net_old_x) + sizeof(net_old_y), sizeof(net_new_x));
    memcpy(&net_new_y, buffer + sizeof(net_old_x) + sizeof(net_old_y) + sizeof(net_new_x), sizeof(net_new_y));

    coordinates->old_x = ntohl(net_old_x);
    coordinates->old_y = ntohl(net_old_y);
    coordinates->new_x = ntohl(net_new_x);
    coordinates->new_y = ntohl(net_new_y);
}

ssize_t socket_read_full(const struct p101_env *env, int sockfd, uint8_t *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t addrlen)
{
    size_t total_read;

    P101_TRACE(env);

    total_read = 0;

    while(total_read < size)
    {
        ssize_t bytes_read;

        bytes_read = recvfrom(sockfd, buffer + total_read, size - total_read, flags, addr, &addrlen);

        if(bytes_read == -1)
        {
            return -1;
        }

        if(bytes_read == 0)
        {
            return -2;    // EOF reached before reading desired size
        }

        total_read += (size_t)bytes_read;
    }

    return (ssize_t)total_read;
}

ssize_t socket_write_full(const struct p101_env *env, int sockfd, const uint8_t *buffer, size_t size, const struct sockaddr *addr, socklen_t addrlen)
{
    size_t total_written;

    P101_TRACE(env);

    total_written = 0;

    while(total_written < size)
    {
        ssize_t bytes_written;

        bytes_written = sendto(sockfd, buffer + total_written, size - total_written, 0, addr, addrlen);

        if(bytes_written == -1)
        {
            return -1;
        }

        total_written += (size_t)bytes_written;
    }

    return (ssize_t)total_written;
}

void socket_close(const struct p101_env *env, struct p101_error *err, const struct context *context)
{
    P101_TRACE(env);

    if(close(context->settings.sockfd) == -1)
    {
        P101_ERROR_RAISE_USER(err, "socket close failed", EXIT_FAILURE);
    }
}

#ifndef UDP_GAME_NETWORK_H
#define UDP_GAME_NETWORK_H

#include "../include/structs.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <p101_env/env.h>
#include <p101_posix/sys/p101_socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void    socket_create(const struct p101_env *env, struct p101_error *err, int *sockfd, int domain);
void    socket_bind(const struct p101_env *env, struct p101_error *err, int sockfd, in_port_t port, struct sockaddr_storage *addr);
void    serialize_position_to_buffer(const struct p101_env *env, const struct coordinates *coordinates, uint8_t *buffer);
void    deserialize_position_from_buffer(const struct p101_env *env, struct coordinates *coordinates, const uint8_t *buffer);
ssize_t socket_read_full(const struct p101_env *env, int sockfd, struct sockaddr *addr, socklen_t *addrlen, uint8_t *buffer, size_t size);
ssize_t socket_write_full(const struct p101_env *env, int sockfd, const struct sockaddr *addr, socklen_t addrlen, const uint8_t *buffer, size_t size);
void    socket_close(const struct p101_env *env, struct p101_error *err, const struct context *context);

#endif    // UDP_GAME_NETWORK_H

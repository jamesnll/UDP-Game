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

void socket_create(const struct p101_env *env, struct p101_error *err, struct context *context);
void socket_bind(const struct p101_env *env, struct p101_error *err, struct context *context);
void serialize_position_to_buffer(const struct p101_env *env, const struct coordinates *coordinates, uint8_t *buffer);
void deserialize_position_from_buffer(const struct p101_env *env, struct coordinates *coordinates, const uint8_t *buffer);
// void socket_read_full();
// void socket_write_full();
void socket_close(const struct p101_env *env, struct p101_error *err, const struct context *context);

#endif    // UDP_GAME_NETWORK_H

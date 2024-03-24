#ifndef UDP_GAME_ARGUMENTS_H
#define UDP_GAME_ARGUMENTS_H

#define BASE_TEN 10

#include "../include/structs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <p101_c/p101_string.h>
#include <p101_posix/p101_string.h>
#include <p101_unix/p101_getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

in_port_t parse_in_port_t(const struct p101_env *env, struct p101_error *err, const char *port_str);
void      convert_address(const struct p101_env *env, struct p101_error *err, const char *ip_address, struct sockaddr_storage *addr);

#endif    // UDP_GAME_ARGUMENTS_H

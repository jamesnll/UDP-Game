#ifndef UDP_GAME_STRUCTS_H
#define UDP_GAME_STRUCTS_H

#include <netinet/in.h>
#include <stdint.h>

#define MESSAGE_LENGTH 128

struct arguments
{
    int         argc;
    const char *program_name;
    const char *message;
    const char *ip_address;
    const char *port_str;
    char      **argv;
};

struct settings
{
    const char             *ip_address;
    int                     sockfd;
    in_port_t               port;
    struct sockaddr_storage addr;
};

struct context
{
    struct arguments *arguments;
    struct settings   settings;
    int               exit_code;
    char             *exit_message;
};

struct coordinates
{
    uint32_t x;
    uint32_t y;
};
#endif    // UDP_GAME_STRUCTS_H

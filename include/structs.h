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
    const char *src_ip_address;
    const char *src_port_str;
    const char *dest_ip_address;
    const char *dest_port_str;
    char      **argv;
};

struct settings
{
    const char             *src_ip_address;
    in_port_t               src_port;
    const char             *dest_ip_address;
    in_port_t               dest_port;
    int                     sockfd;
    struct sockaddr_storage src_addr;
    struct sockaddr_storage dest_addr;
    socklen_t               src_addr_len;
    socklen_t               dest_addr_len;
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

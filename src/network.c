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

void socket_close(const struct p101_env *env, struct p101_error *err, const struct context *context)
{
    P101_TRACE(env);

    if(close(context->settings.sockfd) == -1)
    {
        P101_ERROR_RAISE_USER(err, "socket close failed", EXIT_FAILURE);
    }
}

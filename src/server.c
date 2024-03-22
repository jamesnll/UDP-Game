#include "../include/arguments.h"
#include "../include/signal_handler.h"
#include <p101_c/p101_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int                ret_val;
    struct p101_error *error;
    struct p101_env   *env;
    struct arguments   arguments;
    struct context     context;

    error = p101_error_create(false);

    if(error == NULL)
    {
        ret_val = EXIT_FAILURE;
        goto done;
    }

    env = p101_env_create(error, true, NULL);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_error;
    }

    p101_memset(env, &arguments, 0, sizeof(arguments));    // Set memory of arguments to 0
    p101_memset(env, &context, 0, sizeof(context));        // Set memory of context to 0
    context.arguments       = &arguments;
    context.arguments->argc = argc;
    context.arguments->argv = argv;

    parse_arguments(env, error, &context);
    check_arguments(env, error, &context);
    parse_in_port_t(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }
    convert_address(env, error, &context);
    if(p101_error_has_error(error))
    {
        ret_val = EXIT_FAILURE;
        goto free_env;
    }

    setup_signal_handler();
    while(!exit_flag)
    {
    }

    ret_val = EXIT_SUCCESS;

free_env:
    free(context.exit_message);
    free(env);

free_error:
    if(p101_error_has_error(error))
    {
        fprintf(stderr, "Error: %s\n", p101_error_get_message(error));
    }
    p101_error_reset(error);
    free(error);

done:
    printf("Exit code: %d\n", ret_val);
    return ret_val;
}

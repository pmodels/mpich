/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_Append_env(HYD_Env_t * env_list, char **client_arg, int id)
{
    int i, j;
    HYD_Env_t *env;
    char *envstr, *tmp[HYDU_NUM_JOIN_STR];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; client_arg[i]; i++);
    env = env_list;
    while (env) {
        j = 0;

        tmp[j++] = MPIU_Strdup(env->env_name);
        tmp[j++] = MPIU_Strdup("=");
        tmp[j++] = env->env_value ? MPIU_Strdup(env->env_value) : MPIU_Strdup("");
        tmp[j++] = NULL;

        status = HYDU_String_alloc_and_join(tmp, &envstr);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("String utils returned error while joining strings\n");
            goto fn_fail;
        }

        client_arg[i++] = MPIU_Strdup(envstr);
        HYDU_FREE(envstr);
        for (j = 0; tmp[j]; j++)
            HYDU_FREE(tmp[j]);

        client_arg[i++] = MPIU_Strdup(";");

        client_arg[i++] = MPIU_Strdup("export");
        client_arg[i++] = MPIU_Strdup(env->env_name);
        client_arg[i++] = MPIU_Strdup(";");

        env = env->next;
    }
    client_arg[i++] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Append_exec(char **exec, char **client_arg)
{
    int i, j;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; client_arg[i]; i++);
    for (j = 0; exec[j]; j++)
        client_arg[i++] = MPIU_Strdup(exec[j]);
    client_arg[i++] = NULL;

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_Append_wdir(char **client_arg, char *wdir)
{
    int arg;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (arg = 0; client_arg[arg]; arg++);
    client_arg[arg++] = MPIU_Strdup("cd");
    client_arg[arg++] = MPIU_Strdup(wdir);
    client_arg[arg++] = MPIU_Strdup(";");
    client_arg[arg++] = NULL;

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_Dump_args(char **args)
{
    int arg;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (arg = 0; args[arg]; arg++)
        fprintf(stderr, "%s ", args[arg]);
    fprintf(stderr, "\n");

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_Get_base_path(char *execname, char **path)
{
    char *str[HYDU_NUM_JOIN_STR];
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    str[i++] = strtok(execname, "/");
    do {
        str[i++] = "/";
        str[i++] = strtok(NULL, "/");
    } while (str[i - 1]);
    str[i - 3] = NULL;

    status = HYDU_String_alloc_and_join(str, path);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("String utils returned error while joining strings\n");
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

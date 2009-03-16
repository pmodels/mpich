/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_Append_env(HYD_Env_t * env_list, char **client_arg)
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

        tmp[j++] = MPIU_Strdup("'");
        tmp[j++] = MPIU_Strdup(env->env_name);
        tmp[j++] = MPIU_Strdup("=");
        tmp[j++] = env->env_value ? MPIU_Strdup(env->env_value) : MPIU_Strdup("");
        tmp[j++] = MPIU_Strdup("'");
        tmp[j++] = NULL;

        status = HYDU_String_alloc_and_join(tmp, &envstr);
        HYDU_ERR_POP(status, "unable to join strings\n");

        client_arg[i++] = MPIU_Strdup(envstr);
        HYDU_FREE(envstr);
        for (j = 0; tmp[j]; j++)
            HYDU_FREE(tmp[j]);

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
        printf("%s ", args[arg]);
    printf("\n");

    HYDU_FUNC_EXIT();
    return status;
}


void HYDU_Free_args(char **args)
{
    int arg;

    HYDU_FUNC_ENTER();

    for (arg = 0; args[arg]; arg++)
        HYDU_FREE(args[arg]);

    HYDU_FUNC_EXIT();
}


HYD_Status HYDU_Get_base_path(char *execname, char **path)
{
    char *loc;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *path = MPIU_Strdup(execname);
    loc = strrchr(*path, '/');
    if (loc) {
        loc++;
        *loc = 0;
    }
    else {
        HYDU_FREE(*path);
        *path = MPIU_Strdup("");
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_Chdir(const char *dir)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (chdir(dir) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "chdir failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

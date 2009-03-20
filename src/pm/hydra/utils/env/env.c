/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static HYD_Status env_to_str(HYD_Env_t * env, char **str)
{
    int i;
    char *tmp[HYDU_NUM_JOIN_STR];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = MPIU_Strdup("'");
    tmp[i++] = MPIU_Strdup(env->env_name);
    tmp[i++] = MPIU_Strdup("=");
    tmp[i++] = env->env_value ? MPIU_Strdup(env->env_value) : MPIU_Strdup("");
    tmp[i++] = MPIU_Strdup("'");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, str);
    HYDU_ERR_POP(status, "unable to join strings\n");

    for (i = 0; tmp[i]; i++)
        HYDU_FREE(tmp[i]);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


static HYD_Env_t *env_dup(HYD_Env_t env)
{
    HYD_Env_t *tenv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(tenv, HYD_Env_t *, sizeof(HYD_Env_t), status);
    memcpy(tenv, &env, sizeof(HYD_Env_t));
    tenv->next = NULL;
    tenv->env_name = MPIU_Strdup(env.env_name);
    tenv->env_value = env.env_value ? MPIU_Strdup(env.env_value) : NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return tenv;

  fn_fail:
    if (tenv)
        HYDU_FREE(tenv);
    tenv = NULL;
    goto fn_exit;
}


HYD_Status HYDU_list_append_env_to_str(HYD_Env_t * env_list, char **str_list)
{
    int i;
    HYD_Env_t *env;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; str_list[i]; i++);
    env = env_list;
    while (env) {
        status = env_to_str(env, &str_list[i++]);
        HYDU_ERR_POP(status, "env_to_str returned error\n");

        env = env->next;
    }
    str_list[i++] = NULL;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_list_global_env(HYD_Env_t ** env_list)
{
    HYD_Env_t *env;
    char *env_name, *env_value, *env_str;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    i = 0;
    while (environ[i]) {
        HYDU_MALLOC(env, HYD_Env_t *, sizeof(HYD_Env_t), status);

        env_str = MPIU_Strdup(environ[i]);
        env_name = strtok(env_str, "=");
        env_value = strtok(NULL, "=");
        env->env_name = MPIU_Strdup(env_name);
        env->env_value = env_value ? MPIU_Strdup(env_value) : NULL;
        HYDU_FREE(env_str);

        status = HYDU_append_env_to_list(*env, env_list);
        HYDU_ERR_POP(status, "unable to add env to list\n");

        HYDU_env_free(env);

        i++;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Env_t *HYDU_env_list_dup(HYD_Env_t * env)
{
    HYD_Env_t *tenv, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run = env;
    tenv = NULL;
    while (run) {
        status = HYDU_append_env_to_list(*run, &tenv);
        HYDU_ERR_POP(status, "unable to add env to list\n");
        run = run->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return tenv;

  fn_fail:
    tenv = NULL;
    goto fn_exit;
}


HYD_Status HYDU_env_create(HYD_Env_t ** env, char *env_name, char *env_value)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*env, HYD_Env_t *, sizeof(HYD_Env_t), status);
    (*env)->env_name = MPIU_Strdup(env_name);
    (*env)->env_value = env_value ? MPIU_Strdup(env_value) : NULL;
    (*env)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_env_free(HYD_Env_t * env)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (env->env_name)
        HYDU_FREE(env->env_name);
    if (env->env_value)
        HYDU_FREE(env->env_value);
    HYDU_FREE(env);

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_env_free_list(HYD_Env_t * env)
{
    HYD_Env_t *run, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run = env;
    while (run) {
        tmp = run->next;
        HYDU_env_free(run);
        run = tmp;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Env_t *HYDU_env_lookup(HYD_Env_t env, HYD_Env_t * env_list)
{
    HYD_Env_t *run;

    HYDU_FUNC_ENTER();

    run = env_list;
    while (run->next) {
        if (!strcmp(run->env_name, env.env_name))
            goto fn_exit;
        run = run->next;
    }
    run = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return run;
}


HYD_Status HYDU_append_env_to_list(HYD_Env_t env, HYD_Env_t ** env_list)
{
    HYD_Env_t *run, *tenv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    tenv = env_dup(env);
    if (tenv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unable to dup env\n");

    tenv->next = NULL;

    /* Add the structure to the end of the list */
    if (*env_list == NULL) {
        *env_list = tenv;
    }
    else {
        run = *env_list;

        while (1) {
            if (!strcmp(run->env_name, env.env_name)) {
                /* If we found an entry for this environment variable, just update it */
                if (run->env_value != NULL && tenv->env_value != NULL) {
                    HYDU_FREE(run->env_value);
                    run->env_value = MPIU_Strdup(tenv->env_value);
                }
                else if (run->env_value != NULL) {
                    HYDU_FREE(run->env_value);
                    run->env_value = NULL;
                }
                else if (env.env_value != NULL) {
                    run->env_value = MPIU_Strdup(tenv->env_value);
                }

                HYDU_FREE(tenv->env_name);
                if (tenv->env_value)
                    HYDU_FREE(tenv->env_value);
                HYDU_FREE(tenv);

                break;
            }
            else if (run->next == NULL) {
                run->next = tenv;
                break;
            }
            run = run->next;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYDU_putenv(char *env_str)
{
    HYDU_FUNC_ENTER();

    MPIU_PutEnv(env_str);

    HYDU_FUNC_EXIT();
    return;
}

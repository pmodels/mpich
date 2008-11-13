/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_env.h"
#include "hydra_mem.h"

HYD_Status HYDU_Env_global_list(HYDU_Env_t ** env_list)
{
    HYDU_Env_t *env;
    char *env_name, *env_value, *env_str;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    i = 0;
    while (environ[i]) {
        HYDU_MALLOC(env, HYDU_Env_t *, sizeof(HYDU_Env_t), status);

        env_str = MPIU_Strdup(environ[i]);
        env_name = strtok(env_str, "=");
        env_value = strtok(NULL, "=");
        env->env_name = MPIU_Strdup(env_name);
        env->env_value = env_value ? MPIU_Strdup(env_value) : NULL;
        env->env_type = HYDU_ENV_STATIC;
        HYDU_FREE(env_str);

        status = HYDU_Env_add_to_list(env_list, *env);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("launcher returned error adding env to list\n");
            goto fn_fail;
        }
        HYDU_Env_free(env);

        i++;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYDU_Env_type_str(HYDU_Env_type_t type)
{
    char *str;

    if (type == HYDU_ENV_STATIC)
        str = MPIU_Strdup("STATIC");
    else if (type == HYDU_ENV_AUTOINC)
        str = MPIU_Strdup("AUTOINC");
    else
        str = NULL;

    return str;
}


HYDU_Env_t *HYDU_Env_dup(HYDU_Env_t env)
{
    HYDU_Env_t *tenv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(tenv, HYDU_Env_t *, sizeof(HYDU_Env_t), status);
    memcpy(tenv, &env, sizeof(HYDU_Env_t));
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


HYDU_Env_t *HYDU_Env_found_in_list(HYDU_Env_t * env_list, HYDU_Env_t * env)
{
    HYDU_Env_t *run;

    HYDU_FUNC_ENTER();

    run = env_list;
    while (run->next) {
        if (!strcmp(run->env_name, env->env_name))
            goto fn_exit;
        run = run->next;
    }
    run = NULL;

    goto fn_exit;

  fn_exit:
    HYDU_FUNC_EXIT();
    return run;
}


HYD_Status HYDU_Env_add_to_list(HYDU_Env_t ** env_list, HYDU_Env_t env)
{
    HYDU_Env_t *run, *tenv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    tenv = HYDU_Env_dup(env);
    if (tenv == NULL) {
        HYDU_Error_printf("unable to dup environment\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

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
                run->env_type = tenv->env_type;

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


HYDU_Env_t *HYDU_Env_listdup(HYDU_Env_t * env)
{
    HYDU_Env_t *tenv, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run = env;
    tenv = NULL;
    while (run) {
        status = HYDU_Env_add_to_list(&tenv, *run);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("unable to add env to list\n");
            tenv = NULL;
            goto fn_fail;
        }
        run = run->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return tenv;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Env_create(HYDU_Env_t ** env, char *env_name, char *env_value,
                           HYDU_Env_type_t env_type, int start)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*env, HYDU_Env_t *, sizeof(HYDU_Env_t), status);
    (*env)->env_name = MPIU_Strdup(env_name);
    (*env)->env_value = env_value ? MPIU_Strdup(env_value) : NULL;
    (*env)->env_type = env_type;
    (*env)->start_val = start;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Env_free(HYDU_Env_t * env)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (env->env_name)
        HYDU_FREE(env->env_name);
    if (env->env_value)
        HYDU_FREE(env->env_value);
    HYDU_FREE(env);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Env_free_list(HYDU_Env_t * env)
{
    HYDU_Env_t *run, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run = env;
    while (run) {
        tmp = run->next;
        HYDU_Env_free(run);
        run = tmp;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_status HYDU_env_to_str(struct HYD_env *env, char **str)
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("'");
    tmp[i++] = HYDU_strdup(env->env_name);
    tmp[i++] = HYDU_strdup("=");
    tmp[i++] = env->env_value ? HYDU_strdup(env->env_value) : HYDU_strdup("");
    tmp[i++] = HYDU_strdup("'");
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


HYD_status HYDU_str_to_env(char *str, struct HYD_env **env)
{
    char *env_name, *env_value;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC((*env), struct HYD_env *, sizeof(struct HYD_env), status);
    env_name = strtok(str, "=");
    env_value = strtok(NULL, "=");
    (*env)->env_name = HYDU_strdup(env_name);
    (*env)->env_value = env_value ? HYDU_strdup(env_value) : NULL;
    (*env)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    if (*env)
        HYDU_FREE(*env);
    *env = NULL;
    goto fn_exit;
}


static struct HYD_env *env_dup(struct HYD_env env)
{
    struct HYD_env *tenv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(tenv, struct HYD_env *, sizeof(struct HYD_env), status);
    memcpy(tenv, &env, sizeof(struct HYD_env));
    tenv->next = NULL;
    tenv->env_name = HYDU_strdup(env.env_name);
    tenv->env_value = env.env_value ? HYDU_strdup(env.env_value) : NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return tenv;

  fn_fail:
    if (tenv)
        HYDU_FREE(tenv);
    tenv = NULL;
    goto fn_exit;
}


HYD_status HYDU_list_inherited_env(struct HYD_env **env_list)
{
    struct HYD_env *env;
    char *env_str;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    i = 0;
    while (environ[i]) {
        env_str = HYDU_strdup(environ[i]);

        status = HYDU_str_to_env(env_str, &env);
        HYDU_ERR_POP(status, "error converting string to env\n");

        status = HYDU_append_env_to_list(*env, env_list);
        HYDU_ERR_POP(status, "unable to add env to list\n");

        HYDU_env_free(env);
        HYDU_FREE(env_str);

        i++;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


struct HYD_env *HYDU_env_list_dup(struct HYD_env *env)
{
    struct HYD_env *tenv, *run;
    HYD_status status = HYD_SUCCESS;

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


HYD_status HYDU_env_create(struct HYD_env **env, const char *env_name, char *env_value)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*env, struct HYD_env *, sizeof(struct HYD_env), status);
    (*env)->env_name = HYDU_strdup(env_name);
    (*env)->env_value = env_value ? HYDU_strdup(env_value) : NULL;
    (*env)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_env_free(struct HYD_env *env)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (env->env_name)
        HYDU_FREE(env->env_name);
    if (env->env_value)
        HYDU_FREE(env->env_value);
    HYDU_FREE(env);

    HYDU_FUNC_EXIT();
    return status;
}


HYD_status HYDU_env_free_list(struct HYD_env * env)
{
    struct HYD_env *run, *tmp;
    HYD_status status = HYD_SUCCESS;

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


struct HYD_env *HYDU_env_lookup(char *env_name, struct HYD_env *env_list)
{
    struct HYD_env *run;

    HYDU_FUNC_ENTER();

    run = env_list;
    while (run->next) {
        if (!strcmp(run->env_name, env_name))
            goto fn_exit;
        run = run->next;
    }
    run = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return run;
}


HYD_status HYDU_append_env_to_list(struct HYD_env env, struct HYD_env ** env_list)
{
    struct HYD_env *run, *tenv;
    HYD_status status = HYD_SUCCESS;

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
                    run->env_value = HYDU_strdup(tenv->env_value);
                }
                else if (run->env_value != NULL) {
                    HYDU_FREE(run->env_value);
                    run->env_value = NULL;
                }
                else if (env.env_value != NULL) {
                    run->env_value = HYDU_strdup(tenv->env_value);
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


HYD_status HYDU_putenv(struct HYD_env *env, HYD_env_overwrite_t overwrite)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *str;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If the overwrite flag is false, just exit */
    if (getenv(env->env_name) && (overwrite == HYD_ENV_OVERWRITE_FALSE))
        goto fn_exit;

    i = 0;
    tmp[i++] = HYDU_strdup(env->env_name);
    tmp[i++] = HYDU_strdup("=");
    tmp[i++] = env->env_value ? HYDU_strdup(env->env_value) : HYDU_strdup("");
    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, &str);
    HYDU_ERR_POP(status, "unable to join strings\n");

    putenv(str);

    for (i = 0; tmp[i]; i++)
        HYDU_FREE(tmp[i]);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_putenv_list(struct HYD_env *env_list, HYD_env_overwrite_t overwrite)
{
    struct HYD_env *env;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (env = env_list; env; env = env->next) {
        status = HYDU_putenv(env, overwrite);
        HYDU_ERR_POP(status, "putenv failed\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_comma_list_to_env_list(char *str, struct HYD_env **env_list)
{
    char *env_name;
    struct HYD_env *env;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    env_name = strtok(str, ",");
    do {
        status = HYDU_env_create(&env, env_name, NULL);
        HYDU_ERR_POP(status, "unable to create env struct\n");

        status = HYDU_append_env_to_list(*env, env_list);
        HYDU_ERR_POP(status, "unable to add env to list\n");
    } while ((env_name = strtok(NULL, ",")));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

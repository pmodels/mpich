/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"

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


HYD_status HYDU_list_inherited_env(struct HYD_env **env_list)
{
    char *env_str = NULL, *env_name;
    int i, ret;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    i = 0;
    while (environ[i]) {
        env_str = HYDU_strdup(environ[i]);
        env_name = strtok(env_str, "=");

        status = HYDT_bsci_query_env_inherit(env_name, &ret);
        HYDU_ERR_POP(status, "error querying environment propagation\n");

        HYDU_FREE(env_str);
        env_str = NULL;

        if (!ret) {
            i++;
            continue;
        }

        status = HYDU_append_env_str_to_list(environ[i], env_list);
        HYDU_ERR_POP(status, "unable to add env to list\n");

        i++;
    }

  fn_exit:
    if (env_str)
        HYDU_FREE(env_str);
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
        status = HYDU_append_env_to_list(run->env_name, run->env_value, &tenv);
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


HYD_status HYDU_env_create(struct HYD_env **env, const char *env_name, const char *env_value)
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

HYD_status HYDU_append_env_to_list(const char *env_name, const char *env_value,
                                   struct HYD_env ** env_list)
{
    struct HYD_env *run, *tenv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_env_create(&tenv, env_name, env_value);
    HYDU_ERR_POP(status, "unable to create env structure\n");

    tenv->next = NULL;

    /* Add the structure to the end of the list */
    if (*env_list == NULL) {
        *env_list = tenv;
    }
    else {
        run = *env_list;

        while (1) {
            if (!strcmp(run->env_name, env_name)) {
                /* If we found an entry for this environment variable, just update it */
                if (run->env_value != NULL && tenv->env_value != NULL) {
                    HYDU_FREE(run->env_value);
                    run->env_value = HYDU_strdup(tenv->env_value);
                }
                else if (run->env_value != NULL) {
                    HYDU_FREE(run->env_value);
                    run->env_value = NULL;
                }
                else if (env_value != NULL) {
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

HYD_status HYDU_append_env_str_to_list(const char *str, struct HYD_env **env_list)
{
    char *my_str = NULL;
    char *env_name, *env_value;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    my_str = env_value = HYDU_strdup(str);
    /* don't use strtok, it will mangle env values that contain '=' */
    env_name = MPL_strsep(&env_value, "=");
    HYDU_ASSERT(env_name != NULL, status);

    status = HYDU_append_env_to_list(env_name, env_value, env_list);
    HYDU_ERR_POP(status, "unable to append env to list\n");

  fn_exit:
    if (my_str)
        HYDU_FREE(my_str);
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
    if (MPL_env2str(env->env_name, (const char **) &str) &&
        overwrite == HYD_ENV_OVERWRITE_FALSE)
        goto fn_exit;

    i = 0;
    tmp[i++] = HYDU_strdup(env->env_name);
    tmp[i++] = HYDU_strdup("=");
    tmp[i++] = env->env_value ? HYDU_strdup(env->env_value) : HYDU_strdup("");
    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, &str);
    HYDU_ERR_POP(status, "unable to join strings\n");

    MPL_putenv(str);

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
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    env_name = strtok(str, ",");
    do {
        status = HYDU_append_env_to_list(env_name, NULL, env_list);
        HYDU_ERR_POP(status, "unable to add env to list\n");
    } while ((env_name = strtok(NULL, ",")));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

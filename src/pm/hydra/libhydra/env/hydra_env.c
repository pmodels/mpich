/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_env.h"
#include "hydra_err.h"
#include "hydra_mem.h"
#include "hydra_str.h"

HYD_status HYD_env_to_str(struct HYD_env *env, char **str)
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    i = 0;
    tmp[i++] = MPL_strdup(env->env_name);
    tmp[i++] = MPL_strdup("=");
    tmp[i++] = env->env_value ? MPL_strdup(env->env_value) : MPL_strdup("");
    tmp[i++] = NULL;

    status = HYD_str_alloc_and_join(tmp, str);
    HYD_ERR_POP(status, "unable to join strings\n");

    for (i = 0; tmp[i]; i++)
        MPL_free(tmp[i]);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_env_list_inherited(struct HYD_env **env_list)
{
    int i;
    char *env_name, *env_value;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    *env_list = NULL;
    for (i = 0; environ[i]; i++) {
        env_name = strtok(environ[i], "=");
        env_value = strtok(NULL, "=");

        status = HYD_env_append_to_list(env_name, env_value, env_list);
        HYD_ERR_POP(status, "unable to append env to list\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_env_create(struct HYD_env **env, const char *env_name, const char *env_value)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(*env, struct HYD_env *, sizeof(struct HYD_env), status);
    (*env)->env_name = MPL_strdup(env_name);
    (*env)->env_value = env_value ? MPL_strdup(env_value) : NULL;
    (*env)->next = NULL;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_env_free(struct HYD_env *env)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (env->env_name)
        MPL_free(env->env_name);
    if (env->env_value)
        MPL_free(env->env_value);
    MPL_free(env);

    HYD_FUNC_EXIT();
    return status;
}


HYD_status HYD_env_append_to_list(const char *env_name, const char *env_value,
                                  struct HYD_env ** env_list)
{
    struct HYD_env *run, *tenv;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_env_create(&tenv, env_name, env_value);
    HYD_ERR_POP(status, "unable to create env structure\n");

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
                    MPL_free(run->env_value);
                    run->env_value = MPL_strdup(tenv->env_value);
                }
                else if (run->env_value != NULL) {
                    MPL_free(run->env_value);
                    run->env_value = NULL;
                }
                else if (env_value != NULL) {
                    run->env_value = MPL_strdup(tenv->env_value);
                }

                MPL_free(tenv->env_name);
                if (tenv->env_value)
                    MPL_free(tenv->env_value);
                MPL_free(tenv);

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
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

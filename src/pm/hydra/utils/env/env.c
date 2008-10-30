/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_env.h"
#include "hydra_mem.h"

/*
 * HYD_Set_env: add an environment variable into the list of
 * environment variables that (might) need to be propagated.
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYDU_Set_env"
HYD_Status HYDU_Set_env(HYD_Env_info_t * env_info, char * env_name, char * env_value)
{
    HYD_Env_element_t * p;
    HYD_Status status;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(p, HYD_Env_element_t *, sizeof(HYD_Env_element_t), status);
    HYDU_STRDUP(env_name, p->env_name, char *, status);
    HYDU_STRDUP(env_value, p->env_value, char *, status);

    p->next = env_info->env_list;
    env_info->env_list = p;
    env_info->env_list_length++;

fn_exit:
    HYDU_FUNC_EXIT();
    status = HYD_NO_MEM;
    return status;

fn_fail:
    goto fn_exit;
}


/*
 * HYD_Unset_all_env: unset all environment variables.
 *
 * Note that not all systems support unsetenv (e.g., System V derived
 * systems such as Solaris), so we also need to provide our own
 * implementation, though it's not as efficient.
 *
 * In addition, there are various ways to determine the "current"
 * environment variables. One is to pass a third arg to main; the
 * other is a global variable.
 *
 * Also, we prefer the environ variable over envp, because exec often
 * prefers what is in environ rather than the envp (envp appears to be
 * a copy, and unsetting the env doesn't always change the environment
 * that exec creates.  This seems wrong, but it was what was observed
 * on Linux).
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYDU_Unset_all_env"
#if defined HAVE_EXTERN_ENVIRON
HYD_Status HYDU_Unset_all_env(char ** envp)
{
    /* Ignore envp because environ is the real array that controls the
     * environment used by getenv/putenv/etc */
    char **ep = environ;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while (*ep)
	*ep++ = NULL;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
#elif defined HAVE_UNSETENV
HYD_Status HYDU_Unset_all_env(char ** envp)
{
    HYD_Status status = HYD_SUCCESS;
    int i;

    HYDU_FUNC_ENTER();

    for (i = 0; envp[i]; i++)
	unsetenv(envp[i]);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
#else
#error "No way to unset environment variables"
#endif /* HAVE_EXTERN_ENVIRON */

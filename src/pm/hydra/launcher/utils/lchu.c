/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "csi.h"

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHU_Add_env_to_list"
HYD_Status HYD_LCHU_Add_env_to_list(HYD_CSI_Env_t ** env_list, HYD_CSI_Env_t * env)
{
    HYD_CSI_Env_t * run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    env->next = NULL;

    /* Add the structure to the end of the list */
    if (*env_list == NULL) {
	*env_list = env;
    }
    else {
	run = *env_list;

	while (1) {
	    if (!strcmp(run->env_name, env->env_name)) {
		/* If we found an entry for this environment variable, just update it */
		if (run->env_value != NULL && env->env_value != NULL) {
		    strcpy(run->env_value, env->env_value);
		}
		else if (run->env_value != NULL) {
		    run->env_value = NULL;
		}
		else if (env->env_value != NULL) {
		    HYDU_MALLOC(run->env_value, char *, strlen(env->env_value), status);
		    strcpy(run->env_value, env->env_value);
		}
		run->env_type = env->env_type;
		break;
	    }
	    else if (run->next == NULL) {
		run->next = env;
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

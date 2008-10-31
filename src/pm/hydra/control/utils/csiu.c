/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "csi.h"

HYD_CSI_Handle * csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_CSU_Time_left"
int HYD_CSU_Time_left(void)
{
    struct timeval now;
    int time_left;

    HYDU_FUNC_ENTER();

    if (csi_handle->timeout.tv_sec < 0) {
	time_left = -1;
	goto fn_exit;
    }
    else {
	gettimeofday(&now, NULL);
	time_left = (1000 * (csi_handle->timeout.tv_sec - now.tv_sec + csi_handle->start.tv_sec));
	if (time_left < 0)
	    time_left = 0;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return time_left;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_CSU_Free_env_list"
void HYD_CSU_Free_env_list(HYDU_Env_t * env_list)
{
    HYDU_Env_t * env1, * env2;

    env1 = env_list;
    while (env1) {
	env2 = env1->next;
	if (env1->env_name)
	    HYDU_FREE(env1->env_name);
	if (env1->env_value)
	    HYDU_FREE(env1->env_value);
	HYDU_FREE(env1);
	env1 = env2;
    }
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_CSU_Free_exec_list"
void HYD_CSU_Free_exec_list(HYD_CSI_Exec_t * exec_list)
{
    HYD_CSI_Exec_t * exec1, * exec2;

    exec1 = exec_list;
    while (exec1) {
	exec2 = exec1->next;
	HYDU_FREE(exec1->arg);
	HYDU_FREE(exec1);
	exec1 = exec2;
    }
}

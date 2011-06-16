/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "debugger.h"

struct MPIR_PROCDESC *MPIR_proctable = NULL;
int MPIR_proctable_size = 0;
int MPIR_i_am_starter = 0;
int MPIR_partial_attach_ok = 0;

volatile int MPIR_debug_state = 0;
char *MPIR_debug_abort_string = 0;

volatile int MPIR_being_debugged = 0;

int (*MPIR_breakpointFn) (void) = MPIR_Breakpoint;

int MPIR_Breakpoint(void)
{
    return 0;
}

HYD_status HYDT_dbg_setup_procdesc(struct HYD_pg * pg)
{
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    int i, j, np;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(MPIR_proctable, struct MPIR_PROCDESC *,
                pg->pg_process_count * sizeof(struct MPIR_PROCDESC), status);

    for (proxy = pg->proxy_list, i = 0; proxy; proxy = proxy->next) {
        j = 0;
        for (exec = proxy->exec_list; exec; exec = exec->next) {
            for (np = 0; np < exec->proc_count; np++) {
                MPIR_proctable[i].host_name = HYDU_strdup(proxy->node->hostname);
                MPIR_proctable[i].pid = proxy->pid[j++];
                if (exec->exec[0])
                    MPIR_proctable[i].executable_name = HYDU_strdup(exec->exec[0]);
                else
                    MPIR_proctable[i].executable_name = NULL;
                i++;
            }
        }
    }

    MPIR_proctable_size = pg->pg_process_count;
    MPIR_debug_state = MPIR_DEBUG_SPAWNED;
    (*MPIR_breakpointFn) ();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDT_dbg_free_procdesc(void)
{
    int i;

    for (i = 0; i < MPIR_proctable_size; i++) {
        if (MPIR_proctable[i].host_name)
            HYDU_FREE(MPIR_proctable[i].host_name);
        if (MPIR_proctable[i].executable_name)
            HYDU_FREE(MPIR_proctable[i].executable_name);
    }
    HYDU_FREE(MPIR_proctable);
}

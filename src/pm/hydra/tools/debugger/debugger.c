/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
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
    int i, j, k, np, round;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(MPIR_proctable, struct MPIR_PROCDESC *,
                pg->pg_process_count * sizeof(struct MPIR_PROCDESC), status);

    round = 0;
    /* We need to allocate the MPIR_proctable in COMM_WORLD rank
     * order.  We do this in multiple rounds.  In each round, we
     * allocate the proctable entries for the executables on the proxy
     * that form a contiguous rank list.  Then we move to the next
     * proxy.  When we run out of proxies, we go back to the first
     * proxy and find the next set of contiguous ranks on that
     * proxy. */
    for (proxy = pg->proxy_list, i = 0;; proxy = proxy->next) {
        j = 0;
        k = 0;
        if (!proxy) {
            proxy = pg->proxy_list;
            round++;
        }
        for (exec = proxy->exec_list; exec; exec = exec->next) {
            for (np = 0; np < exec->proc_count; np++) {
                if (k + np >= ((round + 1) * proxy->node->core_count))
                    break;
                if (k + np < round * proxy->node->core_count)
                    continue;
                /* avoid storing multiple copies of the host name */
                if (i > 0 && strcmp(MPIR_proctable[i - 1].host_name, proxy->node->hostname) == 0)
                    MPIR_proctable[i].host_name = MPIR_proctable[i - 1].host_name;
                else
                    MPIR_proctable[i].host_name = HYDU_strdup(proxy->node->hostname);
                MPIR_proctable[i].pid = proxy->pid[(proxy->node->core_count * round) + j];
                j++;
                if (exec->exec[0]) {
                    /* avoid storing multiple copies of the executable name */
                    if (i > 0 && strcmp(exec->exec[0], MPIR_proctable[i - 1].executable_name) == 0)
                        MPIR_proctable[i].executable_name = MPIR_proctable[i - 1].executable_name;
                    else
                        MPIR_proctable[i].executable_name = HYDU_strdup(exec->exec[0]);
                }
                else {
                    MPIR_proctable[i].executable_name = NULL;
                }
                i++;
            }
            k += exec->proc_count;
        }

        if (i >= pg->pg_process_count)
            break;
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
        /* skip over duplicate pointers when freeing */
        if (MPIR_proctable[i].host_name) {
            if (i == 0 || MPIR_proctable[i].host_name != MPIR_proctable[i - 1].host_name)
                HYDU_FREE(MPIR_proctable[i].host_name);
        }
        if (MPIR_proctable[i].executable_name) {
            if (i == 0 ||
                MPIR_proctable[i].executable_name != MPIR_proctable[i - 1].executable_name)
                HYDU_FREE(MPIR_proctable[i].executable_name);
        }
    }
    HYDU_FREE(MPIR_proctable);
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_debugger.h"
#include "hydra_mem.h"
#include "hydra_node.h"

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

HYD_status HYD_dbg_setup_procdesc(int process_count, struct HYD_exec * exec_list, int *pid,
                                  int node_count, struct HYD_node * node_list)
{
    struct HYD_exec *exec;
    int i, exec_proc_count, node_id, node_core_count;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(MPIR_proctable, struct MPIR_PROCDESC *, process_count * sizeof(struct MPIR_PROCDESC),
               status);

    exec = exec_list;
    exec_proc_count = 0;

    node_id = 0;
    node_core_count = 0;

    for (i = 0; i < process_count; i++) {
        MPIR_proctable[i].executable_name = exec->exec[0];
        exec_proc_count++;

        if (exec_proc_count == exec->proc_count) {
            exec = exec->next;
            exec_proc_count = 0;
        }

        MPIR_proctable[i].host_name = node_list[i].hostname;
        node_core_count++;

        if (node_core_count == node_list[i].core_count) {
            node_id++;
            node_core_count = 0;

            if (node_id == node_count)
                node_id = 0;
        }

        MPIR_proctable[i].pid = pid[i];
    }

    MPIR_proctable_size = process_count;
    MPIR_debug_state = MPIR_DEBUG_SPAWNED;
    (*MPIR_breakpointFn) ();

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_dbg_free_procdesc(void)
{
    MPL_free(MPIR_proctable);
}

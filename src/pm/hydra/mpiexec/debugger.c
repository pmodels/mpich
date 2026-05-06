/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "debugger.h"
#include "mpl_hash.h"

struct MPIR_PROCDESC *MPIR_proctable = NULL;
int MPIR_proctable_size = 0;
int MPIR_i_am_starter = 0;
int MPIR_partial_attach_ok = 0;

volatile int MPIR_debug_state = 0;
char *MPIR_debug_abort_string = 0;

volatile int MPIR_being_debugged = 0;

static struct MPL_hash *MPIR_debug_str_hash = NULL;

int (*MPIR_breakpointFn) (void) = MPIR_Breakpoint;

int MPIR_Breakpoint(void)
{
    return 0;
}

HYD_status HYDT_dbg_setup_procdesc(struct HYD_pg * pg)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(MPIR_proctable, struct MPIR_PROCDESC *,
                        pg->pg_process_count * sizeof(struct MPIR_PROCDESC), status);

    /* We need to allocate the MPIR_proctable in COMM_WORLD rank order. */
    /* We use MPL_hash for duplicating the strings */
    if (MPIR_debug_str_hash) {
        MPL_hash_free(MPIR_debug_str_hash);
    }
    MPIR_debug_str_hash = MPL_hash_new();

    /* store all name strings in MPIR_debug_str_hash first before exposing the internal string pointers */
#define HASH_NAME_STR(name) MPL_hash_set(MPIR_debug_str_hash, name, MPL_HASH_KEY)

    for (int i_proxy = 0; i_proxy < pg->proxy_count; i_proxy++) {
        struct HYD_proxy *proxy = &pg->proxy_list[i_proxy];
        int j = 0;
        for (struct HYD_exec * exec = proxy->exec_list; exec; exec = exec->next) {
            for (int i = 0; i < exec->proc_count; i++) {
                int rank = exec->start_rank + i;
                MPIR_proctable[rank].host_name = proxy->node->hostname;
                MPIR_proctable[rank].pid = proxy->pid[j];
                HASH_NAME_STR(MPIR_proctable[rank].host_name);
                if (exec->exec[0]) {
                    MPIR_proctable[rank].executable_name = exec->exec[0];
                    HASH_NAME_STR(MPIR_proctable[rank].executable_name);
                } else {
                    MPIR_proctable[rank].executable_name = NULL;
                }
                j++;
            }
        }
    }
    /* replace the name strings with allocated strings in MPIR_debug_str_hash.
     * NOTE: MPIR_debug_str_hash should be frozen to avoid a realloc that invalidates the string pointers.
     */
#define REPLACE_NAME_STR(name) \
    if (name) { \
        name = MPL_hash_get(MPIR_debug_str_hash, name); \
    }

    for (int rank = 0; rank < pg->pg_process_count; rank++) {
        HYDU_ASSERT(MPIR_proctable[rank].host_name, status);
        REPLACE_NAME_STR(MPIR_proctable[rank].host_name);
        REPLACE_NAME_STR(MPIR_proctable[rank].executable_name);
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
    if (MPIR_debug_str_hash) {
        MPL_hash_free(MPIR_debug_str_hash);
        MPIR_debug_str_hash = NULL;
    }
    MPL_free(MPIR_proctable);
}

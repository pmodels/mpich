/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_exec.h"
#include "hydra_mem.h"
#include "hydra_str.h"

HYD_status HYD_exec_alloc(struct HYD_exec **exec)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(*exec, struct HYD_exec *, sizeof(struct HYD_exec), status);
    (*exec)->exec[0] = NULL;
    (*exec)->proc_count = -1;
    (*exec)->next = NULL;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_exec_free_list(struct HYD_exec *exec_list)
{
    struct HYD_exec *exec, *run;

    HYD_FUNC_ENTER();

    exec = exec_list;
    while (exec) {
        run = exec->next;
        HYD_str_free_list(exec->exec);

        MPL_free(exec);
        exec = run;
    }

    HYD_FUNC_EXIT();
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"
#include "bsci.h"

HYD_status HYD_pmiserv_barrier(struct HYD_proxy *proxy, int process_fd, int pgid,
                               struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(proxy->pgid);

    pg->barrier_count++;
    if (pg->barrier_count == pg->proxy_count) {
        pg->barrier_count = 0;

        HYD_pmiserv_bcast_keyvals(proxy, process_fd);

        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, 1, "barrier_out");
        for (int i = 0; i < pg->proxy_count; i++) {
            status = HYD_pmiserv_pmi_reply(&pg->proxy_list[i], process_fd, &pmi_response);
            HYDU_ERR_POP(status, "error writing PMI line\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmiserv_abort(struct HYD_proxy *proxy, int process_fd, int pgid,
                             struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;

    HYDU_FUNC_ENTER();

    int exitcode;
    const char *error_msg;
    pmi_errno = PMIU_msg_get_query_abort(pmi, &exitcode, &error_msg);
    HYDU_ASSERT(!pmi_errno, status);

  fn_exit:
    /* clean everything up and exit */
    status = HYDT_bsci_wait_for_completion(0);
    exit(exitcode);

    /* never get here */
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

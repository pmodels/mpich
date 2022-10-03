/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmiserv_pmi_reply(struct HYD_proxy * proxy, int process_fd, struct PMIU_cmd * pmi)
{
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI_RESPONSE;
    hdr.pgid = proxy->pgid;
    hdr.proxy_id = proxy->proxy_id;
    hdr.u.pmi.process_fd = process_fd;
    return HYD_pmcd_pmi_send(proxy->control_fd, pmi, &hdr, HYD_server_info.user_global.debug);
}

HYD_status HYD_pmiserv_bcast_keyvals(struct HYD_proxy * proxy, int process_fd)
{
    int keyval_count, arg_count, j;
    struct HYD_pmcd_pmi_kvs_pair *run;
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = PMISERV_pg_by_id(proxy->pgid);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* find the number of keyvals */
    keyval_count = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next)
        keyval_count++;

    keyval_count -= pg_scratch->keyval_dist_count;

    if (keyval_count) {
        struct PMIU_cmd pmi;
        PMIU_msg_set_query(&pmi, PMIU_WIRE_V1, PMIU_CMD_KVSCACHE, false /* not static */);
        arg_count = 1;
        for (run = pg_scratch->kvs->key_pair, j = 0; run; run = run->next, j++) {
            if (j < pg_scratch->keyval_dist_count)
                continue;

            PMIU_cmd_add_str(&pmi, run->key, run->val);

            arg_count++;
            if (arg_count >= MAX_PMI_ARGS) {
                pg_scratch->keyval_dist_count += (arg_count - 1);
                for (int i = 0; i < pg->proxy_count; i++) {
                    status = HYD_pmiserv_pmi_reply(&pg->proxy_list[i], process_fd, &pmi);
                    HYDU_ERR_POP(status, "error writing PMI line\n");
                }

                PMIU_msg_set_query(&pmi, PMIU_WIRE_V1, PMIU_CMD_KVSCACHE, false /* not static */);
                arg_count = 1;
            }
        }

        if (arg_count > 1) {
            pg_scratch->keyval_dist_count += (arg_count - 1);
            for (int i = 0; i < pg->proxy_count; i++) {
                status = HYD_pmiserv_pmi_reply(&pg->proxy_list[i], process_fd, &pmi);
                HYDU_ERR_POP(status, "error writing PMI line\n");
            }
        }
        PMIU_cmd_free_buf(&pmi);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

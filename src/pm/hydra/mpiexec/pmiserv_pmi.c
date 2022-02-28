/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle = { 0 };

struct HYD_proxy *HYD_pmcd_pmi_find_proxy(int fd)
{
    struct HYD_proxy *proxy;

    HASH_FIND_INT(HYD_server_info.proxy_hash, &fd, proxy);

    return proxy;
}

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmiserv_pmi_reply(int fd, int pid, struct PMIU_cmd * pmi)
{
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI_RESPONSE;
    hdr.u.pmi.pid = pid;
    return HYD_pmcd_pmi_send(fd, pmi, &hdr, HYD_server_info.user_global.debug);
}

HYD_status HYD_pmiserv_bcast_keyvals(int fd, int pid)
{
    int keyval_count, arg_count, j;
    struct HYD_pmcd_pmi_kvs_pair *run;
    struct HYD_proxy *proxy, *tproxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    /* find the number of keyvals */
    keyval_count = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next)
        keyval_count++;

    keyval_count -= pg_scratch->keyval_dist_count;

    struct PMIU_cmd pmi;
    if (keyval_count) {
        PMIU_cmd_init_static(&pmi, 1, "keyval_cache");
        arg_count = 1;
        for (run = pg_scratch->kvs->key_pair, j = 0; run; run = run->next, j++) {
            if (j < pg_scratch->keyval_dist_count)
                continue;

            PMIU_cmd_add_str(&pmi, run->key, run->val);

            arg_count++;
            if (arg_count >= MAX_PMI_ARGS) {
                pg_scratch->keyval_dist_count += (arg_count - 1);
                for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
                    status = HYD_pmiserv_pmi_reply(tproxy->control_fd, pid, &pmi);
                    HYDU_ERR_POP(status, "error writing PMI line\n");
                }

                PMIU_cmd_init_static(&pmi, 1, "keyval_cache");
                arg_count = 1;
            }
        }

        if (arg_count > 1) {
            pg_scratch->keyval_dist_count += (arg_count - 1);
            for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
                status = HYD_pmiserv_pmi_reply(tproxy->control_fd, pid, &pmi);
                HYDU_ERR_POP(status, "error writing PMI line\n");
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

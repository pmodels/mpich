/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

static HYD_status fn_barrier_in(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy, *tproxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    proxy->pg->barrier_count++;
    if (proxy->pg->barrier_count == proxy->pg->proxy_count) {
        proxy->pg->barrier_count = 0;

        HYD_pmiserv_bcast_keyvals(fd, pid);

        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, 1, "barrier_out");
        for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
            status = HYD_pmiserv_pmi_reply(tproxy->control_fd, pid, &pmi_response);
            HYDU_ERR_POP(status, "error writing PMI line\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* NOTE: this is an aggregate put from proxy with multiple key=val pairs */
static HYD_status fn_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    /* FIXME: leak of pmi's abstraction */
    for (int i = 0; i < pmi->num_tokens; i++) {
        int ret;
        status = HYD_pmcd_pmi_add_kvs(pmi->tokens[i].key, pmi->tokens[i].val,
                                      pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add key pair to kvs\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *kvsname, *key, *val;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    kvsname = PMIU_cmd_find_keyval(pmi, "kvsname");
    HYDU_ERR_CHKANDJUMP(status, kvsname == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: kvsname\n");

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    val = NULL;
    if (!strcmp(key, "PMI_dead_processes")) {
        val = pg_scratch->dead_processes;
        goto found_val;
    }

    if (strcmp(pg_scratch->kvs->kvsname, kvsname))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "kvsname (%s) does not match this group's kvs space (%s)\n",
                            kvsname, pg_scratch->kvs->kvsname);

    /* Try to find the key */
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            val = run->val;
            break;
        }
    }

    struct PMIU_cmd pmi_response;
  found_val:
    PMIU_cmd_init_static(&pmi_response, 1, "get_result");
    if (val) {
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
        PMIU_cmd_add_str(&pmi_response, "value", val);
    } else {
        static char tmp[100];
        MPL_snprintf(tmp, 100, "key_%s_not_found", key);

        PMIU_cmd_add_str(&pmi_response, "rc", "-1");
        PMIU_cmd_add_str(&pmi_response, "msg", tmp);
        PMIU_cmd_add_str(&pmi_response, "value", "unknown");
    }

    status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_abort(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    /* set a default exit code of 1 */
    int exitcode = 1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (PMIU_cmd_find_keyval(pmi, "exitcode") == NULL)
        HYDU_ERR_POP(status, "cannot find token: exitcode\n");

    exitcode = strtol(PMIU_cmd_find_keyval(pmi, "exitcode"), NULL, 10);

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


/* TODO: abort, create_kvs, destroy_kvs, getbyidx */
static struct HYD_pmcd_pmi_handle pmi_v1_handle_fns_foo[] = {
    {"barrier_in", fn_barrier_in},
    {"put", fn_put},
    {"get", fn_get},
    {"abort", fn_abort},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1 = pmi_v1_handle_fns_foo;

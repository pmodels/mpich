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
#include "uthash.h"
#include "utarray.h"

static struct HYD_barrier *barrier_group_create(struct HYD_pg *pg, const char *group, int count)
{
    struct HYD_barrier *s;
    s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
    if (!s) {
        goto fn_exit;
    }

    s->name = MPL_strdup(group);
    s->total_count = count;
    s->barrier_count = 0;
    utarray_new(s->proxy_list, &ut_int_icd, MPL_MEM_OTHER);

  fn_exit:
    return s;
}

static HYD_status barrier_group_finish(struct HYD_pg *pg, struct HYD_barrier *s,
                                       int process_fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    /* barrier_out */
    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "barrier_out");
    PMIU_cmd_add_str(&pmi_response, "group", s->name);

    for (int *p = (int *)utarray_front(s->proxy_list); p != NULL;
         p = (int *) utarray_next(s->proxy_list, p)) {
        status = HYD_pmiserv_pmi_reply(&pg->proxy_list[*p], process_fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    }

    PMIU_cmd_free_buf(&pmi_response);

    HASH_DEL(pg->barriers, s);
    MPL_free((char *) s->name);
    utarray_free(s->proxy_list);
    MPL_free(s);

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmiserv_barrier(struct HYD_proxy *proxy, int process_fd, int pgid,
                               struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    const char *group;
    int count, total_count;
    PMIU_CMD_GET_STRVAL_WITH_DEFAULT(pmi, "group", group, "WORLD");
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmi, "count", count, 0);
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmi, "total_count", total_count, 0);
    HYDU_ASSERT(count > 0 && total_count > 0, status);

    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(proxy->pgid);

    struct HYD_barrier *s;
    HASH_FIND_STR(pg->barriers, group, s);
    if (!s) {
        s = barrier_group_create(pg, group, total_count);
        HYDU_ASSERT(s, status);
        HASH_ADD_KEYPTR(hh, pg->barriers, s->name, strlen(s->name), s, MPL_MEM_OTHER);
    }

    int proxy_id = proxy->proxy_id;
    utarray_push_back(s->proxy_list, &proxy_id, MPL_MEM_OTHER);
    s->barrier_count += count;
    if (s->barrier_count == s->total_count) {
        HYD_pmiserv_bcast_keyvals(proxy, process_fd);

        status = barrier_group_finish(pg, s, process_fd, pmi);
        HYDU_ERR_POP(status, "error sending barrier_out to proxies\n");
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

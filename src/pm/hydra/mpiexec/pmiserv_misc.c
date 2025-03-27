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

static struct HYD_barrier *barrier_group_create(struct HYD_pg *pg, const char *group)
{
    struct HYD_barrier *s;
    s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
    if (!s) {
        goto fn_exit;
    }

    s->name = MPL_strdup(group);
    s->barrier_count = 0;
    s->has_upstream = false;    /* not used */

    if (strcmp(group, "WORLD") == 0) {
        s->num_procs = pg->proxy_count;
        s->proc_list = HYD_GROUP_ALL;
    } else if (strcmp(group, "NODE") == 0) {
        assert(0);
    } else {
        int *proxy_map = MPL_calloc(pg->proxy_count, sizeof(int), MPL_MEM_OTHER);
        const char *str = group;
        while (*str) {
            int rank = atoi(str);
            int proxy_id = pg->rankmap[rank] - pg->min_node_id;
            proxy_map[proxy_id]++;
            /* skip to next comma-separated id */
            while (*str && *str != ',') {
                str++;
            }
            if (*str == ',') {
                str++;
            }
        }

        /* populate barrier's proc_list (actually proxy list) */
        s->proc_list = MPL_malloc(pg->proxy_count * sizeof(int), MPL_MEM_OTHER);
        int num_proxy = 0;
        for (int i = 0; i < pg->proxy_count; i++) {
            if (proxy_map[i]) {
                s->proc_list[num_proxy++] = i;
            }
        }
        s->num_procs = num_proxy;
    }

  fn_exit:
    return s;
}

static HYD_status barrier_group_finish(struct HYD_pg *pg, struct HYD_barrier *s,
                                       int process_fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    /* barrier_out */
    struct PMIU_cmd pmi_response;
    PMIU_msg_set_response_barrier(pmi, &pmi_response, false /* is_static */ , s->name);

    if (s->proc_list == HYD_GROUP_ALL) {
        for (int i = 0; i < pg->proxy_count; i++) {
            status = HYD_pmiserv_pmi_reply(&pg->proxy_list[i], process_fd, &pmi_response);
            HYDU_ERR_POP(status, "error sending PMI response\n");
        }
    } else {
        for (int i = 0; i < s->num_procs; i++) {
            int j = s->proc_list[i];
            status = HYD_pmiserv_pmi_reply(&pg->proxy_list[j], process_fd, &pmi_response);
            HYDU_ERR_POP(status, "error sending PMI response\n");
        }
    }

    HASH_DEL(pg->barriers, s);
    MPL_free(s->name);
    if (s->proc_list != HYD_GROUP_ALL) {
        MPL_free(s->proc_list);
    }
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
    int pmi_errno = PMIU_msg_get_query_barrier(pmi, &group);
    HYDU_ASSERT(!pmi_errno, status);

    if (!group) {
        group = "WORLD";
    }

    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(proxy->pgid);

    struct HYD_barrier *s;
    HASH_FIND_STR(pg->barriers, group, s);
    if (!s) {
        s = barrier_group_create(pg, group);
        HYDU_ASSERT(s, status);
        HASH_ADD_KEYPTR(hh, pg->barriers, s->name, strlen(s->name), s, MPL_MEM_OTHER);
    }

    s->barrier_count++;
    if (s->barrier_count == s->num_procs) {
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

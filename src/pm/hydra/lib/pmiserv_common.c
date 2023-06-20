/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "pmiserv_common.h"
#include "topo.h"

/* There are two types of wire protocols:
 *   1. direct HDR protocols, used between mpiexec and proxies. It's
 *      a direct struct HYD_pmcd_hdr followed with a potential payload.
 *   2. PMI wire protocols, text string based. There are two versions:
 *      * PMI-1: cmd=xxx attr1=xxx ... attrn=xxx\n
 *      * PMI-2: length cmd=xxx;attr1=xxx;...;attrn=xxx;
 *               , where length is a 6-character decimal number.
 *
 * This file provides utilitis for handling these wire protocols.
 */

void HYD_pmcd_init_header(struct HYD_pmcd_hdr *hdr)
{
    memset(hdr, 0, sizeof(struct HYD_pmcd_hdr));
    hdr->cmd = CMD_INVALID;
    hdr->buflen = -1;
}

void HYD_pmcd_pmi_dump(struct PMIU_cmd *pmicmd)
{
    char *buf = NULL;
    int buflen = 0;

    PMIU_cmd_output(pmicmd, &buf, &buflen);
    if (buf[buflen - 1] == '\n') {
        HYDU_dump_noprefix(stdout, "%s", buf);
    } else {
        HYDU_dump_noprefix(stdout, "%s\n", buf);
    }
}

HYD_status HYD_pmcd_pmi_send(int fd, struct PMIU_cmd *pmi, struct HYD_pmcd_hdr *hdr, int debug)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    char *buf = NULL;
    int buflen = 0;

    int pmi_errno = PMIU_cmd_output(pmi, &buf, &buflen);
    HYDU_ASSERT(!pmi_errno, status);

    if (debug) {
        if (hdr) {
            HYDU_dump(stdout, "Sending internal PMI command (proxy:%d:%d):\n",
                      hdr->pgid, hdr->proxy_id);
        } else {
            HYDU_dump(stdout, "Sending PMI command:\n");
        }
        if (buf[buflen - 1] == '\n') {
            HYDU_dump_noprefix(stdout, "    %s", buf);
        } else {
            HYDU_dump_noprefix(stdout, "%s\n", buf);
        }
    }

    int sent = 0, closed;
    if (hdr) {
        hdr->buflen = buflen;
        hdr->u.pmi.pmi_version = pmi->version;

        status = HYDU_sock_write(fd, hdr, sizeof(*hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to send hdr\n");
        HYDU_ASSERT(!closed, status);
    }

    status = HYDU_sock_write(fd, buf, buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI message\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_kvs **kvs)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*kvs, struct HYD_kvs *, sizeof(struct HYD_kvs), status);
    (*kvs)->key_pair = NULL;
    (*kvs)->iter_end = NULL;
    (*kvs)->iter_cur = NULL;
    (*kvs)->iter_new_only = false;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_free_pmi_kvs_list(struct HYD_kvs *kvs_list)
{
    struct HYD_kvs_pair *key_pair, *tmp;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
        tmp = key_pair->next;
        MPL_free(key_pair);
        key_pair = tmp;
    }
    MPL_free(kvs_list);

    HYDU_FUNC_EXIT();
}

HYD_status HYD_kvs_find(struct HYD_kvs *kvs_list, const char *key, const char **val, int *found)
{
    HYDU_FUNC_ENTER();
    *found = 0;

    struct HYD_kvs_pair *run;
    for (run = kvs_list->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            *val = run->val;
            *found = 1;
            goto fn_exit;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return HYD_SUCCESS;
}

HYD_status HYD_pmcd_pmi_add_kvs(const char *key, const char *val, struct HYD_kvs * kvs, int *ret)
{
    struct HYD_kvs_pair *key_pair;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(key_pair, struct HYD_kvs_pair *, sizeof(struct HYD_kvs_pair), status);
    snprintf(key_pair->key, PMI_MAXKEYLEN, "%s", key);
    snprintf(key_pair->val, PMI_MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    *ret = 0;

    if (kvs->key_pair == NULL) {
        kvs->key_pair = key_pair;
    } else {
#ifdef PMI_KEY_CHECK
        const char *dummy_val;
        int found;
        HYD_kvs_find(kvs, key, &val, &found);
        if (found) {
            *ret = -1;
            goto fn_fail;
        }
#endif
        key_pair->next = kvs->key_pair;
        kvs->key_pair = key_pair;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    MPL_free(key_pair);
    goto fn_exit;
}

void HYD_kvs_iter_begin(struct HYD_kvs *kvs_list, bool new_only)
{
    kvs_list->iter_begin = kvs_list->key_pair;
    kvs_list->iter_cur = kvs_list->iter_begin;
    kvs_list->iter_new_only = new_only;
}

void HYD_kvs_iter_end(struct HYD_kvs *kvs_list)
{
    if (kvs_list->iter_new_only) {
        kvs_list->iter_end = kvs_list->iter_begin;
    }
    kvs_list->iter_begin = NULL;
    kvs_list->iter_new_only = false;
}

bool HYD_kvs_iter_next(struct HYD_kvs *kvs_list, const char **key, const char **val)
{
    if (kvs_list->iter_cur == NULL ||
        (kvs_list->iter_new_only && kvs_list->iter_cur == kvs_list->iter_end)) {
        return false;
    } else {
        *key = kvs_list->iter_cur->key;
        *val = kvs_list->iter_cur->val;
        kvs_list->iter_cur = kvs_list->iter_cur->next;
        return true;
    }
}

const char *HYD_pmcd_cmd_name(int cmd)
{
    switch (cmd) {
        case CMD_INVALID:
            return "CMD_INVALID";
        case CMD_PROC_INFO:
            return "CMD_PROC_INFO";
        case CMD_PMI_RESPONSE:
            return "CMD_PMI_RESPONSE";
        case CMD_SIGNAL:
            return "CMD_SIGNAL";
        case CMD_STDIN:
            return "CMD_STDIN";
        case CMD_PID_LIST:
            return "CMD_PID_LIST";
        case CMD_EXIT_STATUS:
            return "CMD_EXIT_STATUS";
        case CMD_PMI:
            return "CMD_PMI";
        case CMD_STDOUT:
            return "CMD_STDOUT";
        case CMD_STDERR:
            return "CMD_STDERR";
        case CMD_PROCESS_TERMINATED:
            return "CMD_PROCESS_TERMINATED";
        default:
            return "UNKNOWN";
    }
}

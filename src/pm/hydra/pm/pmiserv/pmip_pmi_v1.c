/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmip_pmi.h"
#include "pmip.h"
#include "bsci.h"
#include "demux.h"
#include "topo.h"
#include "hydt_ftb.h"

static HYD_status send_cmd_upstream(const char *start, int fd, char *args[])
{
    int i, j, sent, closed;
    char *tmp[HYD_NUM_TMP_STRINGS], *buf;
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    j = 0;
    tmp[j++] = HYDU_strdup(start);
    for (i = 0; args[i]; i++) {
        tmp[j++] = HYDU_strdup(args[i]);
        if (args[i + 1])
            tmp[j++] = HYDU_strdup(" ");
    }
    tmp[j] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &buf);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PMI_CMD;
    hdr.pid = fd;
    hdr.buflen = strlen(buf);
    hdr.pmi_version = 1;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "forwarding command (%s) upstream\n", buf);
    }

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
    HYDU_ASSERT(!closed, status);

    HYDU_FREE(buf);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_cmd_downstream(int fd, const char *cmd)
{
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "PMI response: %s", cmd);
    }

    status = HYDU_sock_write(fd, cmd, strlen(cmd), &sent, &closed);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    /* FIXME: We cannot abort when we are not able to send data
     * downstream. The upper layer needs to handle this based on
     * whether we want to abort or not.*/
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_init(int fd, char *args[])
{
    int pmi_version, pmi_subversion;
    const char *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1)
        tmp = HYDU_strdup("cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n");
    else if (pmi_version == 2 && pmi_subversion == 0)
        tmp = HYDU_strdup("cmd=response_to_init pmi_version=2 pmi_subversion=0 rc=0\n");
    else        /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);

    status = send_cmd_downstream(fd, tmp);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(tmp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_initack(int fd, char *args[])
{
    int id, i;
    char *val;
    struct HYD_pmcd_token *tokens;
    int token_count;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "pmiid");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find pmiid token\n");
    id = atoi(val);

    /* Store the PMI_ID to fd mapping */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_rank[i] == id) {
            HYD_pmcd_pmip.downstream.pmi_fd[i] = fd;
            HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 1;
            break;
        }
    }
    HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=initack\ncmd=set size=");
    tmp[i++] = HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_process_count);

    /* FIXME: allow for multiple ranks per PMI ID */
    tmp[i++] = HYDU_strdup("\ncmd=set rank=");
    tmp[i++] = HYDU_int_to_str(id);

    tmp[i++] = HYDU_strdup("\ncmd=set debug=");
    tmp[i++] = HYDU_int_to_str(HYD_pmcd_pmip.user_global.debug);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");
    HYDU_free_strlist(tmp);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_maxes(int fd, char *args[])
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=maxes kvsname_max=");
    tmp[i++] = HYDU_int_to_str(PMI_MAXKVSLEN);
    tmp[i++] = HYDU_strdup(" keylen_max=");
    tmp[i++] = HYDU_int_to_str(PMI_MAXKEYLEN);
    tmp[i++] = HYDU_strdup(" vallen_max=");
    tmp[i++] = HYDU_int_to_str(PMI_MAXVALLEN);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_appnum(int fd, char *args[])
{
    int i, idx;
    struct HYD_exec *exec;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Get the process index */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
        if (HYD_pmcd_pmip.downstream.pmi_fd[i] == fd)
            break;
    idx = i;
    HYDU_ASSERT(idx < HYD_pmcd_pmip.local.proxy_process_count, status);

    i = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {
        i += exec->proc_count;
        if (idx < i)
            break;
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=appnum appnum=");
    tmp[i++] = HYDU_int_to_str(exec->appnum);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_my_kvsname(int fd, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=my_kvsname kvsname=");
    tmp[i++] = HYDU_strdup(HYD_pmcd_pmip.local.kvs->kvs_name);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_usize(int fd, char *args[])
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=universe_size size=");
    tmp[i++] = HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_core_map.total);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *key;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: key\n");

    if (!strcmp(key, "PMI_process_mapping")) {
        i = 0;
        tmp[i++] = HYDU_strdup("cmd=get_result rc=0 msg=success value=");
        tmp[i++] = HYDU_strdup(HYD_pmcd_pmip.system_global.pmi_process_mapping);
        tmp[i++] = HYDU_strdup("\n");
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);

        status = send_cmd_downstream(fd, cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
        HYDU_FREE(cmd);
    }
    else if (!strcmp(key, "hydra_node_topomap")) {
        char *map;

        status = HYDT_topo_get_topomap(&map);
        HYDU_ERR_POP(status, "error getting topology map\n");

        i = 0;

        tmp[i++] = HYDU_strdup("cmd=get_result rc=");
        if (map) {
            tmp[i++] = HYDU_strdup("0 msg=success value=");
            tmp[i++] = HYDU_strdup(map);
        }
        else {
            tmp[i++] = HYDU_strdup("-1 msg=hydra_node_topomap_not_found value=unknown");
        }
        tmp[i++] = HYDU_strdup("\n");
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);

        status = send_cmd_downstream(fd, cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
        HYDU_FREE(cmd);

        HYDU_FREE(map);
    }
    else if (!strcmp(key, "hydra_node_processmap")) {
        char *map;

        status = HYDT_topo_get_processmap(&map);
        HYDU_ERR_POP(status, "error getting topology map\n");

        i = 0;

        tmp[i++] = HYDU_strdup("cmd=get_result rc=");
        if (map) {
            tmp[i++] = HYDU_strdup("0 msg=success value=");
            tmp[i++] = HYDU_strdup(map);
        }
        else {
            tmp[i++] = HYDU_strdup("-1 msg=hydra_node_processmap_not_found value=unknown");
        }
        tmp[i++] = HYDU_strdup("\n");
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);

        status = send_cmd_downstream(fd, cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
        HYDU_FREE(cmd);

        HYDU_FREE(map);
    }
    else {
        status = send_cmd_upstream("cmd=get ", fd, args);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_in(int fd, char *args[])
{
    static int barrier_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    barrier_count++;
    if (barrier_count == HYD_pmcd_pmip.local.proxy_process_count) {
        barrier_count = 0;

        status = send_cmd_upstream("cmd=barrier_in", fd, args);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_out(int fd, char *args[])
{
    const char *cmd;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = HYDU_strdup("cmd=barrier_out\n");

    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        status = send_cmd_downstream(HYD_pmcd_pmip.downstream.pmi_fd[i], cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    }

    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_finalize(int fd, char *args[])
{
    const char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = HYDU_strdup("cmd=finalize_ack\n");

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_pmcd_pmip_pmi_handle pmi_v1_handle_fns_foo[] = {
    {"init", fn_init},
    {"initack", fn_initack},
    {"get_maxes", fn_get_maxes},
    {"get_appnum", fn_get_appnum},
    {"get_my_kvsname", fn_get_my_kvsname},
    {"get_universe_size", fn_get_usize},
    {"get", fn_get},
    {"barrier_in", fn_barrier_in},
    {"barrier_out", fn_barrier_out},
    {"finalize", fn_finalize},
    {"\0", NULL}
};

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_v1 = pmi_v1_handle_fns_foo;

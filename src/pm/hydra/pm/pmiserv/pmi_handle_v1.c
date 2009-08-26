/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "demux.h"
#include "pmi_handle.h"
#include "pmi_handle_v1.h"

/* TODO: abort, create_kvs, destroy_kvs, getbyidx, spawn */
static struct HYD_PMCD_pmi_handle_fns pmi_v1_handle_fns_foo[] = {
    {"initack", HYD_PMCD_pmi_handle_v1_initack},
    {"get_maxes", HYD_PMCD_pmi_handle_v1_get_maxes},
    {"get_appnum", HYD_PMCD_pmi_handle_v1_get_appnum},
    {"get_my_kvsname", HYD_PMCD_pmi_handle_v1_get_my_kvsname},
    {"barrier_in", HYD_PMCD_pmi_handle_v1_barrier_in},
    {"put", HYD_PMCD_pmi_handle_v1_put},
    {"get", HYD_PMCD_pmi_handle_v1_get},
    {"get_universe_size", HYD_PMCD_pmi_handle_v1_get_usize},
    {"finalize", HYD_PMCD_pmi_handle_v1_finalize},
    {"\0", NULL}
};

static struct HYD_PMCD_pmi_handle pmi_v1_foo = { PMI_V1_DELIM, pmi_v1_handle_fns_foo };

struct HYD_PMCD_pmi_handle *HYD_PMCD_pmi_v1 = &pmi_v1_foo;

HYD_Status HYD_PMCD_pmi_handle_v1_initack(int fd, char *args[])
{
    int id, rank, i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_PMCD_pmi_pg_t *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    id = atoi(strtok(NULL, "="));

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=initack\ncmd=set size=");
    tmp[i++] = HYDU_int_to_str(HYD_pg_list->num_procs);
    tmp[i++] = HYDU_strdup("\ncmd=set rank=");

    status = HYD_PMCD_pmi_id_to_rank(id, &rank);
    HYDU_ERR_POP(status, "unable to convert ID to rank\n");
    tmp[i++] = HYDU_int_to_str(rank);

    tmp[i++] = HYDU_strdup("\ncmd=set debug=");
    tmp[i++] = HYDU_int_to_str(HYD_handle.debug);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

    HYDU_FREE(cmd);

    run = HYD_pg_list;
    while (run->next)
        run = run->next;

    /* Add the process to the last PG */
    status = HYD_PMCD_pmi_add_process_to_pg(run, fd, id);
    HYDU_ERR_POP(status, "unable to add process to pg\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_get_maxes(int fd, char *args[])
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=maxes kvsname_max=");
    tmp[i++] = HYDU_int_to_str(MAXKVSNAME);
    tmp[i++] = HYDU_strdup(" keylen_max=");
    tmp[i++] = HYDU_int_to_str(MAXKEYLEN);
    tmp[i++] = HYDU_strdup(" vallen_max=");
    tmp[i++] = HYDU_int_to_str(MAXVALLEN);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_get_appnum(int fd, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = HYD_PMCD_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unable to find process structure\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=appnum appnum=");
    tmp[i++] = HYDU_int_to_str(process->node->pg->id);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_get_my_kvsname(int fd, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = HYD_PMCD_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=my_kvsname kvsname=");
    tmp[i++] = HYDU_strdup(process->node->pg->kvs->kvs_name);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_barrier_in(int fd, char *args[])
{
    HYD_PMCD_pmi_process_t *process, *prun;
    HYD_PMCD_pmi_node_t *node;
    const char *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = HYD_PMCD_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    process->node->pg->barrier_count++;

    /* All the processes have arrived at the barrier; send a
     * barrier_out message to everyone. */
    if (process->node->pg->barrier_count == process->node->pg->num_procs) {
        cmd = "cmd=barrier_out\n";
        for (node = process->node->pg->node_list; node; node = node->next) {
            for (prun = node->process_list; prun; prun = prun->next) {
                status = HYDU_sock_writeline(prun->fd, cmd, strlen(cmd));
                HYDU_ERR_POP(status, "error writing PMI line\n");
            }
        }

        process->node->pg->barrier_count = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_put(int fd, char *args[])
{
    int i, ret;
    HYD_PMCD_pmi_process_t *process;
    HYD_PMCD_pmi_kvs_pair_t *key_pair, *run;
    char *kvsname, *key, *val;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    kvsname = strtok(NULL, "=");
    strtok(args[1], "=");
    key = strtok(NULL, "=");
    strtok(args[2], "=");
    val = strtok(NULL, "=");

    /* Find the group id corresponding to this fd */
    process = HYD_PMCD_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    if (strcmp(process->node->pg->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "kvsname (%s) does not match this process' kvs space (%s)\n",
                             kvsname, process->node->pg->kvs->kvs_name);

    status = HYD_PMCD_pmi_add_kvs(key, val, process->node->pg->kvs, &ret);
    HYDU_ERR_POP(status, "unable to add keypair to kvs\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=put_result rc=");
    tmp[i++] = HYDU_int_to_str(ret);
    if (ret == 0) {
        tmp[i++] = HYDU_strdup(" msg=success");
    }
    else {
        tmp[i++] = HYDU_strdup(" msg=duplicate_key");
        tmp[i++] = HYDU_strdup(key);
    }
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_get(int fd, char *args[])
{
    int i, found, ret;
    HYD_PMCD_pmi_process_t *process;
    HYD_PMCD_pmi_kvs_pair_t *run;
    char *kvsname, *key, *node_list;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    kvsname = strtok(NULL, "=");
    strtok(args[1], "=");
    key = strtok(NULL, "=");

    /* Find the group id corresponding to this fd */
    process = HYD_PMCD_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    if (strcmp(process->node->pg->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "kvsname (%s) does not match this process' kvs space (%s)\n",
                             kvsname, process->node->pg->kvs->kvs_name);

    /* Try to find the key */
    found = 0;
    for (run = process->node->pg->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        /* Didn't find the job attribute; see if we know how to
         * generate it */
        if (strcmp(key, "process-mapping") == 0) {
            /* Create a vector format */
            status = HYD_PMCD_pmi_process_mapping(process, HYD_PMCD_pmi_vector, &node_list);
            HYDU_ERR_POP(status, "Unable to get process mapping information\n");

            if (strlen(node_list) > MAXVALLEN)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "key value larger than maximum allowed\n");

            status = HYD_PMCD_pmi_add_kvs("process-mapping", node_list,
                                          process->node->pg->kvs, &ret);
            HYDU_ERR_POP(status, "unable to add process_mapping to KVS\n");
        }

        /* Search for the key again */
        for (run = process->node->pg->kvs->key_pair; run; run = run->next) {
            if (!strcmp(run->key, key)) {
                found = 1;
                break;
            }
        }
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=get_result rc=");
    if (found) {
        tmp[i++] = HYDU_strdup("0 msg=success value=");
        tmp[i++] = HYDU_strdup(run->val);
    }
    else {
        tmp[i++] = HYDU_strdup("-1 msg=key_");
        tmp[i++] = HYDU_strdup(key);
        tmp[i++] = HYDU_strdup("_not_found value=unknown");
    }
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_finalize(int fd, char *args[])
{
    const char *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = "cmd=finalize_ack\n";
    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

    if (status == HYD_SUCCESS) {
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to register fd\n");
        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_handle_v1_get_usize(int fd, char *args[])
{
    int usize, i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_query_usize(&usize);
    HYDU_ERR_POP(status, "unable to get bootstrap universe size\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=universe_size size=");
    tmp[i++] = HYDU_int_to_str(usize);
    tmp[i++] = HYDU_strdup("\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

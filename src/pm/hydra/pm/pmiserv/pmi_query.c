/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "pmi_query.h"

HYD_Handle handle;
HYD_PMCD_pmi_pg_t *pg_list = NULL;

static HYD_Status allocate_kvs(HYD_PMCD_pmi_kvs_t ** kvs, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, HYD_PMCD_pmi_kvs_t *, sizeof(HYD_PMCD_pmi_kvs_t), status);
    MPIU_Snprintf((*kvs)->kvs_name, MAXNAMELEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status create_pg(HYD_PMCD_pmi_pg_t ** pg, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*pg, HYD_PMCD_pmi_pg_t *, sizeof(HYD_PMCD_pmi_pg_t), status);
    (*pg)->id = pgid;
    (*pg)->num_procs = 0;
    (*pg)->barrier_count = 0;
    (*pg)->process = NULL;

    status = allocate_kvs(&(*pg)->kvs, pgid);
    HYDU_ERR_POP(status, "unable to allocate kvs space\n");

    (*pg)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status add_process_to_pg(HYD_PMCD_pmi_pg_t * pg, int fd)
{
    HYD_PMCD_pmi_process_t *process, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(process, HYD_PMCD_pmi_process_t *, sizeof(HYD_PMCD_pmi_process_t), status);
    process->fd = fd;
    process->pg = pg;
    process->next = NULL;
    if (pg->process == NULL)
        pg->process = process;
    else {
        tmp = pg->process;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = process;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_create_pg(void)
{
    struct HYD_Proc_params *proc_params;
    int num_procs;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the number of processes in the PG */
    num_procs = 0;
    proc_params = handle.proc_params;
    while (proc_params) {
        num_procs += proc_params->exec_proc_count;
        proc_params = proc_params->next;
    }

    status = create_pg(&pg_list, 0);
    HYDU_ERR_POP(status, "unable to create pg\n");
    pg_list->num_procs = num_procs;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_initack(int fd, char *args[])
{
    int id, size, debug, i;
    char *ssize, *srank, *sdebug, *tmp[HYDU_NUM_JOIN_STR], *cmd;
    struct HYD_Proc_params *proc_params;
    HYD_PMCD_pmi_pg_t *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    id = atoi(strtok(NULL, "="));

    size = 0;
    proc_params = handle.proc_params;
    while (proc_params) {
        size += proc_params->exec_proc_count;
        proc_params = proc_params->next;
    }
    debug = handle.debug;

    status = HYDU_int_to_str(size, &ssize);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    status = HYDU_int_to_str(id, &srank);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    status = HYDU_int_to_str(debug, &sdebug);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    i = 0;
    tmp[i++] = "cmd=initack\ncmd=set size=";
    tmp[i++] = ssize;
    tmp[i++] = "\ncmd=set rank=";
    tmp[i++] = srank;
    tmp[i++] = "\ncmd=set debug=";
    tmp[i++] = sdebug;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

    HYDU_FREE(ssize);
    HYDU_FREE(srank);
    HYDU_FREE(sdebug);
    HYDU_FREE(cmd);

    run = pg_list;
    while (run->next)
        run = run->next;

    /* Add the process to the last PG */
    status = add_process_to_pg(run, fd);
    HYDU_ERR_POP(status, "unable to add process to pg\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_init(int fd, char *args[])
{
    int pmi_version, pmi_subversion;
    char *tmp[HYDU_NUM_JOIN_STR];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1) {
        /* We support PMI v1.0 and 1.1 */
        tmp[0] = "cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n";
        status = HYDU_sock_writeline(fd, tmp[0], strlen(tmp[0]));
        HYDU_ERR_POP(status, "error writing PMI line\n");
    }
    else {
        /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_get_maxes(int fd, char *args[])
{
    int i;
    char *tmp[HYDU_NUM_JOIN_STR], *cmd;
    char *maxkvsname, *maxkeylen, *maxvallen;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_int_to_str(MAXKVSNAME, &maxkvsname);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    status = HYDU_int_to_str(MAXKEYLEN, &maxkeylen);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    status = HYDU_int_to_str(MAXVALLEN, &maxvallen);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    i = 0;
    tmp[i++] = "cmd=maxes kvsname_max=";
    tmp[i++] = maxkvsname;
    tmp[i++] = " keylen_max=";
    tmp[i++] = maxkeylen;
    tmp[i++] = " vallen_max=";
    tmp[i++] = maxvallen;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

    HYDU_FREE(maxkvsname);
    HYDU_FREE(maxkeylen);
    HYDU_FREE(maxvallen);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_PMCD_pmi_process_t *find_process(int fd)
{
    HYD_PMCD_pmi_pg_t *pg;
    HYD_PMCD_pmi_process_t *process = NULL;

    pg = pg_list;
    while (pg) {
        process = pg->process;
        while (process) {
            if (process->fd == fd)
                break;
            process = process->next;
        }
        pg = pg->next;
    }

    return process;
}


HYD_Status HYD_PMCD_pmi_query_get_appnum(int fd, char *args[])
{
    char *tmp[HYDU_NUM_JOIN_STR], *cmd;
    char *sapp_num;
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) /* We didn't find the process */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unable to find process structure\n");

    status = HYDU_int_to_str(process->pg->id, &sapp_num);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    i = 0;
    tmp[i++] = "cmd=appnum appnum=";
    tmp[i++] = sapp_num;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

    HYDU_FREE(sapp_num);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_get_my_kvsname(int fd, char *args[])
{
    char *tmp[HYDU_NUM_JOIN_STR], *cmd;
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    i = 0;
    tmp[i++] = "cmd=my_kvsname kvsname=";
    tmp[i++] = process->pg->kvs->kvs_name;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_barrier_in(int fd, char *args[])
{
    HYD_PMCD_pmi_process_t *process, *run;
    char *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    process->pg->barrier_count++;

    /* All the processes have arrived at the barrier; send a
     * barrier_out message to everyone. */
    if (process->pg->barrier_count == process->pg->num_procs) {
        cmd = "cmd=barrier_out\n";
        run = process->pg->process;     /* The first process in the list */
        while (run) {
            status = HYDU_sock_writeline(run->fd, cmd, strlen(cmd));
            HYDU_ERR_POP(status, "error writing PMI line\n");
            run = run->next;
        }

        process->pg->barrier_count = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_put(int fd, char *args[])
{
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_PMCD_pmi_kvs_pair_t *key_pair, *run;
    char *kvsname, *key, *val, *key_pair_str = NULL;
    char *tmp[HYDU_NUM_JOIN_STR], *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    kvsname = strtok(NULL, "=");
    strtok(args[1], "=");
    key = strtok(NULL, "=");
    strtok(args[2], "=");
    val = strtok(NULL, "=");

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    if (strcmp(process->pg->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "kvsname (%s) does not match this process' kvs space (%s)\n",
                             kvsname, process->pg->kvs->kvs_name);

    HYDU_MALLOC(key_pair, HYD_PMCD_pmi_kvs_pair_t *, sizeof(HYD_PMCD_pmi_kvs_pair_t), status);
    MPIU_Snprintf(key_pair->key, MAXKEYLEN, "%s", key);
    MPIU_Snprintf(key_pair->val, MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    i = 0;
    tmp[i++] = "cmd=put_result rc=";
    if (process->pg->kvs->key_pair == NULL) {
        process->pg->kvs->key_pair = key_pair;
        tmp[i++] = "0 msg=success";
    }
    else {
        run = process->pg->kvs->key_pair;
        while (run->next) {
            if (!strcmp(run->key, key_pair->key)) {
                tmp[i++] = "-1 msg=duplicate_key";
                key_pair_str = MPIU_Strdup(key_pair->key);
                tmp[i++] = key_pair_str;
                break;
            }
            run = run->next;
        }
        run->next = key_pair;
        tmp[i++] = "0 msg=success";
    }
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);
    HYDU_FREE(key_pair_str);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_get(int fd, char *args[])
{
    int i;
    HYD_PMCD_pmi_process_t *process;
    HYD_PMCD_pmi_kvs_pair_t *run;
    char *kvsname, *key;
    char *tmp[HYDU_NUM_JOIN_STR], *cmd, *key_val_str = NULL;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    kvsname = strtok(NULL, "=");
    strtok(args[1], "=");
    key = strtok(NULL, "=");

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    if (strcmp(process->pg->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "kvsname (%s) does not match this process' kvs space (%s)\n",
                             kvsname, process->pg->kvs->kvs_name);

    i = 0;
    tmp[i++] = "cmd=get_result rc=";
    if (process->pg->kvs->key_pair == NULL) {
        tmp[i++] = "-1 msg=key_";
        tmp[i++] = key;
        tmp[i++] = "_not_found value=unknown";
    }
    else {
        run = process->pg->kvs->key_pair;
        while (run) {
            if (!strcmp(run->key, key)) {
                tmp[i++] = "0 msg=success value=";
                key_val_str = MPIU_Strdup(run->val);
                tmp[i++] = key_val_str;
                break;
            }
            run = run->next;
        }
        if (run == NULL) {
            tmp[i++] = "-1 msg=key_";
            tmp[i++] = key;
            tmp[i++] = "_not_found value=unknown";
        }
    }
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);
    HYDU_FREE(key_val_str);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_finalize(int fd, char *args[])
{
    char *cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = "cmd=finalize_ack\n";
    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_query_get_usize(int fd, char *args[])
{
    int usize, i;
    char *tmp[HYDU_NUM_JOIN_STR], *cmd, *usize_str;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_get_usize(&usize);
    HYDU_ERR_POP(status, "unable to get bootstrap universe size\n");

    status = HYDU_int_to_str(usize, &usize_str);
    HYDU_ERR_POP(status, "unable to convert int to string\n");

    i = 0;
    tmp[i++] = "cmd=universe_size size=";
    tmp[i++] = usize_str;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    status = HYDU_sock_writeline(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status free_pmi_process_list(HYD_PMCD_pmi_process_t * process_list)
{
    HYD_PMCD_pmi_process_t *process, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    process = process_list;
    while (process) {
        tmp = process->next;
        HYDU_FREE(process);
        process = tmp;
    }

    HYDU_FUNC_EXIT();
    return status;
}


static HYD_Status free_pmi_kvs_list(HYD_PMCD_pmi_kvs_t * kvs_list)
{
    HYD_PMCD_pmi_kvs_pair_t *key_pair, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
        tmp = key_pair->next;
        HYDU_FREE(key_pair);
        key_pair = tmp;
    }
    HYDU_FREE(kvs_list);

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_PMCD_pmi_finalize(void)
{
    HYD_PMCD_pmi_pg_t *pg, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = pg_list;
    while (pg) {
        tmp = pg->next;

        status = free_pmi_process_list(pg->process);
        HYDU_ERR_POP(status, "unable to free process list\n");

        status = free_pmi_kvs_list(pg->kvs);
        HYDU_ERR_POP(status, "unable to free kvs list\n");

        HYDU_FREE(pg);
        pg = tmp;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

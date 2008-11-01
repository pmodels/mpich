/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "pmcu_pmi.h"

HYD_CSI_Handle * csi_handle;
HYD_PMCU_pmi_pg_t * pg_list = NULL;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "allocate_kvs"
static HYD_Status allocate_kvs(HYD_PMCU_pmi_kvs_t ** kvs, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, HYD_PMCU_pmi_kvs_t *, sizeof(HYD_PMCU_pmi_kvs_t), status);
    MPIU_Snprintf((*kvs)->kvs_name, MAXNAMELEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "create_pg"
static HYD_Status create_pg(HYD_PMCU_pmi_pg_t ** pg, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*pg, HYD_PMCU_pmi_pg_t *, sizeof(HYD_PMCU_pmi_pg_t), status);
    (*pg)->id = pgid;
    (*pg)->num_procs = 0;
    (*pg)->barrier_count = 0;
    (*pg)->process = NULL;

    status = allocate_kvs(&(*pg)->kvs, pgid);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("Unable to allocate kvs space\n");
	status = HYD_INTERNAL_ERROR;
	goto fn_fail;
    }

    (*pg)->next = NULL;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "add_process_to_pg"
static HYD_Status add_process_to_pg(HYD_PMCU_pmi_pg_t * pg, int fd)
{
    HYD_PMCU_pmi_process_t * process, * tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(process, HYD_PMCU_pmi_process_t *, sizeof(HYD_PMCU_pmi_process_t), status);
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


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_Create_pg"
HYD_Status HYD_PMCU_Create_pg(void)
{
    HYD_PMCU_pmi_pg_t * run;
    struct HYD_CSI_Proc_params * proc_params;
    int num_procs;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the number of processes in the PG */
    num_procs = 0;
    proc_params = csi_handle->proc_params;
    while (proc_params) {
	num_procs += proc_params->user_num_procs;
	proc_params = proc_params->next;
    }

    status = create_pg(&pg_list, 0);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to create pg\n");
	goto fn_fail;
    }
    pg_list->num_procs = num_procs;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_initack"
HYD_Status HYD_PMCU_pmi_initack(int fd, char * args[])
{
    int id, size, debug, i;
    char * ssize, * srank, * sdebug, * tmp[HYDU_NUM_JOIN_STR], * cmd;
    struct HYD_CSI_Proc_params * proc_params;
    HYD_PMCU_pmi_pg_t * pg, * run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    id = atoi(strtok(NULL, "="));

    size = 0;
    proc_params = csi_handle->proc_params;
    while (proc_params) {
	size += proc_params->user_num_procs;
	proc_params = proc_params->next;
    }
    debug = csi_handle->debug;

    HYDU_Int_to_str(size, ssize, status);
    HYDU_Int_to_str(id, srank, status);
    HYDU_Int_to_str(debug, sdebug, status);

    i = 0;
    tmp[i++] = "cmd=initack\ncmd=set size=";
    tmp[i++] = ssize;
    tmp[i++] = "\ncmd=set rank=";
    tmp[i++] = srank;
    tmp[i++] = "\ncmd=set debug=";
    tmp[i++] = sdebug;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(ssize);
    HYDU_FREE(srank);
    HYDU_FREE(sdebug);
    HYDU_FREE(cmd);

    run = pg_list;
    while (run->next)
	run = run->next;

    /* Add the process to the last PG */
    status = add_process_to_pg(run, fd);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to add process to pg\n");
	goto fn_fail;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_init"
HYD_Status HYD_PMCU_pmi_init(int fd, char * args[])
{
    int pmi_version, pmi_subversion, i;
    char * tmp[HYDU_NUM_JOIN_STR];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1) {
	/* We support PMI v1.0 and 1.1 */
	tmp[0] = "cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n";
	status = HYDU_Sock_writeline(fd, tmp[0], strlen(tmp[0]));
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	    goto fn_fail;
	}
    }
    else {
	/* PMI version mismatch */
	HYDU_Error_printf("got a pmi version mismatch; pmi_version: %d; pmi_subversion: %d\n",
			  pmi_version, pmi_subversion);
	goto fn_fail;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_get_maxes"
HYD_Status HYD_PMCU_pmi_get_maxes(int fd, char * args[])
{
    int i;
    char * tmp[HYDU_NUM_JOIN_STR], * cmd;
    char * maxkvsname, * maxkeylen, * maxvallen;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_Int_to_str(MAXKVSNAME, maxkvsname, status);
    HYDU_Int_to_str(MAXKEYLEN, maxkeylen, status);
    HYDU_Int_to_str(MAXVALLEN, maxvallen, status);

    i = 0;
    tmp[i++] = "cmd=maxes kvsname_max=";
    tmp[i++] = maxkvsname;
    tmp[i++] = " keylen_max=";
    tmp[i++] = maxkeylen;
    tmp[i++] = " vallen_max=";
    tmp[i++] = maxvallen;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
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


static HYD_PMCU_pmi_process_t * find_process(int fd)
{
    HYD_PMCU_pmi_pg_t * pg;
    HYD_PMCU_pmi_process_t * process;

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


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_get_appnum"
HYD_Status HYD_PMCU_pmi_get_appnum(int fd, char * args[])
{
    char * tmp[HYDU_NUM_JOIN_STR], * cmd;
    char * sapp_num;
    int i;
    HYD_PMCU_pmi_process_t * process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) { /* We didn't find the process */
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("could not find the process structure\n");
	goto fn_fail;
    }

    HYDU_Int_to_str(process->pg->id, sapp_num, status);

    i = 0;
    tmp[i++] = "cmd=appnum appnum=";
    tmp[i++] = sapp_num;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(cmd);

    HYDU_FREE(sapp_num);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_get_my_kvsname"
HYD_Status HYD_PMCU_pmi_get_my_kvsname(int fd, char * args[])
{
    char * tmp[HYDU_NUM_JOIN_STR], * cmd;
    int i;
    HYD_PMCU_pmi_process_t * process;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) { /* We didn't find the process */
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("could not find the process structure for fd %d\n", fd);
	goto fn_fail;
    }

    i = 0;
    tmp[i++] = "cmd=my_kvsname kvsname=";
    tmp[i++] = process->pg->kvs->kvs_name;
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(cmd);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_barrier_in"
HYD_Status HYD_PMCU_pmi_barrier_in(int fd, char * args[])
{
    HYD_PMCU_pmi_process_t * process, * run;
    char * cmd;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) { /* We didn't find the process */
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("could not find the process structure for fd %d\n", fd);
	goto fn_fail;
    }

    process->pg->barrier_count++;

    /* All the processes have arrived at the barrier; send a
     * barrier_out message to everyone. */
    if (process->pg->barrier_count == process->pg->num_procs) {
	cmd = "cmd=barrier_out\n";
	run = process->pg->process; /* The first process in the list */
	while (run) {
	    status = HYDU_Sock_writeline(run->fd, cmd, strlen(cmd));
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("sock utils returned error when writing PMI line\n");
		goto fn_fail;
	    }
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


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_put"
HYD_Status HYD_PMCU_pmi_put(int fd, char * args[])
{
    int i;
    HYD_PMCU_pmi_process_t * process;
    HYD_PMCU_pmi_kvs_pair_t * key_pair, * run;
    char * kvsname, * key, * val;
    char * tmp[HYDU_NUM_JOIN_STR], * cmd;
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
    if (process == NULL) { /* We didn't find the process */
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("could not find the process structure for fd %d\n", fd);
	goto fn_fail;
    }

    if (strcmp(process->pg->kvs->kvs_name, kvsname)) {
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("kvsname (%s) does not match this process' kvs space (%s)\n",
			  kvsname, process->pg->kvs->kvs_name);
	goto fn_fail;
    }

    HYDU_MALLOC(key_pair, HYD_PMCU_pmi_kvs_pair_t *, sizeof(HYD_PMCU_pmi_kvs_pair_t), status);
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
		tmp[i++] = MPIU_Strdup(key_pair->key);
		break;
	    }
	    run = run->next;
	}
	run->next = key_pair;
	tmp[i++] = "0 msg=success";
    }
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(cmd);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_get"
HYD_Status HYD_PMCU_pmi_get(int fd, char * args[])
{
    int i;
    HYD_PMCU_pmi_process_t * process;
    HYD_PMCU_pmi_kvs_pair_t * run;
    char * kvsname, * key;
    char * tmp[HYDU_NUM_JOIN_STR], * cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    kvsname = strtok(NULL, "=");
    strtok(args[1], "=");
    key = strtok(NULL, "=");

    /* Find the group id corresponding to this fd */
    process = find_process(fd);
    if (process == NULL) { /* We didn't find the process */
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("could not find the process structure for fd %d\n", fd);
	goto fn_fail;
    }

    if (strcmp(process->pg->kvs->kvs_name, kvsname)) {
	status = HYD_INTERNAL_ERROR;
	HYDU_Error_printf("kvsname (%s) does not match this process' kvs space (%s)\n",
			  kvsname, process->pg->kvs->kvs_name);
	goto fn_fail;
    }

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
		tmp[i++] = MPIU_Strdup(run->val);
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

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(cmd);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_finalize"
HYD_Status HYD_PMCU_pmi_finalize(int fd, char * args[])
{
    char * cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = "cmd=finalize_ack\n";
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_pmi_get_usize"
HYD_Status HYD_PMCU_pmi_get_usize(int fd, char * args[])
{
    int usize, i;
    char * tmp[HYDU_NUM_JOIN_STR], * cmd, * usize_str;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_Get_universe_size(&usize);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("Unable to get universe size from the bootstrap server\n");
	goto fn_fail;
    }

    HYDU_Int_to_str(usize, usize_str, status);

    i = 0;
    tmp[i++] = "cmd=universe_size size=";
    tmp[i++] = MPIU_Strdup(usize_str);
    tmp[i++] = "\n";
    tmp[i++] = NULL;

    HYDU_STR_ALLOC_AND_JOIN(tmp, cmd, status);
    status = HYDU_Sock_writeline(fd, cmd, strlen(cmd));
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned error when writing PMI line\n");
	goto fn_fail;
    }
    HYDU_FREE(cmd);

    HYDU_FREE(usize_str);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "free_pmi_process_list"
static HYD_Status free_pmi_process_list(HYD_PMCU_pmi_process_t * process_list)
{
    HYD_PMCU_pmi_process_t * process, * tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    process = process_list;
    while (process) {
	tmp = process->next;
	HYDU_FREE(process);
	process = tmp;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "free_pmi_kvs_list"
static HYD_Status free_pmi_kvs_list(HYD_PMCU_pmi_kvs_t * kvs_list)
{
    HYD_PMCU_pmi_kvs_pair_t * key_pair, * tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
	tmp = key_pair->next;
	HYDU_FREE(key_pair);
	key_pair = tmp;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCU_Finalize"
HYD_Status HYD_PMCU_Finalize(void)
{
    HYD_PMCU_pmi_pg_t * pg, * tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = pg_list;
    while (pg) {
	tmp = pg->next;

	status = free_pmi_process_list(pg->process);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("unable to free process list\n");
	    goto fn_fail;
	}

	status = free_pmi_kvs_list(pg->kvs);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("unable to free kvs list\n");
	    goto fn_fail;
	}

	HYDU_FREE(pg);
	pg = tmp;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "mpiexec.h"
#include "mpl_uthash.h"

/* *INDENT-OFF* */
struct mpiexec_params_s mpiexec_params = {
    .rmk = NULL,
    .launcher = NULL,
    .launcher_exec = NULL,

    .binding = NULL,
    .mapping = NULL,
    .membind = NULL,

    .debug = -1,
    .usize = MPIEXEC_USIZE__UNSET,

    .tree_width = -1,

    .auto_cleanup = -1,

    .base_path = NULL,
    .port_range = NULL,
    .nameserver = NULL,
    .localhost = NULL,

    .global_node_list = NULL,
    .global_node_count = 0,
    .global_core_count = -1,
    .global_active_processes = NULL,

    .ppn = -1,
    .print_all_exitcodes = -1,
    .timeout = -1,

    .envprop = MPIEXEC_ENVPROP__UNSET,
    .envlist_count = 0,
    .envlist = NULL,

    .primary = {
        .list = NULL,
        .serialized_buf_len = 0,
        .serialized_buf = NULL,
    },

    .secondary = {
        .list = NULL,
        .serialized_buf_len = 0,
        .serialized_buf = NULL,
    },

    .prepend_pattern = NULL,
    .outfile_pattern = NULL,
    .errfile_pattern = NULL,

    .pid_ref_count = -1,
};
/* *INDENT-ON* */

struct mpiexec_pg *mpiexec_pg_hash = NULL;

int *contig_pids;

static void signal_cb(int signum)
{
    struct mpiexec_cmd cmd;
    static int sigint_count = 0;
    int sent, closed;

    HYD_FUNC_ENTER();

    cmd.type = MPIEXEC_CMD_TYPE__SIGNAL;
    cmd.signum = signum;

    /* SIGINT is a partially special signal. The first time we see it,
     * we will send it to the processes. The next time, we will treat
     * it as a SIGKILL (user convenience to force kill processes). */
    if (signum == SIGINT && ++sigint_count > 1)
        exit(1);
    else if (signum == SIGINT) {
        /* First Ctrl-C */
        HYD_PRINT(stdout, "Sending Ctrl-C to processes as requested\n");
        HYD_PRINT(stdout, "Press Ctrl-C again to force abort\n");
    }

    HYD_sock_write(mpiexec_params.signal_pipe[0], &cmd, sizeof(cmd), &sent, &closed,
                   HYD_SOCK_COMM_TYPE__BLOCKING);

    HYD_FUNC_EXIT();
    return;
}

static HYD_status cmd_bcast_root(struct MPX_cmd cmd, struct mpiexec_pg *pg, void *buf)
{
    int sent, closed;
    struct HYD_int_hash *hash, *thash;
    HYD_status status = HYD_SUCCESS;

    MPL_HASH_ITER(hh, pg->downstream.fd_control_hash, hash, thash) {
        status =
            HYD_sock_write(hash->key, &cmd, sizeof(cmd), &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error sending cwd cmd to proxy\n");
        HYD_ASSERT(!closed, status);

        if (cmd.data_len) {
            status =
                HYD_sock_write(hash->key, buf, cmd.data_len, &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error sending cwd to proxy\n");
            HYD_ASSERT(!closed, status);
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cleanup_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    struct mpiexec_cmd mpiexec_cmd;
    struct MPX_cmd cmd;
    struct mpiexec_pg *pg, *tmp;
    int recvd, closed;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status =
        HYD_sock_read(mpiexec_params.signal_pipe[1], &mpiexec_cmd, sizeof(struct mpiexec_cmd),
                      &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading mpiexec command\n");
    HYD_ASSERT(!closed, status);

    cmd.type = MPX_CMD_TYPE__SIGNAL;
    cmd.data_len = 0;
    cmd.u.signal.signum = mpiexec_cmd.signum;

    MPL_HASH_ITER(hh, mpiexec_pg_hash, pg, tmp) {
        status = cmd_bcast_root(cmd, pg, NULL);
        HYD_ERR_POP(status, "error pushing cmd downstream\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status get_node_list(void)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    if (mpiexec_params.global_node_count == 0) {
        /* Node list is not created yet. The user might not have
         * provided the host file. Find an RMK to query. */

        /* try to autodetect */
        if (mpiexec_params.rmk == NULL) {
            const char *rmk = HYD_rmk_detect();
            if (rmk)
                mpiexec_params.rmk = MPL_strdup(rmk);
        }

        /* if we found an RMK, ask it for a list of global_node_list */
        if (mpiexec_params.rmk) {
            status =
                HYD_rmk_query_node_list(mpiexec_params.rmk, &mpiexec_params.global_node_count,
                                        &mpiexec_params.global_node_list);
            HYD_ERR_POP(status, "error querying rmk for a node list\n");
        }

        /* if we didn't find anything, use localhost */
        if (mpiexec_params.global_node_count == 0) {
            char localhost[HYD_MAX_HOSTNAME_LEN] = { 0 };
            int max_global_node_count = 0;

            /* The RMK didn't give us anything back; use localhost */
            if (gethostname(localhost, HYD_MAX_HOSTNAME_LEN) < 0)
                HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "unable to get local hostname\n");

            status =
                HYD_node_list_append(localhost, 1, &mpiexec_params.global_node_list,
                                     &mpiexec_params.global_node_count, &max_global_node_count);
            HYD_ERR_POP(status, "unable to add to node list\n");
        }
    }

    if (mpiexec_params.ppn != -1)
        for (i = 0; i < mpiexec_params.global_node_count; i++)
            mpiexec_params.global_node_list[i].core_count = mpiexec_params.ppn;

    if (mpiexec_params.tree_width == 0)
        mpiexec_params.tree_width = mpiexec_params.global_node_count;

    HYD_MALLOC(mpiexec_params.global_active_processes, int *,
               mpiexec_params.global_node_count * sizeof(int), status);
    for (i = 0; i < mpiexec_params.global_node_count; i++)
        mpiexec_params.global_active_processes[i] = 0;

    mpiexec_params.global_core_count = 0;
    for (i = 0; i < mpiexec_params.global_node_count; i++)
        mpiexec_params.global_core_count += mpiexec_params.global_node_list[i].core_count;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status find_launcher(void)
{
    HYD_status status = HYD_SUCCESS;

    /* check environment variables */
    if (mpiexec_params.launcher == NULL) {
        if (MPL_env2str("HYDRA_LAUNCHER", (const char **) &mpiexec_params.launcher))
            mpiexec_params.launcher = MPL_strdup(mpiexec_params.launcher);
    }
    if (mpiexec_params.launcher == NULL) {
        if (MPL_env2str("HYDRA_BOOTSTRAP", (const char **) &mpiexec_params.launcher))
            mpiexec_params.launcher = MPL_strdup(mpiexec_params.launcher);
    }

    /* if there was an RMK set, see if we can use that as a launcher
     * as well */
    if (mpiexec_params.rmk)
        if (HYD_bstrap_query_avail(mpiexec_params.rmk))
            mpiexec_params.launcher = MPL_strdup(mpiexec_params.rmk);

    /* fallback to the default launcher */
    if (mpiexec_params.launcher == NULL)
        mpiexec_params.launcher = MPL_strdup(HYDRA_DEFAULT_BSTRAP);

    /* if we still do not have a launcher, abort */
    if (mpiexec_params.launcher == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "no appropriate launcher found\n");

    /* try to find a launcher executable */
    if (mpiexec_params.launcher_exec == NULL) {
        if (MPL_env2str("HYDRA_LAUNCHER_EXEC", (const char **) &mpiexec_params.launcher_exec))
            mpiexec_params.launcher_exec = MPL_strdup(mpiexec_params.launcher_exec);
    }

    if (mpiexec_params.launcher_exec == NULL) {
        if (MPL_env2str("HYDRA_BOOTSTRAP_EXEC", (const char **) &mpiexec_params.launcher_exec))
            mpiexec_params.launcher_exec = MPL_strdup(mpiexec_params.launcher_exec);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status push_env_downstream(struct mpiexec_pg *pg)
{
    struct HYD_env *env, *inherit;
    int env_count;
    char **env_args;
    int i;
    struct MPX_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* setup primary and secondary environment lists */
    mpiexec_params.primary.list = NULL;
    mpiexec_params.secondary.list = NULL;

    status = HYD_env_list_inherited(&inherit);
    HYD_ERR_POP(status, "unable to get the inherited env list\n");

    if (mpiexec_params.envprop == MPIEXEC_ENVPROP__UNSET) {
        mpiexec_params.secondary.list = inherit;
    }
    else if (mpiexec_params.envprop == MPIEXEC_ENVPROP__ALL) {
        mpiexec_params.primary.list = inherit;
    }
    else if (mpiexec_params.envprop == MPIEXEC_ENVPROP__NONE) {
        /* inherited env is completely ignored */
    }
    else if (mpiexec_params.envprop == MPIEXEC_ENVPROP__LIST) {
        /* pick out specific variables from the inherited env and drop
         * the rest */
        for (i = 0; i < mpiexec_params.envlist_count; i++) {
            for (env = inherit; env; env = env->next) {
                if (!strcmp(mpiexec_params.envlist[i], env->env_name)) {
                    status =
                        HYD_env_append_to_list(env->env_name, env->env_value,
                                               &mpiexec_params.primary.list);
                    HYD_ERR_POP(status, "error setting env\n");
                }
            }
            if (!env) {
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "cannot propagate env %s\n",
                                   mpiexec_params.envlist[i]);
            }
        }
    }

    /* Preset common environment options for disabling STDIO buffering
     * in Fortran */
    HYD_env_append_to_list("GFORTRAN_UNBUFFERED_PRECONNECTED", "y", &mpiexec_params.primary.list);

    env_count = 0;
    for (env = mpiexec_params.primary.list; env; env = env->next)
        env_count++;
    if (env_count) {
        HYD_MALLOC(env_args, char **, env_count * sizeof(char *), status);
        for (env = mpiexec_params.primary.list, i = 0; env; env = env->next, i++) {
            status = HYD_env_to_str(env, &env_args[i]);
            HYD_ERR_POP(status, "error converting env to str\n");
        }
        MPL_args_serialize(env_count, env_args, &mpiexec_params.primary.serialized_buf_len,
                           &mpiexec_params.primary.serialized_buf);
        for (i = 0; i < env_count; i++)
            MPL_free(env_args[i]);
        MPL_free(env_args);
    }

    env_count = 0;
    for (env = mpiexec_params.secondary.list; env; env = env->next)
        env_count++;
    if (env_count) {
        HYD_MALLOC(env_args, char **, env_count * sizeof(char *), status);
        for (env = mpiexec_params.secondary.list, i = 0; env; env = env->next, i++) {
            status = HYD_env_to_str(env, &env_args[i]);
            HYD_ERR_POP(status, "error converting env to str\n");
        }
        MPL_args_serialize(env_count, env_args, &mpiexec_params.secondary.serialized_buf_len,
                           &mpiexec_params.secondary.serialized_buf);
        for (i = 0; i < env_count; i++)
            MPL_free(env_args[i]);
        MPL_free(env_args);
    }

    MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
    cmd.type = MPX_CMD_TYPE__PRIMARY_ENV;
    cmd.data_len = mpiexec_params.primary.serialized_buf_len;

    status = cmd_bcast_root(cmd, pg, mpiexec_params.primary.serialized_buf);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

    cmd.type = MPX_CMD_TYPE__SECONDARY_ENV;
    cmd.data_len = mpiexec_params.secondary.serialized_buf_len;

    status = cmd_bcast_root(cmd, pg, mpiexec_params.secondary.serialized_buf);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status push_cwd_downstream(struct mpiexec_pg *pg)
{
    struct MPX_cmd cmd;
    char *cwd;
    HYD_status status = HYD_SUCCESS;

    /* send the cwd for the execs */
    MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
    cmd.type = MPX_CMD_TYPE__CWD;
    cwd = HYD_getcwd();
    cmd.data_len = strlen(cwd) + 1;

    status = cmd_bcast_root(cmd, pg, cwd);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

    MPL_free(cwd);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status push_exec_downstream(struct mpiexec_pg *pg)
{
    int count;
    struct HYD_exec *exec;
    struct MPX_cmd cmd;
    int node_id;
    HYD_status status = HYD_SUCCESS;

    /* send the actual execs */
    for (exec = pg->exec_list; exec; exec = exec->next) {
        int exec_arg_count;
        void *exec_serialized_buf;
        int exec_serialized_buf_len;

        for (exec_arg_count = 0; exec->exec[exec_arg_count]; exec_arg_count++);
        MPL_args_serialize(exec_arg_count, exec->exec, &exec_serialized_buf_len,
                           &exec_serialized_buf);

        MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
        cmd.type = MPX_CMD_TYPE__EXEC;
        cmd.data_len = exec_serialized_buf_len;
        cmd.u.exec.exec_proc_count = exec->proc_count;

        status = cmd_bcast_root(cmd, pg, exec_serialized_buf);
        HYD_ERR_POP(status, "error pushing generic command downstream\n");

        MPL_free(exec_serialized_buf);
    }

    /* update our active process information on each node */
    count = 0;
    node_id = 0;
    for (exec = pg->exec_list; exec;) {
        int available_cores;

        if (count == 0)
            count = exec->proc_count;

        available_cores =
            mpiexec_params.global_node_list[node_id].core_count -
            (mpiexec_params.global_active_processes[node_id] %
             mpiexec_params.global_node_list[node_id].core_count);

        if (count < available_cores) {
            mpiexec_params.global_active_processes[node_id] += count;
            count = 0;
        }
        else {
            mpiexec_params.global_active_processes[node_id] += available_cores;
            count -= mpiexec_params.global_node_list[node_id].core_count;
        }

        node_id++;
        if (node_id == mpiexec_params.global_node_count)
            node_id = 0;

        if (count == 0)
            exec = exec->next;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status push_mapping_info_downstream(struct mpiexec_pg *pg)
{
    struct MPX_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
    cmd.type = MPX_CMD_TYPE__PMI_PROCESS_MAPPING;
    cmd.data_len = strlen(pg->pmi_process_mapping) + 1;

    status = cmd_bcast_root(cmd, pg, pg->pmi_process_mapping);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status initiate_process_launch(struct mpiexec_pg *pg)
{
    struct MPX_cmd cmd;
    char *kvsname;
    HYD_status status = HYD_SUCCESS;

    HYD_MALLOC(kvsname, char *, PMI_MAXKVSLEN, status);
    MPL_snprintf(kvsname, PMI_MAXKVSLEN, "kvs_%d_0", (int) getpid());

    MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
    cmd.type = MPX_CMD_TYPE__KVSNAME;
    cmd.data_len = strlen(kvsname) + 1;

    status = cmd_bcast_root(cmd, pg, kvsname);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

    cmd.type = MPX_CMD_TYPE__LAUNCH_PROCESSES;
    cmd.data_len = 0;

    status = cmd_bcast_root(cmd, pg, NULL);
    HYD_ERR_POP(status, "error pushing generic command downstream\n");

    MPL_free(kvsname);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    struct MPX_cmd cmd;
    int recvd, closed;
    char *buf;
    struct mpiexec_pg *pg = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_sock_read(fd, &cmd, sizeof(cmd), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading command\n");

    /* if the downstream control is closed, it is done */
    if (closed) {
        HYD_dmx_deregister_fd(fd);
        close(fd);
        goto fn_exit;
    }

    if (cmd.type == MPX_CMD_TYPE__PMI_BARRIER_IN) {
        MPL_HASH_FIND_INT(mpiexec_pg_hash, &cmd.u.barrier_in.pgid, pg);

        status = mpiexec_pmi_barrier(pg);
        HYD_ERR_POP(status, "error handling PMI barrier\n");
    }
    else if (cmd.type == MPX_CMD_TYPE__KVCACHE_IN) {
        struct HYD_int_hash *hash;

        MPL_HASH_FIND_INT(mpiexec_pg_hash, &cmd.u.kvcache.pgid, pg);

        MPL_HASH_FIND_INT(pg->downstream.fd_control_hash, &fd, hash);
        HYD_ASSERT(hash->val < pg->num_downstream, status);

        HYD_ASSERT(pg->downstream.kvcache[hash->val] == NULL, status);
        HYD_ASSERT(pg->downstream.kvcache_size[hash->val] == 0, status);
        HYD_ASSERT(pg->downstream.kvcache_num_blocks[hash->val] == 0, status);

        pg->downstream.kvcache_num_blocks[hash->val] = cmd.u.kvcache.num_blocks;
        pg->downstream.kvcache_size[hash->val] = cmd.data_len;

        HYD_MALLOC(pg->downstream.kvcache[hash->val], char *,
                   pg->downstream.kvcache_size[hash->val], status);

        status =
            HYD_sock_read(fd, pg->downstream.kvcache[hash->val],
                          pg->downstream.kvcache_size[hash->val], &recvd, &closed,
                          HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading PMI command\n");
    }
    else if (cmd.type == MPX_CMD_TYPE__STDOUT) {
        HYD_MALLOC(buf, char *, cmd.data_len, status);
        status =
            HYD_sock_read(fd, buf, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        status =
            mpiexec_stdout_cb(cmd.u.stdoe.pgid, cmd.u.stdoe.proxy_id, cmd.u.stdoe.pmi_id, buf,
                              cmd.data_len);
        HYD_ERR_POP(status, "error calling stdout cb\n");

        MPL_free(buf);
    }
    else if (cmd.type == MPX_CMD_TYPE__STDERR) {
        HYD_MALLOC(buf, char *, cmd.data_len, status);
        status =
            HYD_sock_read(fd, buf, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        status =
            mpiexec_stderr_cb(cmd.u.stdoe.pgid, cmd.u.stdoe.proxy_id, cmd.u.stdoe.pmi_id, buf,
                              cmd.data_len);
        HYD_ERR_POP(status, "error calling stderr cb\n");

        MPL_free(buf);
    }
    else if (cmd.type == MPX_CMD_TYPE__PID) {
        int *proxy_pids;
        int n_proxy_pids;
        int *proxy_pmi_ids;
        int i;

        /* Find the proxy id of the proxy sending the data */
        n_proxy_pids = cmd.data_len / (2 * sizeof(int));

        /* Read the pid data from the socket */
        HYD_MALLOC(proxy_pids, int *, cmd.data_len / 2, status);
        status =
            HYD_sock_read(fd, proxy_pids, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);

        /* Read the pmi_id data from the socket */
        HYD_MALLOC(proxy_pmi_ids, int *, cmd.data_len / 2, status);
        status =
            HYD_sock_read(fd, proxy_pmi_ids, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);

        /* Move pid to the correct place in the pid array */
        for (i = 0; i < n_proxy_pids; i++) {
            contig_pids[proxy_pmi_ids[i]] = proxy_pids[i];
        }

        MPL_free(proxy_pids);
        MPL_free(proxy_pmi_ids);

        mpiexec_params.pid_ref_count++;

        MPL_HASH_FIND_INT(mpiexec_pg_hash, &cmd.u.pids.pgid, pg);

        /* If we have all of the pids, post the list to the MPIR_PROCDESC struct so the debugger can find it */
        if (mpiexec_params.pid_ref_count == pg->num_downstream) {
            HYD_dbg_setup_procdesc(pg->total_proc_count, pg->exec_list, contig_pids, pg->node_count, pg->node_list);

            MPL_free(contig_pids);
        }
    }
    else {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "received unknown cmd %d\n", cmd.type);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status compute_pmi_process_mapping(struct mpiexec_pg *pg)
{
    int sid, nn, cc, i;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("(vector"), status);

    sid = 0;
    nn = 0;
    cc = 0;
    for (i = 0; i < pg->node_count; i++) {
        if (nn == 0) {
            nn++;
            cc = pg->node_list[i].core_count;
            continue;
        }

        if (cc == pg->node_list[i].core_count)
            nn++;
        else {
            /* stash this set and move forward */
            HYD_STRING_STASH(stash, MPL_strdup(",("), status);
            HYD_STRING_STASH(stash, HYD_str_from_int(sid), status);
            HYD_STRING_STASH(stash, MPL_strdup(","), status);
            HYD_STRING_STASH(stash, HYD_str_from_int(nn), status);
            HYD_STRING_STASH(stash, MPL_strdup(","), status);
            HYD_STRING_STASH(stash, HYD_str_from_int(cc), status);
            HYD_STRING_STASH(stash, MPL_strdup(")"), status);

            sid = i;
            nn = 1;
            cc = pg->node_list[i].core_count;
        }
    }

    HYD_STRING_STASH(stash, MPL_strdup(",("), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(sid), status);
    HYD_STRING_STASH(stash, MPL_strdup(","), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(nn), status);
    HYD_STRING_STASH(stash, MPL_strdup(","), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(cc), status);
    HYD_STRING_STASH(stash, MPL_strdup("))"), status);

    HYD_STRING_SPIT(stash, pg->pmi_process_mapping, status);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

#define MAX_CMD_ARGS (64)

int main(int argc, char **argv)
{
    struct HYD_exec *exec;
    int exit_status = 0, i;
    struct mpiexec_pg *pg;
    char *args[MAX_CMD_ARGS];
    int pgid = 0, core_count;
    struct HYD_int_hash *hash, *thash;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_print_set_prefix_str("mpiexec");
    HYD_ERR_POP(status, "unable to set dbg prefix\n");

    status = HYD_signal_set_common(signal_cb);
    HYD_ERR_POP(status, "unable to set signal\n");

    /* parse our argv to see what the user explicitly set */
    status = mpiexec_get_parameters(argv);
    HYD_ERR_POP(status, "error parsing parameters\n");

    MPL_env2int("MPIEXEC_TIMEOUT", &mpiexec_params.timeout);

    if (MPL_env2str("MPIEXEC_PORTRANGE", (const char **) &mpiexec_params.port_range) ||
        MPL_env2str("MPIEXEC_PORT_RANGE", (const char **) &mpiexec_params.port_range))
        mpiexec_params.port_range = MPL_strdup(mpiexec_params.port_range);

    if (mpiexec_params.debug == -1 && MPL_env2bool("HYDRA_DEBUG", &mpiexec_params.debug) == 0)
        mpiexec_params.debug = 0;

    status = get_node_list();
    HYD_ERR_POP(status, "unable to find an RMK and the node list\n");

    status = find_launcher();
    HYD_ERR_POP(status, "unable to find a valid launcher\n");

    MPL_HASH_FIND_INT(mpiexec_pg_hash, &pgid, pg);

    pg->total_proc_count = 0;
    for (exec = pg->exec_list; exec; exec = exec->next) {
        if (exec->proc_count == -1)
            exec->proc_count = mpiexec_params.global_core_count;
        pg->total_proc_count += exec->proc_count;
    }

    HYD_MALLOC(contig_pids, int *, pg->total_proc_count, status);

    if (mpiexec_params.usize == MPIEXEC_USIZE__SYSTEM)
        mpiexec_params.usize = mpiexec_params.global_core_count;
    else if (mpiexec_params.usize == MPIEXEC_USIZE__INFINITE)
        mpiexec_params.usize = -1;

    /* figure out how many global_node_list we need to cover the total processes
     * required */
    pg->node_count = 0;
    core_count = 0;
    for (i = 0; i < mpiexec_params.global_node_count; i++) {
        pg->node_count++;
        core_count += mpiexec_params.global_node_list[i].core_count;
        if (core_count >= pg->total_proc_count)
            break;
    }

    HYD_MALLOC(pg->node_list, struct HYD_node *, pg->node_count * sizeof(struct HYD_node), status);
    memcpy(pg->node_list, mpiexec_params.global_node_list,
           pg->node_count * sizeof(struct HYD_node));

    status = compute_pmi_process_mapping(pg);
    HYD_ERR_POP(status, "error computing PMI process mapping\n");

    /* signal_pipe[0] is for sending signals to self; we add signal_pipe[1]
     * to the demux engine so we can break out of the demux wait when
     * there's a signal */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, mpiexec_params.signal_pipe) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "pipe error\n");

    status = HYD_dmx_register_fd(mpiexec_params.signal_pipe[1], HYD_DMX_POLLIN, NULL, cleanup_cb);
    HYD_ERR_POP(status, "error registering signal_pipe\n");

    /* let's see what we can pass to the proxy as command-line arguments */
    i = 0;
    if (getenv("HYDRA_BSTRAP_XTERM")) {
        args[i++] = MPL_strdup("xterm");
        args[i++] = MPL_strdup("-e");
        args[i++] = MPL_strdup("gdb");
        args[i++] = MPL_strdup("--args");
    }
    if (getenv("HYDRA_BSTRAP_VALGRIND")) {
        args[i++] = MPL_strdup("valgrind");
        args[i++] = MPL_strdup("--track-origins=yes");
        args[i++] = MPL_strdup("--leak-check=full");
    }
    {
        char *tmp[HYD_NUM_TMP_STRINGS] = { NULL };
        int j;

        j = 0;
        tmp[j++] = MPL_strdup(mpiexec_params.base_path);
        tmp[j++] = MPL_strdup("/");
        tmp[j++] = MPL_strdup(HYDRA_PMI_PROXY);
        tmp[j++] = NULL;

        status = HYD_str_alloc_and_join(tmp, &args[i]);
        HYD_ERR_POP(status, "unable to join strings\n");
        HYD_str_free_list(tmp);

        i++;
    }
    args[i++] = MPL_strdup("--usize");
    args[i++] = HYD_str_from_int(mpiexec_params.usize);
    args[i++] = NULL;

    status =
        HYD_bstrap_setup(mpiexec_params.base_path, mpiexec_params.launcher,
                         mpiexec_params.launcher_exec, pg->node_count, pg->node_list, -1,
                         mpiexec_params.port_range, args, 0, &pg->num_downstream,
                         &pg->downstream.fd_stdin, &pg->downstream.fd_stdout_hash,
                         &pg->downstream.fd_stderr_hash, &pg->downstream.fd_control_hash,
                         &pg->downstream.proxy_id, &pg->downstream.pid, mpiexec_params.debug,
                         mpiexec_params.tree_width);
    HYD_ERR_POP(status, "error setting up the boostrap proxies\n");

    HYD_str_free_list(args);

    HYD_MALLOC(pg->downstream.kvcache, void **, pg->num_downstream * sizeof(void *), status);
    HYD_MALLOC(pg->downstream.kvcache_size, int *, pg->num_downstream * sizeof(int), status);
    HYD_MALLOC(pg->downstream.kvcache_num_blocks, int *, pg->num_downstream * sizeof(int), status);
    for (i = 0; i < pg->num_downstream; i++) {
        pg->downstream.kvcache[i] = NULL;
        pg->downstream.kvcache_size[i] = 0;
        pg->downstream.kvcache_num_blocks[i] = 0;
    }

    status = push_env_downstream(pg);
    HYD_ERR_POP(status, "error setting up the env propagation\n");

    status = push_cwd_downstream(pg);
    HYD_ERR_POP(status, "error setting up the cwd propagation\n");

    status = push_exec_downstream(pg);
    HYD_ERR_POP(status, "error setting up the exec propagation\n");

    status = push_mapping_info_downstream(pg);
    HYD_ERR_POP(status, "error setting up the pmi process mapping propagation\n");

    status = initiate_process_launch(pg);
    HYD_ERR_POP(status, "error setting up the pmi_id propagation\n");

    MPL_HASH_ITER(hh, pg->downstream.fd_control_hash, hash, thash) {
        status = HYD_dmx_register_fd(hash->key, HYD_DMX_POLLIN, NULL, control_cb);
        HYD_ERR_POP(status, "error registering control fd\n");
    }

    MPL_HASH_ITER(hh, pg->downstream.fd_stdout_hash, hash, thash) {
        status = HYD_dmx_splice(hash->key, STDOUT_FILENO);
        HYD_ERR_POP(status, "error splicing stdout fd\n");
    }

    MPL_HASH_ITER(hh, pg->downstream.fd_stderr_hash, hash, thash) {
        status = HYD_dmx_splice(hash->key, STDERR_FILENO);
        HYD_ERR_POP(status, "error splicing stderr fd\n");
    }

    status = HYD_dmx_splice(STDIN_FILENO, pg->downstream.fd_stdin);
    HYD_ERR_POP(status, "error splicing stdin fd\n");

    /* wait for downstream processes to terminate */
    MPL_HASH_ITER(hh, pg->downstream.fd_control_hash, hash, thash) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
    }

    MPL_HASH_ITER(hh, pg->downstream.fd_stdout_hash, hash, thash) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
    }

    MPL_HASH_ITER(hh, pg->downstream.fd_stderr_hash, hash, thash) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
    }

    for (i = 0; i < pg->num_downstream; i++) {
        int ret;

        waitpid(pg->downstream.pid[i], &ret, 0);

        if (ret) {
            if (WIFEXITED(ret)) {
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "downstream exited with status %d\n",
                                   WEXITSTATUS(ret));
            }
            else if (WIFSIGNALED(ret)) {
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL,
                                   "downstream was killed by signal %d (%s)\n", WTERMSIG(ret),
                                   strsignal(WTERMSIG(ret)));
            }
        }
    }


    /* cleanup memory allocations to keep valgrind happy */
    status = HYD_bstrap_finalize(mpiexec_params.launcher);
    HYD_ERR_POP(status, "error finalizing bstrap\n");

    status = HYD_dmx_deregister_fd(mpiexec_params.signal_pipe[1]);
    HYD_ERR_POP(status, "error deregistering fd\n");
    close(mpiexec_params.signal_pipe[0]);
    close(mpiexec_params.signal_pipe[1]);

    MPL_HASH_ITER(hh, pg->downstream.fd_stdout_hash, hash, thash) {
        status = HYD_dmx_unsplice(hash->key);
        HYD_ERR_POP(status, "error deregistering fd\n");

        MPL_HASH_DEL(pg->downstream.fd_stdout_hash, hash);
        MPL_free(hash);
    }

    MPL_HASH_ITER(hh, pg->downstream.fd_stderr_hash, hash, thash) {
        status = HYD_dmx_unsplice(hash->key);
        HYD_ERR_POP(status, "error deregistering fd\n");

        MPL_HASH_DEL(pg->downstream.fd_stderr_hash, hash);
        MPL_free(hash);
    }

    status = HYD_dmx_unsplice(STDIN_FILENO);
    HYD_ERR_POP(status, "error deregistering fd\n");

    MPL_HASH_DEL(mpiexec_pg_hash, pg);

    if (pg->node_list)
        MPL_free(pg->node_list);
    if (pg->exec_list)
        HYD_exec_free_list(pg->exec_list);

    if (pg->downstream.proxy_id)
        MPL_free(pg->downstream.proxy_id);
    if (pg->downstream.pid)
        MPL_free(pg->downstream.pid);

    MPL_HASH_ITER(hh, pg->downstream.fd_control_hash, hash, thash) {
        MPL_HASH_DEL(pg->downstream.fd_control_hash, hash);
        MPL_free(hash);
    }

    for (i = 0; i < pg->num_downstream; i++)
        if (pg->downstream.kvcache[i])
            MPL_free(pg->downstream.kvcache[i]);
    if (pg->downstream.kvcache)
        MPL_free(pg->downstream.kvcache);
    if (pg->downstream.kvcache_size)
        MPL_free(pg->downstream.kvcache_size);
    if (pg->downstream.kvcache_num_blocks)
        MPL_free(pg->downstream.kvcache_num_blocks);

    if (pg->pmi_process_mapping)
        MPL_free(pg->pmi_process_mapping);

    MPL_free(pg);

    if (mpiexec_params.rmk)
        MPL_free(mpiexec_params.rmk);
    if (mpiexec_params.launcher)
        MPL_free(mpiexec_params.launcher);
    if (mpiexec_params.launcher_exec)
        MPL_free(mpiexec_params.launcher_exec);
    if (mpiexec_params.binding)
        MPL_free(mpiexec_params.binding);
    if (mpiexec_params.mapping)
        MPL_free(mpiexec_params.mapping);
    if (mpiexec_params.membind)
        MPL_free(mpiexec_params.membind);
    if (mpiexec_params.base_path)
        MPL_free(mpiexec_params.base_path);
    if (mpiexec_params.port_range)
        MPL_free(mpiexec_params.port_range);
    if (mpiexec_params.nameserver)
        MPL_free(mpiexec_params.nameserver);
    if (mpiexec_params.localhost)
        MPL_free(mpiexec_params.localhost);

    if (mpiexec_params.global_node_list)
        MPL_free(mpiexec_params.global_node_list);
    if (mpiexec_params.global_active_processes)
        MPL_free(mpiexec_params.global_active_processes);

    for (i = 0; i < mpiexec_params.envlist_count; i++)
        MPL_free(mpiexec_params.envlist[i]);
    if (mpiexec_params.envlist)
        MPL_free(mpiexec_params.envlist);

    if (mpiexec_params.primary.list)
        HYD_env_free_list(mpiexec_params.primary.list);
    if (mpiexec_params.primary.serialized_buf)
        MPL_free(mpiexec_params.primary.serialized_buf);

    if (mpiexec_params.secondary.list)
        HYD_env_free_list(mpiexec_params.secondary.list);
    if (mpiexec_params.secondary.serialized_buf)
        MPL_free(mpiexec_params.secondary.serialized_buf);

    if (mpiexec_params.prepend_pattern)
        MPL_free(mpiexec_params.prepend_pattern);
    if (mpiexec_params.outfile_pattern)
        MPL_free(mpiexec_params.outfile_pattern);
    if (mpiexec_params.errfile_pattern)
        MPL_free(mpiexec_params.errfile_pattern);

  fn_exit:
    HYD_FUNC_EXIT();
    if (status != HYD_SUCCESS)
        return -1;
    else
        return exit_status;

  fn_fail:
    goto fn_exit;
}

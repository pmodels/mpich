/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "proxy.h"

static HYD_status cmd_bcast_non_root(int fd, struct MPX_cmd cmd, void **data)
{
    struct HYD_int_hash *hash, *tmp;
    int sent, recvd, closed;
    void *buf = NULL;
    HYD_status status = HYD_SUCCESS;

    if (cmd.data_len) {
        HYD_MALLOC(buf, void *, cmd.data_len, status);
        status =
            HYD_sock_read(fd, buf, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading command from launcher\n");
        HYD_ASSERT(!closed, status);
    }

    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.control_fd_hash, hash, tmp) {
        status =
            HYD_sock_write(hash->key, &cmd, sizeof(cmd), &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading command\n");
        HYD_ASSERT(!closed, status);

        if (cmd.data_len) {
            status =
                HYD_sock_write(hash->key, buf, cmd.data_len, &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error reading command\n");
            HYD_ASSERT(!closed, status);
        }
    }

    if (cmd.data_len)
        *data = buf;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_upstream_control_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    struct MPX_cmd cmd;
    int sent, recvd, closed;
    struct HYD_exec *exec;
    int i;
    struct HYD_int_hash *hash, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_sock_read(fd, &cmd, sizeof(cmd), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading command\n");
    HYD_ASSERT(!closed, status);

    if (cmd.type == MPX_CMD_TYPE__PRIMARY_ENV) {
        status = cmd_bcast_non_root(fd, cmd, &proxy_params.all.primary_env.serial_buf);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        proxy_params.all.primary_env.serial_buf_len = cmd.data_len;

        if (cmd.data_len) {
            MPL_args_deserialize(cmd.data_len, proxy_params.all.primary_env.serial_buf,
                                 &proxy_params.all.primary_env.argc,
                                 &proxy_params.all.primary_env.argv);
            for (i = 0; i < proxy_params.all.primary_env.argc; i++)
                putenv(proxy_params.all.primary_env.argv[i]);
        }
    }
    else if (cmd.type == MPX_CMD_TYPE__SECONDARY_ENV) {
        status = cmd_bcast_non_root(fd, cmd, &proxy_params.all.secondary_env.serial_buf);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        proxy_params.all.secondary_env.serial_buf_len = cmd.data_len;

        if (cmd.data_len) {
            MPL_args_deserialize(cmd.data_len, proxy_params.all.secondary_env.serial_buf,
                                 &proxy_params.all.secondary_env.argc,
                                 &proxy_params.all.secondary_env.argv);
            for (i = 0; i < proxy_params.all.secondary_env.argc; i++)
                putenv(proxy_params.all.secondary_env.argv[i]);
        }
    }
    else if (cmd.type == MPX_CMD_TYPE__PMI_PROCESS_MAPPING) {
        status = cmd_bcast_non_root(fd, cmd, (void **) &proxy_params.all.pmi_process_mapping);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");
    }
    else if (cmd.type == MPX_CMD_TYPE__CWD) {
        status = cmd_bcast_non_root(fd, cmd, (void **) &proxy_params.cwd);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        if (chdir(proxy_params.cwd) < 0) {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unable to chdir to %s\n",
                               proxy_params.cwd);
        }
    }
    else if (cmd.type == MPX_CMD_TYPE__EXEC) {
        void *exec_serialized_buf;
        char **args;
        int num_args;

        status = cmd_bcast_non_root(fd, cmd, &exec_serialized_buf);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        HYD_MALLOC(exec, struct HYD_exec *, sizeof(struct HYD_exec), status);
        exec->proc_count = cmd.u.exec.exec_proc_count;
        proxy_params.all.global_process_count += exec->proc_count;
        MPL_args_deserialize(cmd.data_len, exec_serialized_buf, &num_args, &args);
        for (i = 0; i < num_args; i++)
            exec->exec[i] = MPL_strdup(args[i]);
        exec->exec[i] = NULL;
        exec->next = NULL;

        if (proxy_params.all.complete_exec_list == NULL)
            proxy_params.all.complete_exec_list = exec;
        else {
            struct HYD_exec *e;

            for (e = proxy_params.all.complete_exec_list; e->next; e = e->next);
            e->next = exec;
        }

        for (i = 0; i < num_args; i++)
            MPL_free(args[i]);
        MPL_free(args);
        MPL_free(exec_serialized_buf);
    }
    else if (cmd.type == MPX_CMD_TYPE__KVSNAME) {
        status = cmd_bcast_non_root(fd, cmd, (void **) &proxy_params.all.kvsname);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");
    }
    else if (cmd.type == MPX_CMD_TYPE__SIGNAL) {
        status = cmd_bcast_non_root(fd, cmd, NULL);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        /* send signal to all processes */
        MPL_HASH_ITER(hh, proxy_params.immediate.process.pid_hash, hash, tmp) {
            if (kill(hash->key, cmd.u.signal.signum) < 0) {
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "error sending signal %d to process\n",
                                   cmd.u.signal.signum);
            }
        }
    }
    else if (cmd.type == MPX_CMD_TYPE__LAUNCH_PROCESSES) {
        HYD_ASSERT(cmd.data_len == 0, status);

        status = cmd_bcast_non_root(fd, cmd, NULL);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        proxy_ready_to_launch = 1;
    }
    else if (cmd.type == MPX_CMD_TYPE__KVCACHE_OUT) {
        char *buf;

        status = cmd_bcast_non_root(fd, cmd, (void **) &buf);
        HYD_ERR_POP(status, "error forwarding cmd downstream\n");

        status =
            proxy_pmi_kvcache_out(cmd.u.kvcache.num_blocks, (int *) buf,
                                  (char *) (buf + 2 * cmd.u.kvcache.num_blocks * sizeof(int)),
                                  cmd.data_len - 2 * cmd.u.kvcache.num_blocks * sizeof(int));
        HYD_ERR_POP(status, "error inserting keys into kvcache\n");

        MPL_free(buf);
    }
    else if (cmd.type == MPX_CMD_TYPE__PMI_BARRIER_OUT) {
        status = proxy_barrier_out(-1, NULL);
        HYD_ERR_POP(status, "error calling barrier_out\n");

        /* forward the command downstream */
        MPL_HASH_ITER(hh, proxy_params.immediate.proxy.control_fd_hash, hash, tmp) {
            status =
                HYD_sock_write(hash->key, &cmd, sizeof(cmd), &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error writing command\n");
            HYD_ASSERT(!closed, status);
        }
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_downstream_control_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    struct MPX_cmd cmd;
    int sent, recvd, closed;
    void *buf;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* we do not really interpret downstream commands, but simply pass
     * them on upstream.  this is very similar to splicing, but is
     * more structured: we first read the header to understand how
     * much data is coming in, read all of that data, and pass on all
     * of that data upstream. */

    status = HYD_sock_read(fd, &cmd, sizeof(cmd), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading command\n");

    /* if the downstream control is closed, it is done */
    if (closed) {
        HYD_dmx_deregister_fd(fd);
        close(fd);
        goto fn_exit;
    }

    switch (cmd.type) {
    case MPX_CMD_TYPE__STDOUT:
    case MPX_CMD_TYPE__STDERR:
        {
            /* read the actual data */
            HYD_MALLOC(buf, void *, cmd.data_len, status);
            status =
                HYD_sock_read(fd, buf, cmd.data_len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error reading data\n");
            HYD_ASSERT(!closed, status);

            /* we have the command and data; send it upstream */
            status =
                HYD_sock_write(proxy_params.root.upstream_fd, &cmd, sizeof(cmd), &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error writing command\n");
            HYD_ASSERT(!closed, status);

            status =
                HYD_sock_write(proxy_params.root.upstream_fd, buf, cmd.data_len, &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error writing data\n");
            HYD_ASSERT(!closed, status);

            MPL_free(buf);

            break;
        }

    case MPX_CMD_TYPE__KVCACHE_IN:
        {
            struct HYD_int_hash *hash;

            /* simply stash the downstream kvcache.  when we get all our
             * barrier_in commands, we will repackage all of these caches
             * and flush them upstream. */

            MPL_HASH_FIND_INT(proxy_params.immediate.proxy.control_fd_hash, &fd, hash);

            proxy_params.immediate.proxy.kvcache_size[hash->val] = cmd.data_len;
            proxy_params.immediate.proxy.kvcache_num_blocks[hash->val] = cmd.u.kvcache.num_blocks;

            HYD_MALLOC(proxy_params.immediate.proxy.kvcache[hash->val], void *,
                       proxy_params.immediate.proxy.kvcache_size[hash->val], status);
            status =
                HYD_sock_read(fd, proxy_params.immediate.proxy.kvcache[hash->val],
                              proxy_params.immediate.proxy.kvcache_size[hash->val], &recvd, &closed,
                              HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error reading kvcache from downstream\n");

            break;
        }

    case MPX_CMD_TYPE__PMI_BARRIER_IN:
        {
            status = proxy_barrier_in(-1, NULL);
            HYD_ERR_POP(status, "error calling pmip_barrier\n");

            break;
        }
    case MPX_CMD_TYPE__PID:
        {
            int rel_proxy_id;

            /* Find the proxy id of the proxy sending the data */
            rel_proxy_id = cmd.u.pids.proxy_id - proxy_params.root.proxy_id;
            n_proxy_pids[rel_proxy_id] = cmd.data_len / sizeof(int);

            /* Read the pid data from the socket */
            HYD_MALLOC(proxy_pids[rel_proxy_id], int *, cmd.data_len / 2, status);
            status =
                HYD_sock_read(fd, proxy_pids[rel_proxy_id], cmd.data_len / 2, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);

            /* Read the pmi_id data from the socket */
            HYD_MALLOC(proxy_pmi_ids[rel_proxy_id], int *, cmd.data_len / 2, status);
            status =
                HYD_sock_read(fd, proxy_pmi_ids[rel_proxy_id], cmd.data_len / 2, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);

            /* Call the function to stitch it all together */
            proxy_send_pids_upstream();
        }

    default:
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "received unknown cmd %d\n", cmd.type);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

#define STDOE_BUF_SIZE  (16384)

static HYD_status stdoe_cb(int type, int fd, HYD_dmx_event_t events, void *userp)
{
    struct MPX_cmd cmd;
    int sent, recvd, closed;
    char buf[STDOE_BUF_SIZE];
    struct HYD_int_hash *hash;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* read as much data as we can */
    status =
        HYD_sock_read(fd, buf, STDOE_BUF_SIZE, &recvd, &closed, HYD_SOCK_COMM_TYPE__NONBLOCKING);
    HYD_ERR_POP(status, "error reading data\n");

    /* if the socket closed, deregister and continue */
    if (closed) {
        HYD_dmx_deregister_fd(fd);
        close(fd);
        goto fn_exit;
    }

    /* create a command to wrap the data and push it up */
    cmd.type = type;
    cmd.data_len = recvd;
    cmd.u.stdoe.pgid = proxy_params.all.pgid;
    cmd.u.stdoe.proxy_id = proxy_params.root.proxy_id;
    if (type == MPX_CMD_TYPE__STDOUT)
        MPL_HASH_FIND_INT(proxy_params.immediate.process.stdout_fd_hash, &fd, hash);
    else
        MPL_HASH_FIND_INT(proxy_params.immediate.process.stderr_fd_hash, &fd, hash);
    HYD_ASSERT(hash, status);
    cmd.u.stdoe.pmi_id = proxy_params.immediate.process.pmi_id[hash->val];

    /* we have the command and data; send it upstream */
    status =
        HYD_sock_write(proxy_params.root.upstream_fd, &cmd, sizeof(cmd), &sent, &closed,
                       HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error writing command\n");
    HYD_ASSERT(!closed, status);

    status =
        HYD_sock_write(proxy_params.root.upstream_fd, buf, recvd, &sent, &closed,
                       HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error writing data\n");
    HYD_ASSERT(!closed, status);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_process_stdout_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = stdoe_cb(MPX_CMD_TYPE__STDOUT, fd, events, userp);
    HYD_ERR_POP(status, "error calling stdoe_cb\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_process_stderr_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = stdoe_cb(MPX_CMD_TYPE__STDERR, fd, events, userp);
    HYD_ERR_POP(status, "error calling stdoe_cb\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_send_pids_upstream()
{
    HYD_status status = HYD_SUCCESS;
    struct MPX_cmd cmd;
    int sent, num_pids = 0, i, next_pid = 0, closed;
    int *contig_data;

    HYD_FUNC_ENTER();

    proxy_params.root.pid_ref_count++;

    if (proxy_params.root.pid_ref_count == proxy_params.immediate.proxy.num_children) {
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            num_pids += n_proxy_pids[i];
        }

        /* Move pids to contiguous array */
        HYD_MALLOC(contig_data, int *, 2 * num_pids * sizeof(int), status);
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            memcpy(&contig_data[next_pid], proxy_pids[i], n_proxy_pids[i] * sizeof(int));
            memcpy(&contig_data[num_pids+next_pid], proxy_pmi_ids[i], n_proxy_pids[i] * sizeof(int));
            next_pid += n_proxy_pids[i];
        }
        MPL_free(proxy_pids);

        /* Send the data to the parent */
        cmd.type = MPX_CMD_TYPE__PID;
        cmd.data_len = num_pids * sizeof(int) * 2;
        cmd.u.pids.proxy_id = proxy_params.root.proxy_id;
        cmd.u.pids.pgid = proxy_params.all.pgid;

        status =
            HYD_sock_write(proxy_params.root.upstream_fd, &cmd, sizeof(cmd), &sent, &closed,
                                HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error writing command\n");
        HYD_ASSERT(!closed, status);

        status =
            HYD_sock_write(proxy_params.root.upstream_fd, contig_data, cmd.data_len, &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error writing data\n");
        HYD_ASSERT(!closed, status);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

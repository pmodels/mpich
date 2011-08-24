/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

static HYD_status cmd_response(int fd, int pid, const char *cmd)
{
    struct HYD_pmcd_hdr hdr;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PMI_RESPONSE;
    hdr.pid = pid;
    hdr.pmi_version = 1;
    hdr.buflen = strlen(cmd);
    status = HYDU_sock_write(fd, &hdr, sizeof(hdr), &sent, &closed);
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE header to proxy\n");
    HYDU_ASSERT(!closed, status);

    if (HYD_server_info.user_global.debug) {
        HYDU_dump(stdout, "PMI response to fd %d pid %d: %s", fd, pid, cmd);
    }

    status = HYDU_sock_write(fd, cmd, strlen(cmd), &sent, &closed);
    HYDU_ERR_POP(status, "unable to send response to command\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_in(int fd, int pid, int pgid, char *args[])
{
    struct HYD_proxy *proxy, *tproxy;
    const char *cmd;
    int proxy_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    proxy_count = 0;
    for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next)
        proxy_count++;

    proxy->pg->barrier_count++;
    if (proxy->pg->barrier_count == proxy_count) {
        proxy->pg->barrier_count = 0;
        cmd = "cmd=barrier_out\n";

        for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
            status = cmd_response(tproxy->control_fd, pid, cmd);
            HYDU_ERR_POP(status, "error writing PMI line\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_put(int fd, int pid, int pgid, char *args[])
{
    int i, ret;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *kvsname, *key, *val;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    kvsname = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "kvsname");
    HYDU_ERR_CHKANDJUMP(status, kvsname == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: kvsname\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: key\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "value");
    if (val == NULL) {
        /* the user sent an empty string */
        val = HYDU_strdup("");
    }

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    if (strcmp(pg_scratch->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "kvsname (%s) does not match this group's kvs space (%s)\n",
                            kvsname, pg_scratch->kvs->kvs_name);

    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, &ret);
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

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, int pid, int pgid, char *args[])
{
    int i;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    char *kvsname, *key, *val;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    kvsname = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "kvsname");
    HYDU_ERR_CHKANDJUMP(status, kvsname == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: kvsname\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: key\n");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    val = NULL;
    if (!strcmp(key, "PMI_dead_processes")) {
        val = pg_scratch->dead_processes;
        goto found_val;
    }

    if (strcmp(pg_scratch->kvs->kvs_name, kvsname))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "kvsname (%s) does not match this group's kvs space (%s)\n",
                            kvsname, pg_scratch->kvs->kvs_name);

    /* Try to find the key */
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            val = run->val;
            break;
        }
    }

  found_val:
    i = 0;
    tmp[i++] = HYDU_strdup("cmd=get_result rc=");
    if (val) {
        tmp[i++] = HYDU_strdup("0 msg=success value=");
        tmp[i++] = HYDU_strdup(val);
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

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static char *mcmd_args[HYD_NUM_TMP_STRINGS] = { NULL };

static int mcmd_num_args = 0;

static void segment_tokens(struct HYD_pmcd_token *tokens, int token_count,
                           struct HYD_pmcd_token_segment *segment_list, int *num_segments)
{
    int i, j;

    j = 0;
    segment_list[j].start_idx = 0;
    segment_list[j].token_count = 0;
    for (i = 0; i < token_count; i++) {
        if (!strcmp(tokens[i].key, "endcmd") && (i < token_count - 1)) {
            j++;
            segment_list[j].start_idx = i + 1;
            segment_list[j].token_count = 0;
        }
        else {
            segment_list[j].token_count++;
        }
    }
    *num_segments = j + 1;
}

static HYD_status fn_spawn(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_token *tokens;
    struct HYD_exec *exec_list = NULL, *exec;
    struct HYD_env *env;
    struct HYD_node *node;

    char key[PMI_MAXKEYLEN], *val;
    int nprocs, preput_num, info_num, ret;
    char *execname, *path = NULL;

    struct HYD_pmcd_token_segment *segment_list = NULL;

    int token_count, i, j, k, new_pgid, total_spawns;
    int argcnt, num_segments;
    char *control_port, *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL };
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; args[i]; i++)
        mcmd_args[mcmd_num_args++] = HYDU_strdup(args[i]);
    mcmd_args[mcmd_num_args] = NULL;

    status = HYD_pmcd_pmi_args_to_tokens(mcmd_args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");


    /* Here's the order of things we do:
     *
     *   1. Break the token list into multiple segments, each segment
     *      corresponding to a command. Each command represents
     *      information for one executable.
     *
     *   2. Allocate a process group for the new set of spawned
     *      processes
     *
     *   3. Get all the common keys and deal with them
     *
     *   4. Create an executable list based on the segments.
     *
     *   5. Create a proxy list using the created executable list and
     *      spawn it.
     */

    /* Break the token list into multiple segments and create an
     * executable list based on the segments. */
    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "totspawns");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: totspawns\n");
    total_spawns = atoi(val);

    HYDU_MALLOC(segment_list, struct HYD_pmcd_token_segment *,
                total_spawns * sizeof(struct HYD_pmcd_token_segment), status);

    segment_tokens(tokens, token_count, segment_list, &num_segments);

    if (num_segments != total_spawns) {
        /* We didn't read the entire PMI string; wait for the rest to
         * arrive */
        goto fn_exit;
    }
    else {
        /* Got the entire PMI string; free the arguments and reset */
        HYDU_free_strlist(mcmd_args);
        mcmd_num_args = 0;
    }

    /* Allocate a new process group */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);
    new_pgid = pg->pgid + 1;

    status = HYDU_alloc_pg(&pg->next, new_pgid);
    HYDU_ERR_POP(status, "unable to allocate process group\n");

    pg = pg->next;

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg->spawner_pg = proxy->pg;

    for (j = 0; j < total_spawns; j++) {
        /* For each segment, we create an exec structure */
        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "nprocs");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: nprocs\n");
        nprocs = atoi(val);
        pg->pg_process_count += nprocs;

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "argcnt");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: argcnt\n");
        argcnt = atoi(val);

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "info_num");
        if (val)
            info_num = atoi(val);
        else
            info_num = 0;

        if (exec_list == NULL) {
            status = HYDU_alloc_exec(&exec_list);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec_list->appnum = 0;
            exec = exec_list;
        }
        else {
            for (exec = exec_list; exec->next; exec = exec->next);
            status = HYDU_alloc_exec(&exec->next);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec->next->appnum = exec->appnum + 1;
            exec = exec->next;
        }

        /* Info keys */
        for (i = 0; i < info_num; i++) {
            char *info_key, *info_val;

            HYDU_snprintf(key, PMI_MAXKEYLEN, "info_key_%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_key = val;

            HYDU_snprintf(key, PMI_MAXKEYLEN, "info_val_%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_val = val;

            if (!strcmp(info_key, "path")) {
                path = HYDU_strdup(info_val);
            }
            else if (!strcmp(info_key, "wdir")) {
                exec->wdir = HYDU_strdup(info_val);
            }
            else if (!strcmp(info_key, "host")) {
                status = HYDU_process_mfile_token(info_val, 1, &pg->user_node_list);
                HYDU_ERR_POP(status, "error create node list\n");
            }
            else if (!strcmp(info_key, "hostfile")) {
                status =
                    HYDU_parse_hostfile(info_val, &pg->user_node_list,
                                        HYDU_process_mfile_token);
                HYDU_ERR_POP(status, "error parsing hostfile\n");
            }
            else {
                /* Unrecognized info key; ignore */
            }
        }

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "execname");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: execname\n");
        if (path == NULL)
            execname = HYDU_strdup(val);
        else {
            i = 0;
            tmp[i++] = HYDU_strdup(path);
            tmp[i++] = HYDU_strdup("/");
            tmp[i++] = HYDU_strdup(val);
            tmp[i++] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &execname);
            HYDU_ERR_POP(status, "error while joining strings\n");
            HYDU_free_strlist(tmp);
        }

        i = 0;
        exec->exec[i++] = execname;
        for (k = 0; k < argcnt; k++) {
            HYDU_snprintf(key, PMI_MAXKEYLEN, "arg%d", k + 1);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            exec->exec[i++] = HYDU_strdup(val);
        }
        exec->exec[i++] = NULL;

        exec->proc_count = nprocs;

        /* It is not clear what kind of environment needs to get
         * passed to the spawned process. Don't set anything here, and
         * let the proxy do whatever it does by default. */
        exec->env_prop = NULL;

        status = HYDU_env_create(&env, "PMI_SPAWNED", "1");
        HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
        exec->user_env = env;
    }

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg->pg_process_count = 0;
    for (exec = exec_list; exec; exec = exec->next)
        pg->pg_process_count += exec->proc_count;

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* Get the common keys and deal with them */
    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "preput_num");
    if (val)
        preput_num = atoi(val);
    else
        preput_num = 0;

    for (i = 0; i < preput_num; i++) {
        char *preput_key, *preput_val;

        HYDU_snprintf(key, PMI_MAXKEYLEN, "preput_key_%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_key = val;

        HYDU_snprintf(key, PMI_MAXKEYLEN, "preput_val_%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_val = val;

        status = HYD_pmcd_pmi_add_kvs(preput_key, preput_val, pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add keypair to kvs\n");
    }

    /* Create the proxy list */
    if (pg->user_node_list) {
        status = HYDU_create_proxy_list(exec_list, pg->user_node_list, pg);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    else {
        status = HYDU_create_proxy_list(exec_list, HYD_server_info.node_list, pg);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    HYDU_free_exec_list(exec_list);

    if (pg->user_node_list) {
        pg->pg_core_count = 0;
        for (i = 0, node = pg->user_node_list; node; node = node->next, i++) {
            pg->pg_core_count += node->core_count;
            node->node_id = i;
        }
    }
    else {
        pg->pg_core_count = 0;
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            pg->pg_core_count += proxy->node->core_count;
    }

    status = HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                 HYD_server_info.local_hostname,
                                                 HYD_server_info.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) new_pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    /* Go to the last PG */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);

    status = HYD_pmcd_pmi_fill_in_proxy_args(proxy_args, control_port, new_pgid);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");
    HYDU_FREE(control_port);

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_args, pg->proxy_list, NULL);
    HYDU_ERR_POP(status, "launcher cannot launch processes\n");

    {
        char *cmd_str[HYD_NUM_TMP_STRINGS], *cmd;

        i = 0;
        cmd_str[i++] = HYDU_strdup("cmd=spawn_result rc=0");
        cmd_str[i++] = HYDU_strdup("\n");
        cmd_str[i++] = NULL;

        status = HYDU_str_alloc_and_join(cmd_str, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(cmd_str);

        status = cmd_response(fd, pid, cmd);
        HYDU_ERR_POP(status, "error writing PMI line\n");
        HYDU_FREE(cmd);
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_free_strlist(proxy_args);
    if (segment_list)
        HYDU_FREE(segment_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_publish_name(int fd, int pid, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *val;
    int i, token_count;
    struct HYD_pmcd_token *tokens;
    char *name, *port;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");
    name = HYDU_strdup(val);

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "port")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: port\n");
    port = HYDU_strdup(val);

    status = HYD_pmcd_pmi_publish(name, port, &success);
    HYDU_ERR_POP(status, "error publishing service\n");

    i = 0;
    if (success)
        tmp[i++] = HYDU_strdup("cmd=publish_result info=ok rc=0 msg=success\n");
    else
        tmp[i++] = HYDU_strdup("cmd=publish_result info=ok rc=1 msg=key_already_present\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_unpublish_name(int fd, int pid, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *name;
    int i, token_count;
    struct HYD_pmcd_token *tokens;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");

    status = HYD_pmcd_pmi_unpublish(name, &success);
    HYDU_ERR_POP(status, "error unpublishing service\n");

    i = 0;
    if (success)
        tmp[i++] = HYDU_strdup("cmd=unpublish_result info=ok rc=0 msg=success\n");
    else
        tmp[i++] = HYDU_strdup("cmd=unpublish_result info=ok rc=1 msg=service_not_found\n");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_lookup_name(int fd, int pid, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *name, *value;
    int i, token_count;
    struct HYD_pmcd_token *tokens;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");

    status = HYD_pmcd_pmi_lookup(name, &value);
    HYDU_ERR_POP(status, "error while looking up service\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=lookup_result");
    if (value) {
        tmp[i++] = HYDU_strdup(" port=");
        tmp[i++] = HYDU_strdup(value);
        tmp[i++] = HYDU_strdup(" info=ok rc=0 msg=success\n");
    }
    else {
        tmp[i++] = HYDU_strdup(" rc=1 msg=service_not_found\n");
    }
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
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
    {"spawn", fn_spawn},
    {"publish_name", fn_publish_name},
    {"unpublish_name", fn_unpublish_name},
    {"lookup_name", fn_lookup_name},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1 = pmi_v1_handle_fns_foo;

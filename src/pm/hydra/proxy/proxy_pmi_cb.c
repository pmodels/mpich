/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "proxy.h"
#include "mpl_uthash.h"

#define debug(...)                              \
    {                                           \
        if (proxy_params.debug)                  \
            HYD_PRINT(stdout, __VA_ARGS__);     \
    }

static struct proxy_kv_hash *put_cache_hash = NULL;
static struct proxy_kv_hash *kvlist = NULL;

static HYD_status send_cmd_downstream(int fd, const char *cmd)
{
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    debug("PMI response: %s", cmd);

    status = HYD_sock_write(fd, cmd, strlen(cmd), &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error writing PMI line\n");
    /* FIXME: We cannot abort when we are not able to send data
     * downstream. The upper layer needs to handle this based on
     * whether we want to abort or not.*/
    HYD_ASSERT(!closed, status);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status flush_put_cache(void)
{
    struct proxy_kv_hash *hash, *tmp;
    int local_hash_count, total_hash_count, buflen = 0, i;
    char *buf = NULL, *tbuf;
    int *lengths = NULL;
    struct MPX_cmd cmd;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    local_hash_count = MPL_HASH_COUNT(put_cache_hash);

    total_hash_count = local_hash_count;
    for (i = 0; i < proxy_params.immediate.proxy.num_children; i++)
        total_hash_count += proxy_params.immediate.proxy.kvcache_num_blocks[i];

    if (total_hash_count == 0)
        goto fn_exit;


    /* setup the local serialized hash */
    if (local_hash_count) {
        HYD_MALLOC(lengths, int *, 2 * local_hash_count * sizeof(int), status);

        /* hash lengths */
        buflen = 0;
        i = 0;
        MPL_HASH_ITER(hh, put_cache_hash, hash, tmp) {
            /* keep space for end of string characters */
            lengths[i] = strlen(hash->key) + 1;
            lengths[i + 1] = strlen(hash->val) + 1;
            buflen += lengths[i] + lengths[i + 1];
            i += 2;
        }

        /* serialize the local hash */
        HYD_MALLOC(buf, char *, buflen, status);

        i = 0;
        tbuf = buf;
        MPL_HASH_ITER(hh, put_cache_hash, hash, tmp) {
            MPL_strncpy(tbuf, hash->key, strlen(hash->key) + 1);
            tbuf += strlen(hash->key) + 1;

            MPL_strncpy(tbuf, hash->val, strlen(hash->val) + 1);
            tbuf += strlen(hash->val) + 1;

            MPL_HASH_DEL(put_cache_hash, hash);
            MPL_free(hash->key);
            MPL_free(hash->val);
            MPL_free(hash);
        }
    }

    /* repackage all kvcaches (including the local cache) and push it
     * upstream */
    MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
    cmd.type = MPX_CMD_TYPE__KVCACHE_IN;
    cmd.u.kvcache.pgid = proxy_params.all.pgid;
    cmd.u.kvcache.num_blocks = total_hash_count;
    cmd.data_len = buflen + 2 * local_hash_count * sizeof(int);
    for (i = 0; i < proxy_params.immediate.proxy.num_children; i++)
        cmd.data_len += proxy_params.immediate.proxy.kvcache_size[i];

    /* push the command header */
    status =
        HYD_sock_write(proxy_params.root.upstream_fd, &cmd, sizeof(cmd), &sent, &closed,
                       HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error sending cmd upstream\n");

    /* push the hash lengths */
    if (local_hash_count) {
        status =
            HYD_sock_write(proxy_params.root.upstream_fd, lengths,
                           2 * local_hash_count * sizeof(int), &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error sending hash lengths upstream\n");
    }

    for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
        if (proxy_params.immediate.proxy.kvcache_num_blocks[i]) {
            status =
                HYD_sock_write(proxy_params.root.upstream_fd,
                               proxy_params.immediate.proxy.kvcache[i],
                               2 * proxy_params.immediate.proxy.kvcache_num_blocks[i] * sizeof(int),
                               &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error sending hash lengths upstream\n");
        }
    }

    /* push the actual hashes */
    if (local_hash_count) {
        status =
            HYD_sock_write(proxy_params.root.upstream_fd, buf, buflen, &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error sending hashes upstream\n");
    }

    for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
        if (proxy_params.immediate.proxy.kvcache_num_blocks[i]) {
            status =
                HYD_sock_write(proxy_params.root.upstream_fd,
                               ((char *) proxy_params.immediate.proxy.kvcache[i]) +
                               2 * proxy_params.immediate.proxy.kvcache_num_blocks[i] * sizeof(int),
                               proxy_params.immediate.proxy.kvcache_size[i] -
                               2 * proxy_params.immediate.proxy.kvcache_num_blocks[i] * sizeof(int),
                               &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error sending hash lengths upstream\n");
        }
    }

    if (lengths)
        MPL_free(lengths);
    if (buf)
        MPL_free(buf);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_pmi_kvcache_out(int num_blocks, int *kvlen, char *kvcache, int buflen)
{
    int i;
    struct proxy_kv_hash *hash;
    HYD_status status = HYD_SUCCESS;

    for (i = 0; i < 2 * num_blocks;) {
        HYD_MALLOC(hash, struct proxy_kv_hash *, sizeof(struct proxy_kv_hash), status);

        hash->key = MPL_strdup(kvcache);
        kvcache += kvlen[i];
        hash->val = MPL_strdup(kvcache);
        kvcache += kvlen[i + 1];
        i += 2;

        MPL_HASH_ADD_STR(kvlist, key, hash);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_init(int fd, struct proxy_kv_hash *pmi_args)
{
    int pmi_version, pmi_subversion;
    char *tmp = NULL;
    struct proxy_kv_hash *hash;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_HASH_FIND_STR(pmi_args, "pmi_version", hash);
    HYD_ASSERT(hash, status);
    pmi_version = atoi(hash->val);

    MPL_HASH_FIND_STR(pmi_args, "pmi_subversion", hash);
    HYD_ASSERT(hash, status);
    pmi_subversion = atoi(hash->val);

    if (pmi_version == 1 && pmi_subversion <= 1)
        tmp = MPL_strdup("cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n");
    else if (pmi_version == 2 && pmi_subversion == 0)
        tmp = MPL_strdup("cmd=response_to_init pmi_version=2 pmi_subversion=0 rc=0\n");
    else        /* PMI version mismatch */
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "PMI version mismatch; %d.%d\n", pmi_version,
                           pmi_subversion);

    status = send_cmd_downstream(fd, tmp);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(tmp);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_maxes(int fd, struct proxy_kv_hash *pmi_args)
{
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("cmd=maxes kvsname_max="), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(PMI_MAXKVSLEN), status);
    HYD_STRING_STASH(stash, MPL_strdup(" keylen_max="), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(PMI_MAXKEYLEN), status);
    HYD_STRING_STASH(stash, MPL_strdup(" vallen_max="), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(PMI_MAXVALLEN), status);
    HYD_STRING_STASH(stash, MPL_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(cmd);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_appnum(int fd, struct proxy_kv_hash *pmi_args)
{
    int skipped = 0, i;
    struct HYD_exec *exec;
    struct HYD_int_hash *hash;
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_HASH_FIND_INT(proxy_params.immediate.process.pmi_fd_hash, &fd, hash);

    for (exec = proxy_params.all.complete_exec_list, i = 0; exec; exec = exec->next, i++) {
        if (skipped + exec->proc_count <= hash->val) {
            skipped += exec->proc_count;
            continue;
        }

        break;
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("cmd=appnum appnum="), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(i), status);
    HYD_STRING_STASH(stash, MPL_strdup("\n"), status);
    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(cmd);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_my_kvsname(int fd, struct proxy_kv_hash *pmi_args)
{
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("cmd=my_kvsname kvsname="), status);
    HYD_STRING_STASH(stash, MPL_strdup(proxy_params.all.kvsname), status);
    HYD_STRING_STASH(stash, MPL_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(cmd);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_put(int fd, struct proxy_kv_hash *pmi_args)
{
    struct proxy_kv_hash *hash, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(hash, struct proxy_kv_hash *, sizeof(struct proxy_kv_hash), status);

    MPL_HASH_FIND_STR(pmi_args, "key", tmp);
    hash->key = MPL_strdup(tmp->val);
    MPL_HASH_FIND_STR(pmi_args, "value", tmp);
    hash->val = MPL_strdup(tmp->val);
    MPL_HASH_ADD_STR(put_cache_hash, key, hash);

    status = send_cmd_downstream(fd, "cmd=put_result rc=0 msg=success\n");
    HYD_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, struct proxy_kv_hash *pmi_args)
{
    struct HYD_string_stash stash;
    struct proxy_kv_hash *hash, *tmp;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_HASH_FIND_STR(pmi_args, "key", hash);

    if (!strcmp(hash->val, "PMI_process_mapping")) {
        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, MPL_strdup("cmd=get_result rc=0 msg=success value="), status);
        HYD_STRING_STASH(stash, MPL_strdup(proxy_params.all.pmi_process_mapping), status);
        HYD_STRING_STASH(stash, MPL_strdup("\n"), status);
        HYD_STRING_SPIT(stash, cmd, status);

        status = send_cmd_downstream(fd, cmd);
        HYD_ERR_POP(status, "error sending PMI response\n");
        MPL_free(cmd);
    }
    else {
        MPL_HASH_FIND_STR(kvlist, hash->val, tmp);

        if (tmp) {
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, MPL_strdup("cmd=get_result rc="), status);
            HYD_STRING_STASH(stash, MPL_strdup("0 msg=success value="), status);
            HYD_STRING_STASH(stash, MPL_strdup(tmp->val), status);
            HYD_STRING_STASH(stash, MPL_strdup("\n"), status);

            HYD_STRING_SPIT(stash, cmd, status);

            status = send_cmd_downstream(fd, cmd);
            HYD_ERR_POP(status, "error sending PMI response\n");
            MPL_free(cmd);
        }
        else {
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, MPL_strdup("cmd=get_result rc="), status);
            HYD_STRING_STASH(stash, MPL_strdup("-1 msg=key_"), status);
            HYD_STRING_STASH(stash, MPL_strdup(hash->val), status);
            HYD_STRING_STASH(stash, MPL_strdup("_not_found value=unknown\n"), status);

            HYD_STRING_SPIT(stash, cmd, status);

            status = send_cmd_downstream(fd, cmd);
            HYD_ERR_POP(status, "error sending PMI response\n");
            MPL_free(cmd);
        }
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_barrier_in(int fd, struct proxy_kv_hash *pmi_args)
{
    struct MPX_cmd cmd;
    int sent, closed;
    static int barrier_ref_count = 0;
    HYD_status status = HYD_SUCCESS;

    barrier_ref_count++;

    if (barrier_ref_count ==
        proxy_params.immediate.proxy.num_children + proxy_params.immediate.process.num_children) {
        barrier_ref_count = 0;

        status = flush_put_cache();
        HYD_ERR_POP(status, "error flushing pmi put cache\n");

        MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
        cmd.type = MPX_CMD_TYPE__PMI_BARRIER_IN;
        cmd.u.barrier_in.pgid = proxy_params.all.pgid;

        status =
            HYD_sock_write(proxy_params.root.upstream_fd, &cmd, sizeof(cmd), &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error sending cmd upstream\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status proxy_barrier_out(int fd, struct proxy_kv_hash *pmi_args)
{
    char *cmd;
    struct HYD_int_hash *hash, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* we need to read the actual kvcache from upstream */
    cmd = MPL_strdup("cmd=barrier_out\n");

    MPL_HASH_ITER(hh, proxy_params.immediate.process.pmi_fd_hash, hash, tmp) {
        status = send_cmd_downstream(hash->key, cmd);
        HYD_ERR_POP(status, "error sending PMI response\n");
    }

    MPL_free(cmd);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_usize(int fd, struct proxy_kv_hash *pmi_args)
{
    char *cmd;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("cmd=universe_size size="), status);
    HYD_STRING_STASH(stash, HYD_str_from_int(proxy_params.usize), status);
    HYD_STRING_STASH(stash, MPL_strdup("\n"), status);
    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(cmd);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_finalize(int fd, struct proxy_kv_hash *pmi_args)
{
    char *cmd;
    struct proxy_kv_hash *kvhash, *kvtmp;
    static int finalize_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    cmd = MPL_strdup("cmd=finalize_ack\n");

    status = send_cmd_downstream(fd, cmd);
    HYD_ERR_POP(status, "error sending PMI response\n");
    MPL_free(cmd);

    status = HYD_dmx_deregister_fd(fd);
    HYD_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    finalize_count++;

    if (finalize_count == proxy_params.immediate.process.num_children) {
        MPL_HASH_ITER(hh, kvlist, kvhash, kvtmp) {
            MPL_HASH_DEL(kvlist, kvhash);
            MPL_free(kvhash->key);
            MPL_free(kvhash->val);
            MPL_free(kvhash);
        }
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_abort(int fd, struct proxy_kv_hash *pmi_args)
{
    struct HYD_int_hash *hash, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_dmx_deregister_fd(fd);
    HYD_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    MPL_HASH_ITER(hh, proxy_params.immediate.process.pid_hash, hash, tmp) {
        kill(hash->key, SIGTERM);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct proxy_pmi_handle pmi_handlers[] = {
    {"init", fn_init},
    {"get_maxes", fn_get_maxes},
    {"get_appnum", fn_get_appnum},
    {"get_my_kvsname", fn_get_my_kvsname},
    {"get", fn_get},
    {"put", fn_put},
    {"barrier_in", proxy_barrier_in},
    {"get_universe_size", fn_get_usize},
    {"finalize", fn_finalize},
    {"abort", fn_abort},
    {"\0", NULL}
};

struct proxy_pmi_handle *proxy_pmi_handlers = pmi_handlers;

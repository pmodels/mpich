/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "proxy.h"

/* *INDENT-OFF* */
struct proxy_params proxy_params = {
    .debug = 0,
    .tree_width = -1,

    .root = {
        .proxy_id = -1,
        .node_id = -1,
        .upstream_fd = -1,
        .hostname = NULL,
        .subtree_size = -1,
        .pid_ref_count = -1,
    },

    .immediate = {
        .proxy = {
            .num_children = -1,
            .fd_control_hash = NULL,
            .fd_stdin_hash = NULL,
            .fd_stdout_hash = NULL,
            .fd_stderr_hash = NULL,
            .pid_hash = NULL,
            .block_start = NULL,
            .block_size = NULL,
            .kvcache = NULL,
            .kvcache_size = NULL,
            .kvcache_num_blocks = NULL,
        },

        .process = {
            .num_children = -1,
            .fd_stdout_hash = NULL,
            .fd_stderr_hash = NULL,
            .fd_pmi_hash = NULL,
            .pid_hash = NULL,
            .pmi_id = NULL,
        },
    },

    .all = {
        .pgid = -1,
        .kvsname = NULL,
        .complete_exec_list = NULL,
        .global_process_count = 0,
        .primary_env = {
            .serial_buf = NULL,
            .serial_buf_len = -1,
        },
        .secondary_env = {
            .serial_buf = NULL,
            .serial_buf_len = -1,
        },
        .pmi_process_mapping = NULL,
    },
};
/* *INDENT-ON* */

int proxy_ready_to_launch = 0;

int **proxy_pids;
int *n_proxy_pids;
int **proxy_pmi_ids;

static HYD_status check_params(void)
{
    HYD_status status = HYD_SUCCESS;

    /* root */
    HYD_ASSERT(proxy_params.root.proxy_id != -1, status);
    HYD_ASSERT(proxy_params.root.node_id != -1, status);
    HYD_ASSERT(proxy_params.root.upstream_fd != -1, status);
    HYD_ASSERT(proxy_params.root.hostname, status);
    HYD_ASSERT(proxy_params.root.subtree_size != -1, status);

    /* immediate:proxy */
    HYD_ASSERT(!proxy_params.immediate.proxy.num_children ||
               proxy_params.immediate.proxy.kvcache, status);
    HYD_ASSERT(!proxy_params.immediate.proxy.num_children ||
               proxy_params.immediate.proxy.kvcache_size, status);
    HYD_ASSERT(!proxy_params.immediate.proxy.num_children ||
               proxy_params.immediate.proxy.kvcache_num_blocks, status);

    /* immediate:process */
    HYD_ASSERT(proxy_params.immediate.process.num_children != -1, status);
    HYD_ASSERT(proxy_params.immediate.process.pmi_id, status);

    /* all */
    HYD_ASSERT(proxy_params.all.pgid != -1, status);
    HYD_ASSERT(proxy_params.all.kvsname, status);
    HYD_ASSERT(proxy_params.all.complete_exec_list, status);
    HYD_ASSERT(proxy_params.all.global_process_count, status);
    HYD_ASSERT(proxy_params.all.primary_env.serial_buf_len == 0 ||
               proxy_params.all.primary_env.serial_buf, status);
    HYD_ASSERT(proxy_params.all.secondary_env.serial_buf_len == 0 ||
               proxy_params.all.secondary_env.serial_buf, status);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status get_params(int argc, char **argv)
{
    HYD_status status = HYD_SUCCESS;

    while (--argc && ++argv) {
        if (!strcmp(*argv, "--usize")) {
            ++argv;
            --argc;
            proxy_params.usize = atoi(*argv);
        }
    }

    return status;
}

static HYD_status get_bstrap_params(void)
{
    int ret, i;
    const char *str = NULL;
    char *tmp;
    struct HYD_int_hash *hash;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    ret = MPL_env2int("HYDRA_BSTRAP_PGID", &proxy_params.all.pgid);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_PGID environment\n");

    ret = MPL_env2int("HYDRA_BSTRAP_PROXY_ID", &proxy_params.root.proxy_id);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_PROXY_ID environment\n");

    ret = MPL_env2int("HYDRA_BSTRAP_NODE_ID", &proxy_params.root.node_id);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_NODE_ID environment\n");

    ret = MPL_env2int("HYDRA_BSTRAP_UPSTREAM_FD", &proxy_params.root.upstream_fd);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_UPSTREAM_FD environment\n");

    ret = MPL_env2int("HYDRA_BSTRAP_SUBTREE_SIZE", &proxy_params.root.subtree_size);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_SUBTREE_SIZE environment\n");

    ret = MPL_env2int("HYDRA_BSTRAP_TREE_WIDTH", &proxy_params.tree_width);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_TREE_WIDTH environment\n");

    ret =
        MPL_env2int("HYDRA_BSTRAP_NUM_DOWNSTREAM_PROXIES",
                    &proxy_params.immediate.proxy.num_children);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_NUM_DOWNSTREAM_PROXIES environment\n");

    if (proxy_params.immediate.proxy.num_children) {
        ret = MPL_env2str("HYDRA_BSTRAP_DOWNSTREAM_CONTROL", &str);
        if (ret < 0)
            HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DOWNSTREAM_CONTROL environment\n");
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            if (i == 0)
                tmp = strtok((char *) str, ",");
            else
                tmp = strtok(NULL, ",");
            HYD_ASSERT(tmp, status);

            HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
            hash->key = atoi(tmp);
            hash->val = i;
            MPL_HASH_ADD_INT(proxy_params.immediate.proxy.fd_control_hash, key, hash);
        }
        tmp = strtok(NULL, ",");
        HYD_ASSERT(tmp == NULL, status);

        ret = MPL_env2str("HYDRA_BSTRAP_DOWNSTREAM_STDIN", &str);
        if (ret < 0)
            HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DOWNSTREAM_STDIN environment\n");
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            if (i == 0)
                tmp = strtok((char *) str, ",");
            else
                tmp = strtok(NULL, ",");
            HYD_ASSERT(tmp, status);

            HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
            hash->key = atoi(tmp);
            hash->val = i;
            MPL_HASH_ADD_INT(proxy_params.immediate.proxy.fd_stdin_hash, key, hash);
        }
        tmp = strtok(NULL, ",");
        HYD_ASSERT(tmp == NULL, status);

        ret = MPL_env2str("HYDRA_BSTRAP_DOWNSTREAM_STDOUT", &str);
        if (ret < 0)
            HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DOWNSTREAM_STDOUT environment\n");
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            if (i == 0)
                tmp = strtok((char *) str, ",");
            else
                tmp = strtok(NULL, ",");
            HYD_ASSERT(tmp, status);

            HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
            hash->key = atoi(tmp);
            hash->val = i;
            MPL_HASH_ADD_INT(proxy_params.immediate.proxy.fd_stdout_hash, key, hash);
        }
        tmp = strtok(NULL, ",");
        HYD_ASSERT(tmp == NULL, status);

        ret = MPL_env2str("HYDRA_BSTRAP_DOWNSTREAM_STDERR", &str);
        if (ret < 0)
            HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DOWNSTREAM_STDERR environment\n");
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            if (i == 0)
                tmp = strtok((char *) str, ",");
            else
                tmp = strtok(NULL, ",");
            HYD_ASSERT(tmp, status);

            HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
            hash->key = atoi(tmp);
            hash->val = i;
            MPL_HASH_ADD_INT(proxy_params.immediate.proxy.fd_stderr_hash, key, hash);
        }
        tmp = strtok(NULL, ",");
        HYD_ASSERT(tmp == NULL, status);

        ret = MPL_env2str("HYDRA_BSTRAP_DOWNSTREAM_PID", &str);
        if (ret < 0)
            HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DOWNSTREAM_PID environment\n");
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++) {
            if (i == 0)
                tmp = strtok((char *) str, ",");
            else
                tmp = strtok(NULL, ",");
            HYD_ASSERT(tmp, status);

            HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
            hash->key = atoi(tmp);
            hash->val = i;
            MPL_HASH_ADD_INT(proxy_params.immediate.proxy.pid_hash, key, hash);
        }
        tmp = strtok(NULL, ",");
        HYD_ASSERT(tmp == NULL, status);
    }

    ret = MPL_env2str("HYDRA_BSTRAP_LOCALHOST", &proxy_params.root.hostname);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_LOCALHOST environment\n");
    proxy_params.root.hostname = MPL_strdup(proxy_params.root.hostname);

    ret = MPL_env2int("HYDRA_BSTRAP_DEBUG", &proxy_params.debug);
    if (ret < 0)
        HYD_ERR_POP(status, "error reading HYDRA_BSTRAP_DEBUG environment\n");

    /* allocate stuff that relies on the above information */
    if (proxy_params.immediate.proxy.num_children) {
        HYD_MALLOC(proxy_params.immediate.proxy.kvcache, void **,
                   proxy_params.immediate.proxy.num_children * sizeof(void *), status);
        HYD_MALLOC(proxy_params.immediate.proxy.kvcache_size, int *,
                   proxy_params.immediate.proxy.num_children * sizeof(int), status);
        HYD_MALLOC(proxy_params.immediate.proxy.kvcache_num_blocks, int *,
                   proxy_params.immediate.proxy.num_children * sizeof(int), status);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_exec *get_exec_by_id(int id)
{
    int skipped = 0;
    struct HYD_exec *exec;

    for (exec = proxy_params.all.complete_exec_list; exec; exec = exec->next) {
        if (skipped + exec->proc_count <= id) {
            skipped += exec->proc_count;
            continue;
        }

        break;
    }

    return exec;
}

static HYD_status launch_processes(void)
{
    int i;
    char *envval;
    char **envargs;
    struct HYD_env *env;
    HYD_status status = HYD_SUCCESS;

    HYD_MALLOC(envargs, char **, 3 * sizeof(char *), status);

    envval = HYD_str_from_int(proxy_params.all.global_process_count);
    HYD_env_create(&env, "PMI_SIZE", envval);
    MPL_free(envval);

    status = HYD_env_to_str(env, &envargs[0]);
    HYD_ERR_POP(status, "error converting env to string\n");
    HYD_env_free(env);

    for (i = 0; i < proxy_params.immediate.process.num_children; i++) {
        struct HYD_exec *exec = get_exec_by_id(i);
        int stdin_fd, stdout_fd, stderr_fd, pid, *tmp;
        int sockpair[2];
        struct HYD_int_hash *hash;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) < 0)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "pipe error\n");

        /* the parent process keeps sockpair[0].  make sure it is not
         * shared by the child process. */
        status = HYD_sock_cloexec(sockpair[0]);
        HYD_ERR_POP(status, "unable to set control socket to close on exec\n");

        HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
        hash->key = sockpair[0];
        hash->val = i;
        MPL_HASH_ADD_INT(proxy_params.immediate.process.fd_pmi_hash, key, hash);

        status = HYD_dmx_register_fd(sockpair[0], HYD_DMX_POLLIN, NULL, proxy_process_pmi_cb);
        HYD_ERR_POP(status, "unable to register fd\n");

        envval = HYD_str_from_int(proxy_params.immediate.process.pmi_id[i]);
        HYD_env_create(&env, "PMI_RANK", envval);
        MPL_free(envval);

        status = HYD_env_to_str(env, &envargs[1]);
        HYD_ERR_POP(status, "error converting env to string\n");
        HYD_env_free(env);

        envval = HYD_str_from_int(sockpair[1]);
        HYD_env_create(&env, "PMI_FD", envval);
        MPL_free(envval);

        status = HYD_env_to_str(env, &envargs[2]);
        HYD_ERR_POP(status, "error converting env to string\n");
        HYD_env_free(env);

        tmp = (!proxy_params.all.pgid && !proxy_params.root.proxy_id && !i) ? &stdin_fd : NULL;

        status = HYD_spawn(exec->exec, 3, envargs, tmp, &stdout_fd, &stderr_fd, &pid, -1);
        HYD_ERR_POP(status, "error creating process\n");

        MPL_free(envargs[1]);
        MPL_free(envargs[2]);

        close(sockpair[1]);

        HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
        hash->key = pid;
        hash->val = i;
        MPL_HASH_ADD_INT(proxy_params.immediate.process.pid_hash, key, hash);

        status = HYD_dmx_register_fd(stdout_fd, HYD_DMX_POLLIN, NULL, proxy_process_stdout_cb);
        HYD_ERR_POP(status, "unable to register fd\n");

        HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
        hash->key = stdout_fd;
        hash->val = i;
        MPL_HASH_ADD_INT(proxy_params.immediate.process.fd_stdout_hash, key, hash);

        status = HYD_dmx_register_fd(stderr_fd, HYD_DMX_POLLIN, NULL, proxy_process_stderr_cb);
        HYD_ERR_POP(status, "unable to register fd\n");

        HYD_MALLOC(hash, struct HYD_int_hash *, sizeof(struct HYD_int_hash), status);
        hash->key = stderr_fd;
        hash->val = i;
        MPL_HASH_ADD_INT(proxy_params.immediate.process.fd_stderr_hash, key, hash);

        if (tmp) {
            status = HYD_dmx_splice(STDIN_FILENO, stdin_fd);
            HYD_ERR_POP(status, "error splicing stdin fd\n");
        }
    }

    MPL_free(envargs[0]);
    MPL_free(envargs);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

#define MAPPING_PARSE_ERROR() abort()
/* advance _c until we find a non whitespace character */
#define MAPPING_SKIP_SPACE(_c) while (isspace(*(_c))) ++(_c)
/* return true iff _c points to a character valid as an indentifier, i.e., [-_a-zA-Z0-9] */
#define MAPPING_ISIDENT(_c) (isalnum(_c) || (_c) == '-' || (_c) == '_')

/* give an error iff *_c != _e */
#define MAPPING_EXPECT_C(_c, _e) do { if (*(_c) != _e) MAPPING_PARSE_ERROR(); } while (0)
#define MAPPING_EXPECT_AND_SKIP_C(_c, _e) do { MAPPING_EXPECT_C(_c, _e); ++c; } while (0)
/* give an error iff the first |_m| characters of the string _s are equal to _e */
#define MAPPING_EXPECT_S(_s, _e) (MPL_strncmp(_s, _e, strlen(_e)) == 0 && !MAPPING_ISIDENT((_s)[strlen(_e)]))

typedef enum {
    UNKNOWN_MAPPING = -1,
    NULL_MAPPING = 0,
    VECTOR_MAPPING
} mapping_type_t;

typedef struct {
    int start_id;
    int count;
    int size;
} map_block_t;

static int parse_mapping(char *map_str, mapping_type_t * type, map_block_t ** map, int *nblocks)
{
    HYD_status status = HYD_SUCCESS;
    char *c = map_str, *d;
    int num_blocks = 0;
    int i;

    /* parse string of the form:
     * '(' <format> ',' '(' <num> ',' <num> ',' <num> ')' {',' '(' <num> ',' <num> ',' <num> ')'} ')'
     *
     * the values of each 3-tuple have the following meaning (X,Y,Z):
     * X - node id start value
     * Y - number of nodes with size Z
     * Z - number of processes assigned to each node
     */

    if (!strlen(map_str)) {
        /* An empty-string indicates an inability to determine or express the
         * process layout on the part of the process manager.  Consider this a
         * non-fatal error case. */
        *type = NULL_MAPPING;
        *map = NULL;
        *nblocks = 0;
        goto fn_exit;
    }

    MAPPING_SKIP_SPACE(c);
    MAPPING_EXPECT_AND_SKIP_C(c, '(');
    MAPPING_SKIP_SPACE(c);

    d = c;
    if (MAPPING_EXPECT_S(d, "vector"))
        *type = VECTOR_MAPPING;
    else
        MAPPING_PARSE_ERROR();
    c += strlen("vector");
    MAPPING_SKIP_SPACE(c);

    /* first count the number of block descriptors */
    d = c;
    while (*d) {
        if (*d == '(')
            ++num_blocks;
        ++d;
    }

    *map = MPL_malloc(sizeof(map_block_t) * num_blocks);

    /* parse block descriptors */
    for (i = 0; i < num_blocks; ++i) {
        MAPPING_EXPECT_AND_SKIP_C(c, ',');
        MAPPING_SKIP_SPACE(c);

        MAPPING_EXPECT_AND_SKIP_C(c, '(');
        MAPPING_SKIP_SPACE(c);

        if (!isdigit(*c))
            MAPPING_PARSE_ERROR();
        (*map)[i].start_id = (int) strtol(c, &c, 0);
        MAPPING_SKIP_SPACE(c);

        MAPPING_EXPECT_AND_SKIP_C(c, ',');
        MAPPING_SKIP_SPACE(c);

        if (!isdigit(*c))
            MAPPING_PARSE_ERROR();
        (*map)[i].count = (int) strtol(c, &c, 0);
        MAPPING_SKIP_SPACE(c);

        MAPPING_EXPECT_AND_SKIP_C(c, ',');
        MAPPING_SKIP_SPACE(c);

        if (!isdigit(*c))
            MAPPING_PARSE_ERROR();
        (*map)[i].size = (int) strtol(c, &c, 0);

        MAPPING_EXPECT_AND_SKIP_C(c, ')');
        MAPPING_SKIP_SPACE(c);
    }

    MAPPING_EXPECT_AND_SKIP_C(c, ')');

    *nblocks = num_blocks;
  fn_exit:
    return status;
}

static int populate_ids_from_mapping(char *mapping, int sz, int *out_nodemap)
{
    HYD_status status = HYD_SUCCESS;
    /* PMI_process_mapping is available */
    mapping_type_t mt = UNKNOWN_MAPPING;
    map_block_t *mb = NULL;
    int nblocks = 0;
    int rank;
    int block, block_node, node_proc;
    int found_wrap;

    status = parse_mapping(mapping, &mt, &mb, &nblocks);
    if (status)
        HYD_ERR_POP(status, "error parsing node mapping");
    HYD_ASSERT(mt == VECTOR_MAPPING, status)

        /* allocate nodes to ranks */
        found_wrap = 0;
    for (rank = 0;;) {
        /* FIXME: The patch is hacky because it assumes that seeing a
         * start node ID of 0 means a wrap around.  This is not
         * necessarily true.  A user-defined node list can, in theory,
         * use the node ID 0 without actually creating a wrap around.
         * The reason this patch still works in this case is because
         * Hydra creates a new node list starting from node ID 0 for
         * user-specified nodes during MPI_Comm_spawn{_multiple}.  If
         * a different process manager searches for allocated nodes in
         * the user-specified list, this patch will break. */

        /* If we found that the blocks wrap around, repeat loops
         * should only start at node id 0 */
        for (block = 0; found_wrap && mb[block].start_id; block++);

        for (; block < nblocks; block++) {
            if (mb[block].start_id == 0)
                found_wrap = 1;
            for (block_node = 0; block_node < mb[block].count; block_node++) {
                for (node_proc = 0; node_proc < mb[block].size; node_proc++) {
                    out_nodemap[rank] = mb[block].start_id + block_node;
                    if (++rank == sz)
                        goto fn_exit;
                }
            }
        }
    }

  fn_exit:
    MPL_free(mb);
    return status;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int main(int argc, char **argv)
{
    int *process_ret;
    struct HYD_int_hash *hash, *tmp;
    char dbg_prefix[2 * HYD_MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;
    int *nodemap, i, local_rank;

    status = HYD_print_set_prefix_str("proxy:unset");
    HYD_ERR_POP(status, "unable to set dbg prefix\n");

    HYD_MALLOC(proxy_pids, int **, proxy_params.immediate.proxy.num_children * sizeof(int *), status);
    HYD_MALLOC(n_proxy_pids, int *, proxy_params.immediate.proxy.num_children * sizeof(int), status);
    HYD_MALLOC(proxy_pmi_ids, int **, proxy_params.immediate.proxy.num_children * sizeof(int *), status);

    /* To launch the MPI processes, we follow a process:
     * (1) get parameters from the bstrap, as arguments or from
     * upstream, (2) make sure all the parameters we need are
     * available, (3) close all downstream stdins (downstream proxies
     * never get stdin), (4) launch processes, (5) send pids back upstream to
     * put in the PROCDESC struct for debuggers. */

    /* step 1(a): get parameters */
    status = get_params(argc, argv);
    HYD_ERR_POP(status, "error getting parameters\n");
    status = get_bstrap_params();
    HYD_ERR_POP(status, "error getting parameters\n");

    MPL_snprintf(dbg_prefix, 2 * HYD_MAX_HOSTNAME_LEN, "proxy:%d:%d", proxy_params.all.pgid,
                 proxy_params.root.proxy_id);
    status = HYD_print_set_prefix_str((const char *) dbg_prefix);
    HYD_ERR_POP(status, "unable to set dbg prefix\n");

    status =
        HYD_dmx_register_fd(proxy_params.root.upstream_fd, HYD_DMX_POLLIN, NULL,
                            proxy_upstream_control_cb);
    HYD_ERR_POP(status, "unable to register fd\n");

    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_control_hash, hash, tmp) {
        status = HYD_dmx_register_fd(hash->key, HYD_DMX_POLLIN, NULL, proxy_downstream_control_cb);
        HYD_ERR_POP(status, "unable to register fd\n");
    }

    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_stdout_hash, hash, tmp) {
        status = HYD_dmx_splice(hash->key, STDOUT_FILENO);
        HYD_ERR_POP(status, "unable to splice stdout\n");
    }

    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_stderr_hash, hash, tmp) {
        status = HYD_dmx_splice(hash->key, STDERR_FILENO);
        HYD_ERR_POP(status, "unable to splice stderr\n");
    }

    /* step 1(b): get the upstream part of the parameters */
    while (!proxy_ready_to_launch) {
        status = HYD_dmx_wait_for_event(-1);
        HYD_ERR_POP(status, "demux engine error waiting for event\n");
    }

    /* step 1(c): calculate local PMI ids from mapping string */
    HYD_MALLOC(nodemap, int *, proxy_params.all.global_process_count * sizeof(int), status);
    populate_ids_from_mapping(proxy_params.all.pmi_process_mapping,
                              proxy_params.all.global_process_count, nodemap);
    proxy_params.immediate.process.num_children = 0;
    for (i = 0; i < proxy_params.all.global_process_count; i++) {
        if (nodemap[i] == proxy_params.root.node_id)
            proxy_params.immediate.process.num_children++;
    }
    HYD_MALLOC(proxy_params.immediate.process.pmi_id, int *,
               proxy_params.immediate.process.num_children * sizeof(int), status);
    for (i = 0, local_rank = 0; i < proxy_params.all.global_process_count; i++)
        if (nodemap[i] == proxy_params.root.node_id)
            proxy_params.immediate.process.pmi_id[local_rank++] = i;
    MPL_free(nodemap);

    /* step 2: make sure all parameters are correctly set */
    status = check_params();
    HYD_ERR_POP(status, "error checking parameters\n");

    /* step 3: close all downstream stdin fds */
    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_stdin_hash, hash, tmp) {
        close(hash->key);
    }

    /* step 4: launch processes */
    status = launch_processes();
    HYD_ERR_POP(status, "error launching_processes\n");

    /* step 5: report PIDs back to mpiexec for debugger */
    i = 0;
    MPL_HASH_ITER(hh, proxy_params.immediate.process.pid_hash, hash, tmp) {
        proxy_pids[0][i++] = hash->key; /* The pid of the child process */
    }
    n_proxy_pids[0] = proxy_params.immediate.process.num_children;
    memcpy(proxy_pmi_ids, proxy_params.immediate.process.pmi_id, n_proxy_pids[0] * sizeof(int));
    proxy_send_pids_upstream();

    /* The launch is now complete: we wait for the processes to
     * complete their execution, in a five-step process: (1) wait for
     * events till the stdout/stderr/pmi-fd sockets of all MPI
     * processes have closed, (2) wait for the MPI processes to
     * actually terminate (should be quick unless the process
     * explicitly closed its stdout/stderr), (3) wait for events till
     * the stdout/stderr sockets of all downstream proxies have
     * closed, (4) wait for the downstream proxies to actually
     * terminate, (5) deregister upstream fd and stdin */

    /* step 1: wait for MPI process stdout/stderr/pmi-fd to close */
    MPL_HASH_ITER(hh, proxy_params.immediate.process.fd_stdout_hash, hash, tmp) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
        MPL_HASH_DEL(proxy_params.immediate.process.fd_stdout_hash, hash);
        MPL_free(hash);
    }
    MPL_HASH_ITER(hh, proxy_params.immediate.process.fd_stderr_hash, hash, tmp) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
        MPL_HASH_DEL(proxy_params.immediate.process.fd_stderr_hash, hash);
        MPL_free(hash);
    }
    MPL_HASH_ITER(hh, proxy_params.immediate.process.fd_pmi_hash, hash, tmp) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }
        MPL_HASH_DEL(proxy_params.immediate.process.fd_pmi_hash, hash);
        MPL_free(hash);
    }

    /* step 2: wait for MPI process to terminate */
    HYD_MALLOC(process_ret, int *, proxy_params.immediate.process.num_children * sizeof(int),
               status);
    MPL_HASH_ITER(hh, proxy_params.immediate.process.pid_hash, hash, tmp) {
        waitpid(hash->key, &process_ret[hash->val], 0);
        MPL_HASH_DEL(proxy_params.immediate.process.pid_hash, hash);
        MPL_free(hash);
    }

    /* step 3: wait for proxy stdout/stderr to close */
    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_stdout_hash, hash, tmp) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }

        status = HYD_dmx_unsplice(hash->key);
        HYD_ERR_POP(status, "error unsplicing fd\n");

        MPL_HASH_DEL(proxy_params.immediate.proxy.fd_stdout_hash, hash);
        close(hash->key);
    }
    MPL_HASH_ITER(hh, proxy_params.immediate.proxy.fd_stderr_hash, hash, tmp) {
        while (HYD_dmx_query_fd_registration(hash->key)) {
            status = HYD_dmx_wait_for_event(-1);
            HYD_ERR_POP(status, "error waiting for event\n");
        }

        status = HYD_dmx_unsplice(hash->key);
        HYD_ERR_POP(status, "error unsplicing fd\n");

        MPL_HASH_DEL(proxy_params.immediate.proxy.fd_stderr_hash, hash);
        close(hash->key);
    }

    /* step 4: wait for proxies to terminate */
    MPL_HASH_ITER(hh, proxy_params.immediate.process.pid_hash, hash, tmp) {
        int ret = 0;

        waitpid(hash->key, &ret, 0);

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

        MPL_HASH_DEL(proxy_params.immediate.process.pid_hash, hash);
    }

    /* step 5: deregister upstream fd and stdin */
    if (!proxy_params.all.pgid && !proxy_params.root.proxy_id) {
        status = HYD_dmx_unsplice(STDIN_FILENO);
        HYD_ERR_POP(status, "error deregistering stdin\n");
    }

    status = HYD_dmx_deregister_fd(proxy_params.root.upstream_fd);
    HYD_ERR_POP(status, "error deregistering upstream fd\n");

    if (proxy_params.cwd)
        MPL_free(proxy_params.cwd);

    if (proxy_params.root.hostname)
        MPL_free(proxy_params.root.hostname);

    HYD_ASSERT(proxy_params.immediate.proxy.fd_control_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.proxy.fd_stdin_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.proxy.fd_stdout_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.proxy.fd_stderr_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.proxy.pid_hash == NULL, status);

    if (proxy_params.immediate.proxy.block_start)
        MPL_free(proxy_params.immediate.proxy.block_start);
    if (proxy_params.immediate.proxy.block_size)
        MPL_free(proxy_params.immediate.proxy.block_size);

    if (proxy_params.immediate.proxy.num_children) {
        for (i = 0; i < proxy_params.immediate.proxy.num_children; i++)
            if (proxy_params.immediate.proxy.kvcache[i])
                MPL_free(proxy_params.immediate.proxy.kvcache[i]);
        MPL_free(proxy_params.immediate.proxy.kvcache);
        MPL_free(proxy_params.immediate.proxy.kvcache_size);
        MPL_free(proxy_params.immediate.proxy.kvcache_num_blocks);
    }

    HYD_ASSERT(proxy_params.immediate.process.fd_stdout_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.process.fd_stderr_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.process.fd_pmi_hash == NULL, status);
    HYD_ASSERT(proxy_params.immediate.process.pid_hash == NULL, status);

    if (proxy_params.immediate.process.pmi_id)
        MPL_free(proxy_params.immediate.process.pmi_id);

    if (proxy_params.all.kvsname)
        MPL_free(proxy_params.all.kvsname);
    if (proxy_params.all.complete_exec_list)
        HYD_exec_free_list(proxy_params.all.complete_exec_list);

    if (proxy_params.all.primary_env.serial_buf)
        MPL_free(proxy_params.all.primary_env.serial_buf);
    if (proxy_params.all.primary_env.argc) {
        for (i = 0; i < proxy_params.all.primary_env.argc; i++)
            MPL_free(proxy_params.all.primary_env.argv[i]);
        MPL_free(proxy_params.all.primary_env.argv);
    }

    if (proxy_params.all.secondary_env.serial_buf)
        MPL_free(proxy_params.all.secondary_env.serial_buf);
    if (proxy_params.all.secondary_env.argc) {
        for (i = 0; i < proxy_params.all.secondary_env.argc; i++)
            MPL_free(proxy_params.all.secondary_env.argv[i]);
        MPL_free(proxy_params.all.secondary_env.argv);
    }

    if (proxy_params.all.pmi_process_mapping)
        MPL_free(proxy_params.all.pmi_process_mapping);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

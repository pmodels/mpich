/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_bstrap.h"
#include "hydra_bstrap_common.h"
#include "hydra_demux.h"
#include "hydra_sock.h"
#include "hydra_err.h"
#include "hydra_str.h"
#include "hydra_mem.h"
#include "hydra_node.h"

static const char *upstream_host = NULL;
static int upstream_port = -1;
static int upstream_fd = -1;
static int pgid = -1;
static int proxy_id = -1;
static int node_id = -1;
static const char *launcher = NULL;
static const char *launcher_exec = NULL;
static const char *base_path = NULL;
static const char *port_range = NULL;
static int subtree_size = -1;
static int tree_width = -1;
static char **proxy_args = NULL;
static int num_downstream_proxies = 0;
static int *downstream_stdin = NULL;
static int *downstream_stdout = NULL;
static int *downstream_stderr = NULL;
static int *downstream_control = NULL;
static int *downstream_proxy_id = NULL;
static int *downstream_pid = NULL;
static int debug = 0;

static const char *localhost = NULL;

static HYD_status get_params(int argc, char **argv)
{
    HYD_status status = HYD_SUCCESS;

    while (--argc && ++argv) {
        if (!strcmp(*argv, "--upstream-host")) {
            ++argv;
            --argc;
            upstream_host = MPL_strdup(*argv);
        }
        else if (!strcmp(*argv, "--upstream-port")) {
            ++argv;
            --argc;
            upstream_port = atoi(*argv);
        }
        else if (!strcmp(*argv, "--upstream-fd")) {
            ++argv;
            --argc;
            upstream_fd = atoi(*argv);
        }
        else if (!strcmp(*argv, "--pgid")) {
            ++argv;
            --argc;
            pgid = atoi(*argv);
        }
        else if (!strcmp(*argv, "--proxy-id")) {
            ++argv;
            --argc;
            proxy_id = atoi(*argv);
        }
        else if (!strcmp(*argv, "--node-id")) {
            ++argv;
            --argc;
            node_id = atoi(*argv);
        }
        else if (!strcmp(*argv, "--launcher")) {
            ++argv;
            --argc;
            launcher = MPL_strdup(*argv);
        }
        else if (!strcmp(*argv, "--launcher-exec")) {
            ++argv;
            --argc;
            launcher_exec = MPL_strdup(*argv);
        }
        else if (!strcmp(*argv, "--base-path")) {
            ++argv;
            --argc;
            base_path = MPL_strdup(*argv);
        }
        else if (!strcmp(*argv, "--port-range")) {
            ++argv;
            --argc;
            port_range = MPL_strdup(*argv);
        }
        else if (!strcmp(*argv, "--subtree-size")) {
            ++argv;
            --argc;
            subtree_size = atoi(*argv);
        }
        else if (!strcmp(*argv, "--tree-width")) {
            ++argv;
            --argc;
            tree_width = atoi(*argv);
        }
        else if (!strcmp(*argv, "--debug")) {
            debug = 1;
        }
        else {
            int i = 0;
            HYD_MALLOC(proxy_args, char **, (argc + 1) * sizeof(char *), status);
            for (i = 0; i < argc; i++) {
                proxy_args[i] = MPL_strdup(*argv);
                argv++;
            }
            proxy_args[i] = NULL;

            break;
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status set_env_ints(const char *env_name, int num_vals, int *env_vals)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *str;
    int i, j;
    HYD_status status = HYD_SUCCESS;

    i = 0;
    tmp[i++] = MPL_strdup(env_name);
    tmp[i++] = MPL_strdup("=");
    for (j = 0; j < num_vals; j++) {
        tmp[i++] = HYD_str_from_int(env_vals[j]);
        if (j < num_vals - 1)
            tmp[i++] = MPL_strdup(",");
    }
    tmp[i++] = NULL;
    status = HYD_str_alloc_and_join(tmp, &str);
    HYD_ERR_POP(status, "unable to join strings\n");

    putenv(str);

    for (i = 0; tmp[i]; i++)
        MPL_free(tmp[i]);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status set_env_str(const char *env_name, const char *env_val)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *str;
    int i;
    HYD_status status = HYD_SUCCESS;

    i = 0;
    tmp[i++] = MPL_strdup(env_name);
    tmp[i++] = MPL_strdup("=");
    tmp[i++] = MPL_strdup(env_val);
    tmp[i++] = NULL;
    status = HYD_str_alloc_and_join(tmp, &str);
    HYD_ERR_POP(status, "unable to join strings\n");

    putenv(str);

    for (i = 0; tmp[i]; i++)
        MPL_free(tmp[i]);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status upstream_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    struct HYDI_bstrap_cmd cmd;
    int recvd, closed;
    HYD_status status = HYD_SUCCESS;

    /* We got a command from upstream */
    status = HYD_sock_read(fd, &cmd, sizeof(cmd), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading command from launcher\n");
    HYD_ASSERT(!closed, status);

    if (cmd.type == HYDI_BSTRAP_CMD__HOSTLIST) {
        struct HYD_node *node_list;

        /* host list is the last thing we should receive; deregister
         * the upstream fd so we do not accidentally receive any
         * commands that we intended for the PMI proxy while we are
         * waiting to setup the rest of the bstrap subtree */
        status = HYD_dmx_deregister_fd(fd);
        HYD_ERR_POP(status, "error deregistering fd\n");

        HYD_MALLOC(node_list, struct HYD_node *, subtree_size * sizeof(struct HYD_node), status);
        status =
            HYD_sock_read(fd, node_list, subtree_size * sizeof(struct HYD_node), &recvd, &closed,
                          HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading command from launcher\n");
        HYD_ASSERT(!closed, status);
        localhost = MPL_strdup(node_list[0].hostname);

        /* launch the remaining bstrap proxies in this subtree */
        if (subtree_size > 1) {
            status =
                HYD_bstrap_setup(base_path, launcher, launcher_exec, subtree_size - 1,
                                 node_list + 1, proxy_id, port_range, proxy_args, pgid,
                                 &num_downstream_proxies, &downstream_stdin, &downstream_stdout,
                                 &downstream_stderr, &downstream_control, &downstream_proxy_id,
                                 &downstream_pid, debug, tree_width);
            HYD_ERR_POP(status, "error setting up the bstrap proxies\n");

            /* finalize the bstrap immediately since we will never
             * need to use it again */
            status = HYD_bstrap_finalize(launcher);
            HYD_ERR_POP(status, "error finalizing bstrap\n");
        }

        /* set the envs needed to propagate our information to the PMI
         * proxy */
        status = set_env_ints("HYDRA_BSTRAP_PGID", 1, &pgid);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_PROXY_ID", 1, &proxy_id);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_NODE_ID", 1, &node_id);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_UPSTREAM_FD", 1, &upstream_fd);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_SUBTREE_SIZE", 1, &subtree_size);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_TREE_WIDTH", 1, &tree_width);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_NUM_DOWNSTREAM_PROXIES", 1, &num_downstream_proxies);
        HYD_ERR_POP(status, "unable to set environment\n");

        status =
            set_env_ints("HYDRA_BSTRAP_DOWNSTREAM_CONTROL", num_downstream_proxies,
                         downstream_control);
        HYD_ERR_POP(status, "unable to set environment\n");

        status =
            set_env_ints("HYDRA_BSTRAP_DOWNSTREAM_STDIN", num_downstream_proxies, downstream_stdin);
        HYD_ERR_POP(status, "unable to set environment\n");

        status =
            set_env_ints("HYDRA_BSTRAP_DOWNSTREAM_STDOUT", num_downstream_proxies,
                         downstream_stdout);
        HYD_ERR_POP(status, "unable to set environment\n");

        status =
            set_env_ints("HYDRA_BSTRAP_DOWNSTREAM_STDERR", num_downstream_proxies,
                         downstream_stderr);
        HYD_ERR_POP(status, "unable to set environment\n");

        status =
            set_env_ints("HYDRA_BSTRAP_DOWNSTREAM_PID", num_downstream_proxies, downstream_pid);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_str("HYDRA_BSTRAP_LOCALHOST", localhost);
        HYD_ERR_POP(status, "unable to set environment\n");

        status = set_env_ints("HYDRA_BSTRAP_DEBUG", 1, &debug);
        HYD_ERR_POP(status, "unable to set environment\n");

        /* we are set; launch the PMI proxy */
        /* unsafe cast to char ** before passing proxy_args to execvp;
         * the function does not modify the arguments, but is just
         * badly defined */
        if (execvp(proxy_args[0], (char **) proxy_args) < 0) {
            HYD_ERR_PRINT("execvp error on file %s (%s)\n", proxy_args[0], MPL_strerror(errno));
            exit(-1);
        }
    }
    else {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unknown cmd type: %d\n", cmd.type);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char **argv)
{
    struct HYDI_bstrap_cmd cmd;
    int sent, closed;
    char dbg_prefix[2 * HYD_MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    status = HYD_print_set_prefix_str("bstrap:unset");
    HYD_ERR_POP(status, "unable to set dbg prefix\n");

    status = get_params(argc, argv);
    HYD_ERR_POP(status, "Error initializing bstrap params\n");

    MPL_snprintf(dbg_prefix, 2 * HYD_MAX_HOSTNAME_LEN, "bstrap:%d:%d", pgid, proxy_id);
    status = HYD_print_set_prefix_str((const char *) dbg_prefix);
    HYD_ERR_POP(status, "unable to set dbg prefix\n");

    /* if we did not already get an upstream fd, we should have gotten
     * a hostname and port; connect to it. */
    if (upstream_fd == -1) {
        status = HYD_sock_connect(upstream_host, upstream_port, &upstream_fd, 0, HYD_CONNECT_DELAY);
        HYD_ERR_POP(status, "unable to connect to server %s at port %d (check for firewalls!)\n",
                    upstream_host, upstream_port);
    }

    /* first step is to send our proxy id upstream, so the upstream
     * can associate us with the fd */
    cmd.type = HYDI_BSTRAP_CMD__PROXY_ID;
    cmd.u.proxy_id.id = proxy_id;
    status =
        HYD_sock_write(upstream_fd, &cmd, sizeof(struct HYDI_bstrap_cmd), &sent, &closed,
                       HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error sending proxy id upstream\n");
    HYD_ASSERT(!closed, status);

    status = HYD_dmx_register_fd(upstream_fd, HYD_DMX_POLLIN, NULL, upstream_cb);
    HYD_ERR_POP(status, "unable to register fd\n");

    /* wait till we got our environments */
    while (1) {
        status = HYD_dmx_wait_for_event(-1);
        HYD_ERR_POP(status, "error sending proxy ID upstream\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

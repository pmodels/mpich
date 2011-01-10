/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "ui.h"
#include "uiu.h"

static struct stdoe_fd {
    int fd;
    char *pattern;
    struct stdoe_fd *next;
} *stdoe_fd_list = NULL;

void HYD_uiu_init_params(void)
{
    HYDU_init_user_global(&HYD_server_info.user_global);

    HYD_server_info.base_path = NULL;

    HYD_server_info.port_range = NULL;
    HYD_server_info.interface_env_name = NULL;

    HYD_server_info.nameserver = NULL;
    HYD_server_info.local_hostname = NULL;

    HYD_server_info.stdout_cb = NULL;
    HYD_server_info.stderr_cb = NULL;

    HYD_server_info.node_list = NULL;
    HYD_server_info.global_core_count = 0;

    HYDU_init_pg(&HYD_server_info.pg_list, 0);

    HYD_server_info.pg_list.pgid = 0;
    HYD_server_info.pg_list.next = NULL;

#if defined ENABLE_PROFILING
    HYD_server_info.enable_profiling = -1;
    HYD_server_info.num_pmi_calls = 0;
#endif /* ENABLE_PROFILING */

    HYD_ui_info.prepend_pattern = NULL;
    HYD_ui_info.outfile_pattern = NULL;
    HYD_ui_info.errfile_pattern = NULL;

    stdoe_fd_list = NULL;
}

void HYD_uiu_free_params(void)
{
    struct stdoe_fd *tmp, *run;

    HYDU_finalize_user_global(&HYD_server_info.user_global);

    if (HYD_server_info.base_path)
        HYDU_FREE(HYD_server_info.base_path);

    if (HYD_server_info.port_range)
        HYDU_FREE(HYD_server_info.port_range);

    if (HYD_server_info.interface_env_name)
        HYDU_FREE(HYD_server_info.interface_env_name);

    if (HYD_server_info.nameserver)
        HYDU_FREE(HYD_server_info.nameserver);

    if (HYD_server_info.local_hostname)
        HYDU_FREE(HYD_server_info.local_hostname);

    if (HYD_server_info.node_list)
        HYDU_free_node_list(HYD_server_info.node_list);

    if (HYD_server_info.pg_list.proxy_list)
        HYDU_free_proxy_list(HYD_server_info.pg_list.proxy_list);

    if (HYD_server_info.pg_list.next)
        HYDU_free_pg_list(HYD_server_info.pg_list.next);

    if (HYD_ui_info.prepend_pattern)
        HYDU_FREE(HYD_ui_info.prepend_pattern);

    if (HYD_ui_info.outfile_pattern)
        HYDU_FREE(HYD_ui_info.outfile_pattern);

    if (HYD_ui_info.errfile_pattern)
        HYDU_FREE(HYD_ui_info.errfile_pattern);

    for (run = stdoe_fd_list; run;) {
        close(run->fd);
        tmp = run->next;
        HYDU_FREE(run);
        run = tmp;
    }

    /* Re-initialize everything to default values */
    HYD_uiu_init_params();
}

void HYD_uiu_print_params(void)
{
    struct HYD_env *env;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    int i;

    HYDU_FUNC_ENTER();

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "mpiexec options:\n");
    HYDU_dump_noprefix(stdout, "----------------\n");
    HYDU_dump_noprefix(stdout, "  Base path: %s\n", HYD_server_info.base_path);
    HYDU_dump_noprefix(stdout, "  Launcher: %s\n", HYD_server_info.user_global.launcher);
    HYDU_dump_noprefix(stdout, "  Debug level: %d\n", HYD_server_info.user_global.debug);
    HYDU_dump_noprefix(stdout, "  Enable X: %d\n", HYD_server_info.user_global.enablex);

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "  Global environment:\n");
    HYDU_dump_noprefix(stdout, "  -------------------\n");
    for (env = HYD_server_info.user_global.global_env.inherited; env; env = env->next)
        HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);

    if (HYD_server_info.user_global.global_env.system) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  Hydra internal environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------------\n");
        for (env = HYD_server_info.user_global.global_env.system; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    if (HYD_server_info.user_global.global_env.user) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  User set environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------\n");
        for (env = HYD_server_info.user_global.global_env.user; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_dump_noprefix(stdout, "    Proxy information:\n");
    HYDU_dump_noprefix(stdout, "    *********************\n");
    i = 1;
    for (proxy = HYD_server_info.pg_list.proxy_list; proxy; proxy = proxy->next) {
        HYDU_dump_noprefix(stdout, "      Proxy ID: %2d\n", i++);
        HYDU_dump_noprefix(stdout, "      -----------------\n");
        HYDU_dump_noprefix(stdout, "        Proxy name: %s\n", proxy->node.hostname);
        HYDU_dump_noprefix(stdout, "        Process count: %d\n", proxy->node.core_count);
        HYDU_dump_noprefix(stdout, "        Start PID: %d\n", proxy->start_pid);
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "        Proxy exec list:\n");
        HYDU_dump_noprefix(stdout, "        ....................\n");
        for (exec = proxy->exec_list; exec; exec = exec->next)
            HYDU_dump_noprefix(stdout, "          Exec: %s; Process count: %d\n",
                               exec->exec[0], exec->proc_count);
    }

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_FUNC_EXIT();

    return;
}

static HYD_status resolve_pattern_string(const char *pattern, char **str, int pgid, int proxy_id,
                                       int rank)
{
    int offset, i;
    char *tstr, *s_rank, *s_pgid, *s_proxy_id, *s_host, *s;
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    tstr = *str = HYDU_strdup(pattern);

    offset = 0;
    i = 0;
    do {
        s_rank = strstr(*str, "%r");
        s_pgid = strstr(*str, "%g");
        s_proxy_id = strstr(*str, "%p");
        s_host = strstr(*str, "%h");

        s = s_rank;
        if (s == NULL || (s_pgid && s_pgid < s))
            s = s_pgid;
        if (s == NULL || (s_proxy_id && s_proxy_id < s))
            s = s_proxy_id;
        if (s == NULL || (s_host && s_host < s))
            s = s_host;

        if (s)
            *s = 0;

        tmp[i++] = HYDU_strdup(*str);

        if (s) {
            if (s[1] == 'r') {
                HYDU_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", rank);
            }
            else if (s[1] == 'g') {
                HYDU_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", pgid);
            }
            else if (s[1] == 'p') {
                HYDU_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", proxy_id);
            }
            else if (s[1] == 'h') {
                for (pg = &HYD_server_info.pg_list; pg; pg = pg->next)
                    if (pg->pgid == pgid)
                        break;
                HYDU_ASSERT(pg, status);

                for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
                    if (proxy->proxy_id == proxy_id)
                        break;
                HYDU_ASSERT(proxy, status);

                HYDU_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%s", proxy->node.hostname);
            }
            else {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized pattern\n");
            }
            i++;

            *str = s + 2;
        }
        else
            *str = NULL;
    } while (*str);

    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, str);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status stdoe_cb(int _fd, int pgid, int proxy_id, int rank, void *_buf, int buflen)
{
    int fd = _fd;
    char *pattern_resolve, *pattern = NULL;
    struct stdoe_fd *tmp, *run;
    int sent, closed, mark, i;
    char *buf = (char *) _buf, *prepend;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pattern = (_fd == STDOUT_FILENO) ? HYD_ui_info.outfile_pattern :
        (_fd == STDERR_FILENO) ? HYD_ui_info.errfile_pattern : NULL;

    if (pattern) {
        /* See if the pattern already exists */
        status = resolve_pattern_string(pattern, &pattern_resolve, pgid, proxy_id, rank);

        for (run = stdoe_fd_list; run; run = run->next)
            if (!strcmp(run->pattern, pattern_resolve))
                break;

        if (run) {
            fd = run->fd;
            HYDU_FREE(pattern_resolve);
        }
        else {
            HYDU_MALLOC(tmp, struct stdoe_fd *, sizeof(struct stdoe_fd), status);
            tmp->pattern = pattern_resolve;
            tmp->fd = open(tmp->pattern, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            HYDU_ASSERT(tmp->fd >= 0, status);
            tmp->next = NULL;

            if (stdoe_fd_list == NULL)
                stdoe_fd_list = tmp;
            else {
                for (run = stdoe_fd_list; run->next; run = run->next);
                run->next = tmp;
            }

            fd = tmp->fd;
        }
    }

    if (HYD_ui_info.prepend_pattern == NULL) {
        status = HYDU_sock_write(fd, buf, buflen, &sent, &closed);
        HYDU_ERR_POP(status, "unable to write data to stdout/stderr\n");
        HYDU_ASSERT(!closed, status);
    }
    else {
        status = resolve_pattern_string(HYD_ui_info.prepend_pattern, &prepend, pgid, proxy_id,
                                      rank);
        HYDU_ERR_POP(status, "error resolving pattern\n");

        mark = 0;
        for (i = 0; i < buflen; i++) {
            if (buf[i] == '\n' || i == buflen - 1) {
                status = HYDU_sock_write(fd, (const void *) prepend, strlen(prepend), &sent,
                                         &closed);
                status = HYDU_sock_write(fd, (const void *) &buf[mark], i - mark + 1,
                                         &sent, &closed);
                HYDU_ERR_POP(status, "unable to write data to stdout/stderr\n");
                HYDU_ASSERT(!closed, status);
                mark = i + 1;
            }
        }

        HYDU_FREE(prepend);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uiu_stdout_cb(int pgid, int proxy_id, int rank, void *buf, int buflen)
{
    return stdoe_cb(STDOUT_FILENO, pgid, proxy_id, rank, buf, buflen);
}

HYD_status HYD_uiu_stderr_cb(int pgid, int proxy_id, int rank, void *buf, int buflen)
{
    return stdoe_cb(STDERR_FILENO, pgid, proxy_id, rank, buf, buflen);
}

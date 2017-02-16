/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "mpiexec.h"

static struct stdoe_fd {
    int fd;
    char *pattern;
    struct stdoe_fd *next;
} *stdoe_fd_list = NULL;

HYD_status mpiexec_alloc_pg(struct mpiexec_pg **pg, int pgid)
{
    struct mpiexec_pg *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_MALLOC(tmp, struct mpiexec_pg *, sizeof(struct mpiexec_pg), status);

    tmp->pgid = pgid;

    tmp->node_count = -1;
    tmp->node_list = NULL;

    tmp->total_proc_count = -1;
    tmp->exec_list = NULL;

    tmp->num_downstream = -1;
    tmp->downstream.fd_stdin = NULL;
    tmp->downstream.fd_stdout = NULL;
    tmp->downstream.fd_stderr = NULL;
    tmp->downstream.fd_control = NULL;
    tmp->downstream.proxy_id = NULL;
    tmp->downstream.pid = NULL;
    tmp->downstream.kvcache = NULL;
    tmp->downstream.kvcache_size = NULL;
    tmp->downstream.kvcache_num_blocks = NULL;

    tmp->barrier_count = 0;

    tmp->pmi_process_mapping = NULL;

    MPL_HASH_ADD_INT(mpiexec_pg_hash, pgid, tmp);

    *pg = tmp;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

void mpiexec_free_params(void)
{
    struct stdoe_fd *tmp, *run;

    if (mpiexec_params.base_path)
        MPL_free(mpiexec_params.base_path);

    if (mpiexec_params.port_range)
        MPL_free(mpiexec_params.port_range);

    if (mpiexec_params.nameserver)
        MPL_free(mpiexec_params.nameserver);

    if (mpiexec_params.localhost)
        MPL_free(mpiexec_params.localhost);

    if (mpiexec_params.prepend_pattern)
        MPL_free(mpiexec_params.prepend_pattern);

    if (mpiexec_params.outfile_pattern)
        MPL_free(mpiexec_params.outfile_pattern);

    if (mpiexec_params.errfile_pattern)
        MPL_free(mpiexec_params.errfile_pattern);

    for (run = stdoe_fd_list; run;) {
        close(run->fd);
        tmp = run->next;
        MPL_free(run);
        run = tmp;
    }

    if (mpiexec_params.rmk)
        MPL_free(mpiexec_params.rmk);

    if (mpiexec_params.launcher)
        MPL_free(mpiexec_params.launcher);

    if (mpiexec_params.launcher_exec)
        MPL_free(mpiexec_params.launcher_exec);

    if (mpiexec_params.binding)
        MPL_free(mpiexec_params.binding);
}

void mpiexec_print_params(void)
{
    struct HYD_env *env;

    HYD_FUNC_ENTER();

    HYD_PRINT_NOPREFIX(stdout, "\n");
    HYD_PRINT_NOPREFIX(stdout, "=================================================");
    HYD_PRINT_NOPREFIX(stdout, "=================================================");
    HYD_PRINT_NOPREFIX(stdout, "\n");
    HYD_PRINT_NOPREFIX(stdout, "mpiexec options:\n");
    HYD_PRINT_NOPREFIX(stdout, "----------------\n");
    HYD_PRINT_NOPREFIX(stdout, "  Base path: %s\n", mpiexec_params.base_path);
    HYD_PRINT_NOPREFIX(stdout, "  Launcher: %s\n", mpiexec_params.launcher);
    HYD_PRINT_NOPREFIX(stdout, "  Debug level: %d\n", mpiexec_params.debug);

    HYD_PRINT_NOPREFIX(stdout, "\n");
    HYD_PRINT_NOPREFIX(stdout, "  Primary environment:\n");
    HYD_PRINT_NOPREFIX(stdout, "  -------------------\n");
    for (env = mpiexec_params.primary.list; env; env = env->next)
        HYD_PRINT_NOPREFIX(stdout, "    %s=%s\n", env->env_name, env->env_value);

    HYD_PRINT_NOPREFIX(stdout, "\n");
    HYD_PRINT_NOPREFIX(stdout, "  Secondary environment:\n");
    HYD_PRINT_NOPREFIX(stdout, "  ---------==----------\n");
    for (env = mpiexec_params.secondary.list; env; env = env->next)
        HYD_PRINT_NOPREFIX(stdout, "    %s=%s\n", env->env_name, env->env_value);

    HYD_PRINT_NOPREFIX(stdout, "\n\n");

    HYD_PRINT_NOPREFIX(stdout, "\n");
    HYD_PRINT_NOPREFIX(stdout, "=================================================");
    HYD_PRINT_NOPREFIX(stdout, "=================================================");
    HYD_PRINT_NOPREFIX(stdout, "\n\n");

    HYD_FUNC_EXIT();

    return;
}

static HYD_status resolve_pattern_string(const char *pattern, char **str, int pgid, int proxy_id,
                                         int rank)
{
    HYD_status status = HYD_SUCCESS;
    int i, pos, tpos;
    char *tmp[HYD_NUM_TMP_STRINGS] = { NULL };
    struct mpiexec_pg *pg;

    HYD_FUNC_ENTER();

    *str = NULL;
    tpos = 0;
    pos = 0;
    i = 0;
    HYD_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
    tmp[i][0] = '\0';

    while (1) {
        HYD_ASSERT(tpos < HYD_TMP_STRLEN, status);
        if (pattern[pos] != '%') {
            tmp[i][tpos++] = pattern[pos++];
            if (pattern[pos - 1] == '\0')
                break;
        }
        else {
            ++pos;      /* consume '%' */

            if (pattern[pos] == '%') {
                tmp[i][tpos++] = pattern[pos++];
                continue;
            }

            /* all remaining valid specifiers need a new temp string */
            tmp[i][tpos] = '\0';
            ++i;
            tpos = 0;
            HYD_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
            tmp[i][0] = '\0';

            switch (pattern[pos]) {
            case 'r':
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", rank);
                break;
            case 'g':
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", pgid);
                break;
            case 'p':
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%d", proxy_id);
                break;
            case 'h':
                MPL_HASH_FIND_INT(mpiexec_pg_hash, &pgid, pg);
                HYD_ASSERT(pg, status);
                MPL_snprintf(tmp[i], HYD_TMP_STRLEN, "%s", pg->node_list[proxy_id].hostname);
                break;
            case '\0':
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "dangling '%%' at end of pattern\n");
            default:
                HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL,
                                   "unrecognized pattern specifier ('%c')\n", pattern[pos]);
                break;
            }

            ++pos;      /* skip past fmt specifier */

            ++i;
            tpos = 0;
            HYD_MALLOC(tmp[i], char *, HYD_TMP_STRLEN, status);
            tmp[i][0] = '\0';
        }
    }

    tmp[++i] = NULL;
    status = HYD_str_alloc_and_join(tmp, str);
    HYD_ERR_POP(status, "unable to join strings\n");

  fn_exit:
    HYD_str_free_list(tmp);
    HYD_FUNC_EXIT();
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

    HYD_FUNC_ENTER();

    pattern =
        (_fd == STDOUT_FILENO) ? mpiexec_params.outfile_pattern : (_fd ==
                                                                   STDERR_FILENO) ? mpiexec_params.
        errfile_pattern : NULL;

    if (pattern) {
        /* See if the pattern already exists */
        status = resolve_pattern_string(pattern, &pattern_resolve, pgid, proxy_id, rank);
        HYD_ERR_POP(status, "error resolving pattern\n");

        for (run = stdoe_fd_list; run; run = run->next)
            if (!strcmp(run->pattern, pattern_resolve))
                break;

        if (run) {
            fd = run->fd;
            MPL_free(pattern_resolve);
        }
        else {
            HYD_MALLOC(tmp, struct stdoe_fd *, sizeof(struct stdoe_fd), status);
            tmp->pattern = pattern_resolve;
            tmp->fd = open(tmp->pattern, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            HYD_ASSERT(tmp->fd >= 0, status);
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

    if (mpiexec_params.prepend_pattern == NULL) {
        status = HYD_sock_write(fd, buf, buflen, &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "unable to write data to stdout/stderr\n");
        HYD_ASSERT(!closed, status);
    }
    else {
        status =
            resolve_pattern_string(mpiexec_params.prepend_pattern, &prepend, pgid, proxy_id, rank);
        HYD_ERR_POP(status, "error resolving pattern\n");

        mark = 0;
        for (i = 0; i < buflen; i++) {
            if (buf[i] == '\n' || i == buflen - 1) {
                if (prepend[0] != '\0') {       /* sock_write barfs on maxlen==0 */
                    status =
                        HYD_sock_write(fd, (const void *) prepend, strlen(prepend), &sent, &closed,
                                       HYD_SOCK_COMM_TYPE__BLOCKING);
                    HYD_ERR_POP(status, "unable to write data to stdout/stderr\n");
                }
                status =
                    HYD_sock_write(fd, (const void *) &buf[mark], i - mark + 1, &sent, &closed,
                                   HYD_SOCK_COMM_TYPE__BLOCKING);
                HYD_ERR_POP(status, "unable to write data to stdout/stderr\n");
                HYD_ASSERT(!closed, status);
                mark = i + 1;
            }
        }

        MPL_free(prepend);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status mpiexec_stdout_cb(int pgid, int proxy_id, int rank, void *buf, int buflen)
{
    return stdoe_cb(STDOUT_FILENO, pgid, proxy_id, rank, buf, buflen);
}

HYD_status mpiexec_stderr_cb(int pgid, int proxy_id, int rank, void *buf, int buflen)
{
    return stdoe_cb(STDERR_FILENO, pgid, proxy_id, rank, buf, buflen);
}

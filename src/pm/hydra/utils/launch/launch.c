/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"

HYD_status HYDU_create_process(char **client_arg, struct HYD_env *env_list,
                               int *in, int *out, int *err, int *pid, int idx)
{
    int inpipe[2], outpipe[2], errpipe[2], tpid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in && (pipe(inpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (out && (pipe(outpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (err && (pipe(errpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */

#if defined HAVE_SETSID
        setsid();
#endif /* HAVE_SETSID */

        close(STDIN_FILENO);
        if (in) {
            close(inpipe[1]);
            if (inpipe[0] != STDIN_FILENO && dup2(inpipe[0], STDIN_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        close(STDOUT_FILENO);
        if (out) {
            close(outpipe[0]);
            if (outpipe[1] != STDOUT_FILENO && dup2(outpipe[1], STDOUT_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        close(STDERR_FILENO);
        if (err) {
            close(errpipe[0]);
            if (errpipe[1] != STDERR_FILENO && dup2(errpipe[1], STDERR_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        /* Forced environment overwrites existing environment */
        if (env_list) {
            status = HYDU_putenv_list(env_list, HYD_ENV_OVERWRITE_TRUE);
            HYDU_ERR_POP(status, "unable to putenv\n");
        }

        if (idx >= 0) {
            status = HYDT_topo_bind(idx);
            HYDU_ERR_POP(status, "bind process failed\n");
        }

        if (execvp(client_arg[0], client_arg) < 0) {
            /* The child process should never get back to the proxy
             * code; if there is an error, just throw it here and
             * exit. */
            HYDU_error_printf("execvp error on file %s (%s)\n", client_arg[0], MPL_strerror(errno));
            exit(-1);
        }
    }
    else {      /* Parent process */
        if (out)
            close(outpipe[1]);
        if (err)
            close(errpipe[1]);
        if (in) {
            close(inpipe[0]);
            *in = inpipe[1];

            status = HYDU_sock_cloexec(*in);
            HYDU_ERR_POP(status, "unable to set close on exec\n");
        }
        if (out)
            *out = outpipe[0];
        if (err)
            *err = errpipe[0];
    }

    if (pid)
        *pid = tpid;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

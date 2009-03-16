/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid)
{
    int inpipe[2], outpipe[2], errpipe[2], tpid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in && (pipe(inpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (errno: %d)\n", errno);

    if (out && (pipe(outpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (errno: %d)\n", errno);

    if (err && (pipe(errpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (errno: %d)\n", errno);

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */
        close(outpipe[0]);
        close(1);
        if (dup2(outpipe[1], 1) < 0)
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (errno: %d)\n", errno);

        close(errpipe[0]);
        close(2);
        if (dup2(errpipe[1], 2) < 0)
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (errno: %d)\n", errno);

        close(inpipe[1]);
        close(0);
        if (in && (dup2(inpipe[0], 0) < 0))
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (errno: %d)\n", errno);

        if (execvp(client_arg[0], client_arg) < 0) {
            if (errno == ENOENT) {
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                     "execvp error: file %s not found\n", client_arg[0]);
            }
            else {
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                     "execvp error (errno: %d)\n", errno);
            }
        }
    }
    else {      /* Parent process */
        close(outpipe[1]);
        close(errpipe[1]);
        if (in)
            *in = inpipe[1];
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

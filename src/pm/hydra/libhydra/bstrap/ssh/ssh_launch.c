/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_bstrap_ssh.h"
#include "ssh_internal.h"
#include "hydra_str.h"
#include "hydra_err.h"
#include "hydra_fs.h"
#include "hydra_spawn.h"

HYD_status HYDI_bstrap_ssh_launch(const char *hostname, const char *launch_exec, char **args,
                                  int *fd_stdin, int *fd_stdout, int *fd_stderr, int *pid,
                                  int debug)
{
    char *targs[HYD_NUM_TMP_STRINGS] = { NULL };
    int idx, i;
    char *lexec = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (launch_exec)
        lexec = MPL_strdup(launch_exec);
    if (lexec == NULL)
        lexec = HYD_find_full_path("ssh");
    if (lexec == NULL)
        lexec = MPL_strdup("/usr/bin/ssh");
    HYD_ASSERT(lexec, status);

    idx = 0;
    targs[idx++] = MPL_strdup(lexec);
    targs[idx++] = MPL_strdup("-x");
    targs[idx++] = MPL_strdup(hostname);
    for (i = 0; args[i]; i++)
        targs[idx++] = MPL_strdup(args[i]);
    targs[idx++] = NULL;

    if (debug) {
        HYD_PRINT(stdout, "Launch arguments: ");
        HYD_str_print_list(targs);
    }

    /* ssh has many types of security controls that do not allow a
     * user to ssh to the same node multiple times very quickly. If
     * this happens, the ssh daemons disables ssh connections causing
     * the job to fail. This is basically a hack to slow down ssh
     * connections to the same node. We check for offset == 0 before
     * applying this hack, so we only slow down the cases where ssh is
     * being used, and not the cases where we fall back to fork. */
    status = HYDI_ssh_store_launch_time(hostname);
    HYD_ERR_POP(status, "error storing launch time\n");

    /* The stdin pointer is a dummy value. We don't just pass it NULL,
     * as older versions of ssh seem to freak out when no stdin socket
     * is provided. */
    status = HYD_spawn(targs, NULL, fd_stdin, fd_stdout, fd_stderr, pid, -1);
    HYD_ERR_POP(status, "create process returned error\n");

  fn_exit:
    HYD_str_free_list(targs);
    if (lexec)
        MPL_free(lexec);
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

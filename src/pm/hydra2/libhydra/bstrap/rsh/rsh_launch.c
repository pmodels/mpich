/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "hydra_bstrap_rsh.h"
#include "hydra_str.h"
#include "hydra_err.h"
#include "hydra_fs.h"
#include "hydra_spawn.h"


HYD_status HYDI_bstrap_rsh_launch(const char *hostname, const char *launch_exec, char **args,
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
        lexec = HYD_find_full_path("rsh");
    if (lexec == NULL)
        lexec = MPL_strdup("/usr/bin/rsh");
    HYD_ASSERT(lexec, status);

    idx = 0;
    targs[idx++] = MPL_strdup(lexec);

    targs[idx++] = MPL_strdup(hostname);

    /* Fill in the remaining arguments */
    /* We do not need to create a quoted version of the string for
     * SLURM. It seems to be internally quoting it anyway. */
    for (i = 0; args[i]; i++)
        targs[idx++] = MPL_strdup(args[i]);

    if (debug) {
        HYD_PRINT(stdout, "Launch arguments: ");
        HYD_str_print_list(targs);
    }


    status = HYD_spawn(targs, 0, NULL, fd_stdin, fd_stdout, fd_stderr, pid, -1);
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

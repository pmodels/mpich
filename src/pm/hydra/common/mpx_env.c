/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"

HYD_status MPX_primary_env_bcast(struct MPX_cmd cmd, int upstream_fd,
                                 struct HYD_int_hash *downstream_fd_hash, int envcount,
                                 char **env)
{
    int len;
    void *buf;
    HYD_status status = HYD_SUCCESS;

    if (upstream_fd == -1) {
        /* root process needs to do some extra work to setup the cmd
         * and serialize the environment into a buffer */
        if (envcount)
            MPL_args_serialize(envcount, env, &len, &buf);

        MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
        cmd.type = MPX_CMD_TYPE__PRIMARY_ENV;
        cmd.data_len = len;
    }

    status = MPX_bcast(cmd, upstream_fd, downstream_fd_hash, buf);
    HYD_ERR_POP(status, "error bcasting env\n");

    MPL_free(buf);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

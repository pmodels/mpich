/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"

HYD_status MPX_bcast(struct MPX_cmd cmd, int upstream_fd, struct HYD_int_hash *downstream_fd_hash,
                     void *buf)
{
    struct HYD_int_hash *hash, *tmp;
    int sent, recvd, closed;
    HYD_status status = HYD_SUCCESS;

    if (upstream_fd != -1) {  /* non-root process */
        /* the command was already read by the proxy, we just need to
         * get the data */
        if (cmd.data_len) {
            status =
                HYD_sock_read(upstream_fd, buf, cmd.data_len, &recvd, &closed,
                              HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error reading command from launcher\n");
            HYD_ASSERT(!closed, status);
        }
    }

    MPL_HASH_ITER(hh, downstream_fd_hash, hash, tmp) {
        /* the command just got created, send it first */
        status =
            HYD_sock_write(hash->key, &cmd, sizeof(cmd), &sent, &closed,
                           HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading command\n");
        HYD_ASSERT(!closed, status);

        /* if there is data to be sent, send it */
        if (cmd.data_len) {
            status =
                HYD_sock_write(hash->key, buf, cmd.data_len, &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error reading command\n");
            HYD_ASSERT(!closed, status);
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

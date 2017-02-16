/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "mpl_uthash.h"
#include "mpiexec.h"

HYD_status mpiexec_pmi_barrier(struct mpiexec_pg *pg)
{
    int i, j;
    struct MPX_cmd cmd;
    int sent, closed;
    int kvcache_num_blocks, kvcache_size;
    HYD_status status = HYD_SUCCESS;

    pg->barrier_count++;

    if (pg->barrier_count == pg->num_downstream) {
        pg->barrier_count = 0;

        kvcache_num_blocks = 0;
        kvcache_size = 0;
        for (i = 0; i < pg->num_downstream; i++) {
            kvcache_num_blocks += pg->downstream.kvcache_num_blocks[i];
            kvcache_size += pg->downstream.kvcache_size[i];
        }

        MPL_VG_MEM_INIT(&cmd, sizeof(cmd));
        for (i = 0; i < pg->num_downstream; i++) {
            if (kvcache_num_blocks) {
                cmd.type = MPX_CMD_TYPE__KVCACHE_OUT;
                cmd.u.kvcache.pgid = pg->pgid;  /* FIXME: redundant */
                cmd.u.kvcache.num_blocks = kvcache_num_blocks;
                cmd.data_len = kvcache_size;

                status =
                    HYD_sock_write(pg->downstream.fd_control[i], &cmd, sizeof(cmd), &sent, &closed,
                                   HYD_SOCK_COMM_TYPE__BLOCKING);
                HYD_ERR_POP(status, "error sending kvcache cmd downstream\n");

                /* send the lengths first */
                for (j = 0; j < pg->num_downstream; j++) {
                    if (pg->downstream.kvcache_num_blocks[j]) {
                        status =
                            HYD_sock_write(pg->downstream.fd_control[i], pg->downstream.kvcache[j],
                                           2 * pg->downstream.kvcache_num_blocks[j] * sizeof(int),
                                           &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
                        HYD_ERR_POP(status, "error sending kvcache cmd downstream\n");
                    }
                }

                /* now send the actual caches */
                for (j = 0; j < pg->num_downstream; j++) {
                    if (pg->downstream.kvcache_num_blocks[j]) {
                        status =
                            HYD_sock_write(pg->downstream.fd_control[i],
                                           ((char *) pg->downstream.kvcache[j]) +
                                           2 * pg->downstream.kvcache_num_blocks[j] * sizeof(int),
                                           pg->downstream.kvcache_size[j] -
                                           2 * pg->downstream.kvcache_num_blocks[j] * sizeof(int),
                                           &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
                        HYD_ERR_POP(status, "error sending kvcache cmd downstream\n");
                    }
                }
            }

            /* send out the actual barrier_out */
            cmd.type = MPX_CMD_TYPE__PMI_BARRIER_OUT;
            status =
                HYD_sock_write(pg->downstream.fd_control[i], &cmd, sizeof(cmd), &sent, &closed,
                               HYD_SOCK_COMM_TYPE__BLOCKING);
            HYD_ERR_POP(status, "error sending barrier out downstream\n");
        }

        if (kvcache_num_blocks) {
            for (i = 0; i < pg->num_downstream; i++) {
                pg->downstream.kvcache_num_blocks[i] = 0;
                pg->downstream.kvcache_size[i] = 0;
                MPL_free(pg->downstream.kvcache[i]);
                pg->downstream.kvcache[i] = NULL;
            }
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

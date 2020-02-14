/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "rndv.h"

/* These are defined in rndv.h */
lmt_rndv_copy_buf_t **rndv_send_copy_bufs;
lmt_rndv_copy_buf_t **rndv_recv_copy_bufs;
MPL_shm_hnd_t *rndv_send_copy_buf_handles;
MPL_shm_hnd_t *rndv_recv_copy_buf_handles;
lmt_rndv_queue_t *rndv_send_queues;
lmt_rndv_queue_t *rndv_recv_queues;

/* Initialize all of the state necesary for the POSIX/LMT/RNDV module. Neither of the two inputs are
 * used.
 *
 * rank - The global rank of the calling process.
 * size - The number of processes in MPI_COMM_WORLD
 */
int MPIDI_POSIX_lmt_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_CHKPMEM_DECL(6);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_LMT_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_INIT);

    /* Allocate space for the copy buffer pointers and handles. The structs themselves will be
     * allocated on demand later. All of these _could_ be sized at num_local-1 instead of num_local
     * since this code won't be used for loopback messages, but it would make tracking the ranks
     * much more difficult so it's much simpler to just have one extra that doesn't get used. */
    MPIR_CHKPMEM_MALLOC(rndv_send_copy_bufs, lmt_rndv_copy_buf_t **,
                        sizeof(lmt_rndv_copy_buf_t *) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV send copy buffers", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(rndv_recv_copy_bufs, lmt_rndv_copy_buf_t **,
                        sizeof(lmt_rndv_copy_buf_t *) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV recv copy buffers", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(rndv_send_copy_buf_handles, MPL_shm_hnd_t *,
                        sizeof(MPL_shm_hnd_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV send copy buffer handles", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(rndv_recv_copy_buf_handles, MPL_shm_hnd_t *,
                        sizeof(MPL_shm_hnd_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV recv copy buffer handles", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(rndv_send_queues, lmt_rndv_queue_t *,
                        sizeof(lmt_rndv_queue_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV send wait event queues", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(rndv_recv_queues, lmt_rndv_queue_t *,
                        sizeof(lmt_rndv_queue_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "LMT RNDV recv wait event queues", MPL_MEM_SHM);

    for (i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        mpi_errno = MPL_shm_hnd_init(&rndv_send_copy_buf_handles[i]);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPL_shm_hnd_init(&rndv_recv_copy_buf_handles[i]);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        rndv_send_copy_bufs[i] = NULL;
        rndv_recv_copy_bufs[i] = NULL;
        rndv_send_queues[i].head = NULL;
        rndv_send_queues[i].tail = NULL;
        rndv_recv_queues[i].head = NULL;
        rndv_recv_queues[i].tail = NULL;
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_LMT_INIT);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* Clean up the state of the POSIX/LMT/RNDV module. */
int MPIDI_POSIX_lmt_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_LMT_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_FINALIZE);

    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        if (rndv_send_copy_bufs[i]) {
            mpi_errno = MPL_shm_seg_detach(rndv_send_copy_buf_handles[i],
                                           (void **) &rndv_send_copy_bufs[i],
                                           sizeof(lmt_rndv_copy_buf_t));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        if (rndv_recv_copy_bufs[i]) {
            mpi_errno = MPL_shm_seg_detach(rndv_recv_copy_buf_handles[i],
                                           (void **) &rndv_recv_copy_bufs[i],
                                           sizeof(lmt_rndv_copy_buf_t));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        mpi_errno = MPL_shm_hnd_finalize(&rndv_send_copy_buf_handles[i]);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPL_shm_hnd_finalize(&rndv_recv_copy_buf_handles[i]);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPL_free(rndv_send_copy_bufs);
    MPL_free(rndv_recv_copy_bufs);
    MPL_free(rndv_send_copy_buf_handles);
    MPL_free(rndv_recv_copy_buf_handles);
    MPL_free(rndv_send_queues);
    MPL_free(rndv_recv_queues);

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

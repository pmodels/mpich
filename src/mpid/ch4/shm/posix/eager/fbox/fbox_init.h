/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_FBOX_INIT_H_INCLUDED
#define POSIX_EAGER_FBOX_INIT_H_INCLUDED

#include "fbox_types.h"

#define MPIDI_POSIX_MAILBOX_INDEX(sender, receiver) ((num_local) * (sender) + (receiver))

extern MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global;

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;

    int i, num_local = 0, local_rank_0 = -1, my_local_rank = -1;

    MPIDI_POSIX_fastbox_t *fastboxes_p = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(5);

    MPIDI_POSIX_eager_fbox_control_global.num_seg = 1;
    MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank = 0;

    /* Populate these values with transformation information about each rank and its original
     * information in MPI_COMM_WORLD. */

    mpi_errno = MPIR_Find_local(MPIR_Process.comm_world, &num_local, &my_local_rank,
                                /* comm_world rank of each local process */
                                &MPIDI_POSIX_eager_fbox_control_global.local_procs,
                                /* local rank of each process in comm_world if it is on the same node */
                                &MPIDI_POSIX_eager_fbox_control_global.local_ranks);

    local_rank_0 = MPIDI_POSIX_eager_fbox_control_global.local_procs[0];
    MPIDI_POSIX_eager_fbox_control_global.num_local = num_local;
    MPIDI_POSIX_eager_fbox_control_global.my_local_rank = my_local_rank;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.seg,
                        MPIDU_shm_seg_info_t *,
                        MPIDI_POSIX_eager_fbox_control_global.num_seg *
                        sizeof(MPIDU_shm_seg_info_t), mpi_errno, "mem_region segments",
                        MPL_MEM_SHM);

    /* Create region with one fastbox for every pair of local processes. */
    mpi_errno =
        MPIDU_shm_seg_alloc(num_local * num_local * sizeof(MPIDI_POSIX_fastbox_t),
                            (void **) &fastboxes_p, MPL_MEM_SHM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Request shared collective barrier vars region */
    mpi_errno = MPIDU_shm_seg_alloc(MAX(sizeof(MPIDU_shm_barrier_t), MPIDU_SHM_CACHE_LINE_LEN),
                                    (void **) &MPIDI_POSIX_eager_fbox_control_global.barrier_region,
                                    MPL_MEM_SHM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Actually allocate the segment and assign regions to the pointers */
    mpi_errno = MPIDU_shm_seg_commit(&MPIDI_POSIX_eager_fbox_control_global.memory,
                                     &MPIDI_POSIX_eager_fbox_control_global.barrier,
                                     num_local, my_local_rank, local_rank_0, rank, MPL_MEM_SHM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Allocate table of pointers to fastboxes */
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes", MPL_MEM_SHM);

    /* Fill in fbox tables */
    for (i = 0; i < num_local; i++) {
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i] =
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(i, my_local_rank)];
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(my_local_rank, i)];

        memset(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i], 0,
               sizeof(MPIDI_POSIX_fastbox_t));
    }

    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_eager_fbox_control_global.barrier, num_local);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);

    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_eager_fbox_control_global.barrier,
                                  MPIDI_POSIX_eager_fbox_control_global.num_local);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPL_free(MPIDI_POSIX_eager_fbox_control_global.seg);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.local_ranks);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.local_procs);

    mpi_errno = MPIDU_shm_seg_destroy(&MPIDI_POSIX_eager_fbox_control_global.memory,
                                      MPIDI_POSIX_eager_fbox_control_global.num_local);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_EAGER_FBOX_INIT_H_INCLUDED */

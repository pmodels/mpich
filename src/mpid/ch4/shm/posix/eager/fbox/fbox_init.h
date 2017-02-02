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

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;

    int i, num_local = 0, local_rank_0 = -1, local_rank = -1;
    int *local_ranks, *local_procs;
    MPIDI_av_entry_t *av = NULL;

    MPIDI_POSIX_fastbox_t *fastboxes_p = NULL;

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(5);

    MPIDI_POSIX_eager_fbox_control_global.num_seg = 1;
    MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank = 0;

    MPIR_CHKPMEM_MALLOC(local_procs, int *, size * sizeof(int), mpi_errno,
                        "local process index array", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(local_ranks, int *, size * sizeof(int), mpi_errno,
                        "mem_region local ranks", MPL_MEM_SHM);
    for (i = 0; i < size; i++) {
        av = MPIDIU_comm_rank_to_av(MPIR_Process.comm_world, i);
        if (MPIDI_av_is_local(av)) {
            if (i == rank) {
                local_rank = num_local;
            }

            if (local_rank_0 == -1)
                local_rank_0 = i;

            local_procs[num_local] = i;
            local_ranks[i] = num_local;
            num_local++;
        }
    }
    MPIDI_POSIX_eager_fbox_control_global.num_local = num_local;
    MPIDI_POSIX_eager_fbox_control_global.local_rank = local_rank;
    MPIDI_POSIX_eager_fbox_control_global.local_ranks = local_ranks;
    MPIDI_POSIX_eager_fbox_control_global.local_procs = local_procs;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.seg,
                        MPIDU_shm_seg_info_t *,
                        MPIDI_POSIX_eager_fbox_control_global.num_seg *
                        sizeof(MPIDU_shm_seg_info_t), mpi_errno, "mem_region segments",
                        MPL_MEM_SHM);

    /* Request fastboxes region */

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
                                     num_local, local_rank, local_rank_0, rank, MPL_MEM_SHM);

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
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(i, local_rank)];
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(local_rank, i)];

        memset(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i], 0,
               sizeof(MPIDI_POSIX_fastbox_t));

        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i]->flag = 0;
    }

    /* Run local procs barrier */

    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_eager_fbox_control_global.barrier, num_local);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_finalize()
{
    int mpi_errno = MPI_SUCCESS;

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_threshold()
{
    return MPIDI_POSIX_FBOX_THRESHOLD;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_connect(int grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_listen(int *grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_accept(int grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_EAGER_FBOX_INIT_H_INCLUDED */

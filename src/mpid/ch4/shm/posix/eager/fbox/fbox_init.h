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
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int grank, int num_local)
{
    int mpi_errno = MPI_SUCCESS;

    int i;

    MPIDI_POSIX_fastbox_t *fastboxes_p = NULL;

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(3);

    MPIDI_POSIX_eager_fbox_control_global.num_seg = 1;
    MPIDI_POSIX_eager_fbox_control_global.my_rank = rank;
    MPIDI_POSIX_eager_fbox_control_global.my_grank = grank;
    MPIDI_POSIX_eager_fbox_control_global.num_local = num_local;

    MPIDI_POSIX_eager_fbox_control_global.last_polled_grank = 0;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.seg,
                        MPIDU_shm_seg_info_ptr_t,
                        MPIDI_POSIX_eager_fbox_control_global.num_seg *
                        sizeof(MPIDU_shm_seg_info_t), mpi_errno, "mem_region segments");

    /* Request fastboxes region */

    mpi_errno =
        MPIDU_shm_seg_alloc(num_local * num_local * sizeof(MPIDI_POSIX_fastbox_t),
                            (void **) &fastboxes_p);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Request shared collective barrier vars region */

    mpi_errno = MPIDU_shm_seg_alloc(MAX(sizeof(MPIDU_shm_barrier_t), MPIDU_SHM_CACHE_LINE_LEN),
                                    (void **) &MPIDI_POSIX_eager_fbox_control_global.
                                    barrier_region);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Actually allocate the segment and assign regions to the pointers */

    mpi_errno = MPIDU_shm_seg_commit(&MPIDI_POSIX_eager_fbox_control_global.memory,
                                     &MPIDI_POSIX_eager_fbox_control_global.barrier,
                                     num_local, grank, 0, rank);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Allocate table of pointers to fastboxes */

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes");
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes");

    /* Fill in fbox tables */

    for (i = 0; i < num_local; i++) {
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i] =
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(i, grank)];
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            &fastboxes_p[MPIDI_POSIX_MAILBOX_INDEX(grank, i)];

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

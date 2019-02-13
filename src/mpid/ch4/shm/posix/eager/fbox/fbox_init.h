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

#define MPIDI_POSIX_SEGMENT_INDEX(receiver) (rank_to_seg[receiver])
#define MPIDI_POSIX_MAILBOX_INDEX(sender,receiver) (((num_in_seg[receiver]) * (num_local)) + (sender))

extern MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global;

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;

    int i, num_local = 0, local_rank_0 = -1, my_local_rank = -1;
    int *local_ranks, *local_procs;
    MPIDI_av_entry_t *av = NULL;

    MPIDI_POSIX_fastbox_t **fastboxes_p = NULL;
    int seg_to_nodeid[MPIDU_SHM_MAX_NUMA_NUM];
    int nodeid_to_seg[MPIDU_SHM_MAX_NUMA_NUM];
    int num_seg = 0;
    int *num_in_seg = NULL;
    int *ranks_per_seg = NULL;
    int *rank_to_seg = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(7);

    /* Get number of shared memory segments & map segment/node to node/segment */
    for (i = 0; i < MPIDU_shm_numa_info.nnodes; i++) {
        if (MPIDU_shm_numa_info.bitmap & (1 << i)) {
            seg_to_nodeid[num_seg] = i;
            nodeid_to_seg[i] = num_seg;
            num_seg++;
        }
    }

    MPIDI_POSIX_eager_fbox_control_global.num_seg = num_seg;
    MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank = 0;

    MPIR_CHKPMEM_MALLOC(local_procs, int *, size * sizeof(int), mpi_errno,
                        "local process index array", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(local_ranks, int *, size * sizeof(int), mpi_errno,
                        "mem_region local ranks", MPL_MEM_SHM);

    /* Populate these values with transformation information about each rank and its original
     * information in MPI_COMM_WORLD. */
    for (i = 0; i < size; i++) {
        av = MPIDIU_comm_rank_to_av(MPIR_Process.comm_world, i);
        if (MPIDI_av_is_local(av)) {
            if (i == rank) {
                my_local_rank = num_local;
            }

            if (local_rank_0 == -1)
                local_rank_0 = i;

            local_procs[num_local] = i;
            local_ranks[i] = num_local;
            num_local++;
        }
    }
    MPIDI_POSIX_eager_fbox_control_global.num_local = num_local;
    MPIDI_POSIX_eager_fbox_control_global.my_local_rank = my_local_rank;
    MPIDI_POSIX_eager_fbox_control_global.local_ranks = local_ranks;
    MPIDI_POSIX_eager_fbox_control_global.local_procs = local_procs;

    /* Allocate memory for fastboxes and mapping info */
    MPIR_CHKLMEM_DECL(4);
    MPIR_CHKLMEM_MALLOC(fastboxes_p, MPIDI_POSIX_fastbox_t **,
                        num_seg * sizeof(MPIDI_POSIX_fastbox_t), mpi_errno, "fbox pointers",
                        MPL_MEM_OTHER);
    MPIR_CHKLMEM_MALLOC(ranks_per_seg, int *, num_seg * sizeof(int), mpi_errno,
                        "processes per segment", MPL_MEM_OTHER);
    MPIR_CHKLMEM_MALLOC(rank_to_seg, int *, num_local * sizeof(int),
                        mpi_errno, "rank to segment map", MPL_MEM_OTHER);
    MPIR_CHKLMEM_MALLOC(num_in_seg, int *, num_local * sizeof(int),
                        mpi_errno, "my number in segment", MPL_MEM_OTHER);

    /* Populate rank to segment mapping info */
    memset(ranks_per_seg, 0, sizeof(int) * num_seg);
    for (i = 0; i < num_local; i++) {
        rank_to_seg[i] = nodeid_to_seg[MPIDU_shm_numa_info.nodeid[i]];
        num_in_seg[i] = ranks_per_seg[rank_to_seg[i]]++;
    }

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.memory,
                        MPIDU_shm_seg_t *,
                        num_seg * sizeof(MPIDU_shm_seg_t), mpi_errno, "mem_region", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.barrier_region,
                        void **, num_seg * sizeof(void *), mpi_errno, "barrier_region",
                        MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.seg, MPIDU_shm_seg_info_t *,
                        MPIDI_POSIX_eager_fbox_control_global.num_seg *
                        sizeof(MPIDU_shm_seg_info_t), mpi_errno, "mem_region segments",
                        MPL_MEM_SHM);

    for (i = 0; i < num_seg; i++) {
        /* Create region with one fastbox for every pair of local processes. */
        mpi_errno =
            MPIDU_shm_seg_alloc(ranks_per_seg[i] * num_local * sizeof(MPIDI_POSIX_fastbox_t),
                                (void **) &fastboxes_p[i],
                                MPIDU_shm_obj_info.object_type[MPIDU_SHM_OBJ__FASTBOXES],
                                MPL_MEM_SHM);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* Request shared collective barrier vars region */
        mpi_errno = MPIDU_shm_seg_alloc(MAX(sizeof(MPIDU_shm_barrier_t), MPIDU_SHM_CACHE_LINE_LEN),
                                        (void **)
                                        &MPIDI_POSIX_eager_fbox_control_global.barrier_region[i],
                                        MPIR_MEMTYPE__DEFAULT, MPL_MEM_SHM);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* Actually allocate the segment and assign regions to the pointers */
        mpi_errno = MPIDU_shm_seg_commit(&MPIDI_POSIX_eager_fbox_control_global.memory[i],
                                         &MPIDI_POSIX_eager_fbox_control_global.barrier,
                                         num_local, my_local_rank, local_rank_0, rank,
                                         seg_to_nodeid[i], MPIDU_SHM_OBJ__FASTBOXES,
                                         MPL_MEM_SHM);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Allocate table of pointers to fastboxes */
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out,
                        MPIDI_POSIX_fastbox_t **, num_local * sizeof(MPIDI_POSIX_fastbox_t *),
                        mpi_errno, "fastboxes", MPL_MEM_SHM);

    /* Fill in fbox tables */
    int segment_idx, mailbox_idx;
    for (i = 0; i < num_local; i++) {
        segment_idx = MPIDI_POSIX_SEGMENT_INDEX(my_local_rank);
        mailbox_idx = MPIDI_POSIX_MAILBOX_INDEX(i, my_local_rank);
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i] =
            &fastboxes_p[segment_idx][mailbox_idx];

        segment_idx = MPIDI_POSIX_SEGMENT_INDEX(i);
        mailbox_idx = MPIDI_POSIX_MAILBOX_INDEX(my_local_rank, i);
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            &fastboxes_p[segment_idx][mailbox_idx];

        memset(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i], 0,
               sizeof(MPIDI_POSIX_fastbox_t));
    }

    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_eager_fbox_control_global.barrier, num_local);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
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
    int i, mpi_errno = MPI_SUCCESS;

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

    for (i = 0; i < MPIDI_POSIX_eager_fbox_control_global.num_seg; i++) {
        mpi_errno = MPIDU_shm_seg_destroy(&MPIDI_POSIX_eager_fbox_control_global.memory[i],
                                          MPIDI_POSIX_eager_fbox_control_global.num_local);
    }
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.memory);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.barrier_region);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_EAGER_FBOX_INIT_H_INCLUDED */

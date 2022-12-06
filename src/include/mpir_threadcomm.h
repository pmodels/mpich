/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_THREADCOMM_H_INCLUDED
#define MPIR_THREADCOMM_H_INCLUDED

#define MPIR_THREADCOMM_FBOX_SIZE   256

typedef struct MPIR_threadcomm_fbox_t {
    union {
        MPL_atomic_int_t data_ready;
        void *dummy;            /* for alignment */
    } u;
    char cell[];
} MPIR_threadcomm_fbox_t;

typedef struct MPIR_Threadcomm {
    MPIR_OBJECT_HEADER;
    MPIR_Comm *comm;
    int num_threads;            /* number of threads in this rank */
    int *rank_offset_table;     /* offset table for inter-process rank to
                                 * threadcomm rank conversion */
    /* for thread_id assignment */
    MPL_atomic_int_t next_id;
    /* thread barrier - two-counter algorithm */
    MPL_atomic_int_t arrive_counter;
    MPL_atomic_int_t leave_counter;
    MPL_atomic_int_t barrier_flag;
    /* fast box */
    MPIR_threadcomm_fbox_t *mailboxes;
} MPIR_Threadcomm;

#define MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank) \
    (rank < (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] && \
     rank >= (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads)

#define MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank) \
    (rank - ((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads))

#define MPIR_THREADCOMM_TID_TO_RANK(threadcomm, tid) \
    (((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads) + tid)

#ifdef ENABLE_THREADCOMM
extern MPL_TLS int MPIR_Threadcomm_thread_id;
#endif

#endif /* MPIR_THREADCOMM_H_INCLUDED */

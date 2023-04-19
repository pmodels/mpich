/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_THREADCOMM_H_INCLUDED
#define MPIR_THREADCOMM_H_INCLUDED

#include "utarray.h"

/* define MPIR_THREADCOMM_USE_FBOX to use fbox for interthread messaging.
 * Comment it off to use lockless MPSC queue instead.
 */
/* #define MPIR_THREADCOMM_USE_FBOX 1 */

#ifdef MPIR_THREADCOMM_USE_FBOX
typedef struct MPIR_threadcomm_fbox_t {
    union {
        MPL_atomic_int_t data_ready;
        void *dummy;            /* for alignment */
    } u;
    char cell[];
} MPIR_threadcomm_fbox_t;

#define MPIR_THREADCOMM_FBOX_SIZE   256
#define MPIR_THREADCOMM_MAX_PAYLOAD (MPIR_THREADCOMM_FBOX_SIZE - sizeof(MPIR_threadcomm_fbox_t))
#define MPIR_THREADCOMM_MAILBOX(threadcomm, src, dst) \
    (MPIR_threadcomm_fbox_t *) (((char *) (threadcomm)->mailboxes) + ((src) + (threadcomm)->num_threads * (dst)) * MPIR_THREADCOMM_FBOX_SIZE)

#else
typedef struct MPIR_threadcomm_cell_t {
    MPL_atomic_ptr_t next;
    char payload[];
} MPIR_threadcomm_cell_t;

typedef struct MPIR_threadcomm_queue_t {
    MPL_atomic_ptr_t head;
    MPL_atomic_ptr_t tail;
    char dummy[MPL_CACHELINE_SIZE];
} MPIR_threadcomm_queue_t;

#define MPIR_THREADCOMM_CELL_SIZE  4096
#define MPIR_THREADCOMM_MAX_PAYLOAD (MPIR_THREADCOMM_CELL_SIZE - sizeof(MPIR_threadcomm_cell_t))

#endif

typedef struct MPIR_Threadcomm {
    MPIR_OBJECT_HEADER;
    MPIR_Comm *comm;
    int num_threads;            /* number of threads in this rank */
    int *rank_offset_table;     /* offset table for inter-process rank to
                                 * threadcomm rank conversion */
    /* for tid assignment */
    MPL_atomic_int_t next_id;
    /* thread barrier - two-counter algorithm */
    MPL_atomic_int_t arrive_counter;
    MPL_atomic_int_t leave_counter;
    MPL_atomic_int_t barrier_flag;
#ifdef MPIR_THREADCOMM_USE_FBOX
    /* fast box */
    MPIR_threadcomm_fbox_t *mailboxes;
#else
    /* lockless MPSC queue */
    MPIR_threadcomm_queue_t *queues;
#endif
} MPIR_Threadcomm;

#define MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank) \
    (rank < (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] && \
     rank >= (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads)

#define MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank) \
    (rank - ((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads))

#define MPIR_THREADCOMM_TID_TO_RANK(threadcomm, tid) \
    (((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads) + tid)

#ifdef ENABLE_THREADCOMM
#define MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_) do { \
        MPIR_Threadcomm *threadcomm = (comm)->threadcomm; \
        if (threadcomm) { \
            int intracomm_size = (comm)->local_size; \
            size_ = threadcomm->rank_offset_table[intracomm_size - 1]; \
            rank_ = MPIR_THREADCOMM_TID_TO_RANK(threadcomm, MPIR_threadcomm_get_tid(threadcomm)); \
        } else { \
            rank_ = (comm)->rank; \
            size_ = (comm)->local_size; \
        } \
    } while (0)

#else
#define MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_) do { \
        MPIR_Assert((comm)->threadcomm == NULL); \
        rank_ = (comm)->rank; \
        size_ = (comm)->local_size; \
    } while (0)

#endif

#ifdef ENABLE_THREADCOMM
typedef struct MPIR_threadcomm_unexp_t {
    struct MPIR_threadcomm_unexp_t *next, *prev;
    char cell[];
} MPIR_threadcomm_unexp_t;

typedef struct MPIR_threadcomm_tls_t {
    MPIR_Threadcomm *threadcomm;
    int tid;
    MPIR_Attribute *attributes;
    /* postponed send request */
    MPIR_Request *pending_list;
    /* recv message queues */
    MPIR_Request *posted_list;
    MPIR_threadcomm_unexp_t *unexp_list;
} MPIR_threadcomm_tls_t;

/* TLS dynamic array to support multiple threadcomms */
extern UT_icd MPIR_threadcomm_icd;
extern MPL_TLS UT_array *MPIR_threadcomm_array;

#define MPIR_THREADCOMM_TLS_ADD(threadcomm, p) \
    do { \
        if (MPIR_threadcomm_array == NULL) { \
            utarray_new(MPIR_threadcomm_array, &MPIR_threadcomm_icd, MPL_MEM_OTHER); \
        } \
        utarray_extend_back(MPIR_threadcomm_array, MPL_MEM_OTHER); \
        p = (MPIR_threadcomm_tls_t *) utarray_back(MPIR_threadcomm_array); \
        p->threadcomm = threadcomm; \
    } while (0)

#define MPIR_THREADCOMM_FIND_IDX(threadcomm, idx) \
    do { \
        MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *); \
        idx = -1; \
        for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) { \
            if (p[i].threadcomm == threadcomm) { \
                idx = i; \
                break; \
            } \
        } \
    } while (0)

#define MPIR_THREADCOMM_TLS_DELETE(threadcomm) \
    do { \
        int idx; \
        MPIR_THREADCOMM_FIND_IDX(threadcomm, idx); \
        if (idx != -1) { \
            utarray_erase(MPIR_threadcomm_array, idx, 1); \
            if (utarray_len(MPIR_threadcomm_array) == 0) { \
                utarray_free(MPIR_threadcomm_array); \
                MPIR_threadcomm_array = NULL; \
            } \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX MPIR_threadcomm_tls_t *MPIR_threadcomm_get_tls(MPIR_Threadcomm *
                                                                        threadcomm)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        if (p[i].threadcomm == threadcomm) {
            return &p[i];
        }
    }
    return NULL;
}

MPL_STATIC_INLINE_PREFIX int MPIR_threadcomm_get_tid(MPIR_Threadcomm * threadcomm)
{
    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);
    return p->tid;
}

int MPIR_Threadcomm_isend_attr(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Threadcomm * threadcomm, int attr,
                               MPIR_Request ** req);
int MPIR_Threadcomm_irecv_attr(void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Threadcomm * threadcomm, int attr,
                               MPIR_Request ** req);
int MPIR_Threadcomm_progress(int *made_progress);

#endif /* ENABLE_THREADCOMM */

#endif /* MPIR_THREADCOMM_H_INCLUDED */

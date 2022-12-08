/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_THREADCOMM_H_INCLUDED
#define MPIR_THREADCOMM_H_INCLUDED

#include "utarray.h"

#define MPIR_THREADCOMM_USE_NONE  0
#define MPIR_THREADCOMM_USE_FBOX  1
#define MPIR_THREADCOMM_USE_QUEUE 2

/* TODO: add transport implementation */
#define MPIR_THREADCOMM_TRANSPORT MPIR_THREADCOMM_USE_NONE

typedef struct MPIR_Threadcomm {
    MPIR_OBJECT_HEADER;
    MPIR_Comm *comm;
    enum {
        MPIR_THREADCOMM_KIND__PERSIST,  /* created via MPIX_Threadcomm_init */
        MPIR_THREADCOMM_KIND__DERIVED,  /* created inside a parallel region,
                                         * must be freed before exit the parallel region */
    } kind;
    int num_threads;            /* number of threads in this rank */
    int *rank_offset_table;     /* offset table for inter-process rank to
                                 * threadcomm rank conversion */
    /* thread barrier - two-counter algorithm */
    MPL_atomic_int_t arrive_counter;
    MPL_atomic_int_t leave_counter;
    MPL_atomic_int_t barrier_flag;
    /* bcast during comm_dup */
    void *bcast_value;
} MPIR_Threadcomm;

#define MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank) \
    (rank < (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] && \
     rank >= (threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads)

#define MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank) \
    (rank - ((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads))

#define MPIR_THREADCOMM_TID_TO_RANK(threadcomm, tid) \
    (((threadcomm)->rank_offset_table[(threadcomm)->comm->rank] - (threadcomm)->num_threads) + tid)

#ifdef ENABLE_THREADCOMM
typedef struct MPIR_threadcomm_tls_t {
    MPIR_Threadcomm *threadcomm;
    int tid;
    MPIR_Attribute *attributes;
    /* postponed send request */
    /* posted receive queue */
    /* unexpected message queue */
} MPIR_threadcomm_tls_t;

/* TLS dynamic array to support multiple threadcomms */
extern UT_icd MPIR_threadcomm_icd;
extern MPL_TLS UT_array *MPIR_threadcomm_array;

/* We can optimize if we are not using MPI_THREAD_MULTIPLE */
extern bool MPIR_threadcomm_was_thread_single;

#define MPIR_THREADCOMM_TLS_ADD(threadcomm, p) \
    do { \
        if (MPIR_threadcomm_array == NULL) { \
            utarray_new(MPIR_threadcomm_array, &MPIR_threadcomm_icd, MPL_MEM_OTHER); \
        } \
        utarray_extend_back(MPIR_threadcomm_array, MPL_MEM_OTHER); \
        p = (MPIR_threadcomm_tls_t *) utarray_back(MPIR_threadcomm_array); \
        p->threadcomm = threadcomm; \
    } while (0)

#define internal_THREADCOMM_FIND_IDX(threadcomm, idx) \
    do { \
        MPIR_threadcomm_tls_t *_p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *); \
        idx = -1; \
        for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) { \
            if (_p[i].threadcomm == threadcomm) { \
                idx = i; \
                break; \
            } \
        } \
    } while (0)

#define MPIR_THREADCOMM_TLS_DELETE(threadcomm) \
    do { \
        int idx; \
        internal_THREADCOMM_FIND_IDX(threadcomm, idx); \
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
                               MPIR_Request ** req, bool has_status);
int MPIR_Threadcomm_progress(int *made_progress);

#endif /* ENABLE_THREADCOMM */

#endif /* MPIR_THREADCOMM_H_INCLUDED */

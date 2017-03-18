/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4I_WORKQ_TYPES_H_INCLUDED
#define CH4I_WORKQ_TYPES_H_INCLUDED

/* Define the work queue implementation type */
#if defined(MPIDI_USE_NMQUEUE)
#include <queue/zm_nmqueue.h>
#define MPIDI_workq_t       zm_nmqueue_t
#define MPIDI_workq_init    zm_nmqueue_init
#define MPIDI_workq_enqueue zm_nmqueue_enqueue
#define MPIDI_workq_dequeue zm_nmqueue_dequeue
#elif defined(MPIDI_USE_MSQUEUE)
#include <queue/zm_msqueue.h>
#define MPIDI_workq_t       zm_msqueue_t
#define MPIDI_workq_init    zm_msqueue_init
#define MPIDI_workq_enqueue zm_msqueue_enqueue
#define MPIDI_workq_dequeue zm_msqueue_dequeue
#elif defined(MPIDI_USE_GLQUEUE)
#include <queue/zm_glqueue.h>
#define MPIDI_workq_t       zm_glqueue_t
#define MPIDI_workq_init    zm_glqueue_init
#define MPIDI_workq_enqueue zm_glqueue_enqueue
#define MPIDI_workq_dequeue zm_glqueue_dequeue
#else
/* Stub implementation to make it compile */
typedef void *MPIDI_workq_t;
MPL_STATIC_INLINE_PREFIX void MPIDI_workq_init(MPIDI_workq_t *q) {}
MPL_STATIC_INLINE_PREFIX void MPIDI_workq_enqueue(MPIDI_workq_t *q, void *p) {}
MPL_STATIC_INLINE_PREFIX void MPIDI_workq_dequeue(MPIDI_workq_t *q, void **pp) {}
#endif

typedef enum MPIDI_workq_op MPIDI_workq_op_t;
enum MPIDI_workq_op {SEND, ISEND, RECV, IRECV, PUT};

typedef struct MPIDI_workq_elemt MPIDI_workq_elemt_t;
typedef struct MPIDI_workq_list  MPIDI_workq_list_t;

struct MPIDI_workq_elemt {
    MPIDI_workq_op_t op;
    union {
        /* Point-to-Point */
        struct {
            const void *send_buf;
            void *recv_buf;
            MPI_Aint count;
            MPI_Datatype datatype;
            int rank;
            int tag;
            MPIR_Comm *comm_ptr;
            int context_offset;
            MPI_Status *status;
            MPIR_Request *request;
        };
        /* RMA */
        struct {
            const void *origin_addr;
            int origin_count;
            MPI_Datatype origin_datatype;
            int target_rank;
            MPI_Aint target_disp;
            int target_count;
            MPI_Datatype target_datatype;
            MPIR_Win *win_ptr;
        };
    };
};

struct MPIDI_workq_list {
    MPIDI_workq_t pend_ops;
    MPIDI_workq_list_t *next, *prev;
};

#endif /* CH4I_WORKQ_TYPES_H_INCLUDED */

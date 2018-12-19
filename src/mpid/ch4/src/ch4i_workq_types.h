/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4I_WORKQ_TYPES_H_INCLUDED
#define CH4I_WORKQ_TYPES_H_INCLUDED

/*
  Multi-threading models
*/
enum {
    MPIDI_CH4_MT_DIRECT,
    MPIDI_CH4_MT_HANDOFF,
    MPIDI_CH4_MT_TRYLOCK,

    MPIDI_CH4_NUM_MT_MODELS,
};

/* For now any thread safety model that is not "direct" requires
 * work queues. These queues might be used for different reasons,
 * thus a new macro to capture that. */
#if !defined(MPIDI_CH4_USE_MT_DIRECT)
#define MPIDI_CH4_USE_WORK_QUEUES
#endif

static const char *MPIDI_CH4_mt_model_names[MPIDI_CH4_NUM_MT_MODELS] = {
    "direct",
    "handoff",
    "trylock",
};

/* Define the work queue implementation type */
#if defined(ENABLE_IZEM_QUEUE)
#include <queue/zm_queue.h>
#define MPIDI_workq_t       zm_queue_t
#define MPIDI_workq_init    zm_queue_init
#define MPIDI_workq_enqueue zm_queue_enqueue
#define MPIDI_workq_dequeue zm_queue_dequeue
#else
/* Stub implementation to make it compile */
typedef void *MPIDI_workq_t;
MPL_STATIC_INLINE_PREFIX void MPIDI_workq_init(MPIDI_workq_t * q)
{
#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPIR_Assert(0);
#endif
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_enqueue(MPIDI_workq_t * q, void *p)
{
#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPIR_Assert(0);
#endif
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_dequeue(MPIDI_workq_t * q, void **pp)
{
#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPIR_Assert(0);
#endif
}
#endif

#define MPIDI_WORKQ_ELEMT_PREALLOC 64   /* Number of elements to preallocate in the "direct" block */

typedef enum MPIDI_workq_op MPIDI_workq_op_t;

/* Indentifies the delegated operation */
enum MPIDI_workq_op { SEND, ISEND, SSEND, ISSEND, RSEND, IRSEND, RECV, IRECV, IMRECV, IPROBE,
    IMPROBE, PUT, GET, ACC, CAS, FAO, GACC
};

typedef struct MPIDI_workq_elemt MPIDI_workq_elemt_t;
typedef struct MPIDI_workq_list MPIDI_workq_list_t;

typedef struct MPIDI_av_entry MPIDI_av_entry_t;

/* Structure to encapsulate MPI operations that are delegated to another thread
 * Can be allocated from an MPI object pool or embedded in another object (e.g. request) */
struct MPIDI_workq_elemt {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPIDI_workq_op_t op;
    OPA_int_t *processed;       /* set to true by the progress thread when
                                 * this work item is done */
    union {
        union {
            struct MPIDI_workq_send {
                const void *send_buf;
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *addr;
                MPIR_Request *request;
            } send;             /* also for ISEND SSEND ISSEND RSEND IRSEND */
            struct MPIDI_workq_recv {
                void *recv_buf;
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *addr;
                MPI_Status *status;
                MPIR_Request *request;
            } recv;
            struct MPIDI_workq_irecv {
                void *recv_buf;
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *addr;
                MPIR_Request *request;
            } irecv;
            struct MPIDI_workq_iprobe {
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *addr;
                MPI_Status *status;
                MPIR_Request *request;
                int *flag;
            } iprobe;
            struct MPIDI_workq_improbe {
                MPI_Aint count;
                MPI_Datatype datatype;
                int rank;
                int tag;
                MPIR_Comm *comm_ptr;
                int context_offset;
                MPIDI_av_entry_t *addr;
                MPI_Status *status;
                MPIR_Request *request;
                int *flag;
                MPIR_Request **message;
            } improbe;
            struct MPIDI_workq_imrecv {
                void *buf;
                MPI_Aint count;
                MPI_Datatype datatype;
                MPIR_Request **message;
                MPIR_Request *request;
            } imrecv;
        } pt2pt;
        union {
            struct MPIDI_workq_put {
                const void *origin_addr;
                int origin_count;
                MPI_Datatype origin_datatype;
                int target_rank;
                MPI_Aint target_disp;
                int target_count;
                MPI_Datatype target_datatype;
                MPIR_Win *win_ptr;
                MPIDI_av_entry_t *addr;
            } put;
            struct MPIDI_workq_get {
                void *origin_addr;
                int origin_count;
                MPI_Datatype origin_datatype;
                int target_rank;
                MPI_Aint target_disp;
                int target_count;
                MPI_Datatype target_datatype;
                MPIR_Win *win_ptr;
                MPIDI_av_entry_t *addr;
            } get;
        } rma;
    } params;
};

/* List structure to implement per-object (e.g. per-communicator, per-window) work queues */
struct MPIDI_workq_list {
    MPIDI_workq_t pend_ops;
    MPIDI_workq_list_t *next, *prev;
};

#endif /* CH4I_WORKQ_TYPES_H_INCLUDED */

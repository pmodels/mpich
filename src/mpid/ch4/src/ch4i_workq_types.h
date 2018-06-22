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

/* Stub implementation for an atomic queue */
typedef void *MPIDI_workq_t;
MPL_STATIC_INLINE_PREFIX void MPIDI_workq_init(MPIDI_workq_t * q)
{
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_enqueue(MPIDI_workq_t * q, void *p)
{
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_dequeue(MPIDI_workq_t * q, void **pp)
{
}

typedef enum MPIDI_workq_op MPIDI_workq_op_t;

/* Indentifies the delegated operation */
enum MPIDI_workq_op { SEND, ISEND, SSEND, ISSEND, RSEND, IRSEND, RECV, IRECV, IPROBE,
    IMPROBE, PUT, GET, POST, COMPLETE, FENCE, LOCK, UNLOCK, LOCK_ALL, UNLOCK_ALL,
    FLUSH, FLUSH_ALL, FLUSH_LOCAL, FLUSH_LOCAL_ALL
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
        struct {
            const void *send_buf;
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
            int *flag;          /* needed for the probe routines */
            MPIR_Request **message;     /* used for mprobe */
        } pt2pt;
        struct {
            const void *origin_addr;
            int origin_count;
            MPI_Datatype origin_datatype;
            void *result_addr;
            int result_count;
            MPI_Datatype result_datatype;
            int target_rank;
            MPI_Aint target_disp;
            int target_count;
            MPI_Datatype target_datatype;
            MPI_Op acc_op;
            MPIR_Group *group;
            int lock_type;
            int assert;         /* assert for sync functions */
            MPIR_Win *win_ptr;
            MPIDI_av_entry_t *addr;
        } rma;
    };
};

/* List structure to implement per-object (e.g. per-communicator, per-window) work queues */
struct MPIDI_workq_list {
    MPIDI_workq_t pend_ops;
    MPIDI_workq_list_t *next, *prev;
};

#endif /* CH4I_WORKQ_TYPES_H_INCLUDED */

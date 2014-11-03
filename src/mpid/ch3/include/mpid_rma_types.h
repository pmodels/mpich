/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_TYPES_H_INCLUDED)
#define MPID_RMA_TYPES_H_INCLUDED

#include "mpidi_ch3_impl.h"

/* Special case RMA operations */

enum MPIDI_RMA_Datatype {
    MPIDI_RMA_DATATYPE_BASIC = 50,
    MPIDI_RMA_DATATYPE_DERIVED = 51
};

/* We start with an arbitrarily chosen number (42), to help with
 * debugging when a packet type is not initialized or wrongly
 * initialized. */
enum MPID_Lock_state {
    MPID_LOCK_NONE = 42,
    MPID_LOCK_SHARED_ALL
};

/*
 * RMA Declarations.  We should move these into something separate from
 * a Request.
 */

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info {   /* for derived datatypes */
    int is_contig;
    int max_contig_blocks;
    MPI_Aint size;
    MPI_Aint extent;
    int dataloop_size;          /* not needed because this info is sent in
                                 * packet header. remove it after lock/unlock
                                 * is implemented in the device */
    void *dataloop;             /* pointer needed to update pointers
                                 * within dataloop on remote side */
    int dataloop_depth;
    int eltype;
    MPI_Aint ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
} MPIDI_RMA_dtype_info;

typedef enum MPIDI_RMA_Pool_type {
    MPIDI_RMA_POOL_WIN = 6,
    MPIDI_RMA_POOL_GLOBAL = 7
} MPIDI_RMA_Pool_type_t;

/* for keeping track of RMA ops, which will be executed at the next sync call */
typedef struct MPIDI_RMA_Op {
    struct MPIDI_RMA_Op *next;  /* pointer to next element in list */

    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;

    void *compare_addr;
    MPI_Datatype compare_datatype;

    void *result_addr;
    int result_count;
    MPI_Datatype result_datatype;

    struct MPID_Request *request;
    MPIDI_RMA_dtype_info dtype_info;
    void *dataloop;

    int target_rank;

    MPIDI_CH3_Pkt_t pkt;
    MPIDI_RMA_Pool_type_t pool_type;
} MPIDI_RMA_Op_t;

typedef struct MPIDI_PT_single_op {
    MPIDI_CH3_Pkt_type_t type;  /* put, get, or accum. */
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    void *data;                 /* for queued puts and accumulates, data is copied here */
    MPI_Request request_handle; /* for gets */
    int data_recd;              /* to indicate if the data has been received */
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_PT_single_op;

typedef struct MPIDI_Win_lock_queue {
    struct MPIDI_Win_lock_queue *next;
    int lock_type;
    MPI_Win source_win_handle;
    int origin_rank;
    struct MPIDI_PT_single_op *pt_single_op;    /* to store info for
                                                 * lock-put-unlock optimization */
} MPIDI_Win_lock_queue;

typedef MPIDI_RMA_Op_t *MPIDI_RMA_Ops_list_t;

#endif /* MPID_RMA_TYPES_H_INCLUDED */

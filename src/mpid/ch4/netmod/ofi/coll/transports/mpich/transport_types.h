/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPICH_TRANSPORT_TYPES_H_INCLUDED
#define MPICH_TRANSPORT_TYPES_H_INCLUDED

#include "../../include/tsp_namespace_pre.h"


typedef struct TSP_dt_t {
    MPI_Datatype mpi_dt;
}
TSP_dt_t;

typedef struct TSP_op_t {
    MPI_Op mpi_op;
}
TSP_op_t;

typedef struct TSP_comm_t {
    struct MPIR_Comm *mpid_comm;
}
TSP_comm_t;

typedef struct TSP_aint_t {
    MPI_Aint mpi_aint_val;
}
TSP_aint_t;

typedef struct TSP_sendrecv_arg_t {
    void              *buf;
    int                count;
    struct TSP_dt_t   *dt;
    int                dest;
    int                tag;
    struct TSP_comm_t *comm;
}
TSP_sendrecv_arg_t;

typedef struct TSP_recv_reduce_arg_t {
    void              *inbuf;
    void              *inoutbuf;
    int                count;
    struct TSP_dt_t   *datatype;
    struct TSP_op_t   *op;
    int                source;
    int                tag;
    struct TSP_comm_t *comm;
    struct TSP_req_t  *req;
    int                done;
    uint64_t           flags;
}
TSP_recv_reduce_arg_t;

typedef struct TSP_addref_dt_arg_t {
    struct TSP_dt_t *dt;
    int              up;
} TSP_addref_dt_arg_t;

typedef struct TSP_addref_op_arg_t {
    struct TSP_op_t *op;
    int              up;
} TSP_addref_op_arg_t;

typedef struct TSP_dtcopy_arg_t {
    void            *tobuf;
    int              tocount;
    struct TSP_dt_t *totype;
    const void      *frombuf;
    int              fromcount;
    struct TSP_dt_t *fromtype;
} TSP_dtcopy_arg_t;

typedef struct TSP_reduce_local_arg_t {
    const void      *inbuf;
    void            *inoutbuf;
    int              count;
    struct TSP_dt_t *dt;
    struct TSP_op_t *op;
} TSP_reduce_local_arg_t;

typedef struct TSP_free_mem_arg_t {
    void  *ptr;
} TSP_free_mem_arg_t;

enum {
    TSP_KIND_SEND,
    TSP_KIND_RECV,
    TSP_KIND_ADDREF_DT,
    TSP_KIND_ADDREF_OP,
    TSP_KIND_DTCOPY,
    TSP_KIND_FREE_MEM,
    TSP_KIND_RECV_REDUCE,
    TSP_KIND_REDUCE_LOCAL,
};

enum {
    TSP_STATE_INIT,
    TSP_STATE_ISSUED,
    TSP_STATE_COMPLETE,
};


typedef struct TSP_req_t {
    struct MPIR_Request *mpid_req[2];
    int        kind;
    int        state;
    uint64_t  *completion_cntr;
    uint64_t  *trigger_cntr;
    int        threshold;
    union {
        TSP_sendrecv_arg_t     sendrecv;
        TSP_addref_dt_arg_t    addref_dt;
        TSP_addref_op_arg_t    addref_op;
        TSP_dtcopy_arg_t       dtcopy;
        TSP_free_mem_arg_t     free_mem;
        TSP_recv_reduce_arg_t   recv_reduce;
        TSP_reduce_local_arg_t reduce_local;
    } nbargs;
}
TSP_req_t;


typedef struct TSP_sched_t {
    uint64_t   cntr;
    uint64_t   cur_thresh;
    uint64_t   posted;
    uint64_t   total;
    uint64_t   completed;
    TSP_req_t  requests[32];
}
TSP_sched_t;


#endif /* MPICH_TRANSPORT_TYPES_H_INCLUDED */

/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TRIGGERED_TRANSPORT_TYPES_H_INCLUDED
#define TRIGGERED_TRANSPORT_TYPES_H_INCLUDED

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

typedef struct TSP_handle_send_arg_t {
    struct MPIR_Request  *mpid_req;
} TSP_handle_send_arg_t;

typedef struct TSP_handle_recv_arg_t {
    struct MPIR_Request *mpid_req;
    size_t        recv_size;
} TSP_handle_recv_arg_t;

enum {
    TSP_KIND_SEND,
    TSP_KIND_RECV,
    TSP_KIND_ADDREF_DT,
    TSP_KIND_ADDREF_OP,
    TSP_KIND_DTCOPY,
    TSP_KIND_FREE_MEM,
    TSP_KIND_RECV_REDUCE,
    TSP_KIND_REDUCE_LOCAL,
    TSP_KIND_HANDLE_SEND,
    TSP_KIND_HANDLE_RECV,
};

enum {
    TSP_STATE_INIT,
    TSP_STATE_ISSUED,
    TSP_STATE_COMPLETE,
};

typedef struct TSP_req_t {
    struct MPIR_Request *mpid_req[2];
    int              kind;
    int              state;
    int              recv_size;
    int              is_contig;
    uint64_t         done_thresh;
    struct fi_trigger_threshold  thresh;
    union {
        TSP_sendrecv_arg_t     sendrecv;
        TSP_addref_dt_arg_t    addref_dt;
        TSP_addref_op_arg_t    addref_op;
        TSP_dtcopy_arg_t       dtcopy;
        TSP_free_mem_arg_t     free_mem;
        TSP_recv_reduce_arg_t   recv_reduce;
        TSP_reduce_local_arg_t reduce_local;
        TSP_handle_recv_arg_t  handle_recv;
        TSP_handle_send_arg_t  handle_send;
    } nbargs;
}
TSP_req_t;

typedef struct TSP_sched_t {
    struct fid_cntr *cntr;
    uint64_t         cur_thresh;
    uint64_t         posted;
    uint64_t         total;
    uint64_t         completed;
    TSP_req_t        requests[32];
}
TSP_sched_t;

typedef struct TSP_global_t {
    struct fid_ep *send_ep;
    struct fid_ep *recv_ep;
}
TSP_global_t;
extern TSP_global_t TSP_global;

#endif /* TRIGGERED_TRANSPORT_TYPES_H_INCLUDED */

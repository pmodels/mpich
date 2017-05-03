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

#include "../../src/tsp_namespace_def.h"

#undef MAX_EDGES
#define MAX_EDGES 64

#undef MAX_REQUESTS
#define MAX_REQUESTS 64

typedef struct TSP_dt_t {
    MPI_Datatype mpi_dt;
}
TSP_dt_t;

typedef struct {
    TSP_dt_t control_dt;
} TSP_global_t;

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
    TSP_KIND_NOOP
};

enum {
    TSP_STATE_INIT,
    TSP_STATE_ISSUED,
    TSP_STATE_COMPLETE,
};

typedef struct{
    int array[MAX_EDGES];
    int used;
    int size;
} TSP_IntArray;


typedef struct TSP_req_t {
    struct TSP_req_t* next_issued;
    struct MPIR_Request *mpid_req[2];
    int        kind;
    int        state;
    int        id; /*a unique id for this task*/

    TSP_IntArray invtcs;
    TSP_IntArray outvtcs;

    int       num_unfinished_dependencies;
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
    uint64_t   total;
    uint64_t   num_completed;
    uint64_t   last_wait; /*used by TSP_wait, to keep track of the last TSP_wait vtx id*/
    TSP_req_t  requests[MAX_REQUESTS];
    /*Store the memory location of all the buffers that were temporarily
    * allocated to execute the schedule. This information is later used
    * to free those memory locations when the schedule is destroyed (TSP_free_sched_mem)
    * Note that the temporary memory allocated by recv_reduce is not 
    * recorded here since the transport already knows about it
    */
    uint64_t   nbufs;
    void      *buf[MAX_REQUESTS];/*size of the array is currently arbitrarily set*/
    
    TSP_req_t  *issued_head;/*head of the issued requests list*/
    TSP_req_t  *req_iter;/*current request under consideration*/
    TSP_req_t  *last_issued; /*points to the last task considered issued in the current pass*/
}
TSP_sched_t;


#endif /* MPICH_TRANSPORT_TYPES_H_INCLUDED */

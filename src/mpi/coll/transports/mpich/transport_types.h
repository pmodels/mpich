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

#undef MPIC_MPICH_MAX_EDGES
#define MPIC_MPICH_MAX_EDGES 64

#undef MPIC_MPICH_MAX_REQUESTS
#define MPIC_MPICH_MAX_REQUESTS 64

typedef MPI_Datatype MPIC_MPICH_dt_t;


typedef struct {
    MPIC_MPICH_dt_t control_dt;
} MPIC_MPICH_global_t;

typedef MPI_Op MPIC_MPICH_op_t;

typedef struct MPIC_MPICH_comm_t {
    struct MPIR_Comm *mpid_comm;
} MPIC_MPICH_comm_t;

typedef struct MPIC_MPICH_aint_t {
    MPI_Aint mpi_aint_val;
} MPIC_MPICH_aint_t;

typedef struct MPIC_MPICH_sendrecv_arg_t {
    void *buf;
    int count;
    MPIC_MPICH_dt_t dt;
    int dest;
    struct MPIC_MPICH_comm_t *comm;
} MPIC_MPICH_sendrecv_arg_t;

typedef struct MPIC_MPICH_recv_reduce_arg_t {
    void *inbuf;
    void *inoutbuf;
    int count;
    MPIC_MPICH_dt_t datatype;
    MPIC_MPICH_op_t op;
    int source;
    struct MPIC_MPICH_comm_t *comm;
    struct MPIC_MPICH_req_t *req;
    int done;
    uint64_t flags;
} MPIC_MPICH_recv_reduce_arg_t;

typedef struct MPIC_MPICH_addref_dt_arg_t {
    MPIC_MPICH_dt_t dt;
    int up;
} MPIC_MPICH_addref_dt_arg_t;

typedef struct MPIC_MPICH_addref_op_arg_t {
    MPIC_MPICH_op_t op;
    int up;
} MPIC_MPICH_addref_op_arg_t;

typedef struct MPIC_MPICH_dtcopy_arg_t {
    void *tobuf;
    int tocount;
    MPIC_MPICH_dt_t totype;
    const void *frombuf;
    int fromcount;
    MPIC_MPICH_dt_t fromtype;
} MPIC_MPICH_dtcopy_arg_t;

typedef struct MPIC_MPICH_reduce_local_arg_t {
    const void      *inbuf;
    void            *inoutbuf;
    int              count;
    MPIC_MPICH_dt_t  dt;
    MPIC_MPICH_op_t  op;
    uint64_t           flags;
} MPIC_MPICH_reduce_local_arg_t;

typedef struct MPIC_MPICH_free_mem_arg_t {
    void *ptr;
} MPIC_MPICH_free_mem_arg_t;

enum {
    MPIC_MPICH_KIND_SEND,
    MPIC_MPICH_KIND_RECV,
    MPIC_MPICH_KIND_ADDREF_DT,
    MPIC_MPICH_KIND_ADDREF_OP,
    MPIC_MPICH_KIND_DTCOPY,
    MPIC_MPICH_KIND_FREE_MEM,
    MPIC_MPICH_KIND_RECV_REDUCE,
    MPIC_MPICH_KIND_REDUCE_LOCAL,
    MPIC_MPICH_KIND_NOOP
};

enum {
    MPIC_MPICH_STATE_INIT,
    MPIC_MPICH_STATE_ISSUED,
    MPIC_MPICH_STATE_COMPLETE,
};

typedef struct {
    int array[MPIC_MPICH_MAX_EDGES];
    int used;
    int size;
} MPIC_MPICH_IntArray;


typedef struct MPIC_MPICH_req_t {
    struct MPIC_MPICH_req_t *next_issued;
    struct MPIR_Request *mpid_req[2];
    int kind;
    int state;
    int id;                     /*a unique id for this task */

    MPIC_MPICH_IntArray invtcs;
    MPIC_MPICH_IntArray outvtcs;

    int num_unfinished_dependencies;
    union {
        MPIC_MPICH_sendrecv_arg_t sendrecv;
        MPIC_MPICH_addref_dt_arg_t addref_dt;
        MPIC_MPICH_addref_op_arg_t addref_op;
        MPIC_MPICH_dtcopy_arg_t dtcopy;
        MPIC_MPICH_free_mem_arg_t free_mem;
        MPIC_MPICH_recv_reduce_arg_t recv_reduce;
        MPIC_MPICH_reduce_local_arg_t reduce_local;
    } nbargs;
} MPIC_MPICH_req_t;


typedef struct MPIC_MPICH_sched_t {
    int tag;
    uint64_t total;
    uint64_t num_completed;
    uint64_t last_wait;         /*used by TSP_wait, to keep track of the last TSP_wait vtx id */
    MPIC_MPICH_req_t requests[MPIC_MPICH_MAX_REQUESTS];
    /*Store the memory location of all the buffers that were temporarily
     * allocated to execute the schedule. This information is later used
     * to free those memory locations when the schedule is destroyed (MPIC_MPICH_free_sched_mem)
     * Note that the temporary memory allocated by recv_reduce is not
     * recorded here since the transport already knows about it
     */
    uint64_t nbufs;
    void *buf[MPIC_MPICH_MAX_REQUESTS]; /*size of the array is currently arbitrarily set */

    MPIC_MPICH_req_t *issued_head;      /*head of the issued requests list */
    MPIC_MPICH_req_t *req_iter; /*current request under consideration */
    MPIC_MPICH_req_t *last_issued;      /*points to the last task considered issued in the current pass */
} MPIC_MPICH_sched_t;

#endif /* MPICH_TRANSPORT_TYPES_H_INCLUDED */

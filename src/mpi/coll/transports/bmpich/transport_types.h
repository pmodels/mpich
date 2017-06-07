/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef BMPICH_TRANSPORT_TYPES_H_INCLUDED
#define BMPICH_TRANSPORT_TYPES_H_INCLUDED


#undef MPIC_BMPICH_MAX_REQUESTS
#define MPIC_BMPICH_MAX_REQUESTS 64

typedef MPI_Datatype MPIC_BMPICH_dt_t;

typedef struct {
    MPIC_BMPICH_dt_t control_dt;
} MPIC_BMPICH_global_t;

typedef MPI_Op MPIC_BMPICH_op_t;

typedef struct MPIC_BMPICH_comm_t {
    struct MPIR_Comm *mpid_comm;
}
MPIC_BMPICH_comm_t;

typedef struct MPIC_BMPICH_aint_t {
    MPI_Aint mpi_aint_val;
}
MPIC_BMPICH_aint_t;

typedef struct MPIC_BMPICH_sendrecv_arg_t {
    void              *buf;
    int                count;
    MPIC_BMPICH_dt_t   dt;
    int                dest;
    int                tag;
    struct MPIC_BMPICH_comm_t *comm;
}
MPIC_BMPICH_sendrecv_arg_t;

typedef struct MPIC_BMPICH_recv_reduce_arg_t {
    void              *inbuf;
    void              *inoutbuf;
    int                count;
    MPIC_BMPICH_dt_t   datatype;
    MPIC_BMPICH_op_t   op;
    int                source;
    int                tag;
    struct MPIC_BMPICH_comm_t *comm;
    struct MPIC_BMPICH_req_t  *req;
    int                done;
    uint64_t           flags;
}
MPIC_BMPICH_recv_reduce_arg_t;

typedef struct MPIC_BMPICH_addref_dt_arg_t {
    MPIC_BMPICH_dt_t dt;
    int              up;
} MPIC_BMPICH_addref_dt_arg_t;

typedef struct MPIC_BMPICH_addref_op_arg_t {
    MPIC_BMPICH_op_t op;
    int              up;
} MPIC_BMPICH_addref_op_arg_t;

typedef struct MPIC_BMPICH_dtcopy_arg_t {
    void            *tobuf;
    int              tocount;
    MPIC_BMPICH_dt_t totype;
    const void      *frombuf;
    int              fromcount;
    MPIC_BMPICH_dt_t fromtype;
} MPIC_BMPICH_dtcopy_arg_t;

typedef struct MPIC_BMPICH_reduce_local_arg_t {
    const void      *inbuf;
    void            *inoutbuf;
    int              count;
    MPIC_BMPICH_dt_t dt;
    MPIC_BMPICH_op_t op;
} MPIC_BMPICH_reduce_local_arg_t;

typedef struct MPIC_BMPICH_free_mem_arg_t {
    void  *ptr;
} MPIC_BMPICH_free_mem_arg_t;

enum {
    MPIC_BMPICH_KIND_SEND,
    MPIC_BMPICH_KIND_RECV,
    MPIC_BMPICH_KIND_ADDREF_DT,
    MPIC_BMPICH_KIND_ADDREF_OP,
    MPIC_BMPICH_KIND_DTCOPY,
    MPIC_BMPICH_KIND_FREE_MEM,
    MPIC_BMPICH_KIND_RECV_REDUCE,
    MPIC_BMPICH_KIND_REDUCE_LOCAL,
    MPIC_BMPICH_KIND_NOOP
};

enum {
    MPIC_BMPICH_STATE_INIT,
    MPIC_BMPICH_STATE_ISSUED,
    MPIC_BMPICH_STATE_COMPLETE,
};

typedef struct MPIC_BMPICH_req_t {
    struct MPIR_Request *mpid_req[2];
    int        kind;

    union {
        MPIC_BMPICH_sendrecv_arg_t     sendrecv;
        MPIC_BMPICH_addref_dt_arg_t    addref_dt;
        MPIC_BMPICH_addref_op_arg_t    addref_op;
        MPIC_BMPICH_dtcopy_arg_t       dtcopy;
        MPIC_BMPICH_free_mem_arg_t     free_mem;
        MPIC_BMPICH_recv_reduce_arg_t   recv_reduce;
        MPIC_BMPICH_reduce_local_arg_t reduce_local;
    } nbargs;
}
MPIC_BMPICH_req_t;


typedef struct MPIC_BMPICH_sched_t {
    uint64_t   total;
    MPIC_BMPICH_req_t  requests[MPIC_BMPICH_MAX_REQUESTS];
    /*Store the memory location of all the buffers that were temporarily
    * allocated to execute the schedule. This information is later used
    * to free those memory locations when the schedule is destroyed (MPIC_BMPICH_free_sched_mem)
    * Note that the temporary memory allocated by recv_reduce is not 
    * recorded here since the transport already knows about it
    */
    uint64_t   nbufs;
    void      *buf[MPIC_BMPICH_MAX_REQUESTS];/*size of the array is currently arbitrarily set*/
    
    int tag;   
}
MPIC_BMPICH_sched_t;


#endif /* BMPICH_TRANSPORT_TYPES_H_INCLUDED */

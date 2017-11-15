/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPICH_TRANSPORT_TYPES_H_INCLUDED
#define MPICH_TRANSPORT_TYPES_H_INCLUDED

/* Maximum number of incoming/outgoing edges in the collective schedule graph.
 * This is used only as an initial hint to allocate memory to store edges */
#undef MPIC_MPICH_MAX_EDGES
#define MPIC_MPICH_MAX_EDGES 1

/* Maximum number of tasks in the collective schedule graph.
 * This is used only as an initial hint to allocate memory to store tasks */
#undef MPIC_MPICH_MAX_TASKS
#define MPIC_MPICH_MAX_TASKS 1

typedef MPI_Datatype MPIC_MPICH_dt_t;
typedef MPI_Op MPIC_MPICH_op_t;

/* Structure to store some global data */
typedef struct {
    MPIC_MPICH_dt_t control_dt;
} MPIC_MPICH_global_t;

/* Transport specific communicator information */
typedef struct MPIC_MPICH_comm_t {
    /* Stores pointer to MPIR_Comm */
    struct MPIR_Comm *mpid_comm;
    /* Database to store collective schedules */
    MPIC_sched_entry_t *sched_db;
} MPIC_MPICH_comm_t;

typedef struct MPIC_MPICH_aint_t {
    MPI_Aint mpi_aint_val;
} MPIC_MPICH_aint_t;

/* Data structure to store send or recv arguments */
typedef struct MPIC_MPICH_sendrecv_arg_t {
    void *buf;
    int count;
    MPIC_MPICH_dt_t dt;
    /* Stores destination in case of send and source in case of recv call */
    int dest;
    struct MPIC_MPICH_comm_t *comm;
} MPIC_MPICH_sendrecv_arg_t;

typedef struct {
   void *buf;
   int count;
   MPIC_MPICH_dt_t dt;
   /*array of ranks to send the data to*/
   int *destinations;
   int num_destinations;
   struct MPIC_MPICH_comm_t *comm;

   /*some data structures to keep track of the progress*/
   /*request array*/
   struct MPIR_Request **mpir_req;
   /*last send that has completed*/
   int last_complete;

} MPIC_MPICH_multicast_arg_t;

/* Data structure to store recv_reduce arguments */
typedef struct MPIC_MPICH_recv_reduce_arg_t {
    void *inbuf;
    void *inoutbuf;
    int count;
    MPIC_MPICH_dt_t datatype;
    MPIC_MPICH_op_t op;
    int source;
    struct MPIC_MPICH_comm_t *comm;
    int done;
    uint64_t flags;
    /* also stored pointer to the vertex of the recv_reduce in the graph */
    struct MPIC_MPICH_vtx_t *vtxp;
} MPIC_MPICH_recv_reduce_arg_t;

/* Data structure to store addref_dt arguments */
typedef struct MPIC_MPICH_addref_dt_arg_t {
    MPIC_MPICH_dt_t dt;
    int up;
} MPIC_MPICH_addref_dt_arg_t;

/* Data structure to store addref_op arguments */
typedef struct MPIC_MPICH_addref_op_arg_t {
    MPIC_MPICH_op_t op;
    int up;
} MPIC_MPICH_addref_op_arg_t;

/* Data structure to store dtcopy arguments */
typedef struct MPIC_MPICH_dtcopy_arg_t {
    void *tobuf;
    int tocount;
    MPIC_MPICH_dt_t totype;
    const void *frombuf;
    int fromcount;
    MPIC_MPICH_dt_t fromtype;
} MPIC_MPICH_dtcopy_arg_t;

/* Data structure to store reduce_local arguments */
typedef struct MPIC_MPICH_reduce_local_arg_t {
    const void *inbuf;
    void *inoutbuf;
    int count;
    MPIC_MPICH_dt_t dt;
    MPIC_MPICH_op_t op;
    /* Flag to tell whether the reduction is inbuf+inoutbuf or inoutbuf+inbuf */
    uint64_t flags;
} MPIC_MPICH_reduce_local_arg_t;

/* Data structure to store free_mem arguments */
typedef struct MPIC_MPICH_free_mem_arg_t {
    void *ptr;
} MPIC_MPICH_free_mem_arg_t;

/* Enumerator for various transport functions */
typedef enum MPIC_MPICH_TASK_KIND {
    MPIC_MPICH_KIND_SEND,
    MPIC_MPICH_KIND_RECV,
    MPIC_MPICH_KIND_MULTICAST,
    MPIC_MPICH_KIND_ADDREF_DT,
    MPIC_MPICH_KIND_ADDREF_OP,
    MPIC_MPICH_KIND_DTCOPY,
    MPIC_MPICH_KIND_FREE_MEM,
    MPIC_MPICH_KIND_RECV_REDUCE,
    MPIC_MPICH_KIND_REDUCE_LOCAL,
    MPIC_MPICH_KIND_NOOP,
    MPIC_MPICH_KIND_FENCE,
    MPIC_MPICH_KIND_WAIT
} MPIC_MPICH_TASK_KIND;

/* Enumerator for various states of a task */
typedef enum MPIC_MPICH_TASK_STATE {
    /* Task is initialized, but not issued.
     * It remains in this state while waiting for
     * the dependencies to complete */
    MPIC_MPICH_STATE_INIT,
    /* Task has been issued but has not completed execution yet */
    MPIC_MPICH_STATE_ISSUED,
    /* Task has completed */
    MPIC_MPICH_STATE_COMPLETE,
} MPIC_MPICH_TASK_STATE;

/* A data structure for an integer array */
typedef struct {
    int *array;
    int used;
    int size;
} MPIC_MPICH_int_array;

/* A data structure for an array of memory locations/pointers */
typedef struct {
    void **array;
    int used;
    int size;
} MPIC_MPICH_ptr_array;

/* Data structure for a vertex in the graph
 * Each vertex corresponds to a task */
typedef struct MPIC_MPICH_vtx_t {
    /* Kind of the task associated with this vertex: enum MPIC_MPICH_TASK_KIND */
    int kind;
    /* Current state of the task associated with this vertex: enum MPIC_MPICH_TASK_STATE */
    int state;
    /* request pointers for the task, recv_reduce task will need two request pointers */
    struct MPIR_Request *mpid_req[2];
    /* a unique id for this vertex */
    int id;

    /* Integer arrays of incoming and outgoing vertices */
    MPIC_MPICH_int_array invtcs;
    MPIC_MPICH_int_array outvtcs;

    int num_unfinished_dependencies;
    /* Union to store task arguments depending on the task type */
    union {
        MPIC_MPICH_sendrecv_arg_t sendrecv;
        MPIC_MPICH_multicast_arg_t multicast;
        MPIC_MPICH_addref_dt_arg_t addref_dt;
        MPIC_MPICH_addref_op_arg_t addref_op;
        MPIC_MPICH_dtcopy_arg_t dtcopy;
        MPIC_MPICH_free_mem_arg_t free_mem;
        MPIC_MPICH_recv_reduce_arg_t recv_reduce;
        MPIC_MPICH_reduce_local_arg_t reduce_local;
    } nbargs;

    /* This transport maintains a linked list of issued vertices. If this vertex is
     * currently issued (that is, state == MPIC_MPICH_STATE_ISSUED), next_issued points
     * to the next issued vertex in the liked list. Else it is set to NULL */
    struct MPIC_MPICH_vtx_t *next_issued;
} MPIC_MPICH_vtx_t;


/* Data structure to store schedule of a collective operation */
typedef struct MPIC_MPICH_sched_t {
    /* Tag value to be used by the tasks in this schedule */
    int tag;
    /* Array of vertices */
    MPIC_MPICH_vtx_t *vtcs;
    /* Total number of vertices */
    int total;
    /* Of the total vertices, number of vertices that have completed
     * This is reset to zero whenever the schedule is reused */
    int num_completed;
    /* Maximum number of vertices that can be stored in the vtcs array. This is increased
     * when more vertices need to be added */
    int max_vtcs;
    /* Maximum number of edges per vertex. This is increase when more edges need to be added */
    int max_edges_per_vtx;

    /* last_wait keeps track of the vertex id of the last MPIR_TSP_wait call
     * so that when the next MPIR_TSP_wait call is made, its incoming dependencies
     * are made only up to the last_wait vertex to avoid redundant dependencies */
    int last_wait;


    /* Store the memory location of all the buffers that were temporarily
     * allocated to execute the schedule. This information is later used to
     * free those memory locations when the schedule is destroyed
     * (MPIC_MPICH_free_sched_mem) Note that the temporary memory allocated by
     * recv_reduce is not recorded here since the transport already knows about it
     */

    MPIC_MPICH_ptr_array buf_array;

    MPIC_MPICH_vtx_t *issued_head; /* head of the issued vertices linked list */
    MPIC_MPICH_vtx_t *vtx_iter;    /* temporary pointer to keep track of the
                                      current vertex under consideration in
                                      MPIR_TSP_test function */
    MPIC_MPICH_vtx_t *last_issued; /* temporary pointer to the last vertex
                                      issued in the current pass of MPIR_TSP_test
                                      function */
} MPIC_MPICH_sched_t;

#endif /* MPICH_TRANSPORT_TYPES_H_INCLUDED */

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PROCESS_H_INCLUDED
#define MPIR_PROCESS_H_INCLUDED

/* Per process data */
typedef struct PreDefined_attrs {
    int appnum;                 /* Application number provided by mpiexec (MPI-2) */
    int host;                   /* host */
    int io;                     /* standard io allowed */
    int lastusedcode;           /* last used error code (MPI-2) */
    int tag_ub;                 /* Maximum message tag */
    int universe;               /* Universe size from mpiexec (MPI-2) */
    int wtime_is_global;        /* Wtime is global over processes in COMM_WORLD */
} PreDefined_attrs;

typedef struct MPIR_Process_t {
    MPL_atomic_int_t mpich_state;       /* Need use atomics due to MPI_Initialized() etc.
                                         * thread-safe per MPI-3.1.  See MPI-Forum ticket 357 */

    /* Fields to be initialized by MPIR_pmi_init() */
    int has_parent;
    int appnum;
    int rank;
    int size;
    int local_rank;
    int local_size;
    int num_nodes;
    int *node_map;              /* int[size], maps rank to node_id */
    int *node_local_map;        /* int[local_size], maps local_id to rank of local proc */
    int *node_root_map;         /* int[num_nodes], maps node_id to the rank of node root */

    /* A dynamic array of node hostnames to support dynamic process node ids */
    UT_array *node_hostnames;

    unsigned world_id;          /* this is a hash of pmi_kvs_name. One use is for
                                 * ofi netmod active message to synchronize seq number. */

    /* -------------- */
    int do_error_checks;        /* runtime error check control */
    struct MPIR_Comm *comm_world;       /* Easy access to comm_world for
                                         * error handler */
    struct MPIR_Comm *comm_self;        /* Easy access to comm_self */
    struct MPIR_Comm *comm_parent;      /* Easy access to comm_parent */
    struct MPIR_Comm *icomm_world;      /* An internal version of comm_world
                                         * that is separate from user's
                                         * versions */
    PreDefined_attrs attrs;     /* Predefined attribute values */
    int tag_bits;               /* number of tag bits supported */
    char *memory_alloc_kinds;   /* memory kinds supported in the world model */

    /* for MPI_Buffer_attach */
    struct MPII_BsendBuffer *bsendbuffer;

    /* The topology routines dimsCreate is independent of any communicator.
     * If this pointer is null, the default routine is used */
    int (*dimsCreate) (int, int, int *);

    /* The topology aware routines:
     * coords - array which stores network coordinate values.
     * coords_dims - stores requested number of dimensions in the specified network.*/
    int *coords;
    int coords_dims;

    /* Attribute dup functions.  Here for lazy initialization */
    int (*attr_dup) (int, MPIR_Attribute *, MPIR_Attribute **);
    int (*attr_free) (int, MPIR_Attribute **);
    /* There is no win_attr_dup function because there can be no MPI_Win_dup
     * function */
    /* Routine to get the messages corresponding to dynamically created
     * error messages */
    const char *(*errcode_to_string) (int);
#ifdef HAVE_CXX_BINDING
    /* Routines to call C++ functions from the C implementation of the
     * MPI reduction and attribute routines */
    void (*cxx_call_op_fn) (const void *, void *, int, MPI_Datatype, MPI_User_function *);
    /* Error handling functions.  As for the attribute functions,
     * we pass the integer file/comm/win, the address of the error code,
     * and the C function to call (itself a function defined by the
     * C++ interface and exported to C).  The first argument is used
     * to specify the kind (comm,file,win) */
#ifndef BUILD_MPI_ABI
    void (*cxx_call_errfn) (int, int *, int *, void (*)(void));
#else
    void (*cxx_call_errfn) (int, ABI_Comm *, int *, void (*)(void));
#endif
#endif                          /* HAVE_CXX_BINDING */
} MPIR_Process_t;
extern MPIR_Process_t MPIR_Process;

int MPIR_build_nodemap(int *nodemap, int sz, int *num_nodes);
int MPIR_build_locality(void);

int MPIR_nodeid_lookup(const char *hostname, int *node_id);
int MPIR_nodeid_init(void);
int MPIR_nodeid_free(void);

#endif /* MPIR_PROCESS_H_INCLUDED */

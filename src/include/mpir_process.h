/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_PROCESS_H_INCLUDED
#define MPIR_PROCESS_H_INCLUDED

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

#ifdef USE_PMIX_API
#include "pmix.h"
#endif

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
    OPA_int_t mpich_state;      /* State of MPICH. Use OPA_int_t to make MPI_Initialized() etc.
                                 * thread-safe per MPI-3.1.  See MPI-Forum ticket 357 */
    int do_error_checks;        /* runtime error check control */
    struct MPIR_Comm *comm_world;       /* Easy access to comm_world for
                                         * error handler */
    struct MPIR_Comm *comm_self;        /* Easy access to comm_self */
    struct MPIR_Comm *comm_parent;      /* Easy access to comm_parent */
    struct MPIR_Comm *icomm_world;      /* An internal version of comm_world
                                         * that is separate from user's
                                         * versions */
    PreDefined_attrs attrs;     /* Predefined attribute values */

#ifdef HAVE_HWLOC
    hwloc_topology_t topology;  /* HWLOC topology */
    hwloc_cpuset_t bindset;     /* process binding */
    int bindset_is_valid;       /* Flag to indicate if the bind set of the process is valid:
                                 * 0 if invalid, 1 if valid */
#endif

#ifdef USE_PMIX_API
    pmix_proc_t pmix_proc;
    pmix_proc_t pmix_wcproc;
#endif

    /* The topology routines dimsCreate is independent of any communicator.
     * If this pointer is null, the default routine is used */
    int (*dimsCreate) (int, int, int *);

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
    void (*cxx_call_errfn) (int, int *, int *, void (*)(void));
#endif                          /* HAVE_CXX_BINDING */
} MPIR_Process_t;
extern MPIR_Process_t MPIR_Process;

#endif /* MPIR_PROCESS_H_INCLUDED */

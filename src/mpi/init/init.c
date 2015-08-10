/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : THREADS
      description : multi-threading cvars

cvars:
    - name        : MPIR_CVAR_ASYNC_PROGRESS
      category    : THREADS
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPICH will initiate an additional thread to
        make asynchronous progress on all communication operations
        including point-to-point, collective, one-sided operations and
        I/O.  Setting this variable will automatically increase the
        thread-safety level to MPI_THREAD_MULTIPLE.  While this
        improves the progress semantics, it might cause a small amount
        of performance overhead for regular MPI operations.  The user
        is encouraged to leave one or more hardware threads vacant in
        order to prevent contention between the application threads
        and the progress thread(s).  The impact of oversubscription is
        highly system dependent but may be substantial in some cases,
        hence this recommendation.

    - name        : MPIR_CVAR_DEFAULT_THREAD_LEVEL
      category    : THREADS
      type        : string
      default     : "MPI_THREAD_SINGLE"
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Sets the default thread level to use when using MPI_INIT.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init = PMPI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init  MPI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init as PMPI_Init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Init(int *argc, char ***argv) __attribute__((weak,alias("PMPI_Init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init
#define MPI_Init PMPI_Init

/* Fortran logical values. extern'd in mpiimpl.h */
/* MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE; */

/* Any internal routines can go here.  Make them static if possible */

/* must go inside this #ifdef block to prevent duplicate storage on darwin */
int MPIR_async_thread_initialized = 0;
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init

/*@
   MPI_Init - Initialize the MPI execution environment

Input Parameters:
+  argc - Pointer to the number of arguments 
-  argv - Pointer to the argument vector

Thread and Signal Safety:
This routine must be called by one thread only.  That thread is called
the `main thread` and must be the thread that calls 'MPI_Finalize'.

Notes:
   The MPI standard does not say what a program can do before an 'MPI_INIT' or
   after an 'MPI_FINALIZE'.  In the MPICH implementation, you should do
   as little as possible.  In particular, avoid anything that changes the
   external state of the program, such as opening files, reading standard
   input or writing to standard output.

Notes for C:
    As of MPI-2, 'MPI_Init' will accept NULL as input parameters. Doing so
    will impact the values stored in 'MPI_INFO_ENV'.

Notes for Fortran:
The Fortran binding for 'MPI_Init' has only the error return
.vb
    subroutine MPI_INIT( ierr )
    integer ierr
.ve

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INIT

.seealso: MPI_Init_thread, MPI_Finalize
@*/
int MPI_Init( int *argc, char ***argv )
{
    static const char FCNAME[] = "MPI_Init";
    int mpi_errno = MPI_SUCCESS;
    int rc ATTRIBUTE((unused));
    int threadLevel, provided;
    MPID_MPI_INIT_STATE_DECL(MPID_STATE_MPI_INIT);

    rc = MPID_Wtime_init();
#ifdef USE_DBG_LOGGING
    MPIU_DBG_PreInit( argc, argv, rc );
#endif

    MPID_MPI_INIT_FUNC_ENTER(MPID_STATE_MPI_INIT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (OPA_load_int(&MPIR_Process.mpich_state) != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						  "**inittwice", NULL );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    /* Temporarily disable thread-safety.  This is needed because the
     * mutexes are not initialized yet, and we don't want to
     * accidentally use them before they are initialized.  We will
     * reset this value once it is properly initialized. */
#if defined MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = 0;
#endif /* MPICH_IS_THREADED */

    MPIR_T_env_init();

    if (!strcmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_MULTIPLE"))
        threadLevel = MPI_THREAD_MULTIPLE;
    else if (!strcmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_SERIALIZED"))
        threadLevel = MPI_THREAD_SERIALIZED;
    else if (!strcmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_FUNNELED"))
        threadLevel = MPI_THREAD_FUNNELED;
    else if (!strcmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_SINGLE"))
        threadLevel = MPI_THREAD_SINGLE;
    else {
        MPL_error_printf("Unrecognized thread level %s\n", MPIR_CVAR_DEFAULT_THREAD_LEVEL);
        exit(1);
    }

    /* If the user requested for asynchronous progress, request for
     * THREAD_MULTIPLE. */
    if (MPIR_CVAR_ASYNC_PROGRESS)
        threadLevel = MPI_THREAD_MULTIPLE;

    mpi_errno = MPIR_Init_thread( argc, argv, threadLevel, &provided );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (MPIR_CVAR_ASYNC_PROGRESS) {
        if (provided == MPI_THREAD_MULTIPLE) {
            mpi_errno = MPIR_Init_async_thread();
            if (mpi_errno) goto fn_fail;

            MPIR_async_thread_initialized = 1;
        }
        else {
            printf("WARNING: No MPI_THREAD_MULTIPLE support (needed for async progress)\n");
        }
    }

    /* ... end of body of routine ... */
    MPID_MPI_INIT_FUNC_EXIT(MPID_STATE_MPI_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_REPORTING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_init", "**mpi_init %p %p", argc, argv);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

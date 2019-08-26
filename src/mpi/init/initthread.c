/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Portions of this code were written by Microsoft. Those portions are
 * Copyright (c) 2007 Microsoft Corporation. Microsoft grants
 * permission to use, reproduce, prepare derivative works, and to
 * redistribute to others. The code is licensed "as is." The User
 * bears the risk of using it. Microsoft gives no express warranties,
 * guarantees or conditions. To the extent permitted by law, Microsoft
 * excludes the implied warranties of merchantability, fitness for a
 * particular purpose and non-infringement.
 */

#include "mpiimpl.h"
#include "mpir_info.h"
#include "datatype.h"
#include "mpi_init.h"
#ifdef HAVE_CRTDBG_H
#include <crtdbg.h>
#endif
#ifdef HAVE_USLEEP
#include <unistd.h>
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : DEBUGGER
      description : cvars relevant to the "MPIR" debugger interface

cvars:
    - name        : MPIR_CVAR_DEBUG_HOLD
      category    : DEBUGGER
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, causes processes to wait in MPI_Init and
        MPI_Initthread for a debugger to be attached.  Once the
        debugger has attached, the variable 'hold' should be set to 0
        in order to allow the process to continue (e.g., in gdb, "set
        hold=0").

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Init_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init_thread = PMPI_Init_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init_thread  MPI_Init_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init_thread as PMPI_Init_thread
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
    __attribute__ ((weak, alias("PMPI_Init_thread")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init_thread
#define MPI_Init_thread PMPI_Init_thread

/* Any internal routines can go here.  Make them static if possible */

/* Global variables can be initialized here */

/* This will help force the load of initinfo.o, which contains data about
   how MPICH was configured. */
extern const char MPII_Version_device[];

int MPIR_Init_thread(int *argc, char ***argv, int required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;

    init_thread_and_enter_cs(required);

    MPID_Wtime_init();
    pre_init_dbg_logging(argc, argv);

    mpi_errno = MPIR_T_env_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = init_global(&required);
    MPIR_ERR_CHECK(mpi_errno);  /* out-of-mem */

    init_topo();
    init_windows();
    init_binding_fortran();
    init_binding_cxx();
    init_binding_f08();

    /* Wait for debugger to attach if requested. */
    if (MPIR_CVAR_DEBUG_HOLD) {
        debugger_hold();
    }

    /* Setting MPICH_MPI_STATE__IN_INIT allows MPI_Initialized and
     * MPI_Finalized to call MPIR_Err_return_comm(0, ...) on error,
     * which checks errhandler in comm_world structure, which should
     * NULL by default before or during init, and treated as fatal.
     */
    /* FIXME: remove MPICH_MPI_STATE__IN_INIT */
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__IN_INIT);

    pre_init_memory_tracing();

    int thread_provided = 0;
    mpi_errno = MPID_Init(argc, argv, required, &thread_provided);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize collectives infrastructure */
    mpi_errno = MPII_Coll_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = post_init_global(thread_provided);
    MPIR_ERR_CHECK(mpi_errno);

    post_init_memory_tracing();
    init_dbg_logging();
    wait_for_debugger();

    /* dup comm_self and creates progress thread (if needed) */
    mpi_errno = init_async(thread_provided);
    MPIR_ERR_CHECK(mpi_errno);

    /* Let the device know that the rest of the init process is completed */
    mpi_errno = MPID_InitCompleted();

    init_thread_and_exit_cs(thread_provided);
    /* Make fields of MPIR_Process global visible and set mpich_state
     * atomically so that MPI_Initialized() etc. are thread safe */
    OPA_write_barrier();
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__POST_INIT);

    if (provided)
        *provided = thread_provided;
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* signal to error handling routines that core services are unavailable */
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__PRE_INIT);
    init_thread_failed_exit_cs();
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}
#endif

/*@
   MPI_Init_thread - Initialize the MPI execution environment

Input Parameters:
+  argc - Pointer to the number of arguments
.  argv - Pointer to the argument vector
-  required - Level of desired thread support

Output Parameters:
.  provided - Level of provided thread support

   Command line arguments:
   MPI specifies no command-line arguments but does allow an MPI
   implementation to make use of them.  See 'MPI_INIT' for a description of
   the command line arguments supported by 'MPI_INIT' and 'MPI_INIT_THREAD'.

   Notes:
   The valid values for the level of thread support are\:
+ MPI_THREAD_SINGLE - Only one thread will execute.
. MPI_THREAD_FUNNELED - The process may be multi-threaded, but only the main
  thread will make MPI calls (all MPI calls are funneled to the
   main thread).
. MPI_THREAD_SERIALIZED - The process may be multi-threaded, and multiple
  threads may make MPI calls, but only one at a time: MPI calls are not
  made concurrently from two distinct threads (all MPI calls are serialized).
- MPI_THREAD_MULTIPLE - Multiple threads may call MPI, with no restrictions.

Notes for Fortran:
   Note that the Fortran binding for this routine does not have the 'argc' and
   'argv' arguments. ('MPI_INIT_THREAD(required, provided, ierror)')


.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER

.seealso: MPI_Init, MPI_Finalize
@*/
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_INIT_STATE_DECL(MPID_STATE_MPI_INIT_THREAD);
    MPIR_FUNC_TERSE_INIT_ENTER(MPID_STATE_MPI_INIT_THREAD);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (OPA_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__PRE_INIT) {
                mpi_errno =
                    MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPI_Init_thread",
                                         __LINE__, MPI_ERR_OTHER, "**inittwice", 0);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Init_thread(argc, argv, required, provided);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

    MPIR_FUNC_TERSE_INIT_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_REPORTING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_init_thread", "**mpi_init_thread %p %p %d %p", argc, argv,
                                 required, provided);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    MPIR_FUNC_TERSE_INIT_EXIT(MPID_STATE_MPI_INIT_THREAD);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

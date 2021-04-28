/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
      class       : none
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

int MPIR_Init_thread(int *argc, char ***argv, int user_required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    int required = user_required;
    int err;

    /**********************************************************************/
    /* Section 1: base components that other components rely on.
     * These need to be intialized first.  They have strong
     * dependencies between each other, which makes it a pain to
     * maintain them.  Hopefully, the number of such components is
     * small. */
    /**********************************************************************/

    MPL_wtime_init();

    MPID_Thread_init(&err);
    MPIR_Assert(err == 0);

    mpi_errno = MPIR_T_env_init();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Err_init();
    MPII_pre_init_dbg_logging(argc, argv);
    MPII_pre_init_memory_tracing();


    /**********************************************************************/
    /* Section 2: contains initialization of local components (no
     * dependency on other processes) that do not need any information
     * from the device.  These components are independent of each
     * other and can be initialized in any order. */
    /**********************************************************************/

    MPIR_Typerep_init();
    MPII_thread_mutex_create();
    MPII_init_request();
    MPII_hwtopo_init();
    MPII_nettopo_init();
    MPII_init_windows();
    MPII_init_binding_cxx();
    MPII_init_binding_f08();

    mpi_errno = MPII_init_local_proc_attrs(&required);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_Coll_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Group_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Datatype_init_predefined();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_DEBUG_HOLD) {
        MPII_debugger_hold();
    }


    /**********************************************************************/
    /* Section 3: preinitialization is complete.  signal to error
     * handling routines that core services are available. */
    /**********************************************************************/

    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__PRE_INIT);


    /**********************************************************************/
    /* Section 4: contains the device initialization and setup and
     * teardown needed for the device initialization. */
    /**********************************************************************/

    /* Setting isThreaded to 0 to trick any operations used within
     * MPID_Init to think that we are running in a single threaded
     * environment. */
#ifdef MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = 0;
#endif

    /* Initialize gpu in mpl in order to support shm gpu module initialization
     * inside MPID_Init */
    if (MPIR_CVAR_ENABLE_GPU) {
        int mpl_errno = MPL_gpu_init();
        MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_init");
    }

    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__IN_INIT);

    mpi_errno = MPID_Init(required, &MPIR_ThreadInfo.thread_provided);
    MPIR_ERR_CHECK(mpi_errno);


    /**********************************************************************/
    /* Section 5: contains post device initialization code.  Anything
     * that we could not do before the device was initialized can be
     * done now.  Try to keep this as small as possible.  Only things
     * that *REALLY* cannot be done before device initialization
     * should come here. */
    /**********************************************************************/

    /* MPIR_Process.attr.tag_ub depends on tag_bits set by the device */
    mpi_errno = MPII_init_tag_ub();
    MPIR_ERR_CHECK(mpi_errno);

    /* pairtypes might need device hooks to be activated so the device
     * can keep track of their creation.  that's why we need to do
     * this after the device initialization.  */
    mpi_errno = MPIR_Datatype_commit_pairtypes();
    MPIR_ERR_CHECK(mpi_errno);

    /* FIXME: this is for rlog, and should be removed when we delete rlog */
    MPII_Timer_init(MPIR_Process.comm_world->rank, MPIR_Process.comm_world->local_size);

    MPII_post_init_memory_tracing();
    MPII_init_dbg_logging();
    MPII_Wait_for_debugger();


    /**********************************************************************/
    /* Section 6: simply lets the device know that we are done with
     * the full initialization, in case it wants to do some final
     * setup. */
    /**********************************************************************/

    mpi_errno = MPID_InitCompleted();
    MPIR_ERR_CHECK(mpi_errno);

    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__POST_INIT);


    /**********************************************************************/
    /* Section 7: we are finally ready to start the asynchronous
     * thread, if needed. */
    /**********************************************************************/

    /* Reset isThreaded to the actual thread level */
#ifdef MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = (MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE);
#endif

    mpi_errno = MPII_init_async();
    MPIR_ERR_CHECK(mpi_errno);

    if (provided)
        *provided = MPIR_ThreadInfo.thread_provided;

    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
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
            if (MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__PRE_INIT) {
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

    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

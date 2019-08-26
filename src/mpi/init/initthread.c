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

    - name        : MPIR_CVAR_ERROR_CHECKING
      category    : ERROR_HANDLING
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, perform checks for errors, typically to verify valid inputs
        to MPI routines.  Only effective when MPICH is configured with
        --enable-error-checking=runtime .

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
    int thread_provided = 0;
    MPIR_Info *info_ptr;

    init_thread_and_enter_cs(required);

    MPID_Wtime_init();

    pre_init_dbg_logging(argc, argv);

    mpi_errno = MPIR_T_env_init();
    MPIR_ERR_CHECK(mpi_errno);

#if defined(MPICH_IS_THREADED)
    /* If the user requested for asynchronous progress, request for
     * THREAD_MULTIPLE. */
    if (MPIR_CVAR_ASYNC_PROGRESS)
        required = MPI_THREAD_MULTIPLE;
#endif

    init_topo();
    init_windows();

    /* We need this inorder to implement IS_THREAD_MAIN */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED) && defined(MPICH_IS_THREADED)
    {
        MPID_Thread_self(&MPIR_ThreadInfo.master_thread);
    }
#endif

#ifdef HAVE_ERROR_CHECKING
    /* Because the PARAM system has not been initialized, temporarily
     * uncondtionally enable error checks.  Once the PARAM system is
     * initialized, this may be reset */
    MPIR_Process.do_error_checks = 1;
#else
    MPIR_Process.do_error_checks = 0;
#endif

    /* Initialize necessary subsystems and setup the predefined attribute
     * values.  Subsystems may change these values. */
    MPIR_Process.attrs.appnum = -1;
    MPIR_Process.attrs.host = MPI_PROC_NULL;
    MPIR_Process.attrs.io = MPI_PROC_NULL;
    MPIR_Process.attrs.lastusedcode = MPI_ERR_LASTCODE;
    MPIR_Process.attrs.universe = MPIR_UNIVERSE_SIZE_NOT_SET;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Set the functions used to duplicate attributes.  These are
     * when the first corresponding keyval is created */
    MPIR_Process.attr_dup = 0;
    MPIR_Process.attr_free = 0;

    init_binding_fortran();
    init_binding_cxx();
    init_binding_f08();

    /* This allows the device to select an alternative function for
     * dimsCreate */
    MPIR_Process.dimsCreate = 0;

    /* "Allocate" from the reserved space for builtin communicators and
     * (partially) initialize predefined communicators.  comm_parent is
     * intially NULL and will be allocated by the device if the process group
     * was started using one of the MPI_Comm_spawn functions. */
    MPIR_Process.comm_world = MPIR_Comm_builtin + 0;
    MPII_Comm_init(MPIR_Process.comm_world);
    MPIR_Process.comm_world->handle = MPI_COMM_WORLD;
    MPIR_Process.comm_world->context_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->recvcontext_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    /* This initialization of the comm name could be done only when
     * comm_get_name is called */
    MPL_strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD", MPI_MAX_OBJECT_NAME);

    MPIR_Process.comm_self = MPIR_Comm_builtin + 1;
    MPII_Comm_init(MPIR_Process.comm_self);
    MPIR_Process.comm_self->handle = MPI_COMM_SELF;
    MPIR_Process.comm_self->context_id = 1 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->recvcontext_id = 1 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    MPL_strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF", MPI_MAX_OBJECT_NAME);

#ifdef MPID_NEEDS_ICOMM_WORLD
    MPIR_Process.icomm_world = MPIR_Comm_builtin + 2;
    MPII_Comm_init(MPIR_Process.icomm_world);
    MPIR_Process.icomm_world->handle = MPIR_ICOMM_WORLD;
    MPIR_Process.icomm_world->context_id = 2 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->recvcontext_id = 2 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    MPL_strncpy(MPIR_Process.icomm_world->name, "MPI_ICOMM_WORLD", MPI_MAX_OBJECT_NAME);

    /* Note that these communicators are not ready for use - MPID_Init
     * will setup self and world, and icomm_world if it desires it. */
#endif

    MPIR_Process.comm_parent = NULL;

    /* Setup the initial communicator list in case we have
     * enabled the debugger message-queue interface */
    MPII_COMML_REMEMBER(MPIR_Process.comm_world);
    MPII_COMML_REMEMBER(MPIR_Process.comm_self);

    /* MPIU_Timer_pre_init(); */

    /* Wait for debugger to attach if requested. */
    if (MPIR_CVAR_DEBUG_HOLD) {
        volatile int hold = 1;
        while (hold)
#ifdef HAVE_USLEEP
            usleep(100);
#endif
        ;
    }
#if defined(HAVE_ERROR_CHECKING) && (HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME)
    MPIR_Process.do_error_checks = MPIR_CVAR_ERROR_CHECKING;
#endif

    /* define MPI as initialized so that we can use MPI functions within
     * MPID_Init if necessary */
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__IN_INIT);

    /* create MPI_INFO_NULL object */
    /* FIXME: Currently this info object is empty, we need to add data to this
     * as defined by the standard. */
    info_ptr = MPIR_Info_builtin + 1;
    info_ptr->handle = MPI_INFO_ENV;
    MPIR_Object_set_ref(info_ptr, 1);
    info_ptr->next = NULL;
    info_ptr->key = NULL;
    info_ptr->value = NULL;

#ifdef USE_MEMORY_TRACING
    MPL_trinit();
#endif

    /* Set the number of tag bits. The device may override this value. */
    MPIR_Process.tag_bits = MPIR_TAG_BITS_DEFAULT;

    /* Create complete request to return in the event of immediately complete
     * operations. Use a SEND request to cover all possible use-cases. */
    MPIR_Process.lw_req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_ERR_CHKANDSTMT(MPIR_Process.lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                        "**nomemreq");
    MPIR_cc_set(&MPIR_Process.lw_req->cc, 0);

    mpi_errno = MPID_Init(argc, argv, required, &thread_provided);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize collectives infrastructure */
    mpi_errno = MPII_Coll_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Set tag_ub as function of tag_bits set by the device */
    MPIR_Process.attrs.tag_ub = MPIR_TAG_USABLE_BITS;

    /* Assert: tag_ub should be a power of 2 minus 1 */
    MPIR_Assert(((unsigned) MPIR_Process.
                 attrs.tag_ub & ((unsigned) MPIR_Process.attrs.tag_ub + 1)) == 0);

    /* Assert: tag_ub is at least the minimum asked for in the MPI spec */
    MPIR_Assert(MPIR_Process.attrs.tag_ub >= 32767);

    /* Capture the level of thread support provided */
    MPIR_ThreadInfo.thread_provided = thread_provided;
    if (provided)
        *provided = thread_provided;
#if defined MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = (thread_provided == MPI_THREAD_MULTIPLE);
#endif /* MPICH_IS_THREADED */

    /* FIXME: Define these in the interface.  Does Timer init belong here? */
    MPII_Timer_init(MPIR_Process.comm_world->rank, MPIR_Process.comm_world->local_size);
#ifdef USE_MEMORY_TRACING
#ifdef MPICH_IS_THREADED
    MPL_trconfig(MPIR_Process.comm_world->rank, MPIR_ThreadInfo.isThreaded);
#else
    MPL_trconfig(MPIR_Process.comm_world->rank, 0);
#endif
    /* Indicate that we are near the end of the init step; memory
     * allocated already will have an id of zero; this helps
     * separate memory leaks in the initialization code from
     * leaks in the "active" code */
#endif

    init_dbg_logging();

    /* FIXME: Does this need to come before the call to MPID_InitComplete?
     * For some debugger support, MPII_Wait_for_debugger may want to use
     * MPI communication routines to collect information for the debugger */
#ifdef HAVE_DEBUGGER_SUPPORT
    MPII_Wait_for_debugger();
#endif

    /* Let the device know that the rest of the init process is completed */
    if (mpi_errno == MPI_SUCCESS)
        mpi_errno = MPID_InitCompleted();

    init_thread_and_exit_cs(thread_provided);
    /* Make fields of MPIR_Process global visible and set mpich_state
     * atomically so that MPI_Initialized() etc. are thread safe */
    OPA_write_barrier();
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__POST_INIT);

    if (MPIR_CVAR_ASYNC_PROGRESS) {
#if MPL_THREAD_PACKAGE_NAME == MPL_THREAD_PACKAGE_ARGOBOTS
        printf("WARNING: Asynchronous progress is not supported with Argobots\n");
        goto fn_fail;
#else
        if (*provided == MPI_THREAD_MULTIPLE) {
            mpi_errno = MPID_Init_async_thread();
            if (mpi_errno)
                goto fn_fail;

            MPIR_async_thread_initialized = 1;
        } else {
            printf("WARNING: No MPI_THREAD_MULTIPLE support (needed for async progress)\n");
        }
#endif
    }

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

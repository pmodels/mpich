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
#include "mpiinfo.h"
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
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) __attribute__((weak,alias("PMPI_Init_thread")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init_thread
#define MPI_Init_thread PMPI_Init_thread

/* Any internal routines can go here.  Make them static if possible */

/* Global variables can be initialized here */
MPICH_PerProcess_t MPIR_Process = { OPA_INT_T_INITIALIZER(MPICH_PRE_INIT) };
     /* all other fields in MPIR_Process are irrelevant */
MPIR_Thread_info_t MPIR_ThreadInfo = { 0 };

/* These are initialized as null (avoids making these into common symbols).
   If the Fortran binding is supported, these can be initialized to 
   their Fortran values (MPI only requires that they be valid between
   MPI_Init and MPI_Finalize) */
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE = 0;
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE = 0;

/* This will help force the load of initinfo.o, which contains data about
   how MPICH was configured. */
extern const char MPIR_Version_device[];

/* Make sure the Fortran symbols are initialized unless it will cause problems
   for C programs linked with the C compilers (i.e., not using the 
   compilation scripts).  These provide the declarations for the initialization
   routine and the variable used to indicate whether the init needs to be
   called. */
#if defined(HAVE_FORTRAN_BINDING) && defined(HAVE_MPI_F_INIT_WORKS_WITH_C)
#ifdef F77_NAME_UPPER
#define mpirinitf_ MPIRINITF
#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
#define mpirinitf_ mpirinitf
#endif
void mpirinitf_(void);
/* Note that we don't include MPIR_F_NeedInit because we unconditionally
   call mpirinitf in this case, and the Fortran binding routines 
   do not test MPIR_F_NeedInit when HAVE_MPI_F_INIT_WORKS_WITH_C is set */
#endif

#ifdef HAVE_WINDOWS_H
/* User-defined abort hook function.  Exiting here will prevent the system from
 * bringing up an error dialog box.
 */
/* style: allow:fprintf:1 sig:0 */
static int assert_hook( int reportType, char *message, int *returnValue )
{
    MPIU_UNREFERENCED_ARG(reportType);
    fprintf(stderr, "%s", message);
    if (returnValue != NULL)
	ExitProcess((UINT)(*returnValue));
    ExitProcess((UINT)(-1));
    return TRUE;
}

/* MPICH dll entry point */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    BOOL result = TRUE;
    hinstDLL;
    lpReserved;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_THREAD_ATTACH:
	    /* allocate thread specific data */
            break;

        case DLL_THREAD_DETACH:
	    /* free thread specific data */
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return result;
}
#endif

#if defined(MPICH_IS_THREADED)

/* These routine handle any thread initialization that my be required */
#undef FUNCNAME
#define FUNCNAME thread_cs_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int thread_cs_init( void )
{
    int err;
    MPID_THREADPRIV_DECL;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX, &err);
    MPIU_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT
    /* MPICH_THREAD_GRANULARITY_PER_OBJECT: Multiple locks */
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX, &err);
    MPIU_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without 
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    MPID_THREADPRIV_INITKEY;
    MPID_THREADPRIV_INIT;

    MPIU_DBG_MSG(THREAD,TYPICAL,"Created global mutex and private storage");
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Thread_CS_Finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Thread_CS_Finalize( void )
{
    int err;

    MPIU_DBG_MSG(THREAD,TYPICAL,"Freeing global mutex and private storage");
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIU_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT
    /* MPICH_THREAD_GRANULARITY_PER_OBJECT: There are multiple locks,
     * one for each logical class (e.g., each type of object) */
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIU_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIU_Assert(err == 0);


#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without 
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    MPID_THREADPRIV_FINALIZE;

    return MPI_SUCCESS;
}
#endif /* MPICH_IS_THREADED */

#ifdef HAVE_F08_BINDING
MPI_Status *MPIR_C_MPI_STATUS_IGNORE;
MPI_Status *MPIR_C_MPI_STATUSES_IGNORE;
char ** MPIR_C_MPI_ARGV_NULL;
char ***MPIR_C_MPI_ARGVS_NULL;
int *MPIR_C_MPI_UNWEIGHTED;
int *MPIR_C_MPI_WEIGHTS_EMPTY;
int *MPIR_C_MPI_ERRCODES_IGNORE;

MPI_F08_Status MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPI_F08_Status MPIR_F08_MPI_STATUSES_IGNORE_OBJ[1];
int MPIR_F08_MPI_IN_PLACE;
int MPIR_F08_MPI_BOTTOM;

/* Althought the two STATUS pointers are required but the MPI3.0,  they are not used in MPICH F08 binding */
MPI_F08_Status *MPI_F08_STATUS_IGNORE = &MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPI_F08_Status *MPI_F08_STATUSES_IGNORE = &MPIR_F08_MPI_STATUSES_IGNORE_OBJ[0];
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Init_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Init_thread(int * argc, char ***argv, int required, int * provided)
{
    int mpi_errno = MPI_SUCCESS;
    int has_args;
    int has_env;
    int thread_provided;
    int exit_init_cs_on_failure = 0;
    MPID_Info *info_ptr;

    /* For any code in the device that wants to check for runtime 
       decisions on the value of isThreaded, set a provisional
       value here. We could let the MPID_Init routine override this */
#if defined MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = required == MPI_THREAD_MULTIPLE;
#endif /* MPICH_IS_THREADED */

#if defined(MPICH_IS_THREADED)
    mpi_errno = thread_cs_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* FIXME: Move to os-dependent interface? */
#ifdef HAVE_WINDOWS_H
    /* prevent the process from bringing up an error message window if mpich 
       asserts */
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, assert_hook);
#ifdef _WIN64
    {
    /* FIXME: (Windows) This severly degrades performance but fixes alignment 
       issues with the datatype code. */
    /* Prevent misaligned faults on Win64 machines */
    UINT mode, old_mode;
    
    old_mode = SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);
    mode = old_mode | SEM_NOALIGNMENTFAULTEXCEPT;
    SetErrorMode(mode);
    }
#endif
#endif

    /* We need this inorder to implement IS_THREAD_MAIN */
#   if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED) && defined(MPICH_IS_THREADED)
    {
	MPID_Thread_self(&MPIR_ThreadInfo.master_thread);
    }
#   endif

#ifdef HAVE_ERROR_CHECKING
    /* Because the PARAM system has not been initialized, temporarily
       uncondtionally enable error checks.  Once the PARAM system is
       initialized, this may be reset */
    MPIR_Process.do_error_checks = 1;
#else
    MPIR_Process.do_error_checks = 0;
#endif

    /* Initialize necessary subsystems and setup the predefined attribute
       values.  Subsystems may change these values. */
    MPIR_Process.attrs.appnum          = -1;
    MPIR_Process.attrs.host            = MPI_PROC_NULL;
    MPIR_Process.attrs.io              = MPI_PROC_NULL;
    MPIR_Process.attrs.lastusedcode    = MPI_ERR_LASTCODE;
    MPIR_Process.attrs.tag_ub          = 0;
    MPIR_Process.attrs.universe        = MPIR_UNIVERSE_SIZE_NOT_SET;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Set the functions used to duplicate attributes.  These are 
       when the first corresponding keyval is created */
    MPIR_Process.attr_dup  = 0;
    MPIR_Process.attr_free = 0;

#ifdef HAVE_CXX_BINDING
    /* Set the functions used to call functions in the C++ binding 
       for reductions and attribute operations.  These are null
       until a C++ operation is defined.  This allows the C code
       that implements these operations to not invoke a C++ code
       directly, which may force the inclusion of symbols known only
       to the C++ compiler (e.g., under more non-GNU compilers, including
       Solaris and IRIX). */
    MPIR_Process.cxx_call_op_fn = 0;

#endif

#ifdef HAVE_F08_BINDING
    MPIR_C_MPI_STATUS_IGNORE = MPI_STATUS_IGNORE;
    MPIR_C_MPI_STATUSES_IGNORE = MPI_STATUSES_IGNORE;
    MPIR_C_MPI_ARGV_NULL = MPI_ARGV_NULL;
    MPIR_C_MPI_ARGVS_NULL = MPI_ARGVS_NULL;
    MPIR_C_MPI_UNWEIGHTED = MPI_UNWEIGHTED;
    MPIR_C_MPI_WEIGHTS_EMPTY = MPI_WEIGHTS_EMPTY;
    MPIR_C_MPI_ERRCODES_IGNORE = MPI_ERRCODES_IGNORE;
#endif

    /* This allows the device to select an alternative function for 
       dimsCreate */
    MPIR_Process.dimsCreate     = 0;

    /* "Allocate" from the reserved space for builtin communicators and
       (partially) initialize predefined communicators.  comm_parent is
       intially NULL and will be allocated by the device if the process group
       was started using one of the MPI_Comm_spawn functions. */
    MPIR_Process.comm_world		    = MPID_Comm_builtin + 0;
    MPIR_Comm_init(MPIR_Process.comm_world);
    MPIR_Process.comm_world->handle	    = MPI_COMM_WORLD;
    MPIR_Process.comm_world->context_id	    = 0 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->recvcontext_id = 0 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->comm_kind	    = MPID_INTRACOMM;
    /* This initialization of the comm name could be done only when 
       comm_get_name is called */
    MPIU_Strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD",
		 MPI_MAX_OBJECT_NAME);

    MPIR_Process.comm_self		    = MPID_Comm_builtin + 1;
    MPIR_Comm_init(MPIR_Process.comm_self);
    MPIR_Process.comm_self->handle	    = MPI_COMM_SELF;
    MPIR_Process.comm_self->context_id	    = 1 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->recvcontext_id  = 1 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->comm_kind	    = MPID_INTRACOMM;
    MPIU_Strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF",
		 MPI_MAX_OBJECT_NAME);

#ifdef MPID_NEEDS_ICOMM_WORLD
    MPIR_Process.icomm_world		    = MPID_Comm_builtin + 2;
    MPIR_Comm_init(MPIR_Process.icomm_world);
    MPIR_Process.icomm_world->handle	    = MPIR_ICOMM_WORLD;
    MPIR_Process.icomm_world->context_id    = 2 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->recvcontext_id= 2 << MPID_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->comm_kind	    = MPID_INTRACOMM;
    MPIU_Strncpy(MPIR_Process.icomm_world->name, "MPI_ICOMM_WORLD",
		 MPI_MAX_OBJECT_NAME);

    /* Note that these communicators are not ready for use - MPID_Init 
       will setup self and world, and icomm_world if it desires it. */
#endif

    MPIR_Process.comm_parent = NULL;

    /* Setup the initial communicator list in case we have 
       enabled the debugger message-queue interface */
    MPIR_COMML_REMEMBER( MPIR_Process.comm_world );
    MPIR_COMML_REMEMBER( MPIR_Process.comm_self );

    /* Call any and all MPID_Init type functions */
    MPIR_Err_init();
    MPIR_Datatype_init();
    MPIR_Group_init();

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
       MPID_Init if necessary */
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_IN_INIT);

    /* We can't acquire any critical sections until this point.  Any
     * earlier the basic data structures haven't been initialized */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    exit_init_cs_on_failure = 1;

    /* create MPI_INFO_NULL object */
    /* FIXME: Currently this info object is empty, we need to add data to this
       as defined by the standard. */
    info_ptr = MPID_Info_builtin + 1;
    info_ptr->handle = MPI_INFO_ENV;
    MPIU_Object_set_ref(info_ptr, 1);
    info_ptr->next  = NULL;
    info_ptr->key   = NULL;
    info_ptr->value = NULL;
    
    mpi_errno = MPID_Init(argc, argv, required, &thread_provided, 
			  &has_args, &has_env);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Assert: tag_ub should be a power of 2 minus 1 */
    MPIU_Assert(((unsigned)MPIR_Process.attrs.tag_ub & ((unsigned)MPIR_Process.attrs.tag_ub + 1)) == 0);

    /* Set aside tag space for tagged collectives and failure notification */
    MPIR_Process.attrs.tag_ub     >>= 3;
    /* The bit for error checking is set in a macro in mpiimpl.h for
     * performance reasons. */
    MPIR_Process.tagged_coll_mask   = MPIR_Process.attrs.tag_ub + 1;

    /* Assert: tag_ub is at least the minimum asked for in the MPI spec */
    MPIU_Assert( MPIR_Process.attrs.tag_ub >= 32767 );

    /* Capture the level of thread support provided */
    MPIR_ThreadInfo.thread_provided = thread_provided;
    if (provided) *provided = thread_provided;
#if defined MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = (thread_provided == MPI_THREAD_MULTIPLE);
#endif /* MPICH_IS_THREADED */

    /* FIXME: Define these in the interface.  Does Timer init belong here? */
    MPIU_dbg_init(MPIR_Process.comm_world->rank);
    MPIU_Timer_init(MPIR_Process.comm_world->rank,
		    MPIR_Process.comm_world->local_size);
#ifdef USE_MEMORY_TRACING
    MPIU_trinit( MPIR_Process.comm_world->rank );
    /* Indicate that we are near the end of the init step; memory 
       allocated already will have an id of zero; this helps 
       separate memory leaks in the initialization code from 
       leaks in the "active" code */
    /* Uncomment this code to leave out any of the MPID_Init/etc 
       memory allocations from the memory leak testing */
    /* MPIU_trid( 1 ); */
#endif
#ifdef USE_DBG_LOGGING
    MPIU_DBG_Init( argc, argv, has_args, has_env, 
		   MPIR_Process.comm_world->rank );
#endif

    /* Initialize the C versions of the Fortran link-time constants.
       
       We now initialize the Fortran symbols from within the Fortran 
       interface in the routine that first needs the symbols.
       This fixes a problem with symbols added by a Fortran compiler that 
       are not part of the C runtime environment (the Portland group
       compilers would do this) 
    */
#if defined(HAVE_FORTRAN_BINDING) && defined(HAVE_MPI_F_INIT_WORKS_WITH_C)
    mpirinitf_();
#endif

    /* FIXME: Does this need to come before the call to MPID_InitComplete?
       For some debugger support, MPIR_WaitForDebugger may want to use
       MPI communication routines to collect information for the debugger */
#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_WaitForDebugger();
#endif

    /* Let the device know that the rest of the init process is completed */
    if (mpi_errno == MPI_SUCCESS) 
	mpi_errno = MPID_InitCompleted();

fn_exit:
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    /* Make fields of MPIR_Process global visible and set mpich_state
       atomically so that MPI_Initialized() etc. are thread safe */
    OPA_write_barrier();
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_POST_INIT);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* signal to error handling routines that core services are unavailable */
    OPA_store_int(&MPIR_Process.mpich_state, MPICH_PRE_INIT);

    if (exit_init_cs_on_failure) {
        MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
#if defined(MPICH_IS_THREADED)
    MPIR_Thread_CS_Finalize();
#endif
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
int MPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    int mpi_errno = MPI_SUCCESS;
    int rc ATTRIBUTE((unused)), reqd = required;
    MPID_MPI_INIT_STATE_DECL(MPID_STATE_MPI_INIT_THREAD);

    rc = MPID_Wtime_init();
#ifdef USE_DBG_LOGGING
    MPIU_DBG_PreInit( argc, argv, rc );
#endif

    MPID_MPI_INIT_FUNC_ENTER(MPID_STATE_MPI_INIT_THREAD);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (OPA_load_int(&MPIR_Process.mpich_state) != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPI_Init_thread", __LINE__, MPI_ERR_OTHER,
						  "**inittwice", 0 );
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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

    /* If the user requested for asynchronous progress, request for
     * THREAD_MULTIPLE. */
    if (MPIR_CVAR_ASYNC_PROGRESS)
        reqd = MPI_THREAD_MULTIPLE;

    mpi_errno = MPIR_Init_thread( argc, argv, reqd, provided );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (MPIR_CVAR_ASYNC_PROGRESS) {
        if (*provided == MPI_THREAD_MULTIPLE) {
            mpi_errno = MPIR_Init_async_thread();
            if (mpi_errno) goto fn_fail;

            MPIR_async_thread_initialized = 1;
        }
        else {
            printf("WARNING: No MPI_THREAD_MULTIPLE support (needed for async progress)\n");
        }
    }

    /* ... end of body of routine ... */

    MPID_MPI_INIT_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_REPORTING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_init_thread",
	    "**mpi_init_thread %p %p %d %p", argc, argv, required, provided);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    MPID_MPI_INIT_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

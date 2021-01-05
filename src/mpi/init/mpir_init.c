/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_info.h"
#include "datatype.h"
#include "mpi_init.h"
#include <strings.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : THREADS
      description : multi-threading cvars

    - name        : DEBUGGER
      description : cvars relevant to the "MPIR" debugger interface

cvars:
    - name        : MPIR_CVAR_DEFAULT_THREAD_LEVEL
      category    : THREADS
      type        : string
      default     : "MPI_THREAD_SINGLE"
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Sets the default thread level to use when using MPI_INIT. This variable
        is case-insensitive.

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

/* This will help force the load of initinfo.o, which contains data about
   how MPICH was configured. */
extern const char MPII_Version_device[];

int MPIR_Init_impl(int *argc, char ***argv)
{
    int mpi_errno = MPI_SUCCESS;

    int threadLevel = MPI_THREAD_SINGLE;
    const char *tmp_str;
    if (MPL_env2str("MPIR_CVAR_DEFAULT_THREAD_LEVEL", &tmp_str)) {
        if (!strcasecmp(tmp_str, "MPI_THREAD_MULTIPLE"))
            threadLevel = MPI_THREAD_MULTIPLE;
        else if (!strcasecmp(tmp_str, "MPI_THREAD_SERIALIZED"))
            threadLevel = MPI_THREAD_SERIALIZED;
        else if (!strcasecmp(tmp_str, "MPI_THREAD_FUNNELED"))
            threadLevel = MPI_THREAD_FUNNELED;
        else if (!strcasecmp(tmp_str, "MPI_THREAD_SINGLE"))
            threadLevel = MPI_THREAD_SINGLE;
        else {
            MPL_error_printf("Unrecognized thread level %s\n", tmp_str);
            exit(1);
        }
    }

    int provided;
    mpi_errno = MPIR_Init_thread_impl(argc, argv, threadLevel, &provided);

    return mpi_errno;
}

int MPIR_Init_thread_impl(int *argc, char ***argv, int user_required, int *provided)
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

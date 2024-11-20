/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_info.h"
#include "mpi_init.h"
#include <strings.h>
#include "mpir_async_things.h"

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

    - name        : MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST
      category    : GPU
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, mpl/ze will use immediate command list for copying

    - name        : MPIR_CVAR_GPU_ROUND_ROBIN_COMMAND_QUEUES
      category    : GPU
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, mpl/ze will use command queues in a round-robin fashion.
        If false, only command queues of index 0 will be used.

    - name        : MPIR_CVAR_NO_COLLECTIVE_FINALIZE
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, prevent MPI_Finalize to invoke collective behavior such as
        barrier or communicating to other processes. Consequently, it may result
        in leaking memory or losing messages due to pre-mature exiting. The
        default is false, which may invoke collective behaviors at finalize.

    - name        : MPIR_CVAR_FINALIZE_WAIT
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, poll progress at MPI_Finalize until reference count on
        MPI_COMM_WORLD and MPI_COMM_SELF reaches zero. This may be necessary
        to prevent remote processes hanging if it has pending communication
        protocols, e.g. a rendezvous send.

    - name        : MPIR_CVAR_INIT_SKIP_PMI_BARRIER
      category    : DEBUGGER
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Skip MPIR_pmi_barrier() in MPI_Init

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* MPIR_world_model_state tracks so we only init and finalize once in world model */
MPL_atomic_int_t MPIR_world_model_state = MPL_ATOMIC_INT_T_INITIALIZER(0);

/* Use init_counter to track when we are initializing for the first time or
 * when we are finalize for the last time and need cleanup states */
/* Note: we are not using atomic variable since it is always accessed under MPIR_init_lock */
static int init_counter;

/* TODO: currently the world model is not distinguished with session model, neither between
 * sessions, in that there is no session pointer attached to communicators, datatypes, etc.
 * To properly reflect the session semantics, we may need to always associate a session
 * pointer to all MPI objects (other than info) and add runtime validation checks everywhere.
 */

/* ------------ Init ------------------- */
void MPIR_Continue_global_init();

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
    mpi_errno = MPII_Init_thread(argc, argv, threadLevel, &provided, NULL);

    return mpi_errno;
}

int MPII_Init_thread(int *argc, char ***argv, int user_required, int *provided,
                     MPIR_Session ** p_session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int required = user_required;
    bool is_world_model = (p_session_ptr == NULL);
    int err;

    MPL_initlock_lock(&MPIR_init_lock);

    if (!is_world_model) {
        mpi_errno = MPIR_Session_create(p_session_ptr, user_required);
        MPIR_ERR_CHECK(mpi_errno);
    }

    init_counter++;
    if (init_counter > 1) {
        goto fn_exit;
    }
    /**********************************************************************/
    /* Section 1: base components that other components rely on.
     * These need to be initialized first.  They have strong
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

    mpi_errno = MPII_init_gpu();
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_context_id_init();
    MPIR_Typerep_init();
    MPII_thread_mutex_create();
    MPII_init_request();
    mpi_errno = MPIR_pmi_init();
    MPIR_ERR_CHECK(mpi_errno);
    MPII_hwtopo_init();
    MPII_nettopo_init();
    MPII_init_windows();
    MPII_init_binding_cxx();
    MPIR_Continue_global_init();

    mpi_errno = MPII_init_local_proc_attrs(&required);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_init_builtin_infos(argc, argv);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_Coll_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Group_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Datatype_init_predefined();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Async_things_init();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_DEBUG_HOLD) {
        MPII_debugger_hold();
    }


    /**********************************************************************/
    /* Section 3: preinitialization is complete.  signal to error
     * handling routines that core services are available. */
    /**********************************************************************/

    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__MPIR_INITIALIZED);


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

    mpi_errno = MPID_Init(required, &MPIR_ThreadInfo.thread_provided);
    MPIR_ERR_CHECK(mpi_errno);

    /* The current default mechanism of MPIR Process Acquisition Interface is to
     * break on MPIR_Breakpoint in mpiexec.hydra. Adding a PMI call that need
     * response from mpiexec will hold the MPI process to provide opportunity for
     * debugger to attach. Currently, the only effective call is PMI_Barrier.
     */
    /* NOTE: potentially we already calls PMI barrier during device business
     * card exchange. But there may be optimizations that make it not true.
     * We are adding a separate PMI barrier call here to ensure the debugger
     * mechanism. And it is also cleaner. If init latency is a concern, we may
     * add a config option to skip it. But focus on optimize PMI Barrier may
     * be a better effort.
     */
    if (!MPIR_CVAR_INIT_SKIP_PMI_BARRIER) {
        mpi_errno = MPIR_pmi_barrier_only();
        MPIR_ERR_CHECK(mpi_errno);
    }

    bool need_init_builtin_comms = true;
#ifdef ENABLE_LOCAL_SESSION_INIT
    need_init_builtin_comms = is_world_model;
#endif
    if (need_init_builtin_comms) {
        mpi_errno = MPIR_init_comm_world();
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_init_comm_self();
        MPIR_ERR_CHECK(mpi_errno);

#ifdef MPID_NEEDS_ICOMM_WORLD
        mpi_errno = MPIR_init_icomm_world();
        MPIR_ERR_CHECK(mpi_errno);
#endif
    }

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

    MPII_post_init_memory_tracing();
    MPII_init_dbg_logging();
    MPII_Wait_for_debugger();

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        MPII_dump_debug_summary();
    }

    /**********************************************************************/
    /* Section 6: simply lets the device know that we are done with
     * the full initialization, in case it wants to do some final
     * setup. */
    /**********************************************************************/

    if (is_world_model) {
        mpi_errno = MPIR_nodeid_init();
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPID_InitCompleted();
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__INITIALIZED);


    /**********************************************************************/
    /* Section 7: we are finally ready to start the asynchronous
     * thread, if needed. */
    /**********************************************************************/

    /* Reset isThreaded to the actual thread level */
#ifdef MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = (MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE);
#endif

  fn_exit:
    if (is_world_model) {
        if (!MPIR_Process.comm_world) {
            mpi_errno = MPIR_init_comm_world();
            MPIR_ERR_CHECK(mpi_errno);
        }

        if (!MPIR_Process.comm_self) {
            mpi_errno = MPIR_init_comm_self();
            MPIR_ERR_CHECK(mpi_errno);
        }

        MPII_world_set_initilized();
    }
    if (provided) {
        *provided = MPIR_ThreadInfo.thread_provided;
    }

    mpi_errno = MPII_init_async();
    MPIR_ERR_CHECK(mpi_errno);

    MPL_initlock_unlock(&MPIR_init_lock);
    return mpi_errno;

  fn_fail:
    MPL_initlock_unlock(&MPIR_init_lock);
    return mpi_errno;
}

int MPIR_Init_thread_impl(int *argc, char ***argv, int user_required, int *provided)
{
    return MPII_Init_thread(argc, argv, user_required, provided, NULL);
}

/* ------------ Finalize ------------------- */

void MPIR_Continue_global_finalize();

int MPII_Finalize(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = MPIR_Process.rank;
    bool is_world_model = (session_ptr == NULL);

    MPL_initlock_lock(&MPIR_init_lock);

    if (!is_world_model) {
        int session_refs = MPIR_Object_get_ref(session_ptr);
        if ((session_refs > 1) && session_ptr->strict_finalize) {
            /* For strict_finalize, we return an error if there still exist
             * other refs to the session (other than the self-ref).
             * In addition, we call MPID_Progress_poke() to allow users to
             * poll for success of the session finalize.
             */
            MPID_Progress_poke();
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_PENDING, "**sessioninuse", "**sessioninuse %d",
                                     session_refs - 1);
            goto fn_fail;
        }

        mpi_errno = MPIR_Session_release(session_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    init_counter--;
    if (init_counter > 0) {
        goto fn_exit;
    }

    mpi_errno = MPII_finalize_async();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Async_things_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Setting isThreaded to 0 to trick any operations used within
     * MPI_Finalize to think that we are running in a single threaded
     * environment. */
#ifdef MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = 0;
#endif

    /* Call the high-priority callbacks */
    MPII_Call_finalize_callbacks(MPIR_FINALIZE_CALLBACK_PRIO + 1, MPIR_FINALIZE_CALLBACK_MAX_PRIO);

    mpi_errno = MPIR_finalize_builtin_comms();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Process_bsend_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Signal the debugger that we are about to exit. */
    MPIR_Debugger_set_aborting(NULL);

    mpi_errno = MPID_Finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_pmi_finalize();

#ifdef ENABLE_QMPI
    MPII_qmpi_teardown();
#endif

    mpi_errno = MPII_Coll_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Call the low-priority (post Finalize) callbacks */
    MPII_Call_finalize_callbacks(0, MPIR_FINALIZE_CALLBACK_PRIO);

    MPII_hwtopo_finalize();
    MPII_nettopo_finalize();

    mpi_errno = MPII_finalize_builtin_infos();
    MPIR_ERR_CHECK(mpi_errno);

    /* Users did not call MPI_T_init_thread(), so we free memories allocated to
     * MPIR_T during MPI_Init here. Otherwise, free them in MPI_T_finalize() */
    if (!MPIR_T_is_initialized())
        MPIR_T_env_finalize();

    /* If performing coverage analysis, make each process sleep for
     * rank * 100 ms, to give time for the coverage tool to write out
     * any files.  It would be better if the coverage tool and runtime
     * was more careful about file updates, though the lack of OS support
     * for atomic file updates makes this harder. */
    MPII_final_coverage_delay(rank);

    mpi_errno = MPII_finalize_gpu();
    MPIR_ERR_CHECK(mpi_errno);

    if (is_world_model) {
        mpi_errno = MPIR_nodeid_free();
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPL_free(MPIR_Process.memory_alloc_kinds);
    MPIR_Process.memory_alloc_kinds = NULL;

    /* All memory should be freed at this point */
    MPIR_Continue_global_finalize();
    MPII_finalize_memory_tracing();

    MPII_thread_mutex_destroy();
    MPIR_Typerep_finalize();
    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__UNINITIALIZED);

  fn_exit:
    if (is_world_model) {
        MPII_world_set_finalized();
    }
    MPL_initlock_unlock(&MPIR_init_lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Finalize_impl(void)
{
    return MPII_Finalize(NULL);
}

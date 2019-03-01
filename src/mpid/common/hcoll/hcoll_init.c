/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "hcoll.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ENABLE_HCOLL
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Enable hcoll collective support.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int MPIDI_HCOLL_state_comm_world_initialized = 0;
static int MPIDI_HCOLL_state_progress_hook_id = 0;

int MPIDI_HCOLL_state_initialized = 0;
int MPIDI_HCOLL_state_enable = -1;
int MPIDI_HCOLL_state_enable_barrier = 1;
int MPIDI_HCOLL_state_enable_bcast = 1;
int MPIDI_HCOLL_state_enable_reduce = 1;
int MPIDI_HCOLL_state_enable_allgather = 1;
int MPIDI_HCOLL_state_enable_allreduce = 1;
int MPIDI_HCOLL_state_enable_alltoall = 1;
int MPIDI_HCOLL_state_enable_alltoallv = 1;
int MPIDI_HCOLL_state_enable_ibarrier = 1;
int MPIDI_HCOLL_state_enable_ibcast = 1;
int MPIDI_HCOLL_state_enable_iallgather = 1;
int MPIDI_HCOLL_state_enable_iallreduce = 1;
int MPIDI_HCOLL_state_world_comm_destroying = 0;

#if defined(MPL_USE_DBG_LOGGING)
MPL_dbg_class MPIR_DBG_HCOLL;
#endif /* MPL_USE_DBG_LOGGING */

void MPIDI_HCOLL_rte_fns_setup(void);

/* This will be called directly by code that is outside the MPIDI_HCOLL
 * class. For now, it is being called only during MPI_Finalize. Hence,
 * we define only an unsafe version of this function. If a safe version
 * is needed in the future, it needs to be defined and declared, possibly
 * with a change in this function's name stating that it is unsafe */
#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_state_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_state_destroy(void *param ATTRIBUTE((unused)))
{
    if (1 == MPIDI_HCOLL_state_initialized) {
        hcoll_finalize();
        MPID_Progress_deactivate_hook(MPIDI_HCOLL_state_progress_hook_id);
        MPID_Progress_deregister_hook(MPIDI_HCOLL_state_progress_hook_id);
    }
    MPIDI_HCOLL_state_initialized = 0;
    return 0;
}

#define CHECK_ENABLE_ENV_VARS(nameEnv, name) \
    do { \
        envar = getenv("HCOLL_ENABLE_" #nameEnv); \
        if (NULL != envar) { \
            MPIDI_HCOLL_state_enable_##name = atoi(envar); \
            MPL_DBG_MSG_D(MPIR_DBG_HCOLL, VERBOSE, "HCOLL_ENABLE_" #nameEnv " = %d\n", MPIDI_HCOLL_state_enable_##name); \
        } \
    } while (0)

/* Only the MPIDI_HCOLL class calls this function. It will be called only
 * once because it sets MPIDI_HCOLL_initialized and this function is called
 * only if MPIDI_HCOLL_initialized is not set. The MPI operations that
 * calls this function are MPI_Init and MPI_Type_commit. While multiple threads
 * could call the latter, the program is erroneous if MPI_Type_commit is called
 * before MPI_Init. Hence, it is guaranteed that this function will be called
 * by a single thread only (even if MPI_Init internally calls MPI_Type_commit).
 * So, this an unsafe function. If a thread safe version is needed in the
 * future, it needs to be defined and declared, possibly with a change in this
 * function's name stating that it is unsafe.*/
#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_state_initialize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_state_initialize(void)
{
    int mpi_errno;
    char *envar;
    hcoll_init_opts_t *init_opts;
    mpi_errno = MPI_SUCCESS;

    MPIDI_HCOLL_state_enable = (MPIR_CVAR_ENABLE_HCOLL | MPIR_CVAR_CH3_ENABLE_HCOLL);
    if (0 >= MPIDI_HCOLL_state_enable) {
        goto fn_exit;
    }
#if defined(MPL_USE_DBG_LOGGING)
    MPIR_DBG_HCOLL = MPL_dbg_class_alloc("HCOLL", "hcoll");
#endif /* MPL_USE_DBG_LOGGING */

    MPIDI_HCOLL_rte_fns_setup();

    hcoll_read_init_opts(&init_opts);
    init_opts->base_tag = MPIR_FIRST_HCOLL_TAG;
    init_opts->max_tag = MPIR_LAST_HCOLL_TAG;

#if defined MPICH_IS_THREADED
    init_opts->enable_thread_support = MPIR_ThreadInfo.isThreaded;
#else
    init_opts->enable_thread_support = 0;
#endif

    mpi_errno = hcoll_init_with_opts(&init_opts);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!MPIDI_HCOLL_state_initialized) {
        MPIDI_HCOLL_state_initialized = 1;
        mpi_errno =
            MPID_Progress_register_hook(MPIDI_HCOLL_state_progress,
                                        &MPIDI_HCOLL_state_progress_hook_id);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPID_Progress_activate_hook(MPIDI_HCOLL_state_progress_hook_id);
    }
    MPIR_Add_finalize(MPIDI_HCOLL_state_destroy, 0, 0);

    CHECK_ENABLE_ENV_VARS(BARRIER, barrier);
    CHECK_ENABLE_ENV_VARS(BCAST, bcast);
    CHECK_ENABLE_ENV_VARS(REDUCE, reduce);
    CHECK_ENABLE_ENV_VARS(ALLGATHER, allgather);
    CHECK_ENABLE_ENV_VARS(ALLREDUCE, allreduce);
    CHECK_ENABLE_ENV_VARS(ALLTOALL, alltoall);
    CHECK_ENABLE_ENV_VARS(ALLTOALLV, alltoallv);
    CHECK_ENABLE_ENV_VARS(IBARRIER, ibarrier);
    CHECK_ENABLE_ENV_VARS(IBCAST, ibcast);
    CHECK_ENABLE_ENV_VARS(IALLGATHER, iallgather);
    CHECK_ENABLE_ENV_VARS(IALLREDUCE, iallreduce);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_mpi_comm_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_mpi_comm_create(MPIR_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int num_ranks;
    int context_destroyed;
    mpi_errno = MPI_SUCCESS;

    if (0 == MPIDI_HCOLL_state_initialized) {
        mpi_errno = MPIDI_HCOLL_state_initialize();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (0 == MPIDI_HCOLL_state_enable) {
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        goto fn_exit;
    }

    if (MPIR_Process.comm_world == comm_ptr) {
        /* This is called only during MPI_Init and hence, no need
         * to worry about a race to set this value */
        MPIDI_HCOLL_state_comm_world_initialized = 1;
    }
    if (!MPIDI_HCOLL_state_comm_world_initialized) {
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        goto fn_exit;
    }
    num_ranks = comm_ptr->local_size;
    if ((MPIR_COMM_KIND__INTRACOMM != comm_ptr->comm_kind) || (2 > num_ranks)
        || comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__NODE_ROOTS
        || comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__NODE) {
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        goto fn_exit;
    }

    if (comm_ptr == MPIR_Process.comm_world) {
        /* This is during MPI_Init and hence, no need to protect this call */
        comm_ptr->hcoll_priv.hcoll_context = hcoll_create_context((rte_grp_handle_t) comm_ptr);
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        comm_ptr->hcoll_priv.hcoll_context = hcoll_create_context((rte_grp_handle_t) comm_ptr);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    if (NULL == comm_ptr->hcoll_priv.hcoll_context) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "Couldn't create hcoll context.");
        goto fn_fail;
    }

    comm_ptr->hcoll_priv.is_hcoll_init = 1;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_mpi_comm_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_mpi_comm_destroy(MPIR_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int context_destroyed;
    if (0 >= MPIDI_HCOLL_state_enable) {
        goto fn_exit;
    }
    mpi_errno = MPI_SUCCESS;

    context_destroyed = 0;
    if (comm_ptr->handle == MPI_COMM_WORLD) {
        /* This is during MPI_Finalize and hence, no need to worry about mutex */
        MPIDI_HCOLL_state_world_comm_destroying = 1;
        if ((NULL != comm_ptr) && (0 != comm_ptr->hcoll_priv.is_hcoll_init)) {
            hcoll_destroy_context(comm_ptr->hcoll_priv.hcoll_context,
                                  (rte_grp_handle_t) comm_ptr, &context_destroyed);
        }
    } else {
        if ((NULL != comm_ptr) && (0 != comm_ptr->hcoll_priv.is_hcoll_init)) {
            MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
            hcoll_destroy_context(comm_ptr->hcoll_priv.hcoll_context,
                                  (rte_grp_handle_t) comm_ptr, &context_destroyed);
            MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        }
    }
    comm_ptr->hcoll_priv.is_hcoll_init = 0;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_HCOLL_state_progress(int *made_progress)
{
    *made_progress = 1;
#if HCOLL_API < HCOLL_VERSION(4,0)
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    hcoll_progress_fn();
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
#else
    /* hcoll_progress_fn() has been deprecated since v4.0 and does nothing.
     * Hence, we don't need any thread safety here */
    hcoll_progress_fn();
#endif
    return MPI_SUCCESS;
}

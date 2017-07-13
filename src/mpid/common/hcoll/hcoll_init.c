/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

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

static int hcoll_initialized = 0;
static int hcoll_comm_world_initialized = 0;
static int hcoll_progress_hook_id = 0;

int hcoll_enable = -1;
int hcoll_enable_barrier = 1;
int hcoll_enable_bcast = 1;
int hcoll_enable_allgather = 1;
int hcoll_enable_allreduce = 1;
int hcoll_enable_ibarrier = 1;
int hcoll_enable_ibcast = 1;
int hcoll_enable_iallgather = 1;
int hcoll_enable_iallreduce = 1;
int hcoll_comm_attr_keyval = MPI_KEYVAL_INVALID;
int world_comm_destroying = 0;

#if defined(MPL_USE_DBG_LOGGING)
MPL_dbg_class MPIR_DBG_HCOLL;
#endif /* MPL_USE_DBG_LOGGING */

void hcoll_rte_fns_setup(void);

#undef FUNCNAME
#define FUNCNAME hcoll_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

int hcoll_destroy(void *param ATTRIBUTE((unused)))
{
    if (1 == hcoll_initialized) {
        hcoll_finalize();
        MPID_Progress_deactivate_hook(hcoll_progress_hook_id);
        MPID_Progress_deregister_hook(hcoll_progress_hook_id);
    }
    hcoll_initialized = 0;
    return 0;
}

static int hcoll_comm_attr_del_fn(MPI_Comm comm, int keyval, void *attr_val, void *extra_data)
{
    int mpi_errno;
    if (MPI_COMM_WORLD == comm) {
        world_comm_destroying = 1;
    }
    mpi_errno = hcoll_group_destroy_notify(attr_val);
    return mpi_errno;
}

#define CHECK_ENABLE_ENV_VARS(nameEnv, name) \
    do { \
        envar = getenv("HCOLL_ENABLE_" #nameEnv); \
        if (NULL != envar) { \
            hcoll_enable_##name = atoi(envar); \
            MPL_DBG_MSG_D(MPIR_DBG_HCOLL, VERBOSE, "HCOLL_ENABLE_" #nameEnv " = %d\n", hcoll_enable_##name); \
        } \
    } while (0)

#undef FUNCNAME
#define FUNCNAME hcoll_initialize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_initialize(void)
{
    int mpi_errno;
    char *envar;
    hcoll_init_opts_t *init_opts;
    mpi_errno = MPI_SUCCESS;

    hcoll_enable = MPIR_CVAR_ENABLE_HCOLL | MPIR_CVAR_CH3_ENABLE_HCOLL;
    if (0 >= hcoll_enable) {
        goto fn_exit;
    }

#if defined(MPL_USE_DBG_LOGGING)
    MPIR_DBG_HCOLL = MPL_dbg_class_alloc("HCOLL", "hcoll");
#endif /* MPL_USE_DBG_LOGGING */

    hcoll_rte_fns_setup();

    hcoll_read_init_opts(&init_opts);
    init_opts->base_tag = MPIR_FIRST_HCOLL_TAG;
    init_opts->max_tag  = MPIR_LAST_HCOLL_TAG;

#if defined MPICH_IS_THREADED
    init_opts->enable_thread_support = MPIR_ThreadInfo.isThreaded;
#else
    init_opts->enable_thread_support = 0;
#endif

    mpi_errno = hcoll_init_with_opts(&init_opts);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!hcoll_initialized) {
        hcoll_initialized = 1;
        mpi_errno = MPID_Progress_register_hook(hcoll_do_progress,
                                                &hcoll_progress_hook_id);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        MPID_Progress_activate_hook(hcoll_progress_hook_id);
    }
    MPIR_Add_finalize(hcoll_destroy, 0, 0);

    mpi_errno =
        MPIR_Comm_create_keyval_impl(MPI_NULL_COPY_FN, hcoll_comm_attr_del_fn,
                                     &hcoll_comm_attr_keyval, NULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    CHECK_ENABLE_ENV_VARS(BARRIER, barrier);
    CHECK_ENABLE_ENV_VARS(BCAST, bcast);
    CHECK_ENABLE_ENV_VARS(ALLGATHER, allgather);
    CHECK_ENABLE_ENV_VARS(ALLREDUCE, allreduce);
    CHECK_ENABLE_ENV_VARS(IBARRIER, ibarrier);
    CHECK_ENABLE_ENV_VARS(IBCAST, ibcast);
    CHECK_ENABLE_ENV_VARS(IALLGATHER, iallgather);
    CHECK_ENABLE_ENV_VARS(IALLREDUCE, iallreduce);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#define INSTALL_COLL_WRAPPER(check_name, name) \
    if (hcoll_enable_##check_name && (NULL != hcoll_collectives.coll_##check_name)) { \
        comm_ptr->coll_fns->name      = hcoll_##name; \
        MPL_DBG_MSG(MPIR_DBG_HCOLL,VERBOSE, #name " wrapper installed"); \
    }

#undef FUNCNAME
#define FUNCNAME hcoll_comm_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_comm_create(MPIR_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int num_ranks;
    int context_destroyed;
    mpi_errno = MPI_SUCCESS;

    if (0 == hcoll_enable) {
        goto fn_exit;
    }

    if (0 == hcoll_initialized) {
        mpi_errno = hcoll_initialize();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (MPIR_Process.comm_world == comm_ptr) {
        hcoll_comm_world_initialized = 1;
    }
    if (!hcoll_comm_world_initialized) {
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
    comm_ptr->hcoll_priv.hcoll_context = hcoll_create_context((rte_grp_handle_t) comm_ptr);
    if (NULL == comm_ptr->hcoll_priv.hcoll_context) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "Couldn't create hcoll context.");
        goto fn_fail;
    }
    mpi_errno =
        MPIR_Comm_set_attr_impl(comm_ptr, hcoll_comm_attr_keyval,
                                (void *) (comm_ptr->hcoll_priv.hcoll_context), MPIR_ATTR_PTR);
    if (mpi_errno) {
        hcoll_destroy_context(comm_ptr->hcoll_priv.hcoll_context,
                              (rte_grp_handle_t) comm_ptr, &context_destroyed);
        MPIR_Assert(context_destroyed);
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        MPIR_ERR_POP(mpi_errno);
    }
    comm_ptr->hcoll_priv.hcoll_origin_coll_fns = comm_ptr->coll_fns;
    comm_ptr->coll_fns = (MPIR_Collops *) MPL_malloc(sizeof(MPIR_Collops));
    memset(comm_ptr->coll_fns, 0, sizeof(MPIR_Collops));
    if (comm_ptr->hcoll_priv.hcoll_origin_coll_fns != 0) {
        memcpy(comm_ptr->coll_fns, comm_ptr->hcoll_priv.hcoll_origin_coll_fns,
               sizeof(MPIR_Collops));
    }
    INSTALL_COLL_WRAPPER(barrier, Barrier);
    INSTALL_COLL_WRAPPER(bcast, Bcast);
    INSTALL_COLL_WRAPPER(allreduce, Allreduce);
    INSTALL_COLL_WRAPPER(allgather, Allgather);

    /* INSTALL_COLL_WRAPPER(ibarrier, Ibarrier_req); */
    /* INSTALL_COLL_WRAPPER(ibcast, Ibcast_req); */
    /* INSTALL_COLL_WRAPPER(iallreduce, Iallreduce_req); */
    /* INSTALL_COLL_WRAPPER(iallgather, Iallgather_req); */

    comm_ptr->hcoll_priv.is_hcoll_init = 1;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME hcoll_comm_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_comm_destroy(MPIR_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int context_destroyed;
    if (0 >= hcoll_enable) {
        goto fn_exit;
    }
    mpi_errno = MPI_SUCCESS;

    if (comm_ptr == MPIR_Process.comm_world) {
        if (MPI_KEYVAL_INVALID != hcoll_comm_attr_keyval) {
            MPIR_Comm_free_keyval_impl(hcoll_comm_attr_keyval);
            hcoll_comm_attr_keyval = MPI_KEYVAL_INVALID;
        }
    }

    context_destroyed = 0;
    if ((NULL != comm_ptr) && (0 != comm_ptr->hcoll_priv.is_hcoll_init)) {
        if (NULL != comm_ptr->coll_fns) {
            MPL_free(comm_ptr->coll_fns);
        }
        comm_ptr->coll_fns = comm_ptr->hcoll_priv.hcoll_origin_coll_fns;
        hcoll_destroy_context(comm_ptr->hcoll_priv.hcoll_context,
                              (rte_grp_handle_t) comm_ptr, &context_destroyed);
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int hcoll_do_progress(int *made_progress)
{
    *made_progress = 1;
    hcoll_progress_fn();

    return MPI_SUCCESS;
}

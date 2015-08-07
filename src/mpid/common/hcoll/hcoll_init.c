#include "hcoll.h"

static int hcoll_initialized = 0;
static int hcoll_comm_world_initialized = 0;
static int hcoll_progress_hook_id = 0;

int hcoll_enable = 1;
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
            MPIU_DBG_MSG_D(CH3_OTHER, VERBOSE, "HCOLL_ENABLE_" #nameEnv " = %d\n", hcoll_enable_##name); \
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
    mpi_errno = MPI_SUCCESS;
    envar = getenv("HCOLL_ENABLE");
    if (NULL != envar) {
        hcoll_enable = atoi(envar);
    }
    if (0 == hcoll_enable) {
        goto fn_exit;
    }
    hcoll_rte_fns_setup();
    /*set INT_MAX/2 as tag_base here by the moment.
     * Need to think more about it.
     * The tag space should be positive, from > ~30 to MPI_TAG_UB value.
     * It looks reasonable to set MPI_TAG_UB as base but it doesn't work for ofacm tags
     * (probably due to wrong conversions from uint to int). That's why I set INT_MAX/2 instead of MPI_TAG_UB.
     * BUT: it won't work for collectives whose sequence number reaches INT_MAX/2 number. In that case tags become negative.
     * Moreover, it even won't work than 0 < (INT_MAX/2 - sequence number) < 30 because it can interleave with internal mpich coll tags.
     */
    hcoll_set_runtime_tag_offset(INT_MAX / 2, MPI_TAG_UB);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = hcoll_init();
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
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE, #name " wrapper installed"); \
    }

#undef FUNCNAME
#define FUNCNAME hcoll_comm_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_comm_create(MPID_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int num_ranks;
    int context_destroyed;
    mpi_errno = MPI_SUCCESS;
    if (0 == hcoll_initialized) {
        mpi_errno = hcoll_initialize();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (0 == hcoll_enable) {
        goto fn_exit;
    }
    if (MPIR_Process.comm_world == comm_ptr) {
        hcoll_comm_world_initialized = 1;
    }
    if (!hcoll_comm_world_initialized) {
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        goto fn_exit;
    }
    num_ranks = comm_ptr->local_size;
    if ((MPID_INTRACOMM != comm_ptr->comm_kind) || (2 > num_ranks)) {
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        goto fn_exit;
    }
    comm_ptr->hcoll_priv.hcoll_context = hcoll_create_context((rte_grp_handle_t) comm_ptr);
    if (NULL == comm_ptr->hcoll_priv.hcoll_context) {
        MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "Couldn't create hcoll context.");
        goto fn_fail;
    }
    mpi_errno =
        MPIR_Comm_set_attr_impl(comm_ptr, hcoll_comm_attr_keyval,
                                (void *) (comm_ptr->hcoll_priv.hcoll_context), MPIR_ATTR_PTR);
    if (mpi_errno) {
        hcoll_destroy_context(comm_ptr->hcoll_priv.hcoll_context,
                              (rte_grp_handle_t) comm_ptr, &context_destroyed);
        MPIU_Assert(context_destroyed);
        comm_ptr->hcoll_priv.is_hcoll_init = 0;
        MPIR_ERR_POP(mpi_errno);
    }
    comm_ptr->hcoll_priv.hcoll_origin_coll_fns = comm_ptr->coll_fns;
    comm_ptr->coll_fns = (MPID_Collops *) MPIU_Malloc(sizeof(MPID_Collops));
    memset(comm_ptr->coll_fns, 0, sizeof(MPID_Collops));
    if (comm_ptr->hcoll_priv.hcoll_origin_coll_fns != 0) {
        memcpy(comm_ptr->coll_fns, comm_ptr->hcoll_priv.hcoll_origin_coll_fns,
               sizeof(MPID_Collops));
    }
    INSTALL_COLL_WRAPPER(barrier, Barrier);
    INSTALL_COLL_WRAPPER(bcast, Bcast);
    INSTALL_COLL_WRAPPER(allreduce, Allreduce);
    INSTALL_COLL_WRAPPER(allgather, Allgather);
    INSTALL_COLL_WRAPPER(ibarrier, Ibarrier_req);
    INSTALL_COLL_WRAPPER(ibcast, Ibcast_req);
    INSTALL_COLL_WRAPPER(iallreduce, Iallreduce_req);
    INSTALL_COLL_WRAPPER(iallgather, Iallgather_req);

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
int hcoll_comm_destroy(MPID_Comm * comm_ptr, void *param)
{
    int mpi_errno;
    int context_destroyed;
    if (0 == hcoll_enable) {
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
            MPIU_Free(comm_ptr->coll_fns);
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
    if (made_progress)
        *made_progress = 0;
    hcoll_progress_fn();

    return MPI_SUCCESS;
}

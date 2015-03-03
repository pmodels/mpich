/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_RMA_SLOTS_SIZE
      category    : CH3
      type        : int
      default     : 262144
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of RMA slots during window creation. Each slot contains
        a linked list of target elements. The distribution of ranks among
        slots follows a round-robin pattern. Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_LOCK_DATA_BYTES
      category    : CH3
      type        : int
      default     : 655360
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size (in bytes) of available lock data this window can provided. If
        current buffered lock data is more than this value, the process will
        drop the upcoming operation data. Requires a positive calue.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


MPIU_THREADSAFE_INIT_DECL(initRMAoptions);

MPIDI_RMA_Win_list_t *MPIDI_RMA_Win_list = NULL, *MPIDI_RMA_Win_list_tail = NULL;

static int win_init(MPI_Aint size, int disp_unit, int create_flavor, int model, MPID_Info * info,
                    MPID_Comm * comm_ptr, MPID_Win ** win_ptr);


#define MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr)                   \
    do {                                                        \
        /* Get ptr to RMAFns, which is embedded in MPID_Win */  \
        MPID_RMAFns *ftable         = &(*(win_ptr))->RMAFns;    \
        ftable->Win_free            = MPIDI_Win_free;           \
        ftable->Win_attach          = MPIDI_Win_attach;         \
        ftable->Win_detach          = MPIDI_Win_detach;         \
        ftable->Win_shared_query    = MPIDI_Win_shared_query;   \
                                                                \
        ftable->Win_set_info        = MPIDI_Win_set_info;       \
        ftable->Win_get_info        = MPIDI_Win_get_info;       \
                                                                \
        ftable->Put                 = MPIDI_Put;                \
        ftable->Get                 = MPIDI_Get;                \
        ftable->Accumulate          = MPIDI_Accumulate;         \
        ftable->Get_accumulate      = MPIDI_Get_accumulate;     \
        ftable->Fetch_and_op        = MPIDI_Fetch_and_op;       \
        ftable->Compare_and_swap    = MPIDI_Compare_and_swap;   \
                                                                \
        ftable->Rput                = MPIDI_Rput;               \
        ftable->Rget                = MPIDI_Rget;               \
        ftable->Raccumulate         = MPIDI_Raccumulate;        \
        ftable->Rget_accumulate     = MPIDI_Rget_accumulate;    \
                                                                \
        ftable->Win_fence           = MPIDI_Win_fence;          \
        ftable->Win_post            = MPIDI_Win_post;           \
        ftable->Win_start           = MPIDI_Win_start;          \
        ftable->Win_complete        = MPIDI_Win_complete;       \
        ftable->Win_wait            = MPIDI_Win_wait;           \
        ftable->Win_test            = MPIDI_Win_test;           \
                                                                \
        ftable->Win_lock            = MPIDI_Win_lock;           \
        ftable->Win_unlock          = MPIDI_Win_unlock;         \
        ftable->Win_lock_all        = MPIDI_Win_lock_all;       \
        ftable->Win_unlock_all      = MPIDI_Win_unlock_all;     \
                                                                \
        ftable->Win_flush           = MPIDI_Win_flush;          \
        ftable->Win_flush_all       = MPIDI_Win_flush_all;      \
        ftable->Win_flush_local     = MPIDI_Win_flush_local;    \
        ftable->Win_flush_local_all = MPIDI_Win_flush_local_all;\
        ftable->Win_sync            = MPIDI_Win_sync;           \
    } while (0)


#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info * info,
                    MPID_Comm * comm_ptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm_ptr->revoked) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPIX_ERR_REVOKED, "**revoked");
    }

    mpi_errno =
        win_init(size, disp_unit, MPI_WIN_FLAVOR_CREATE, MPI_WIN_UNIFIED, info, comm_ptr, win_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    (*win_ptr)->base = base;

    mpi_errno = MPIDI_CH3U_Win_fns.create(base, size, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_allocate(MPI_Aint size, int disp_unit, MPID_Info * info,
                      MPID_Comm * comm_ptr, void *baseptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_ALLOCATE);

    mpi_errno =
        win_init(size, disp_unit, MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED, info, comm_ptr,
                 win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDI_CH3U_Win_fns.allocate(size, disp_unit, info, comm_ptr, baseptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_ALLOCATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_create_dynamic
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_create_dynamic(MPID_Info * info, MPID_Comm * comm_ptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);

    mpi_errno = win_init(0 /* spec defines size to be 0 */ ,
                         1 /* spec defines disp_unit to be 1 */ ,
                         MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED, info, comm_ptr, win_ptr);

    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    (*win_ptr)->base = MPI_BOTTOM;

    mpi_errno = MPIDI_CH3U_Win_fns.create_dynamic(info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
    return mpi_errno;
}


/* The memory allocation functions */
#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPID_Alloc_mem(size_t size, MPID_Info * info_ptr)
{
    void *ap = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ALLOC_MEM);

    ap = MPIDI_Alloc_mem(size, info_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return ap;
}


#undef FUNCNAME
#define FUNCNAME MPID_Free_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_FREE_MEM);

    mpi_errno = MPIDI_Free_mem(ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate_shared
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_allocate_shared(MPI_Aint size, int disp_unit, MPID_Info * info, MPID_Comm * comm_ptr,
                             void *base_ptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);

    mpi_errno =
        win_init(size, disp_unit, MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED, info, comm_ptr, win_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    mpi_errno =
        MPIDI_CH3U_Win_fns.allocate_shared(size, disp_unit, info, comm_ptr, base_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME win_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int win_init(MPI_Aint size, int disp_unit, int create_flavor, int model, MPID_Info * info,
                    MPID_Comm * comm_ptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_Comm *win_comm_ptr;
    int win_target_pool_size;
    MPIDI_RMA_Win_list_t *win_elem;
    MPIU_CHKPMEM_DECL(5);
    MPIDI_STATE_DECL(MPID_STATE_WIN_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_WIN_INIT);

    if (initRMAoptions) {
        MPIU_THREADSAFE_INIT_BLOCK_BEGIN(initRMAoptions);

        MPIDI_CH3_RMA_Init_sync_pvars();
        MPIDI_CH3_RMA_Init_pkthandler_pvars();

        MPIU_THREADSAFE_INIT_CLEAR(initRMAoptions);
        MPIU_THREADSAFE_INIT_BLOCK_END(initRMAoptions);
    }

    *win_ptr = (MPID_Win *) MPIU_Handle_obj_alloc(&MPID_Win_mem);
    MPIU_ERR_CHKANDJUMP1(!(*win_ptr), mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPID_Win_mem");

    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, &win_comm_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    MPIU_Object_set_ref(*win_ptr, 1);

    /* (*win_ptr)->errhandler is set by upper level; */
    /* (*win_ptr)->base is set by caller; */
    (*win_ptr)->size = size;
    (*win_ptr)->disp_unit = disp_unit;
    (*win_ptr)->create_flavor = create_flavor;
    (*win_ptr)->model = model;
    (*win_ptr)->attributes = NULL;
    (*win_ptr)->comm_ptr = win_comm_ptr;

    (*win_ptr)->at_completion_counter = 0;
    (*win_ptr)->shm_base_addrs = NULL;
    /* (*win_ptr)->basic_info_table[] is set by caller; */
    (*win_ptr)->current_lock_type = MPID_LOCK_NONE;
    (*win_ptr)->shared_lock_ref_cnt = 0;
    (*win_ptr)->lock_queue = NULL;
    (*win_ptr)->lock_queue_tail = NULL;
    (*win_ptr)->shm_allocated = FALSE;
    (*win_ptr)->states.access_state = MPIDI_RMA_NONE;
    (*win_ptr)->states.exposure_state = MPIDI_RMA_NONE;
    (*win_ptr)->non_empty_slots = 0;
    (*win_ptr)->accumulated_ops_cnt = 0;
    (*win_ptr)->active_req_cnt = 0;
    (*win_ptr)->fence_sync_req = MPI_REQUEST_NULL;
    (*win_ptr)->start_req = NULL;
    (*win_ptr)->start_ranks_in_win_grp = NULL;
    (*win_ptr)->start_grp_size = 0;
    (*win_ptr)->lock_all_assert = 0;
    (*win_ptr)->lock_epoch_count = 0;
    (*win_ptr)->outstanding_locks = 0;
    (*win_ptr)->current_lock_data_bytes = 0;

    /* Initialize the info flags */
    (*win_ptr)->info_args.no_locks = 0;
    (*win_ptr)->info_args.accumulate_ordering = MPIDI_ACC_ORDER_RAR | MPIDI_ACC_ORDER_RAW |
        MPIDI_ACC_ORDER_WAR | MPIDI_ACC_ORDER_WAW;
    (*win_ptr)->info_args.accumulate_ops = MPIDI_ACC_OPS_SAME_OP_NO_OP;
    (*win_ptr)->info_args.same_size = 0;
    (*win_ptr)->info_args.alloc_shared_noncontig = 0;
    (*win_ptr)->info_args.alloc_shm = FALSE;

    /* Set function pointers on window */
    MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr);

    /* Set info_args on window based on info provided by user */
    mpi_errno = (*win_ptr)->RMAFns.Win_set_info((*win_ptr), info);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    MPIU_CHKPMEM_MALLOC((*win_ptr)->op_pool_start, MPIDI_RMA_Op_t *,
                        sizeof(MPIDI_RMA_Op_t) * MPIR_CVAR_CH3_RMA_OP_WIN_POOL_SIZE, mpi_errno,
                        "RMA op pool");
    (*win_ptr)->op_pool = NULL;
    (*win_ptr)->op_pool_tail = NULL;
    for (i = 0; i < MPIR_CVAR_CH3_RMA_OP_WIN_POOL_SIZE; i++) {
        (*win_ptr)->op_pool_start[i].pool_type = MPIDI_RMA_POOL_WIN;
        MPL_LL_APPEND((*win_ptr)->op_pool, (*win_ptr)->op_pool_tail,
                      &((*win_ptr)->op_pool_start[i]));
    }

    win_target_pool_size =
        MPIR_MIN(MPIR_CVAR_CH3_RMA_TARGET_WIN_POOL_SIZE, MPIR_Comm_size(win_comm_ptr));
    MPIU_CHKPMEM_MALLOC((*win_ptr)->target_pool_start, MPIDI_RMA_Target_t *,
                        sizeof(MPIDI_RMA_Target_t) * win_target_pool_size, mpi_errno,
                        "RMA target pool");
    (*win_ptr)->target_pool = NULL;
    (*win_ptr)->target_pool_tail = NULL;
    for (i = 0; i < win_target_pool_size; i++) {
        (*win_ptr)->target_pool_start[i].pool_type = MPIDI_RMA_POOL_WIN;
        MPL_LL_APPEND((*win_ptr)->target_pool, (*win_ptr)->target_pool_tail,
                      &((*win_ptr)->target_pool_start[i]));
    }

    (*win_ptr)->num_slots = MPIR_MIN(MPIR_CVAR_CH3_RMA_SLOTS_SIZE, MPIR_Comm_size(win_comm_ptr));
    MPIU_CHKPMEM_MALLOC((*win_ptr)->slots, MPIDI_RMA_Slot_t *,
                        sizeof(MPIDI_RMA_Slot_t) * (*win_ptr)->num_slots, mpi_errno, "RMA slots");
    for (i = 0; i < (*win_ptr)->num_slots; i++) {
        (*win_ptr)->slots[i].target_list = NULL;
        (*win_ptr)->slots[i].target_list_tail = NULL;
    }

    if (!(*win_ptr)->info_args.no_locks) {
        MPIU_CHKPMEM_MALLOC((*win_ptr)->lock_entry_pool_start, MPIDI_RMA_Lock_entry_t *,
                            sizeof(MPIDI_RMA_Lock_entry_t) *
                            MPIR_CVAR_CH3_RMA_LOCK_ENTRY_WIN_POOL_SIZE, mpi_errno,
                            "RMA lock entry pool");
        (*win_ptr)->lock_entry_pool = NULL;
        (*win_ptr)->lock_entry_pool_tail = NULL;
        for (i = 0; i < MPIR_CVAR_CH3_RMA_LOCK_ENTRY_WIN_POOL_SIZE; i++) {
            MPL_LL_APPEND((*win_ptr)->lock_entry_pool, (*win_ptr)->lock_entry_pool_tail,
                          &((*win_ptr)->lock_entry_pool_start[i]));
        }
    }

    /* enqueue window into the global list */
    MPIU_CHKPMEM_MALLOC(win_elem, MPIDI_RMA_Win_list_t *, sizeof(MPIDI_RMA_Win_list_t), mpi_errno,
                        "Window list element");
    win_elem->win_ptr = *win_ptr;
    MPL_LL_APPEND(MPIDI_RMA_Win_list, MPIDI_RMA_Win_list_tail, win_elem);

    if (MPIDI_CH3U_Win_hooks.win_init != NULL) {
        mpi_errno =
            MPIDI_CH3U_Win_hooks.win_init(size, disp_unit, create_flavor, model, info, comm_ptr,
                                          win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_WIN_INIT);
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_set_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_set_info(MPID_Win * win, MPID_Info * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_SET_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_SET_INFO);

    mpi_errno = win->RMAFns.Win_set_info(win, info);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_get_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_get_info(MPID_Win * win, MPID_Info ** info_used)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_GET_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_GET_INFO);

    mpi_errno = win->RMAFns.Win_get_info(win, info_used);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_GET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

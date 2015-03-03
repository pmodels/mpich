/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : CH3
      description : cvars that control behavior of ch3

cvars:
    - name        : MPIR_CVAR_CH3_RMA_OP_WIN_POOL_SIZE
      category    : CH3
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the window-private RMA operations pool (in number of
        operations) that stores information about RMA operations that
        could not be issued immediately.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE
      category    : CH3
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the Global RMA operations pool (in number of
        operations) that stores information about RMA operations that
        could not be issued immediatly.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_TARGET_WIN_POOL_SIZE
      category    : CH3
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the window-private RMA target pool (in number of
        targets) that stores information about RMA targets that
        could not be issued immediately.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE
      category    : CH3
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the Global RMA targets pool (in number of
        targets) that stores information about RMA targets that
        could not be issued immediatly.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_LOCK_ENTRY_WIN_POOL_SIZE
      category    : CH3
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the window-private RMA lock entries pool (in number of
        lock entries) that stores information about RMA lock requests that
        could not be satisfied immediatly.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_LOCK_ENTRY_GLOBAL_POOL_SIZE
      category    : CH3
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the Global RMA lock entries pool (in number of
        lock entries) that stores information about RMA lock requests that
        could not be satisfied immediatly.  Requires a positive value.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


MPIDI_RMA_Op_t *global_rma_op_pool = NULL, *global_rma_op_pool_tail =
    NULL, *global_rma_op_pool_start = NULL;
MPIDI_RMA_Target_t *global_rma_target_pool = NULL, *global_rma_target_pool_tail =
    NULL, *global_rma_target_pool_start = NULL;
MPIDI_RMA_Pkt_orderings_t *MPIDI_RMA_Pkt_orderings = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_RMA_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_RMA_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL(3);

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RMA_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_RMA_INIT);

    MPIU_CHKPMEM_MALLOC(global_rma_op_pool_start, MPIDI_RMA_Op_t *,
                        sizeof(MPIDI_RMA_Op_t) * MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA op pool");
    for (i = 0; i < MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE; i++) {
        global_rma_op_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        MPL_LL_APPEND(global_rma_op_pool, global_rma_op_pool_tail, &(global_rma_op_pool_start[i]));
    }

    MPIU_CHKPMEM_MALLOC(global_rma_target_pool_start, MPIDI_RMA_Target_t *,
                        sizeof(MPIDI_RMA_Target_t) * MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA target pool");
    for (i = 0; i < MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE; i++) {
        global_rma_target_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        MPL_LL_APPEND(global_rma_target_pool, global_rma_target_pool_tail,
                      &(global_rma_target_pool_start[i]));
    }

    MPIU_CHKPMEM_MALLOC(MPIDI_RMA_Pkt_orderings, struct MPIDI_RMA_Pkt_orderings *,
                        sizeof(struct MPIDI_RMA_Pkt_orderings), mpi_errno, "RMA packet orderings");
    /* FIXME: here we should let channel to set ordering flags. For now we just set them
     * in CH3 layer. */
    MPIDI_RMA_Pkt_orderings->flush_remote = 1;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RMA_INIT);
    return mpi_errno;

  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_fail;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_RMA_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_RMA_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RMA_FINALIZE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_RMA_FINALIZE);

    MPIU_Free(global_rma_op_pool_start);
    MPIU_Free(global_rma_target_pool_start);
    MPIU_Free(MPIDI_RMA_Pkt_orderings);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RMA_FINALIZE);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_free(MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;
    MPID_Comm *comm_ptr;
    mpir_errflag_t errflag = MPIR_ERR_NONE;
    MPIDI_RMA_Win_list_t *win_elem;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FREE);

    /* it is possible that there is a IBARRIER in MPI_WIN_FENCE with
     * MODE_NOPRECEDE not being completed, we let the progress engine
     * to delete its request when it is completed. */
    if ((*win_ptr)->fence_sync_req != MPI_REQUEST_NULL) {
        MPID_Request *req_ptr;
        MPID_Request_get_ptr((*win_ptr)->fence_sync_req, req_ptr);
        MPID_Request_release(req_ptr);
        (*win_ptr)->fence_sync_req = MPI_REQUEST_NULL;
        (*win_ptr)->states.access_state = MPIDI_RMA_NONE;
    }

    if ((*win_ptr)->states.access_state == MPIDI_RMA_FENCE_GRANTED)
        (*win_ptr)->states.access_state = MPIDI_RMA_NONE;

    MPIU_ERR_CHKANDJUMP((*win_ptr)->states.access_state != MPIDI_RMA_NONE ||
                        (*win_ptr)->states.exposure_state != MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* 1. Here we must wait until all passive locks are released on this target,
     * because for some UNLOCK messages, we do not send ACK back to origin,
     * we must wait until lock is released so that we can free window.
     * 2. We also need to wait until AT completion counter being zero, because
     * this counter is increment everytime we meet a GET-like operation, it is
     * possible that when target entering Win_free, passive epoch is not finished
     * yet and there are still GETs doing on this target.
     * 3. We also need to wait until lock queue becomes empty. It is possible
     * that some lock requests is still waiting in the queue when target is
     * entering Win_free. */
    while ((*win_ptr)->current_lock_type != MPID_LOCK_NONE ||
           (*win_ptr)->at_completion_counter != 0 ||
           (*win_ptr)->lock_queue != NULL || (*win_ptr)->current_lock_data_bytes != 0) {
        mpi_errno = wait_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Barrier_impl((*win_ptr)->comm_ptr, &errflag);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    /* Free window resources in lower layer. */
    if (MPIDI_CH3U_Win_hooks.win_free != NULL) {
        mpi_errno = MPIDI_CH3U_Win_hooks.win_free(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    /* dequeue window from the global list */
    for (win_elem = MPIDI_RMA_Win_list; win_elem && win_elem->win_ptr != *win_ptr;
         win_elem = win_elem->next);
    MPIU_ERR_CHKANDJUMP(win_elem == NULL, mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    MPL_LL_DELETE(MPIDI_RMA_Win_list, MPIDI_RMA_Win_list_tail, win_elem);
    MPIU_Free(win_elem);

    comm_ptr = (*win_ptr)->comm_ptr;
    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    if ((*win_ptr)->basic_info_table != NULL)
        MPIU_Free((*win_ptr)->basic_info_table);
    MPIU_Free((*win_ptr)->op_pool_start);
    MPIU_Free((*win_ptr)->target_pool_start);
    MPIU_Free((*win_ptr)->slots);
    if (!(*win_ptr)->info_args.no_locks) {
        MPIU_Free((*win_ptr)->lock_entry_pool_start);
    }
    MPIU_Assert((*win_ptr)->current_lock_data_bytes == 0);

    /* Free the attached buffer for windows created with MPI_Win_allocate() */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE ||
        (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if ((*win_ptr)->shm_allocated == FALSE && (*win_ptr)->size > 0) {
            MPIU_Free((*win_ptr)->base);
        }
    }

    MPIU_Object_release_ref(*win_ptr, &in_use);
    /* MPI windows don't have reference count semantics, so this should always be true */
    MPIU_Assert(!in_use);
    MPIU_Handle_obj_free(&MPID_Win_mem, *win_ptr);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FREE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_shared_query
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_shared_query(MPID_Win * win_ptr, int target_rank, MPI_Aint * size,
                           int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_SHARED_QUERY);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_SHARED_QUERY);

    *(void **) baseptr = win_ptr->base;
    *size = win_ptr->size;
    *disp_unit = win_ptr->disp_unit;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_SHARED_QUERY);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPIDI_Alloc_mem(size_t size, MPID_Info * info_ptr)
{
    void *ap;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_ALLOC_MEM);

    ap = MPIDI_CH3I_Alloc_mem(size, info_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_ALLOC_MEM);
    return ap;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Free_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_FREE_MEM);

    MPIDI_CH3I_Free_mem(ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_FREE_MEM);
    return mpi_errno;
}

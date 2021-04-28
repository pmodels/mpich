/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
        could not be issued immediately.  Requires a positive value.

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
        could not be issued immediately.  Requires a positive value.

    - name        : MPIR_CVAR_CH3_RMA_TARGET_LOCK_ENTRY_WIN_POOL_SIZE
      category    : CH3
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of the window-private RMA lock entries pool (in number of
        lock entries) that stores information about RMA lock requests that
        could not be satisfied immediately.  Requires a positive value.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


MPIDI_RMA_Op_t *global_rma_op_pool_head = NULL, *global_rma_op_pool_start = NULL;
MPIDI_RMA_Target_t *global_rma_target_pool_head = NULL, *global_rma_target_pool_start = NULL;

int MPIDI_RMA_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_CHKPMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RMA_INIT);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_RMA_INIT);

    MPIR_CHKPMEM_MALLOC(global_rma_op_pool_start, MPIDI_RMA_Op_t *,
                        sizeof(MPIDI_RMA_Op_t) * MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA op pool", MPL_MEM_RMA);
    for (i = 0; i < MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE; i++) {
        global_rma_op_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        DL_APPEND(global_rma_op_pool_head, &(global_rma_op_pool_start[i]));
    }

    MPIR_CHKPMEM_MALLOC(global_rma_target_pool_start, MPIDI_RMA_Target_t *,
                        sizeof(MPIDI_RMA_Target_t) * MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA target pool", MPL_MEM_RMA);
    for (i = 0; i < MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE; i++) {
        global_rma_target_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        DL_APPEND(global_rma_target_pool_head, &(global_rma_target_pool_start[i]));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RMA_INIT);
    return mpi_errno;

  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}


void MPIDI_RMA_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RMA_FINALIZE);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_RMA_FINALIZE);

    MPL_free(global_rma_op_pool_start);
    MPL_free(global_rma_target_pool_start);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RMA_FINALIZE);
}


int MPID_Win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;
    MPIR_Comm *comm_ptr;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FREE);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPID_WIN_FREE);

    MPIR_ERR_CHKANDJUMP(((*win_ptr)->states.access_state != MPIDI_RMA_NONE &&
                         (*win_ptr)->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                         (*win_ptr)->states.access_state != MPIDI_RMA_FENCE_GRANTED) ||
                        ((*win_ptr)->states.exposure_state != MPIDI_RMA_NONE),
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* 1. Here we must wait until all passive locks are released on this target,
     * because for some UNLOCK messages, we do not send ACK back to origin,
     * we must wait until lock is released so that we can free window.
     * 2. We also need to wait until AT completion counter being zero, because
     * this counter is increment every time we meet a GET-like operation, it is
     * possible that when target entering Win_free, passive epoch is not finished
     * yet and there are still GETs doing on this target.
     * 3. We also need to wait until lock queue becomes empty. It is possible
     * that some lock requests is still waiting in the queue when target is
     * entering Win_free. */
    while ((*win_ptr)->current_lock_type != MPID_LOCK_NONE ||
           (*win_ptr)->at_completion_counter != 0 ||
           (*win_ptr)->target_lock_queue_head != NULL ||
           (*win_ptr)->current_target_lock_data_bytes != 0 || (*win_ptr)->sync_request_cnt != 0) {
        mpi_errno = wait_progress_engine();
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Barrier((*win_ptr)->comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Free window resources in lower layer. */
    if (MPIDI_CH3U_Win_hooks.win_free != NULL) {
        mpi_errno = MPIDI_CH3U_Win_hooks.win_free(win_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* dequeue window from the global list */
    MPIR_Assert((*win_ptr)->active == FALSE);
    DL_DELETE(MPIDI_RMA_Win_inactive_list_head, (*win_ptr));

    if (MPIDI_RMA_Win_inactive_list_head == NULL && MPIDI_RMA_Win_active_list_head == NULL) {
        /* this is the last window, de-register RMA progress hook */
        mpi_errno = MPIR_Progress_hook_deregister(MPIDI_CH3I_RMA_Progress_hook_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    comm_ptr = (*win_ptr)->comm_ptr;
    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free((*win_ptr)->basic_info_table);
    MPL_free((*win_ptr)->op_pool_start);
    MPL_free((*win_ptr)->target_pool_start);
    MPL_free((*win_ptr)->slots);
    MPL_free((*win_ptr)->target_lock_entry_pool_start);

    MPIR_Assert((*win_ptr)->current_target_lock_data_bytes == 0);

    /* Free the attached buffer for windows created with MPI_Win_allocate() */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE ||
        (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if ((*win_ptr)->shm_allocated == FALSE && (*win_ptr)->size > 0) {
            MPL_free((*win_ptr)->base);
        }
    }

    MPIR_Object_release_ref(*win_ptr, &in_use);
    /* MPI windows don't have reference count semantics, so this should always be true */
    MPIR_Assert(!in_use);
    MPIR_Handle_obj_free(&MPIR_Win_mem, *win_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPID_WIN_FREE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


struct MPIDI_RMA_Op *global_rma_op_pool = NULL, *global_rma_op_pool_tail = NULL, *global_rma_op_pool_start = NULL;
struct MPIDI_RMA_Target *global_rma_target_pool = NULL, *global_rma_target_pool_tail = NULL, *global_rma_target_pool_start = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_RMA_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_RMA_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL(2);

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RMA_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_RMA_INIT);

    MPIU_CHKPMEM_MALLOC(global_rma_op_pool_start, struct MPIDI_RMA_Op *,
                        sizeof(struct MPIDI_RMA_Op) * MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA op pool");
    for (i = 0; i < MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE; i++) {
        global_rma_op_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        MPL_LL_APPEND(global_rma_op_pool, global_rma_op_pool_tail, &(global_rma_op_pool_start[i]));
    }

    MPIU_CHKPMEM_MALLOC(global_rma_target_pool_start, struct MPIDI_RMA_Target *,
                        sizeof(struct MPIDI_RMA_Target) * MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE,
                        mpi_errno, "RMA target pool");
    for (i = 0; i < MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE; i++) {
        global_rma_target_pool_start[i].pool_type = MPIDI_RMA_POOL_GLOBAL;
        MPL_LL_APPEND(global_rma_target_pool, global_rma_target_pool_tail, &(global_rma_target_pool_start[i]));
    }

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
    int errflag = FALSE;
    MPIDI_RMA_Win_list_t *win_elem;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FREE);

    MPIU_ERR_CHKANDJUMP((*win_ptr)->epoch_state != MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (!(*win_ptr)->shm_allocated) {
        /* when SHM is allocated, we already did a global barrier in
           MPIDI_CH3_SHM_Win_free, so we do not need to do it again here. */
        mpi_errno = MPIR_Barrier_impl((*win_ptr)->comm_ptr, &errflag);
        if (mpi_errno)
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

    MPIU_Free((*win_ptr)->targets);
    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->sizes);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->op_pool_start);
    MPIU_Free((*win_ptr)->target_pool_start);
    MPIU_Free((*win_ptr)->slots);

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

    ap = MPIU_Malloc(size);

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

    MPIU_Free(ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_FREE_MEM);
    return mpi_errno;
}

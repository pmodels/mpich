/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidrma.h"

static int enableShortACC=1;

#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_alloc);
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_set);
extern void MPIDI_CH3_RMA_InitInstr(void);
#endif

#define MPIDI_PASSIVE_TARGET_DONE_TAG  348297
#define MPIDI_PASSIVE_TARGET_RMA_TAG 563924

/* 
 * TODO:
 * Explore use of alternate allocation mechanisms for the RMA queue elements
 * (Because profiling has shown that queue element allocation/deallocation
 * can take a significant amount of time in the RMA operations).
 *    1: Current approach (uses perm memory malloc/free)
 *    2: Preallocate and maintain list (use perm memory malloc, but
 *       free onto window; use first; free on window free)
 *    3: Preallocate and maintain list (use separate memory, but free to
 *       thread/process; free in Finalize handler.  Option to use for
 *       single-threaded to avoid thread overheads)
 * Possible interface
 *    int MPIDI_RMAListAlloc(MPIDI_RMA_Op_t **a,MPID_Win *win)
 *    int MPIDI_RMAListFree(MPIDI_RMA_Op_t *a, MPID_Win *win)
 *    return value is error code (e.g., allocation failure).
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS, total_pt_rma_puts_accs;
    int in_use;
    MPID_Comm *comm_ptr;
    int errflag = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FREE);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FREE);

    MPIU_ERR_CHKANDJUMP((*win_ptr)->epoch_state != MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    mpi_errno = MPIDI_CH3I_Wait_for_pt_ops_finish(*win_ptr);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

    comm_ptr = (*win_ptr)->comm_ptr;
    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Free((*win_ptr)->targets);
    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->sizes);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->pt_rma_puts_accs);

    /* Free the attached buffer for windows created with MPI_Win_allocate() */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE && (*win_ptr)->size > 0) {
        if ((*win_ptr)->shm_allocated != TRUE)
            MPIU_Free((*win_ptr)->base);
    }

    MPIU_Object_release_ref(*win_ptr, &in_use);
    /* MPI windows don't have reference count semantics, so this should always be true */
    MPIU_Assert(!in_use);
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );

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
int MPIDI_Win_shared_query(MPID_Win *win_ptr, int target_rank, MPI_Aint *size,
                           int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_SHARED_QUERY);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_SHARED_QUERY);

    *(void**) baseptr = win_ptr->shm_base_addrs[0];
    *size             = win_ptr->size;
    *disp_unit        = win_ptr->disp_unit;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_SHARED_QUERY);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Put(const void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused)), rank, predefined;
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_PUT);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_PUT);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp,dt_true_lb); 
    
    if (data_sz == 0) {
	goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;
    
    /* If the put is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        mpi_errno = MPIDI_CH3I_Shm_put_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_VC_t *orig_vc, *target_vc;

	/* queue it up */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	MPIU_INSTR_DURATION_START(rmaqueue_set);
	/* FIXME: For contig and very short operations, use a streamlined op */
	new_ptr->type = MPIDI_RMA_PUT;
        /* Cast away const'ness for the origin address, as the
         * MPIDI_RMA_Op_t structure is used for both PUT and GET like
         * operations */
	new_ptr->origin_addr = (void *) origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	MPIU_INSTR_DURATION_END(rmaqueue_set);

	/* check if target is local and shared memory is allocated on window,
	  if so, we do not need to increment reference counts on datatype. This is
	  because this operation will be directly done on shared memory region, instead
	  of sending and receiving through the progress engine, therefore datatype
	  will not be referenced by the progress engine */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
           the same node. However, in ch3:sock, even if origin and target are on the same node, they do
           not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
           which is only set to TRUE when SHM region is allocated in nemesis.
           In future we need to figure out a way to check if origin and target are in the same "SHM comm".
        */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
	if (!(win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
	    /* if source or target datatypes are derived, increment their
	       reference counts */
	    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, predefined);
	    if (!predefined)
	    {
	        MPID_Datatype_get_ptr(origin_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
	    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, predefined);
	    if (!predefined)
	    {
	        MPID_Datatype_get_ptr(target_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_PUT);    
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank, predefined;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp, dt_true_lb); 

    if (data_sz == 0) {
	goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;
    
    /* If the get is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        mpi_errno = MPIDI_CH3I_Shm_get_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_VC_t *orig_vc, *target_vc;

	/* queue it up */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	MPIU_INSTR_DURATION_START(rmaqueue_set);
	/* FIXME: For contig and very short operations, use a streamlined op */
	new_ptr->type = MPIDI_RMA_GET;
	new_ptr->origin_addr = origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	MPIU_INSTR_DURATION_END(rmaqueue_set);
	
	/* check if target is local and shared memory is allocated on window,
	  if so, we do not need to increment reference counts on datatype. This is
	  because this operation will be directly done on shared memory region, instead
	  of sending and receiving through the progress engine, therefore datatype
	  will not be referenced by the progress engine */

        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
	if (!(win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
	    /* if source or target datatypes are derived, increment their
	       reference counts */
	    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, predefined);
	    if (!predefined)
	    {
	        MPID_Datatype_get_ptr(origin_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
	    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, predefined);
	    if (!predefined)
	    {
	        MPID_Datatype_get_ptr(target_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank, origin_predefined, target_predefined;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_ACCUMULATE);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp, dt_true_lb);  
    
    if (data_sz == 0) {
	goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;
    
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
	mpi_errno = MPIDI_CH3I_Shm_acc_op(origin_addr, origin_count, origin_datatype,
					  target_rank, target_disp, target_count, target_datatype,
					  op, win_ptr);
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_VC_t *orig_vc, *target_vc;

	/* queue it up */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	/* If predefined and contiguous, use a simplified element */
	if (origin_predefined && target_predefined && enableShortACC) {
	    MPIU_INSTR_DURATION_START(rmaqueue_set);
	    new_ptr->type = MPIDI_RMA_ACC_CONTIG;
	    /* Only the information needed for the contig/predefined acc */
            /* Cast away const'ness for origin_address as
             * MPIDI_RMA_Op_t contain both PUT and GET like ops */
	    new_ptr->origin_addr = (void *) origin_addr;
	    new_ptr->origin_count = origin_count;
	    new_ptr->origin_datatype = origin_datatype;
	    new_ptr->target_rank = target_rank;
	    new_ptr->target_disp = target_disp;
	    new_ptr->target_count = target_count;
	    new_ptr->target_datatype = target_datatype;
	    new_ptr->op = op;
	    MPIU_INSTR_DURATION_END(rmaqueue_set);
	    goto fn_exit;
	}

	MPIU_INSTR_DURATION_START(rmaqueue_set);
	new_ptr->type = MPIDI_RMA_ACCUMULATE;
        /* Cast away const'ness for origin_address as MPIDI_RMA_Op_t
         * contain both PUT and GET like ops */
	new_ptr->origin_addr = (void *) origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	new_ptr->op = op;
	MPIU_INSTR_DURATION_END(rmaqueue_set);
	
	/* check if target is local and shared memory is allocated on window,
	  if so, we do not need to increment reference counts on datatype. This is
	  because this operation will be directly done on shared memory region, instead
	  of sending and receiving through the progress engine, therefore datatype
	  will not be referenced by the progress engine */

        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
	if (!(win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
	    /* if source or target datatypes are derived, increment their
	       reference counts */
	    if (!origin_predefined)
	    {
	        MPID_Datatype_get_ptr(origin_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
	    if (!target_predefined)
	    {
	        MPID_Datatype_get_ptr(target_datatype, dtp);
	        MPID_Datatype_add_ref(dtp);
	    }
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_ACCUMULATE);
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
void *MPIDI_Alloc_mem( size_t size, MPID_Info *info_ptr )
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
int MPIDI_Free_mem( void *ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_FREE_MEM);

    MPIU_Free(ptr);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_FREE_MEM);
    return mpi_errno;
}

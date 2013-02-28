/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

static int enableShortACC=1;

#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_EXTERN_DECL(wincreate_allgather);
MPIU_INSTR_DURATION_EXTERN_DECL(winfree_rs);
MPIU_INSTR_DURATION_EXTERN_DECL(winfree_complete);
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

    comm_ptr = (*win_ptr)->comm_ptr;
    MPIU_INSTR_DURATION_START(winfree_rs);
    mpi_errno = MPIR_Reduce_scatter_block_impl((*win_ptr)->pt_rma_puts_accs, 
                                               &total_pt_rma_puts_accs, 1, 
                                               MPI_INT, MPI_SUM, comm_ptr, &errflag);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    MPIU_INSTR_DURATION_END(winfree_rs);

    if (total_pt_rma_puts_accs != (*win_ptr)->my_pt_rma_puts_accs)
    {
	MPID_Progress_state progress_state;
            
	/* poke the progress engine until the two are equal */
	MPIU_INSTR_DURATION_START(winfree_complete);
	MPID_Progress_start(&progress_state);
	while (total_pt_rma_puts_accs != (*win_ptr)->my_pt_rma_puts_accs)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
	MPIU_INSTR_DURATION_END(winfree_complete);
    }

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
#define FUNCNAME MPIDI_SHM_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_SHM_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_SHM_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_SHM_WIN_FREE);

    /* Free memory allocated by the default shared memory window
       implementation.  Note that this implementation works only for
       MPI_COMM_SELF and does not map a shared segment. */

    MPIU_Free((*win_ptr)->base);
    MPIU_Free((*win_ptr)->shm_base_addrs);

    mpi_errno = MPIDI_Win_free(win_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_SHM_WIN_FREE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
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

    rank = win_ptr->myrank;
    
    /* If the put is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        void *base;
        int disp_unit;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }

        mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                                   (char *) base + disp_unit * target_disp,
                                   target_count, target_datatype);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

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

    rank = win_ptr->myrank;
    
    /* If the get is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        void *base;
        int disp_unit;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }

        mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp,
                                   target_count, target_datatype, origin_addr,
                                   origin_count, origin_datatype);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

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

    rank = win_ptr->myrank;
    
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        MPI_User_function *uop;
        void *base;
        int disp_unit, shm_op = 0;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            shm_op = 1;
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }
	
	if (op == MPI_REPLACE)
	{
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                       origin_datatype,
                                       (char *) base + disp_unit * target_disp,
                                       target_count, target_datatype);
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    goto fn_exit;
	}
	
	MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN), 
			     mpi_errno, MPI_ERR_OP, "**opnotpredefined",
			     "**opnotpredefined %d", op );
	
	/* get the function by indexing into the op table */
	uop = MPIR_OP_HDL_TO_FN(op);
	
	if (origin_predefined && target_predefined)
	{    
            /* Cast away const'ness for origin_address in order to
             * avoid changing the prototype for MPI_User_function */
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            (*uop)((void *) origin_addr, (char *) base + disp_unit*target_disp,
                   &target_count, &target_datatype);
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
	}
	else
	{
	    /* derived datatype */
	    
	    MPID_Segment *segp;
	    DLOOP_VECTOR *dloop_vec;
	    MPI_Aint first, last;
	    int vec_len, i, type_size, count;
	    MPI_Datatype type;
	    MPI_Aint true_lb, true_extent, extent;
	    void *tmp_buf=NULL, *target_buf;
            const void *source_buf;
	    
	    if (origin_datatype != target_datatype)
	    {
		/* first copy the data into a temporary buffer with
		   the same datatype as the target. Then do the
		   accumulate operation. */
		
		MPIR_Type_get_true_extent_impl(target_datatype, &true_lb, &true_extent);
		MPID_Datatype_get_extent_macro(target_datatype, extent); 
		
		MPIU_CHKLMEM_MALLOC(tmp_buf, void *, 
			target_count * (MPIR_MAX(extent,true_extent)), 
			mpi_errno, "temporary buffer");
		/* adjust for potential negative lower bound in datatype */
		tmp_buf = (void *)((char*)tmp_buf - true_lb);
		
		mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
					   origin_datatype, tmp_buf,
					   target_count, target_datatype);  
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    }

	    if (target_predefined) { 
		/* target predefined type, origin derived datatype */

                if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
                (*uop)(tmp_buf, (char *) base + disp_unit * target_disp,
                       &target_count, &target_datatype);
                if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
	    }
	    else {
	    
		segp = MPID_Segment_alloc();
		MPIU_ERR_CHKANDJUMP1((!segp), mpi_errno, MPI_ERR_OTHER, 
				    "**nomem","**nomem %s","MPID_Segment_alloc"); 
		MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
		first = 0;
		last  = SEGMENT_IGNORE_LAST;
		
		MPID_Datatype_get_ptr(target_datatype, dtp);
		vec_len = dtp->max_contig_blocks * target_count + 1; 
		/* +1 needed because Rob says so */
		MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *, 
				    vec_len * sizeof(DLOOP_VECTOR), 
				    mpi_errno, "dloop vector");
		
		MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);
		
		source_buf = (tmp_buf != NULL) ? tmp_buf : origin_addr;
		target_buf = (char *) base + disp_unit * target_disp;
		type = dtp->eltype;
		type_size = MPID_Datatype_get_basic_size(type);
                if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
		for (i=0; i<vec_len; i++)
		{
		    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
		    (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
			   (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
			   &count, &type);
		}
                if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
		
		MPID_Segment_free(segp);
	    }
	}
    }
    else
    {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

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

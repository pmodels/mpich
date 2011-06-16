/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

static int enableShortACC=1;

MPIU_THREADSAFE_INIT_DECL(initRMAoptions);
#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_DECL(wincreate_allgather);
MPIU_INSTR_DURATION_DECL(winfree_rs);
MPIU_INSTR_DURATION_DECL(winfree_complete);
MPIU_INSTR_DURATION_DECL(rmaqueue_alloc);
extern void MPIDI_CH3_RMA_InitInstr(void);
#endif
extern void MPIDI_CH3_RMA_SetAccImmed( int );

#define MPIDI_PASSIVE_TARGET_DONE_TAG  348297
#define MPIDI_PASSIVE_TARGET_RMA_TAG 563924


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info,
		     MPID_Comm *comm_ptr, MPID_Win **win_ptr )
{
    int mpi_errno=MPI_SUCCESS, i, k, comm_size, rank;
    MPI_Aint *tmp_buf;
    MPID_Comm *win_comm_ptr;
    int errflag = FALSE;
    MPIU_CHKPMEM_DECL(4);
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_CREATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_CREATE);

    /* FIXME: There should be no unreferenced args */
    MPIU_UNREFERENCED_ARG(info);

    if(initRMAoptions) {
	int rc;
	MPIU_THREADSAFE_INIT_BLOCK_BEGIN(initRMAoptions);
	/* Default is to enable the use of the immediate accumulate feature */
	if (!MPL_env2bool( "MPICH_RMA_ACC_IMMED", &rc ))
	    rc = 1;
	MPIDI_CH3_RMA_SetAccImmed(rc);
#ifdef USE_MPIU_INSTR
    /* Define all instrumentation handle used in the CH3 RMA here*/
	MPIU_INSTR_DURATION_INIT(wincreate_allgather,0,"WIN_CREATE:Allgather");
	MPIU_INSTR_DURATION_INIT(winfree_rs,0,"WIN_FREE:ReduceScatterBlock");
	MPIU_INSTR_DURATION_INIT(winfree_complete,0,"WIN_FREE:Complete");
	MPIU_INSTR_DURATION_INIT(rmaqueue_alloc,0,"Allocate RMA Queue element");
	MPIDI_CH3_RMA_InitInstr();

#endif    
	MPIU_THREADSAFE_INIT_CLEAR(initRMAoptions);
	MPIU_THREADSAFE_INIT_BLOCK_END(initRMAoptions);
    }

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    *win_ptr = (MPID_Win *)MPIU_Handle_obj_alloc( &MPID_Win_mem );
    MPIU_ERR_CHKANDJUMP1(!(*win_ptr),mpi_errno,MPI_ERR_OTHER,"**nomem",
			 "**nomem %s","MPID_Win_mem");

    MPIU_Object_set_ref(*win_ptr, 1);

    (*win_ptr)->fence_cnt = 0;
    (*win_ptr)->base = base;
    (*win_ptr)->size = size;
    (*win_ptr)->disp_unit = disp_unit;
    (*win_ptr)->start_group_ptr = NULL; 
    (*win_ptr)->start_assert = 0; 
    (*win_ptr)->attributes = NULL;
    (*win_ptr)->rma_ops_list_head = NULL;
    (*win_ptr)->rma_ops_list_tail = NULL;
    (*win_ptr)->lock_granted = 0;
    (*win_ptr)->current_lock_type = MPID_LOCK_NONE;
    (*win_ptr)->shared_lock_ref_cnt = 0;
    (*win_ptr)->lock_queue = NULL;
    (*win_ptr)->my_counter = 0;
    (*win_ptr)->my_pt_rma_puts_accs = 0;
    
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, &win_comm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    (*win_ptr)->comm_ptr   = win_comm_ptr;
    (*win_ptr)->myrank = rank;

    MPIU_INSTR_DURATION_START(wincreate_allgather);
    /* allocate memory for the base addresses, disp_units, and
       completion counters of all processes */ 
    MPIU_CHKPMEM_MALLOC((*win_ptr)->base_addrs, void **,
			comm_size*sizeof(void *), 
			mpi_errno, "(*win_ptr)->base_addrs");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->disp_units, int *, comm_size*sizeof(int), 
			mpi_errno, "(*win_ptr)->disp_units");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->all_win_handles, MPI_Win *, 
			comm_size*sizeof(MPI_Win), 
			mpi_errno, "(*win_ptr)->all_win_handles");
    
    MPIU_CHKPMEM_MALLOC((*win_ptr)->pt_rma_puts_accs, int *, 
			comm_size*sizeof(int), 
			mpi_errno, "(*win_ptr)->pt_rma_puts_accs");
    for (i=0; i<comm_size; i++)	(*win_ptr)->pt_rma_puts_accs[i] = 0;
    
    /* get the addresses of the windows, window objects, and completion
       counters of all processes.  allocate temp. buffer for communication */
    MPIU_CHKLMEM_MALLOC(tmp_buf, MPI_Aint *, 3*comm_size*sizeof(MPI_Aint),
			mpi_errno, "tmp_buf");
    
    /* FIXME: This needs to be fixed for heterogeneous systems */
    tmp_buf[3*rank]   = MPIU_PtrToAint(base);
    tmp_buf[3*rank+1] = (MPI_Aint) disp_unit;
    tmp_buf[3*rank+2] = (MPI_Aint) (*win_ptr)->handle;
    
    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    tmp_buf, 3 * sizeof(MPI_Aint), MPI_BYTE,
                                    comm_ptr, &errflag);
    MPIU_INSTR_DURATION_END(wincreate_allgather);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");


    k = 0;
    for (i=0; i<comm_size; i++)
    {
	(*win_ptr)->base_addrs[i] = MPIU_AintToPtr(tmp_buf[k++]);
	(*win_ptr)->disp_units[i] = (int) tmp_buf[k++];
	(*win_ptr)->all_win_handles[i] = (MPI_Win) tmp_buf[k++];
    }
        
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_CREATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}




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

    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->pt_rma_puts_accs);

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
#define FUNCNAME MPIDI_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig, rank, predefined;
    MPIDI_RMA_ops *new_ptr;
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_PUT);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_PUT);

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp,dt_true_lb); 
    
    if ((data_sz == 0) || (target_rank == MPI_PROC_NULL))
    {
	goto fn_exit;
    }

    rank = win_ptr->myrank;
    
    /* If the put is a local operation, do it here */
    if (target_rank == rank)
    {
	mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
				   (char *) win_ptr->base + win_ptr->disp_unit *
				   target_disp, target_count, target_datatype); 
    }
    else
    {
	/* queue it up */
	/* FIXME: For short operations, should we use a (per-thread) pool? */
	MPIU_INSTR_DURATION_START(rmaqueue_alloc);
	MPIU_CHKPMEM_MALLOC(new_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops), 
			    mpi_errno, "RMA operation entry");
	MPIU_INSTR_DURATION_END(rmaqueue_alloc);
	if (win_ptr->rma_ops_list_tail) 
	    win_ptr->rma_ops_list_tail->next = new_ptr;
	else
	    win_ptr->rma_ops_list_head = new_ptr;
	win_ptr->rma_ops_list_tail = new_ptr;

	/* FIXME: For contig and very short operations, use a streamlined op */
	new_ptr->next = NULL;  
	new_ptr->type = MPIDI_RMA_PUT;
	new_ptr->origin_addr = origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	
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
    MPIU_CHKPMEM_REAP();
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
    int dt_contig, rank, predefined;
    MPI_Aint dt_true_lb;
    MPIDI_RMA_ops *new_ptr;
    MPID_Datatype *dtp;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET);

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp, dt_true_lb); 

    if ((data_sz == 0) || (target_rank == MPI_PROC_NULL))
    {
	goto fn_exit;
    }

    rank = win_ptr->myrank;
    
    /* If the get is a local operation, do it here */
    if (target_rank == rank)
    {
	mpi_errno = MPIR_Localcopy((char *) win_ptr->base +
				   win_ptr->disp_unit * target_disp,
				   target_count, target_datatype,
				   origin_addr, origin_count,
				   origin_datatype);  
    }
    else
    {
	/* queue it up */
	MPIU_INSTR_DURATION_START(rmaqueue_alloc);
	MPIU_CHKPMEM_MALLOC(new_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops), 
			    mpi_errno, "RMA operation entry");
	MPIU_INSTR_DURATION_END(rmaqueue_alloc);
	if (win_ptr->rma_ops_list_tail) 
	    win_ptr->rma_ops_list_tail->next = new_ptr;
	else
	    win_ptr->rma_ops_list_head = new_ptr;
	win_ptr->rma_ops_list_tail = new_ptr;
            
	/* FIXME: For contig and very short operations, use a streamlined op */
	new_ptr->next = NULL;  
	new_ptr->type = MPIDI_RMA_GET;
	new_ptr->origin_addr = origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	
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
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig, rank, origin_predefined, target_predefined;
    MPI_Aint dt_true_lb;
    MPIDI_RMA_ops *new_ptr;
    MPID_Datatype *dtp;
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_ACCUMULATE);

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
			    dt_contig, data_sz, dtp, dt_true_lb);  
    
    if ((data_sz == 0) || (target_rank == MPI_PROC_NULL))
    {
	goto fn_exit;
    }

    rank = win_ptr->myrank;
    
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank)
    {
	MPI_User_function *uop;
	
	if (op == MPI_REPLACE)
	{
	    mpi_errno = MPIR_Localcopy(origin_addr, origin_count, 
				origin_datatype,
				(char *) win_ptr->base + win_ptr->disp_unit *
				target_disp, target_count, target_datatype); 
	    goto fn_exit;
	}
	
	MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN), 
			     mpi_errno, MPI_ERR_OP, "**opnotpredefined",
			     "**opnotpredefined %d", op );
	
	/* get the function by indexing into the op table */
	uop = MPIR_Op_table[((op)&0xf) - 1];
	
	if (origin_predefined && target_predefined)
	{    
	    (*uop)(origin_addr, (char *) win_ptr->base + win_ptr->disp_unit *
		   target_disp, &target_count, &target_datatype);
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
	    void *tmp_buf=NULL, *source_buf, *target_buf;
	    
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

		(*uop)(tmp_buf, (char *) win_ptr->base + win_ptr->disp_unit *
		   target_disp, &target_count, &target_datatype);
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
		target_buf = (char *) win_ptr->base + 
		    win_ptr->disp_unit * target_disp;
		type = dtp->eltype;
		type_size = MPID_Datatype_get_basic_size(type);
		for (i=0; i<vec_len; i++)
		{
		    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
		    (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
			   (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
			   &count, &type);
		}
		
		MPID_Segment_free(segp);
	    }
	}
    }
    else
    {
	/* queue it up */
	MPIU_INSTR_DURATION_START(rmaqueue_alloc);
	MPIU_CHKPMEM_MALLOC(new_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops), 
			    mpi_errno, "RMA operation entry");
	MPIU_INSTR_DURATION_END(rmaqueue_alloc);
	if (win_ptr->rma_ops_list_tail) 
	    win_ptr->rma_ops_list_tail->next = new_ptr;
	else
	    win_ptr->rma_ops_list_head = new_ptr;
	win_ptr->rma_ops_list_tail = new_ptr;

	/* If predefined and contiguous, use a simplified element */
	if (origin_predefined && target_predefined && enableShortACC) {
	    new_ptr->next = NULL;
	    new_ptr->type = MPIDI_RMA_ACC_CONTIG;
	    /* Only the information needed for the contig/predefined acc */
	    new_ptr->origin_addr = origin_addr;
	    new_ptr->origin_count = origin_count;
	    new_ptr->origin_datatype = origin_datatype;
	    new_ptr->target_rank = target_rank;
	    new_ptr->target_disp = target_disp;
	    new_ptr->target_count = target_count;
	    new_ptr->target_datatype = target_datatype;
	    new_ptr->op = op;
	    goto fn_exit;
	}

	new_ptr->next = NULL;  
	new_ptr->type = MPIDI_RMA_ACCUMULATE;
	new_ptr->origin_addr = origin_addr;
	new_ptr->origin_count = origin_count;
	new_ptr->origin_datatype = origin_datatype;
	new_ptr->target_rank = target_rank;
	new_ptr->target_disp = target_disp;
	new_ptr->target_count = target_count;
	new_ptr->target_datatype = target_datatype;
	new_ptr->op = op;
	
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
    MPIU_CHKPMEM_REAP();
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

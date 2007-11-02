/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

static int MPIDI_CH3I_Send_rma_msg(MPIDI_RMA_ops * rma_op, MPID_Win * win_ptr, 
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info * dtype_info, 
				   void ** dataloop, MPID_Request ** request);
static int MPIDI_CH3I_Recv_rma_msg(MPIDI_RMA_ops * rma_op, MPID_Win * win_ptr, 
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info * dtype_info, 
				   void ** dataloop, MPID_Request ** request); 
static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr, 
					    int *wait_for_rma_done_pkt);
static int MPIDI_CH3I_Send_lock_put_or_acc(MPID_Win *win_ptr);
static int MPIDI_CH3I_Send_lock_get(MPID_Win *win_ptr);


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, done, *recvcnts;
    int *rma_target_proc, *nops_to_proc, i, total_op_count, *curr_ops_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests=NULL; /* array of requests */
    MPI_Win source_win_handle, target_win_handle;
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */
    MPID_Progress_state progress_state;
    MPIU_CHKLMEM_DECL(7);
    MPIU_THREADPRIV_DECL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FENCE);

    MPIU_THREADPRIV_GET;
    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operartions
     * have completed and the lock is released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    /* poke the progress engine */
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
						 "**fail %s", "making progress on the rma messages failed");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }
    
    if (assert & MPI_MODE_NOPRECEDE)
    {
	win_ptr->fence_cnt = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;
	goto fn_exit;
    }
    
    if ((win_ptr->fence_cnt == 0) && ((assert & MPI_MODE_NOSUCCEED) != 1))
    {
	/* win_ptr->fence_cnt == 0 means either this is the very first
	   call to fence or the preceding fence had the
	   MPI_MODE_NOSUCCEED assert. 
	   Do nothing except increment the count. */
	win_ptr->fence_cnt = 1;
    }
    else
    {
	/* This is the second or later fence. Do all the preceding RMA ops. */
	
	MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
	
	/* First inform every process whether it is a target of RMA
	   ops from this process */
	comm_size = comm_ptr->local_size;

	MPIU_CHKLMEM_MALLOC(rma_target_proc, int *, comm_size*sizeof(int),
			    mpi_errno, "rma_target_proc");
	for (i=0; i<comm_size; i++) rma_target_proc[i] = 0;
	
	/* keep track of no. of ops to each proc. Needed for knowing
	   whether or not to decrement the completion counter. The
	   completion counter is decremented only on the last
	   operation. */
	MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size*sizeof(int),
			    mpi_errno, "nops_to_proc");
	for (i=0; i<comm_size; i++) nops_to_proc[i] = 0;

	/* set rma_target_proc[i] to 1 if rank i is a target of RMA
	   ops from this process */
	total_op_count = 0;
	curr_ptr = win_ptr->rma_ops_list;
	while (curr_ptr != NULL)
	{
	    total_op_count++;
	    rma_target_proc[curr_ptr->target_rank] = 1;
	    nops_to_proc[curr_ptr->target_rank]++;
	    curr_ptr = curr_ptr->next;
	}
	
	MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size*sizeof(int),
			    mpi_errno, "curr_ops_cnt");
	for (i=0; i<comm_size; i++) curr_ops_cnt[i] = 0;
	
	if (total_op_count != 0)
	{
	    MPIU_CHKLMEM_MALLOC(requests, MPID_Request **, 
				total_op_count*sizeof(MPID_Request*),
				mpi_errno, "requests");
	    MPIU_CHKLMEM_MALLOC(dtype_infos, MPIDI_RMA_dtype_info *, 
				total_op_count*sizeof(MPIDI_RMA_dtype_info),
				mpi_errno, "dtype_infos");
	    MPIU_CHKLMEM_MALLOC(dataloops, void **, 
				total_op_count*sizeof(void*),
				mpi_errno, "dataloops");
	    for (i=0; i<total_op_count; i++) dataloops[i] = NULL;
	}
	
	/* do a reduce_scatter (with MPI_SUM) on rma_target_proc. As a result,
	   each process knows how many other processes will be doing
	   RMA ops on its window */  
            
	/* first initialize the completion counter. */
	win_ptr->my_counter = comm_size;
            
	/* set up the recvcnts array for reduce scatter */
	MPIU_CHKLMEM_MALLOC(recvcnts, int *, comm_size*sizeof(int),
			    mpi_errno, "recvcnts");
	for (i=0; i<comm_size; i++) recvcnts[i] = 1;
            
	MPIR_Nest_incr();
	mpi_errno = NMPI_Reduce_scatter(MPI_IN_PLACE, rma_target_proc, 
					recvcnts,
					MPI_INT, MPI_SUM, win_ptr->comm);
	/* result is stored in rma_target_proc[0] */
	MPIR_Nest_decr();
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	/* Set the completion counter */
	/* FIXME: MT: this needs to be done atomically because other
	   procs have the address and could decrement it. */
	win_ptr->my_counter = win_ptr->my_counter - comm_size + 
	    rma_target_proc[0];  
            
	i = 0;
	curr_ptr = win_ptr->rma_ops_list;
	while (curr_ptr != NULL)
	{
	    /* The completion counter at the target is decremented only on 
	       the last RMA operation. We indicate the last operation by 
	       passing the source_win_handle only on the last operation. 
	       Otherwise, we pass NULL */
	    if (curr_ops_cnt[curr_ptr->target_rank] ==
		nops_to_proc[curr_ptr->target_rank] - 1) 
		source_win_handle = win_ptr->handle;
	    else 
		source_win_handle = MPI_WIN_NULL;
	    
	    target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
	    
	    switch (curr_ptr->type)
	    {
	    case (MPIDI_RMA_PUT):
	    case (MPIDI_RMA_ACCUMULATE):
		mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
					source_win_handle, target_win_handle, 
					&dtype_infos[i],
					&dataloops[i], &requests[i]);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		break;
	    case (MPIDI_RMA_GET):
		mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
					source_win_handle, target_win_handle, 
					&dtype_infos[i], 
					&dataloops[i], &requests[i]);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		break;
	    default:
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "invalid RMA operation");
		goto fn_exit;
		/* --END ERROR HANDLING-- */
	    }
	    i++;
	    curr_ops_cnt[curr_ptr->target_rank]++;
	    curr_ptr = curr_ptr->next;
	}
	
            
	if (total_op_count)
	{ 
	    done = 1;
	    MPID_Progress_start(&progress_state);
	    while (total_op_count)
	    {
		for (i=0; i<total_op_count; i++)
		{
		    if (requests[i] != NULL)
		    {
			if (*(requests[i]->cc_ptr) != 0)
			{
			    done = 0;
			    break;
			}
			else
			{
			    mpi_errno = requests[i]->status.MPI_ERROR;
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				MPID_Progress_end(&progress_state);
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**fail", "**fail %s", "rma message operation failed");
				goto fn_exit;
			    }
			    /* --END ERROR HANDLING-- */
			    /* if origin datatype was a derived
			       datatype, it will get freed when the
			       request gets freed. */ 
			    MPID_Request_release(requests[i]);
			    requests[i] = NULL;
			}
		    }
		}
                    
		if (done)
		{
		    break;
		}
                    
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Progress_end(&progress_state);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
						     "**fail %s", "making progress on the rma messages failed");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		
		done = 1;
	    } 
	    MPID_Progress_end(&progress_state);
	}
            
	if (total_op_count != 0)
	{
	    for (i=0; i<total_op_count; i++)
	    {
		if (dataloops[i] != NULL)
		{
		    MPIU_Free(dataloops[i]); /* allocated in send_rma_msg or 
						recv_rma_msg */
		}
	    }
	}
	
	/* free MPIDI_RMA_ops_list */
	curr_ptr = win_ptr->rma_ops_list;
	while (curr_ptr != NULL)
	{
	    next_ptr = curr_ptr->next;
	    MPIU_Free(curr_ptr);
	    curr_ptr = next_ptr;
	}
	win_ptr->rma_ops_list = NULL;
	
	/* wait for all operations from other processes to finish */
	if (win_ptr->my_counter)
	{
	    MPID_Progress_start(&progress_state);
	    while (win_ptr->my_counter)
	    {
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Progress_end(&progress_state);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
						     "**fail %s", "making progress on the rma messages failed");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    MPID_Progress_end(&progress_state);
	} 
	
	if (assert & MPI_MODE_NOSUCCEED)
	{
	    win_ptr->fence_cnt = 0;
	}
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FENCE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Send_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info *dtype_info, 
				   void **dataloop, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno=MPI_SUCCESS, predefined;
    int origin_dt_derived, target_dt_derived, origin_type_size, iovcnt, iov_n; 
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *target_dtp=NULL, *origin_dtp=NULL;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);

    if (rma_op->type == MPIDI_RMA_PUT)
    {
        MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        put_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

        put_pkt->count = rma_op->target_count;
        put_pkt->datatype = rma_op->target_datatype;
        put_pkt->dataloop_size = 0;
        put_pkt->target_win_handle = target_win_handle;
        put_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else
    {
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/

    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype, predefined);
    if (!predefined)
    {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else
    {
        origin_dt_derived = 0;
    }

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(rma_op->target_datatype, predefined);
    if (!predefined)
    {
        target_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->target_datatype, target_dtp);
    }
    else
    {
        target_dt_derived = 0;
    }

    if (target_dt_derived)
    {
        /* derived datatype on target. fill derived datatype info */
        dtype_info->is_contig = target_dtp->is_contig;
        dtype_info->n_contig_blocks = target_dtp->n_contig_blocks;
        dtype_info->size = target_dtp->size;
        dtype_info->extent = target_dtp->extent;
        dtype_info->dataloop_size = target_dtp->dataloop_size;
        dtype_info->dataloop_depth = target_dtp->dataloop_depth;
        dtype_info->eltype = target_dtp->eltype;
        dtype_info->dataloop = target_dtp->dataloop;
        dtype_info->ub = target_dtp->ub;
        dtype_info->lb = target_dtp->lb;
        dtype_info->true_ub = target_dtp->true_ub;
        dtype_info->true_lb = target_dtp->true_lb;
        dtype_info->has_sticky_ub = target_dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = target_dtp->has_sticky_lb;

	MPIU_CHKPMEM_MALLOC(*dataloop, void *, target_dtp->dataloop_size, 
			    mpi_errno, "dataloop");

	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        memcpy(*dataloop, target_dtp->dataloop, target_dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        if (rma_op->type == MPIDI_RMA_PUT)
	{
            put_pkt->dataloop_size = target_dtp->dataloop_size;
	}
        else
	{
            accum_pkt->dataloop_size = target_dtp->dataloop_size;
	}
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived)
    {
	/* basic datatype on origin */
        if (!target_dt_derived)
	{
	    /* basic datatype on target */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 2;
        }
        else
	{
	    /* derived datatype on target */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
            iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
            iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)*dataloop;
            iov[2].MPID_IOV_LEN = target_dtp->dataloop_size;

            iov[3].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
            iov[3].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 4;
        }

        mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsgv(vc, iov, iovcnt, request));
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
    }
    else
    {
	/* derived datatype on origin */

        if (!target_dt_derived)
	{
	    /* basic datatype on target */
            iovcnt = 1;
	}
        else
	{
	    /* derived datatype on target */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
            iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
            iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)*dataloop;
            iov[2].MPID_IOV_LEN = target_dtp->dataloop_size;
            iovcnt = 3;
        }

        *request = MPID_Request_create();
        if (*request == NULL) {
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
	    /* --END ERROR HANDLING-- */
        }

        MPIU_Object_set_ref(*request, 2);
        (*request)->kind = MPID_REQUEST_SEND;
	    
        (*request)->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
           is freed. */ 

	(*request)->dev.segment_ptr = MPID_Segment_alloc( );
	/* if (!*request)->dev.segment_ptr) { MPIU_ERR_POP(); } */
        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype,
                          (*request)->dev.segment_ptr, 0);
        (*request)->dev.segment_first = 0;
        (*request)->dev.segment_size = rma_op->origin_count * origin_type_size;
	    
        iov_n = MPID_IOV_LIMIT - iovcnt;
	/* On the initial load of a send iov req, set the OnFinal action (null
	   for point-to-point) */
	(*request)->dev.OnFinal = 0;
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(*request,
                                                     &iov[iovcnt],
                                                     &iov_n); 
        if (mpi_errno == MPI_SUCCESS)
        {
            iov_n += iovcnt;
            
            mpi_errno = MPIU_CALL(MPIDI_CH3,iSendv(vc, *request, iov, iov_n));
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                MPID_Datatype_release((*request)->dev.datatype_ptr);
                MPIU_Object_set_ref(*request, 0);
                MPIDI_CH3_Request_destroy(*request);
                *request = NULL;
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
		MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
        }
        else
        {
	    /* --BEGIN ERROR HANDLING-- */
            MPID_Datatype_release((*request)->dev.datatype_ptr);
            MPIU_Object_set_ref(*request, 0);
            MPIDI_CH3_Request_destroy(*request);
            *request = NULL;
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
	    /* --END ERROR HANDLING-- */
        }
    }

    if (target_dt_derived)
    {
        MPID_Datatype_release(target_dtp);
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Recv_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Recv_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info *dtype_info, 
				   void **dataloop, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_t *get_pkt = &upkt.get;
    int mpi_errno, predefined;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Request *req = NULL;
    MPID_Datatype *dtp;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    req = MPID_Request_create();
    if (req == NULL) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }

    *request = req;

    MPIU_Object_set_ref(req, 2);

    req->dev.user_buf = rma_op->origin_addr;
    req->dev.user_count = rma_op->origin_count;
    req->dev.datatype = rma_op->origin_datatype;
    req->dev.target_win_handle = MPI_WIN_NULL;
    req->dev.source_win_handle = source_win_handle;
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(req->dev.datatype, predefined);
    if (!predefined)
    {
        MPID_Datatype_get_ptr(req->dev.datatype, dtp);
        req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
    get_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    get_pkt->count = rma_op->target_count;
    get_pkt->datatype = rma_op->target_datatype;
    get_pkt->request_handle = req->handle;
    get_pkt->target_win_handle = target_win_handle;
    get_pkt->source_win_handle = source_win_handle;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/
	    
    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(rma_op->target_datatype, predefined);
    if (predefined)
    {
        /* basic datatype on target. simply send the get_pkt. */
        mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req));
    }
    else
    {
        /* derived datatype on target. fill derived datatype info and
           send it along with get_pkt. */

        MPID_Datatype_get_ptr(rma_op->target_datatype, dtp);
        dtype_info->is_contig = dtp->is_contig;
        dtype_info->n_contig_blocks = dtp->n_contig_blocks;
        dtype_info->size = dtp->size;
        dtype_info->extent = dtp->extent;
        dtype_info->dataloop_size = dtp->dataloop_size;
        dtype_info->dataloop_depth = dtp->dataloop_depth;
        dtype_info->eltype = dtp->eltype;
        dtype_info->dataloop = dtp->dataloop;
        dtype_info->ub = dtp->ub;
        dtype_info->lb = dtp->lb;
        dtype_info->true_ub = dtp->true_ub;
        dtype_info->true_lb = dtp->true_lb;
        dtype_info->has_sticky_ub = dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = dtp->has_sticky_lb;

	MPIU_CHKPMEM_MALLOC(*dataloop, void *, dtp->dataloop_size, 
			    mpi_errno, "dataloop");

	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        memcpy(*dataloop, dtp->dataloop, dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        get_pkt->dataloop_size = dtp->dataloop_size;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)get_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
        iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
        iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)*dataloop;
        iov[2].MPID_IOV_LEN = dtp->dataloop_size;
        
        mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsgv(vc, iov, 3, &req));

        /* release the target datatype */
        MPID_Datatype_release(dtp);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
	MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL)
    {
        MPID_Request_release(req);
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}




#undef FUNCNAME
#define FUNCNAME MPIDI_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int nest_level_inc = FALSE;
    int mpi_errno=MPI_SUCCESS;
    MPI_Group win_grp, post_grp;
    int i, post_grp_size, *ranks_in_post_grp, *ranks_in_win_grp, dst, rank;
    MPIU_CHKLMEM_DECL(2);
    MPIU_THREADPRIV_DECL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_POST);

    MPIU_THREADPRIV_GET;
    /* Reset the fence counter so that in case the user has switched from 
       fence to 
       post-wait synchronization, he cannot use the previous fence to mark 
       the beginning of a fence epoch.  */
    win_ptr->fence_cnt = 0;

    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operations
     * have completed and the lock is therefore released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPID_Progress_state progress_state;
	
	/* poke the progress engine */
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "making progress on the rma messages failed");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }
        
    post_grp_size = group_ptr->size;
        
    /* initialize the completion counter */
    win_ptr->my_counter = post_grp_size;
        
    if ((assert & MPI_MODE_NOCHECK) == 0)
    {
	/* NOCHECK not specified. We need to notify the source
	   processes that Post has been called. */  
	
	/* We need to translate the ranks of the processes in
	   post_group to ranks in win_ptr->comm, so that we
	   can do communication */
            
	MPIU_CHKLMEM_MALLOC(ranks_in_post_grp, int *, 
			    post_grp_size * sizeof(int),
			    mpi_errno, "ranks_in_post_grp");
	MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, 
			    post_grp_size * sizeof(int),
			    mpi_errno, "ranks_in_win_grp");
        
	for (i=0; i<post_grp_size; i++)
	{
	    ranks_in_post_grp[i] = i;
	}
        
	nest_level_inc = TRUE;
	MPIR_Nest_incr();
	
	mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	
	post_grp = group_ptr->handle;
	
	mpi_errno = NMPI_Group_translate_ranks(post_grp, post_grp_size,
					       ranks_in_post_grp, win_grp, 
					       ranks_in_win_grp);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	
	NMPI_Comm_rank(win_ptr->comm, &rank);
	
	/* Send a 0-byte message to the source processes */
	for (i=0; i<post_grp_size; i++)
	{
	    dst = ranks_in_win_grp[i];
	    
	    if (dst != rank) {
		mpi_errno = NMPI_Send(&i, 0, MPI_INT, dst, 100, win_ptr->comm);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    }
	}
	
	mpi_errno = NMPI_Group_free(&win_grp);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }    

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (nest_level_inc)
    { 
	MPIR_Nest_decr();
    }
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_POST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_START);

    /* Reset the fence counter so that in case the user has switched from 
       fence to start-complete synchronization, he cannot use the previous 
       fence to mark the beginning of a fence epoch.  */
    win_ptr->fence_cnt = 0;

    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operations
     * have completed and the lock is therefore released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPID_Progress_state progress_state;
	
	/* poke the progress engine */
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "making progress on the rma messages failed");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }
    
    win_ptr->start_group_ptr = group_ptr;
    MPIR_Group_add_ref( group_ptr );
    win_ptr->start_assert = assert;
    
 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_START);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_complete(MPID_Win *win_ptr)
{
    int nest_level_inc = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int comm_size, *nops_to_proc, src, new_total_op_count;
    int i, j, dst, done, total_op_count, *curr_ops_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests; /* array of requests */
    MPI_Win source_win_handle, target_win_handle;
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */
    MPI_Group win_grp, start_grp;
    int start_grp_size, *ranks_in_start_grp, *ranks_in_win_grp, rank;
    MPIU_CHKLMEM_DECL(7);
    MPIU_THREADPRIV_DECL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_COMPLETE);

    MPIU_THREADPRIV_GET;
    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;
        
    /* Translate the ranks of the processes in
       start_group to ranks in win_ptr->comm */
    
    start_grp_size = win_ptr->start_group_ptr->size;
        
    MPIU_CHKLMEM_MALLOC(ranks_in_start_grp, int *, start_grp_size*sizeof(int), 
			mpi_errno, "ranks_in_start_grp");
        
    MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, start_grp_size*sizeof(int), 
			mpi_errno, "ranks_in_win_grp");
        
    for (i=0; i<start_grp_size; i++)
    {
	ranks_in_start_grp[i] = i;
    }
        
    nest_level_inc = TRUE;
    MPIR_Nest_incr();
    
    mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    start_grp = win_ptr->start_group_ptr->handle;

    mpi_errno = NMPI_Group_translate_ranks(start_grp, start_grp_size,
					   ranks_in_start_grp, win_grp, 
					   ranks_in_win_grp);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        
        
    NMPI_Comm_rank(win_ptr->comm, &rank);
    /* If MPI_MODE_NOCHECK was not specified, we need to check if
       Win_post was called on the target processes. Wait for a 0-byte sync
       message from each target process */
    if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0)
    {
	for (i=0; i<start_grp_size; i++)
	{
	    src = ranks_in_win_grp[i];
	    if (src != rank) {
		mpi_errno = NMPI_Recv(NULL, 0, MPI_INT, src, 100,
				      win_ptr->comm, MPI_STATUS_IGNORE);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    }
	}
    }
        
    /* keep track of no. of ops to each proc. Needed for knowing
       whether or not to decrement the completion counter. The
       completion counter is decremented only on the last
       operation. */
        
    MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size*sizeof(int), 
			mpi_errno, "nops_to_proc");
    for (i=0; i<comm_size; i++) nops_to_proc[i] = 0;

    total_op_count = 0;
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
	nops_to_proc[curr_ptr->target_rank]++;
	total_op_count++;
	curr_ptr = curr_ptr->next;
    }
    
    MPIU_CHKLMEM_MALLOC(requests, MPID_Request **, 
			(total_op_count+start_grp_size) * sizeof(MPID_Request*),
			mpi_errno, "requests");
    /* We allocate a few extra requests because if there are no RMA
       ops to a target process, we need to send a 0-byte message just
       to decrement the completion counter. */
        
    MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size*sizeof(int),
			mpi_errno, "curr_ops_cnt");
    for (i=0; i<comm_size; i++) curr_ops_cnt[i] = 0;
    
    if (total_op_count != 0)
    {
	MPIU_CHKLMEM_MALLOC(dtype_infos, MPIDI_RMA_dtype_info *, 
			    total_op_count*sizeof(MPIDI_RMA_dtype_info),
			    mpi_errno, "dtype_infos");
	MPIU_CHKLMEM_MALLOC(dataloops, void **, total_op_count*sizeof(void*),
			    mpi_errno, "dataloops");
	for (i=0; i<total_op_count; i++) dataloops[i] = NULL;
    }
        
    i = 0;
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
	/* The completion counter at the target is decremented only on 
	   the last RMA operation. We indicate the last operation by 
	   passing the source_win_handle only on the last operation. 
	   Otherwise, we pass NULL */
	if (curr_ops_cnt[curr_ptr->target_rank] ==
	    nops_to_proc[curr_ptr->target_rank] - 1) 
	    source_win_handle = win_ptr->handle;
	else 
	    source_win_handle = MPI_WIN_NULL;
	
	target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
	
	switch (curr_ptr->type)
	{
	case (MPIDI_RMA_PUT):
	case (MPIDI_RMA_ACCUMULATE):
	    mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
				source_win_handle, target_win_handle, 
				&dtype_infos[i],
				&dataloops[i], &requests[i]); 
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	case (MPIDI_RMA_GET):
	    mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
				source_win_handle, target_win_handle, 
				&dtype_infos[i], 
				&dataloops[i], &requests[i]);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	default:
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "invalid RMA operation");
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}
	i++;
	curr_ops_cnt[curr_ptr->target_rank]++;
	curr_ptr = curr_ptr->next;
    }
        
    /* If the start_group included some processes that did not end up
       becoming targets of  RMA operations from this process, we need
       to send a dummy message to those processes just to decrement
       the completion counter */
        
    j = i;
    new_total_op_count = total_op_count;
    for (i=0; i<start_grp_size; i++)
    {
	dst = ranks_in_win_grp[i];
	if (dst == rank) {
	    /* FIXME: MT: this has to be done atomically */
	    win_ptr->my_counter -= 1;
	}
	else if (nops_to_proc[dst] == 0)
	{
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
	    MPIDI_VC_t * vc;
	    
	    MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
	    put_pkt->addr = NULL;
	    put_pkt->count = 0;
	    put_pkt->datatype = MPI_INT;
	    put_pkt->target_win_handle = win_ptr->all_win_handles[dst];
	    put_pkt->source_win_handle = win_ptr->handle;
	    
	    MPIDI_Comm_get_vc(comm_ptr, dst, &vc);
	    
	    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, put_pkt,
						      sizeof(*put_pkt),
						      &requests[j]));
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    j++;
	    new_total_op_count++;
	}
    }
        
    if (new_total_op_count)
    {
	MPID_Progress_state progress_state;
	
	done = 1;
	MPID_Progress_start(&progress_state);
	while (new_total_op_count)
	{
	    for (i=0; i<new_total_op_count; i++)
	    {
		if (requests[i] != NULL)
		{
		    if (*(requests[i]->cc_ptr) != 0)
		    {
			done = 0;
			break;
		    }
		    else
		    {
			mpi_errno = requests[i]->status.MPI_ERROR;
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    MPID_Progress_end(&progress_state);
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**fail", 0);
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
			MPID_Request_release(requests[i]);
			requests[i] = NULL;
		    }
		}
	    }
                
	    if (done)
	    {
		break;
	    }
	    
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    done = 1;
	} 
	MPID_Progress_end(&progress_state);
    }
        
    if (total_op_count != 0)
    {
	for (i=0; i<total_op_count; i++)
	{
	    if (dataloops[i] != NULL)
	    {
		MPIU_Free(dataloops[i]);
	    }
	}
    }
        
    /* free MPIDI_RMA_ops_list */
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
	next_ptr = curr_ptr->next;
	MPIU_Free(curr_ptr);
	curr_ptr = next_ptr;
    }
    win_ptr->rma_ops_list = NULL;
    
    mpi_errno = NMPI_Group_free(&win_grp);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* free the group stored in window */
    MPIR_Group_release(win_ptr->start_group_ptr);
    win_ptr->start_group_ptr = NULL; 
    
 fn_exit:
    if (nest_level_inc)
    { 
	MPIR_Nest_decr();
    }
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_COMPLETE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_WAIT);

    /* wait for all operations from other processes to finish */
    if (win_ptr->my_counter)
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while (win_ptr->my_counter)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_WAIT);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    } 

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_WAIT);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_ops *new_ptr;
    MPID_Comm *comm_ptr;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_LOCK);

    MPIU_UNREFERENCED_ARG(assert);

    /* Reset the fence counter so that in case the user has switched from 
       fence to lock-unlock synchronization, he cannot use the previous fence 
       to mark the beginning of a fence epoch.  */
    win_ptr->fence_cnt = 0;

    if (dest == MPI_PROC_NULL) goto fn_exit;
        
    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    
    if (dest == comm_ptr->rank) {
	/* The target is this process itself. We must block until the lock
	 * is acquired. */
            
	/* poke the progress engine until lock is granted */
	if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0)
	{
	    MPID_Progress_state progress_state;
	    
	    MPID_Progress_start(&progress_state);
	    while (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) 
	    {
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Progress_end(&progress_state);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", "**fail %s", "making progress on rma messages failed");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    MPID_Progress_end(&progress_state);
	}
	/* local lock acquired. local puts, gets, accumulates will be done 
	   directly without queueing. */
    }
    
    else {
	/* target is some other process. add the lock request to rma_ops_list */
            
	MPIU_CHKPMEM_MALLOC(new_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops), 
			    mpi_errno, "RMA operation entry");
            
	win_ptr->rma_ops_list = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->type = MPIDI_RMA_LOCK;
	new_ptr->target_rank = dest;
	new_ptr->lock_type = lock_type;
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_LOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    int single_op_opt, type_size;
    MPIDI_RMA_ops *rma_op, *curr_op;
    MPID_Comm *comm_ptr;
    MPID_Request *req=NULL; 
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
    MPIDI_VC_t * vc;
    int wait_for_rma_done_pkt = 0, predefined;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK);

    if (dest == MPI_PROC_NULL) goto fn_exit;
        
    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
        
    if (dest == comm_ptr->rank) {
	/* local lock. release the lock on the window, grant the next one
	 * in the queue, and return. */
	mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
	if (mpi_errno != MPI_SUCCESS) goto fn_exit;
	mpi_errno = MPID_Progress_poke();
	goto fn_exit;
    }
        
    rma_op = win_ptr->rma_ops_list;
    
    if ( (rma_op == NULL) || (rma_op->type != MPIDI_RMA_LOCK) ) { 
	/* win_lock was not called. return error */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**rmasync", 0 );
	goto fn_exit;
    }
        
    if (rma_op->target_rank != dest) {
	/* The target rank is different from the one passed to win_lock! */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**winunlockrank", "**winunlockrank %d %d", dest, rma_op->target_rank);
	goto fn_exit;
    }
        
    if (rma_op->next == NULL) {
	/* only win_lock called, no put/get/acc. Do nothing and return. */
	MPIU_Free(rma_op);
	win_ptr->rma_ops_list = NULL;
	goto fn_exit;
    }
        
    single_op_opt = 0;

    MPIDI_Comm_get_vc(comm_ptr, dest, &vc);
   
    if (rma_op->next->next == NULL) {
	/* Single put, get, or accumulate between the lock and unlock. If it
	 * is of small size and predefined datatype at the target, we
	 * do an optimization where the lock and the RMA operation are
	 * sent in a single packet. Otherwise, we send a separate lock
	 * request first. */
	
	curr_op = rma_op->next;
	
	MPID_Datatype_get_size_macro(curr_op->origin_datatype, type_size);
	
	MPIDI_CH3I_DATATYPE_IS_PREDEFINED(curr_op->target_datatype, predefined);

	if ( predefined &&
	     (type_size * curr_op->origin_count <= vc->eager_max_msg_sz) ) {
	    single_op_opt = 1;
	    /* Set the lock granted flag to 1 */
	    win_ptr->lock_granted = 1;
	    if (curr_op->type == MPIDI_RMA_GET) {
		mpi_errno = MPIDI_CH3I_Send_lock_get(win_ptr);
		wait_for_rma_done_pkt = 0;
	    }
	    else {
		mpi_errno = MPIDI_CH3I_Send_lock_put_or_acc(win_ptr);
		wait_for_rma_done_pkt = 1;
	    }
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	}
    }
        
    if (single_op_opt == 0) {
	
	/* Send a lock packet over to the target. wait for the lock_granted
	 * reply. then do all the RMA ops. */ 
	
	MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
	lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
	lock_pkt->source_win_handle = win_ptr->handle;
	lock_pkt->lock_type = rma_op->lock_type;
	
	/* Set the lock granted flag to 0 */
	win_ptr->lock_granted = 0;
	
	mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, lock_pkt, sizeof(*lock_pkt), &req));
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS) {
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "sending the rma message failed");
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	
	/* release the request returned by iStartMsg */
	if (req != NULL)
	{
	    MPID_Request_release(req);
	}
	
	/* After the target grants the lock, it sends a lock_granted
	 * packet. This packet is received in ch3u_handle_recv_pkt.c.
	 * The handler for the packet sets the win_ptr->lock_granted flag to 1.
	 */
	
	/* poke the progress engine until lock_granted flag is set to 1 */
	if (win_ptr->lock_granted == 0)
	{
	    MPID_Progress_state progress_state;
	    
	    MPID_Progress_start(&progress_state);
	    while (win_ptr->lock_granted == 0)
	    {
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Progress_end(&progress_state);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", "**fail %s", "making progress on the rma messages failed");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    MPID_Progress_end(&progress_state);
	}
	
	/* Now do all the RMA operations */
	mpi_errno = MPIDI_CH3I_Do_passive_target_rma(win_ptr, 
						     &wait_for_rma_done_pkt);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
        
    /* If the lock is a shared lock or we have done the single op
       optimization, we need to wait until the target informs us that
       all operations are done on the target. */ 
    if (wait_for_rma_done_pkt == 1) {
	/* wait until the "pt rma done" packet is received from the 
	   target. This packet resets the win_ptr->lock_granted flag back to 
	   0. */
	
	/* poke the progress engine until lock_granted flag is reset to 0 */
	if (win_ptr->lock_granted != 0)
	{
	    MPID_Progress_state progress_state;
	    
	    MPID_Progress_start(&progress_state);
	    while (win_ptr->lock_granted != 0)
	    {
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Progress_end(&progress_state);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", "**fail %s", "making progress on the rma messages failed");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    MPID_Progress_end(&progress_state);
	}
    }
    else
	win_ptr->lock_granted = 0; 
    
 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_UNLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Do_passive_target_rma
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr, 
					    int *wait_for_rma_done_pkt)
{
    int mpi_errno = MPI_SUCCESS, comm_size, done, i, nops;
    MPIDI_RMA_ops *curr_ptr, *next_ptr, **curr_ptr_ptr, *tmp_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests=NULL; /* array of requests */
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */
    MPI_Win source_win_handle, target_win_handle;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

    if (win_ptr->rma_ops_list->lock_type == MPI_LOCK_EXCLUSIVE) {
        /* exclusive lock. no need to wait for rma done pkt at the end */
        *wait_for_rma_done_pkt = 0;
    }
    else {
        /* shared lock. check if any of the rma ops is a get. If so, move it 
           to the end of the list and do it last, in which case an rma done 
           pkt is not needed. If there is no get, rma done pkt is needed */

        /* First check whether the last operation is a get. Skip the first op, 
           which is a lock. */

        curr_ptr = win_ptr->rma_ops_list->next;
        while (curr_ptr->next != NULL) 
            curr_ptr = curr_ptr->next;
    
        if (curr_ptr->type == MPIDI_RMA_GET) {
            /* last operation is a get. no need to wait for rma done pkt */
            *wait_for_rma_done_pkt = 0;
        }
        else {
            /* go through the list and move the first get operation 
               (if there is one) to the end */
            
            curr_ptr = win_ptr->rma_ops_list->next;
            curr_ptr_ptr = &(win_ptr->rma_ops_list->next);
            
            *wait_for_rma_done_pkt = 1;
            
            while (curr_ptr != NULL) {
                if (curr_ptr->type == MPIDI_RMA_GET) {
                    *wait_for_rma_done_pkt = 0;
                    *curr_ptr_ptr = curr_ptr->next;
                    tmp_ptr = curr_ptr;
                    while (curr_ptr->next != NULL)
                        curr_ptr = curr_ptr->next;
                    curr_ptr->next = tmp_ptr;
                    tmp_ptr->next = NULL;
                    break;
                }
                else {
                    curr_ptr_ptr = &(curr_ptr->next);
                    curr_ptr = curr_ptr->next;
                }
            }
        }
    }

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    /* Ignore the first op in the list because it is a win_lock and do
       the rest */

    curr_ptr = win_ptr->rma_ops_list->next;
    nops = 0;
    while (curr_ptr != NULL) {
        nops++;
        curr_ptr = curr_ptr->next;
    }

    MPIU_CHKLMEM_MALLOC(requests, MPID_Request **, nops*sizeof(MPID_Request*),
			mpi_errno, "requests");
    MPIU_CHKLMEM_MALLOC(dtype_infos, MPIDI_RMA_dtype_info *, 
			nops*sizeof(MPIDI_RMA_dtype_info),
			mpi_errno, "dtype_infos");
    MPIU_CHKLMEM_MALLOC(dataloops, void **, nops*sizeof(void*),
			mpi_errno, "dataloops");

    for (i=0; i<nops; i++)
    {
        dataloops[i] = NULL;
    }
    
    i = 0;
    curr_ptr = win_ptr->rma_ops_list->next;
    target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
    while (curr_ptr != NULL)
    {
        /* To indicate the last RMA operation, we pass the
           source_win_handle only on the last operation. Otherwise, 
           we pass MPI_WIN_NULL. */
        if (i == nops - 1)
            source_win_handle = win_ptr->handle;
        else 
            source_win_handle = MPI_WIN_NULL;
        
        switch (curr_ptr->type)
        {
        case (MPIDI_RMA_PUT):  /* same as accumulate */
        case (MPIDI_RMA_ACCUMULATE):
            win_ptr->pt_rma_puts_accs[curr_ptr->target_rank]++;
            mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
                         source_win_handle, target_win_handle, &dtype_infos[i],
                                                &dataloops[i], &requests[i]);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            break;
        case (MPIDI_RMA_GET):
            mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
                         source_win_handle, target_win_handle, &dtype_infos[i],
                                                &dataloops[i], &requests[i]);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "invalid RMA operation");
            goto fn_exit;
            /* --END ERROR HANDLING-- */
        }
        i++;
        curr_ptr = curr_ptr->next;
    }
    
    if (nops)
    {
	MPID_Progress_state progress_state;
	
	done = 1;
	MPID_Progress_start(&progress_state);
	while (nops)
	{
	    for (i=0; i<nops; i++)
	    {
		if (requests[i] != NULL)
		{
		    if (*(requests[i]->cc_ptr) != 0)
		    {
			done = 0;
			break;
		    }
		    else
		    {
			mpi_errno = requests[i]->status.MPI_ERROR;
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    MPID_Progress_end(&progress_state);
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**fail", "**fail %s", "rma message operation failed");
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
			/* if origin datatype was a derived
			   datatype, it will get freed when the
			   request gets freed. */ 
			MPID_Request_release(requests[i]);
			requests[i] = NULL;
		    }
		}
	    }
	
	    if (done) 
	    {
		break;
	    }
	
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "making progress on the rma messages failed");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    done = 1;
	}
	MPID_Progress_end(&progress_state);
    } 
    
    for (i=0; i<nops; i++)
    {
        if (dataloops[i] != NULL)
        {
            MPIU_Free(dataloops[i]);
        }
    }
    
    /* free MPIDI_RMA_ops_list */
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_put_or_acc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Send_lock_put_or_acc(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, lock_type, origin_dt_derived, iov_n, iovcnt;
    MPIDI_RMA_ops *rma_op;
    MPID_Request *request=NULL;
    MPIDI_VC_t * vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *origin_dtp=NULL;
    int origin_type_size, predefined;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_put_unlock_t *lock_put_unlock_pkt = 
	&upkt.lock_put_unlock;
    MPIDI_CH3_Pkt_lock_accum_unlock_t *lock_accum_unlock_pkt = 
	&upkt.lock_accum_unlock;
        
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);

    lock_type = win_ptr->rma_ops_list->lock_type;

    rma_op = win_ptr->rma_ops_list->next;

    win_ptr->pt_rma_puts_accs[rma_op->target_rank]++;

    if (rma_op->type == MPIDI_RMA_PUT) {
        MPIDI_Pkt_init(lock_put_unlock_pkt, MPIDI_CH3_PKT_LOCK_PUT_UNLOCK);
        lock_put_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
        lock_put_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_put_unlock_pkt->lock_type = lock_type;
 
        lock_put_unlock_pkt->addr = 
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
        lock_put_unlock_pkt->count = rma_op->target_count;
        lock_put_unlock_pkt->datatype = rma_op->target_datatype;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_put_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_put_unlock_pkt);
    }
    
    else if (rma_op->type == MPIDI_RMA_ACCUMULATE) {        
        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;

        lock_accum_unlock_pkt->addr = 
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
        lock_accum_unlock_pkt->count = rma_op->target_count;
        lock_accum_unlock_pkt->datatype = rma_op->target_datatype;
        lock_accum_unlock_pkt->op = rma_op->op;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_accum_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_accum_unlock_pkt);
    }

    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype, predefined);
    if (!predefined)
    {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else
    {
        origin_dt_derived = 0;
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived)
    {
	/* basic datatype on origin */

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
        iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
        iovcnt = 2;

        mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsgv(vc, iov, iovcnt, &request));
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
            goto fn_exit;
        }
	/* --END ERROR HANDLING-- */
    }
    else
    {
	/* derived datatype on origin */

        iovcnt = 1;

        request = MPID_Request_create();
        if (request == NULL) {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
            /* --END ERROR HANDLING-- */
        }

        MPIU_Object_set_ref(request, 2);
        request->kind = MPID_REQUEST_SEND;
	    
        request->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
           is freed. */ 

	request->dev.segment_ptr = MPID_Segment_alloc( );
	/* if (!request->dev.segment_ptr) { MPIU_ERR_POP(); } */
        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype,
                          request->dev.segment_ptr, 0);
        request->dev.segment_first = 0;
        request->dev.segment_size = rma_op->origin_count * origin_type_size;
	    
        iov_n = MPID_IOV_LIMIT - iovcnt;
	/* On the initial load of a send iov req, set the OnFinal action (null
	   for point-to-point) */
	request->dev.OnFinal = 0;
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(request,
                                                     &iov[iovcnt],
                                                     &iov_n); 
        if (mpi_errno == MPI_SUCCESS)
        {
            iov_n += iovcnt;
            
            mpi_errno = MPIU_CALL(MPIDI_CH3,iSendv(vc, request, iov, iov_n));
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                MPID_Datatype_release(request->dev.datatype_ptr);
                MPIU_Object_set_ref(request, 0);
                MPIDI_CH3_Request_destroy(request);
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                goto fn_exit;
            }
	    /* --END ERROR HANDLING-- */
        }
        /* --BEGIN ERROR HANDLING-- */
        else
        {
            MPID_Datatype_release(request->dev.datatype_ptr);
            MPIU_Object_set_ref(request, 0);
            MPIDI_CH3_Request_destroy(request);
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }

    if (request != NULL) {
	if (*(request->cc_ptr) != 0)
        {
	    MPID_Progress_state progress_state;
	    
            MPID_Progress_start(&progress_state);
	    while (*(request->cc_ptr) != 0)
            {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
		    MPID_Progress_end(&progress_state);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", "**fail %s", "rma message operation failed");
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
	    MPID_Progress_end(&progress_state);
        }
        
        mpi_errno = request->status.MPI_ERROR;
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "rma message operation failed");
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
                
        MPID_Request_release(request);
    }

    /* free MPIDI_RMA_ops_list */
    MPIU_Free(win_ptr->rma_ops_list->next);
    MPIU_Free(win_ptr->rma_ops_list);
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Send_lock_get(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, lock_type, predefined;
    MPIDI_RMA_ops *rma_op;
    MPID_Request *rreq=NULL, *sreq=NULL;
    MPIDI_VC_t * vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_get_unlock_t *lock_get_unlock_pkt = 
	&upkt.lock_get_unlock;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);

    lock_type = win_ptr->rma_ops_list->lock_type;

    rma_op = win_ptr->rma_ops_list->next;

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    rreq = MPID_Request_create();
    if (rreq == NULL) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
        /* --END ERROR HANDLING-- */
    }

    MPIU_Object_set_ref(rreq, 2);

    rreq->dev.user_buf = rma_op->origin_addr;
    rreq->dev.user_count = rma_op->origin_count;
    rreq->dev.datatype = rma_op->origin_datatype;
    rreq->dev.target_win_handle = MPI_WIN_NULL;
    rreq->dev.source_win_handle = win_ptr->handle;

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(rreq->dev.datatype, predefined);
    if (!predefined)
    {
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        rreq->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(lock_get_unlock_pkt, MPIDI_CH3_PKT_LOCK_GET_UNLOCK);
    lock_get_unlock_pkt->target_win_handle = 
        win_ptr->all_win_handles[rma_op->target_rank];
    lock_get_unlock_pkt->source_win_handle = win_ptr->handle;
    lock_get_unlock_pkt->lock_type = lock_type;
 
    lock_get_unlock_pkt->addr = 
        (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
    lock_get_unlock_pkt->count = rma_op->target_count;
    lock_get_unlock_pkt->datatype = rma_op->target_datatype;
    lock_get_unlock_pkt->request_handle = rreq->handle;

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_get_unlock_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*lock_get_unlock_pkt);

    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, lock_get_unlock_pkt, 
				      sizeof(*lock_get_unlock_pkt), &sreq));
    if (mpi_errno != MPI_SUCCESS)
    {
     /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
        goto fn_exit;
    /* --END ERROR HANDLING-- */
    }

    /* release the request returned by iStartMsg */
    if (sreq != NULL)
    {
        MPID_Request_release(sreq);
    }

    /* now wait for the data to arrive */
    if (*(rreq->cc_ptr) != 0)
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while (*(rreq->cc_ptr) != 0)
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
		MPID_Progress_end(&progress_state);
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "rma message operation failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }
	MPID_Progress_end(&progress_state);
    }
    
    mpi_errno = rreq->status.MPI_ERROR;
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**fail", "**fail %s", "rma message operation failed");
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
            
    /* if origin datatype was a derived datatype, it will get freed when the rreq gets freed. */ 
    MPID_Request_release(rreq);

    /* free MPIDI_RMA_ops_list */
    MPIU_Free(win_ptr->rma_ops_list->next);
    MPIU_Free(win_ptr->rma_ops_list);
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);
    return mpi_errno;
}

/******************************************************************************
		       Distributed Memory Implementation

Assumption: all of the local windows are located in process local (not shared
or remotely accessible) memory.

ASSUMPTION: only basic datatypes are supported.

ASSUMPTION: only one RMA operation occurs between any two calls
MPID_Win_fence().

******************************************************************************/

struct MPID_Win
{
    /*** insert struct MPIR_Win here ***/
    
    unsigned				state;
    unsigned				cur_epoch;
}

#define MPIR_WIN_CLOSED 0
#define MPIR_WIN_OPEN 1

MPID_Win_create(dwin, info)
{
	dwin->access_epoch_state = EPOCH_STATE_ACTIVE;
	dwin->access_epoch_id = 0;
	dwin->exposure_epoch_state = EPOCH_STATE_ACTIVE;
	dwin->exposure_epoch_id = 0;

	dwin->rhc_cnts = malloc(sizeof(int) * dwin->np * 3);
	zero(dwin->rhc_cnts[0..np-1]);
}
/* MPID_Win_create() */


MPID_Win_free(win)
{
}
/* MPID_Win_free() */


MPID_Win_fence(assert, win)
{
    thread_mutex_lock(dwin->thread_mutex);
    {
	dwin->access_epoch_state = EPOCH_STATE_CLOSED;
	dwin->access_epoch_id++;

	copy(rhc[0..np-1], rhc[np..np*2-1]);
	zero(dwin->rhc_cnts[0..np-1]);
    }
    thread_mutex_unlock(dwin->thread_mutex);
    
    MPI_Alltoall(dwin->rhc_cnts + np, np, MPI_INT,
		 dwin->rhc_cnts + np * 2, np, MPI_INT, dwin->comm)
    dwin->rhc_incoming = sum(rhc[np*2..np*3-1]);
    
    while(dwin->rhc_processed < dwin->rhc_incoming &&
	  dwin->lops_completed < dwin->lops_posted)
    {
	/*
	 * We need to block until such time that all incoming RHCs have been
	 * handled and all locally posted operations have completed.
	 *
	 * NOTE: The number of locally posted operations may increase as a
	 * result of processing RHCs.
	 *
	 * Q: What is the right interface for this blocking operation?  The
	 * operation should block, but it needs to guarantee that forward
	 * progress is being made on both the incoming RHCs and locally posted
	 * operations.  Whatever the correct interface is, we either need to
	 * pass in dwin or declare/cast the counters unsed the above while
	 * statement as volatile.
	 */
	MPID_Win_wait(dwin);
    }
    
    thread_mutex_lock(dwin->thread_mutex);
    {
	if (assert & MPI_MODE_NOSUCCEED)
	{
	    dwin->exposure_epoch_state = EPOCH_STATE_CLOSED;
	}
	else
	{
	    dwin->access_epoch_state = EPOCH_STATE_ACTIVE;
	    dwin->exposure_epoch_state = EPOCH_STATE_ACTIVE;
	}
	
	dwin->exposure_epoch_id++;
	dwin->rhc_processed = 0;
	
	dwin->tag = 0;
    }
    thread_mutex_unlock(dwin->thread_mutex);
    
    /*
     * We could perform a barrier here to ensure that 
     */
    MPI_Barrier(dwin->comm);

}
/* MPID_Win_fence() */


MPID_Get(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, dwin)
{
    if (target_rank == dwin->rank)
    {
	/*
	 * We are attempting to get data from our own memory.
	 *
	 * NOTE: Since we are assuming only basic datatypes are supplied, we
	 * can simple copy the bytes from the local window to the supplied
	 * buffer.
	 */
	target_addr = win.bases[target_rank] + target_offset;
	memcpy(origin_addr, target_addr, origin_byte_count);
    }
    else
    {
	/*
	 * Since we are assuming only basic datatypes are supplied, we do not
	 * need to send the target datatype to the target process. (at least
	 * not yet...)
	 */

	/*
	 * NOTE: the tag 
	 */
	tag = dwin->lops_posted++;
	rc = MPID_Irecv(origin_addr, origin_count, origin_datatype, tag,
		   target_rank, dwin->comm, &req);
	MPIR_Win_req_add(dwin->cur_reqs, req);
	
	MPID_Hid_Win_get_t header;

	header.win = dwin->wins[target_rank];
	header.disp = target_disp;
	header.count = target_count;
	header.datatype = target_datatype;
	header.epoch = win.cur_epoch;
	header.tag = tag;

	iovec[0] = header;
	iovec_cnt = 1;
	MPID_Rhcv(target_rank, dwin->comm, MPID_HID_WIN_GET,
		  iovec, iovec_cnt);

	thread_mutex_lock(dwin->thread_lock);
	{
	    dwin->rhc_cnts[target_rank]++;
	}
	thread_mutex_unlock(dwin->thread_unlock);
    }
}
/* MPID_get() */


MPID_Win_get_hdlr(header)
{
    MPIR_ID_lookup(header->win, &dwin);
    if (header->epoch == dwin->cur_epoch)
    {
	addr = dwin->my_base + dwin->my_disp_unit * header->disp;
	MPI_Isend(addr, header->count, header->datatype, header->rank,
		  header->tag, &req);
	MPIR_Win_req_add(dwin->cur_reqs, req);
    }
    else
    {
    }
}


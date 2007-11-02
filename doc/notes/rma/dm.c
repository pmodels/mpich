/******************************************************************************
		       Distributed Memory Implementation
******************************************************************************/

struct MPID_Win
{
    /*** insert struct MPIR_Win here ***/

    MPI_Win *				handles;
    int *				rhc_counts;
    int					rhc_processed;
}

#define MPIR_WIN_CLOSED 0
#define MPIR_WIN_OPEN 1

MPID_Win_create(dwin, info)
{
    dwin->handles = malloc(sizeof(MPI_Win) * dwin->np);
    MPI_Allreduce(dwin->handle, dwin->handles, dwin->comm);
    
    dwin->access_epoch_state = EPOCH_STATE_ACTIVE;
    dwin->access_epoch_id = 0;
    dwin->exposure_epoch_state = EPOCH_STATE_ACTIVE;
    dwin->exposure_epoch_id = 0;

    dwin->rhc_cnts = malloc(sizeof(int) * dwin->np);
    zero(dwin->rhc_cnts[0..dwin->np-1]);
    rhc_processed = 0;
}
/* MPID_Win_create() */


MPID_Win_free(win)
{
    free(dwin->handles);
    free(dwin->rhc_cnts);
}
/* MPID_Win_free() */


MPID_Win_fence(assert, win)
{
    if (!(assert & MPI_MODE_PRECEDE))
    {
	MPI_Alltoall(dwin->rhc_cnts, 1, MPI_INT,
		     tmp, 1, MPI_INT, dwin->comm);
	rhc_incoming = sum(tmp[0..dwin->np-1]);
    
	while(dwin->rhc_processed < rhc_incoming &&
	      dwin->active_reqs.count > 0 &&
	      dwin->active_flags.count > 0)
	{
	    MPID_Win_wait(dwin);
	}

	zero(dwin->rhc_cnts[0..np-1]);
	dwin->rhc_processed = 0;
	dwin->tag = 0;
    }

    if (!(assert & MPI_MODE_SUCCEED))
    {
	MPI_Barrier(dwin->comm);
    }
}
/* MPID_Win_fence() */


MPID_Get(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, dwin)
{
    if (target_rank == dwin->rank)
    {
	target_offset = win.displ * target_disp;
	target_addr = win.base + target_offset;
	MPIR_Copy_data(target_addr, target_count, target_datatype,
		       origin_addr, origin_count, origin_datatype);
    }
    else
    {
	tag = dwin->tag++;
	req_p = dwin->active_reqs.alloc();
	flag_p = dwin->active_flags.alloc();
	
	rc = MPID_Irecv(origin_addr, origin_count, origin_datatype, tag,
		   target_rank, dwin->comm, req_p);

	header = (dwin->handles[target_rank], target_disp, target_count,
		  target_datatype, tag);
	rc = MPID_Rhcv(target_rank, dwin->comm, MPIDI_Win_get_hdlr,
		       (header), 1, flag_p);
	dwin->rhc_cnts[target_rank]++;
    }
}
/* MPID_get() */


MPIDI_Win_get_hdlr(src, comm, (header))
{
    (handle, disp, count, datatype, tag) = header;
    
    MPIR_ID_lookup(handle, &dwin);
    
    addr = dwin->base + dwin->disp_unit * disp;
    req_p = dwin->active_reqs.alloc();
    rc = MPI_Irsend(addr, count, datatype, src, tag, comm, req_p);
    
    dwin->rhc_processed++;
}
/* MPIDI_Win_get_hdlr() */


MPID_Put(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, dwin)
{
    pack_sz = MPID_Pack_size(origin_count, origin_datatype,
			     dwin->comm, target_rank);
    
    if (target_rank == dwin->rank)
    {
	target_offset = win.displ * target_disp;
	target_addr = win.base + target_offset;
	MPIR_Copy_data(origin_addr, origin_count, origin_datatype,
		       target_addr, target_count, target_datatype);
    }
    else if (pack_sz < EAGER_MAX)
    {
	flag_p = dwin->active_flags.alloc();
	header = (dwin->handles[target_rank], target_disp, target_count,
		  target_datatype);
	pos = 0;
	if (MPIR_Datatype_iscontig(origin_count, origin_datatype))
	{
	    addr = origin_addr;
	}
	else
	{
	    pos = 0;
	    MPID_Pack(origin_addr, origin_count, origin_datatype, tmpbuf, pos,
		      tmpbuf_sz, dwin->comm, target_rank);	
	    addr = tmpbuf;
	}
	MPID_Pack(origin_addr, origin_count, origin_datatype, tmpbuf, pos,
		  tmpbuf_sz, dwin->comm, target_rank);	
	rc = MPID_Rhcv(target_rank, dwin->comm, MPIDI_Win_put_eager_hdlr,
		       (header, addr[0..pos-1]), 2, flag_p);
	dwin->rhc_cnts[target_rank]++;
    }
    else
    {
	tag = dwin->tag++;
	req_p = dwin->active_reqs.alloc();
	flag_p = dwin->active_flags.alloc();
	
	header = (dwin->handles[target_rank], target_disp, target_count,
		  target_datatype, tag);
	rc = MPID_Rhcv(target_rank, dwin->comm, MPIDI_Win_put_hdlr,
		       (header), 1, flag_p);
	dwin->rhc_cnts[target_rank]++;
	
	rc = MPID_Isend(origin_addr, origin_count, origin_datatype, tag,
		   target_rank, dwin->comm, req_p);
    }
}
/* MPID_Put() */


MPIDI_Win_put_eager_hdlr(src, comm, (header,(data,data_sz)))
{
    (handle, disp, count, datatype, tag) = header;
    MPIR_ID_lookup(handle, &dwin);
    
    addr = dwin->base + dwin->disp_unit * disp;
    pos = 0;
    rc = MPID_Unpack(addr, count, datatype, data, pos, data_sz,
		     win->comm, src);
    
    dwin->rhc_processed++;
}
/* MPIDI_Win_put_eager_hdlr() */


MPIDI_Win_put_hdlr(src, comm, (header))
{
    (handle, disp, count, datatype, tag) = header;
    MPIR_ID_lookup(handle, &dwin);
    
    req_p = dwin->active_reqs.alloc();
    
    addr = dwin->base + dwin->disp_unit * disp;
    rc = MPI_Irecv(addr, count, datatype, src, tag, comm, &req);
    MPIDI_Win_req_add(dwin->active_reqs, req_p);
    
    dwin->rhc_processed++;
}
/* MPIDI_Win_put_hdlr() */


MPI_Accumulate(origin_addr, origin_count, origin_datatype,
	       target_rank, target_disp, target_count, target_datatype,
	       op, win)
{
    origin_byte_count = origin_count * origin_datatype->size;
    
    if (target_rank == dwin->rank)
    {
	target_offset = win.displ * target_disp;
	target_addr = win.base + target_offset;
	/* peform requested operation */
    }
    else if (origin_byte_count < EAGER_MAX)
    {
	flag_p = dwin->active_flags.alloc();
	if (MPIR_Datatype_iscontig(origin_count, origin_datatype))
	{
	    addr = origin_addr;
	}
	else
	{
	    pos = 0;
	    MPID_Pack(origin_addr, origin_count, origin_datatype, tmpbuf, pos,
		      tmpbuf_sz, dwin->comm, target_rank);	
	    addr = tmpbuf;
	}
	header = (dwin->handles[target_rank], target_disp, target_count,
		  target_datatype);
	rc = MPID_Rhcv(target_rank, dwin->comm, MPIDI_Win_acc_eager_hdlr,
		       (header, addr[0..data_sz]), 2, flag_p);
	dwin->rhc_cnts[target_rank]++;
    }
    else
    {
	/* ??? */
    }
}
/* MPID_Accumulate() */


MPIDI_Win_acc_contig_hdlr(src, comm, (header,(data,data_sz)))
{
    (handle, disp, count, datatype, tag) = header;
    MPIR_ID_lookup(handle, &dwin);
    
    addr = dwin->base + dwin->disp_unit * disp;
    /* perform requested operation, converting origin data if necessary */
    
    dwin->rhc_processed++;
}
/* MPIDI_Win_acc_eager_hdlr() */


MPIDI_Win_acc_eager_hdlr(src, comm, (header,(data,data_sz)))
{
    (handle, disp, count, datatype, tag) = header;
    MPIR_ID_lookup(handle, &dwin);
    
    addr = dwin->base + dwin->disp_unit * disp;
    /* perform operations */
    
    dwin->rhc_processed++;
}
/* MPIDI_Win_acc_eager_hdlr() */



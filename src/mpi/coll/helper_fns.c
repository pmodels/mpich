/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

#define COPY_BUFFER_SZ 16384

/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPID_CONTEXT_INTRA_COLL or MPID_CONTEXT_INTER_COLL. */

#undef FUNCNAME
#define FUNCNAME MPIC_Probe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPID_Comm *comm_ptr;

    MPID_Comm_get_ptr( comm, comm_ptr );

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;
    
    mpi_errno = MPID_Probe(source, tag, comm_ptr, context_id, status);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_Send
#undef FCNAME
#define FCNAME "MPIC_Send"
int MPIC_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SEND);

    MPIDI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPIC_SEND);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	MPID_Request_release(request_ptr);
    }
 fn_exit:
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_SEND);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) {
        MPID_Request_release(request_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Recv
#undef FCNAME
#define FCNAME "MPIC_Recv"
int MPIC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_RECV);

    MPIDI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPIC_RECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, status, &request_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr);
	if (mpi_errno == MPI_SUCCESS) {
	    if (status != MPI_STATUS_IGNORE) {
		*status = request_ptr->status;
	    }
	    mpi_errno = request_ptr->status.MPI_ERROR;
	}
	else { MPIU_ERR_POP(mpi_errno); }

        MPID_Request_release(request_ptr);
    }
 fn_exit:
    MPIDI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_RECV);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) { 
	MPID_Request_release(request_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Ssend
#undef FCNAME
#define FCNAME "MPIC_Ssend"
int MPIC_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SSEND);

    MPIDI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPIC_SSEND);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Ssend(buf, count, datatype, dest, tag, comm_ptr,
                           context_id, &request_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	MPID_Request_release(request_ptr);
    }
 fn_exit:
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_SSEND);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) {
        MPID_Request_release(request_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv
#undef FCNAME
#define FCNAME "MPIC_Sendrecv"
int MPIC_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Status *status) 
{
    MPID_Request *recv_req_ptr=NULL, *send_req_ptr=NULL;
    int mpi_errno, context_id;
    MPID_Comm *comm_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV);

    MPIDI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPIC_SENDRECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                           comm_ptr, context_id, &recv_req_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, 
                           comm_ptr, context_id, &send_req_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIC_Wait(send_req_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = MPIC_Wait(recv_req_ptr);
    if (mpi_errno) { MPIU_ERR_POPFATAL(mpi_errno); }
    if (status != MPI_STATUS_IGNORE)
        *status = recv_req_ptr->status;
    mpi_errno = recv_req_ptr->status.MPI_ERROR;

    MPID_Request_release(send_req_ptr);
    MPID_Request_release(recv_req_ptr);
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIDI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV);
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

/* NOTE: for regular collectives (as opposed to irregular collectives) calling
 * this function repeatedly will almost always be slower than performing the
 * equivalent inline because of the overhead of the repeated malloc/free */
#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv_replace
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t context_id_offset;
    MPID_Request *sreq;
    MPID_Request *rreq;
    void *tmpbuf = NULL;
    int tmpbuf_size = 0;
    int tmpbuf_count = 0;
    MPID_Comm *comm_ptr;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV_REPLACE);
#ifdef MPID_LOG_ARROWS
    /* The logging macros log sendcount and recvcount */
    int sendcount = count, recvcount = count;
#endif

    MPIDI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPIC_SENDRECV_REPLACE);
    
    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id_offset = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    if (count > 0 && dest != MPI_PROC_NULL)
    {
        MPIR_Pack_size_impl(count, datatype, &tmpbuf_size);
        MPIU_CHKLMEM_MALLOC(tmpbuf, void *, tmpbuf_size, mpi_errno, "temporary send buffer");

        mpi_errno = MPIR_Pack_impl(buf, count, datatype, tmpbuf, tmpbuf_size, &tmpbuf_count);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag,
                           comm_ptr, context_id_offset, &rreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_Isend(tmpbuf, tmpbuf_count, MPI_PACKED, dest,
                           sendtag, comm_ptr, context_id_offset,
                           &sreq);
    if (mpi_errno != MPI_SUCCESS)
    {
        /* --BEGIN ERROR HANDLING-- */
        /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
        MPID_Request_release(rreq);
        MPIU_ERR_POP(mpi_errno);
        /* --END ERROR HANDLING-- */
    }

    if (!MPID_Request_is_complete(sreq) || !MPID_Request_is_complete(rreq))
    {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(sreq) || !MPID_Request_is_complete(rreq))
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS)
            {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                MPIU_ERR_POP(mpi_errno);
                /* --END ERROR HANDLING-- */
            }
        }
        MPID_Progress_end(&progress_state);
    }

    if (status != MPI_STATUS_IGNORE) {
        *status = rreq->status;
    }

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = rreq->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
        }
    }

    MPID_Request_release(sreq);
    MPID_Request_release(rreq);

    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV_REPLACE);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Localcopy
#undef FCNAME
#define FCNAME "MPIR_Localcopy"
int MPIR_Localcopy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
    int mpi_errno = MPI_SUCCESS;
    int sendtype_iscontig, recvtype_iscontig;
    MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz, copy_sz;
    MPI_Aint true_extent, sendtype_true_lb, recvtype_true_lb;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_LOCALCOPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_LOCALCOPY);

    MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_iscontig(recvtype, &recvtype_iscontig);

    MPID_Datatype_get_size_macro(sendtype, sendsize);
    MPID_Datatype_get_size_macro(recvtype, recvsize);
    sdata_sz = sendsize * sendcount;
    rdata_sz = recvsize * recvcount;

    if (!sdata_sz || !rdata_sz)
        goto fn_exit;
    
    if (sdata_sz > rdata_sz)
    {
        MPIU_ERR_SET2(mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz);
        copy_sz = rdata_sz;
    }
    else
    {
        copy_sz = sdata_sz;
    }

    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &true_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &true_extent);

    if (sendtype_iscontig && recvtype_iscontig)
    {
#if defined(HAVE_ERROR_CHECKING)
        MPIU_ERR_CHKMEMCPYANDJUMP(mpi_errno,
                                  ((char *)recvbuf + recvtype_true_lb),
                                  ((char *)sendbuf + sendtype_true_lb),
                                  copy_sz);
#endif
        MPIU_Memcpy(((char *) recvbuf + recvtype_true_lb),
               ((char *) sendbuf + sendtype_true_lb),
               copy_sz);
    }
    else if (sendtype_iscontig)
    {
        MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(recvbuf, recvcount, recvtype, &seg, 0);
	last = copy_sz;
	MPID_Segment_unpack(&seg, 0, &last, (char*)sendbuf + sendtype_true_lb);
        MPIU_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
    }
    else if (recvtype_iscontig)
    {
        MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(sendbuf, sendcount, sendtype, &seg, 0);
	last = copy_sz;
	MPID_Segment_pack(&seg, 0, &last, (char*)recvbuf + recvtype_true_lb);
        MPIU_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
    }
    else
    {
	char * buf;
	MPIDI_msg_sz_t buf_off;
	MPID_Segment sseg;
	MPIDI_msg_sz_t sfirst;
	MPID_Segment rseg;
	MPIDI_msg_sz_t rfirst;

        MPIU_CHKLMEM_MALLOC(buf, char *, COPY_BUFFER_SZ, mpi_errno, "buf");

	MPID_Segment_init(sendbuf, sendcount, sendtype, &sseg, 0);
	MPID_Segment_init(recvbuf, recvcount, recvtype, &rseg, 0);

	sfirst = 0;
	rfirst = 0;
	buf_off = 0;
	
	while (1)
	{
	    MPI_Aint last;
	    char * buf_end;

	    if (copy_sz - sfirst > COPY_BUFFER_SZ - buf_off)
	    {
		last = sfirst + (COPY_BUFFER_SZ - buf_off);
	    }
	    else
	    {
		last = copy_sz;
	    }
	    
	    MPID_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
	    MPIU_Assert(last > sfirst);
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPID_Segment_unpack(&rseg, rfirst, &last, buf);
	    MPIU_Assert(last > rfirst);

	    rfirst = last;

	    if (rfirst == copy_sz)
	    {
		/* successful completion */
		break;
	    }

            /* if the send side finished, but the recv side couldn't unpack it, there's a datatype mismatch */
            MPIU_ERR_CHKANDJUMP(sfirst == copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");        

            /* if not all data was unpacked, copy it to the front of the buffer for next time */
	    buf_off = sfirst - rfirst;
	    if (buf_off > 0)
	    {
		memmove(buf, buf_end - buf_off, buf_off);
	    }
	}
    }
    
    
  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_LOCALCOPY);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_Isend
#undef FCNAME
#define FCNAME "MPIC_Isend"
int MPIC_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_ISEND);

    MPIDI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPIC_ISEND);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); } 

    *request = request_ptr->handle;

 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_ISEND);
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIC_Irecv
#undef FCNAME
#define FCNAME "MPIC_Irecv"
int MPIC_Irecv(void *buf, int count, MPI_Datatype datatype, int
               source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_IRECV);

    MPIDI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPIC_IRECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    *request = request_ptr->handle;

 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIDI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_IRECV);
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

/* FIXME: For the brief-global and finer-grain control, we must ensure that
   the global lock is *not* held when this routine is called. (unless we change
   progress_start/end to grab the lock, in which case we must *still* make
   sure that the lock is not held when this routine is called). */
#undef FUNCNAME
#define FUNCNAME MPIC_Wait
#undef FCNAME
#define FCNAME "MPIC_Wait"
int MPIC_Wait(MPID_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_WAIT);

    MPIDI_PT2PT_FUNC_ENTER(MPID_STATE_MPIC_WAIT);
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	}
	MPID_Progress_end(&progress_state);
    }

 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_WAIT);
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}


/* Fault-tolerance versions.  When a process fails, collectives will
   still complete, however the result may be invalid.  Processes
   directly communicating with the failed process can detect the
   failure, however another mechanism is needed to commuinicate the
   failure to other processes receiving the invalid data.  To do this
   we introduce the _ft versions of the MPIC_ helper functions.  These
   functions take a pointer to an error flag.  When this is set to
   TRUE, the send functions will communicate the failure to the
   receiver.  If a function detects a failure, either by getting a
   failure in the communication operation, or by receiving an error
   indicator from a remote process, it sets the error flag to TRUE.

   In this implementation, we indicate an error to a remote process by
   sending an empty message instead of the requested buffer.  When a
   process receives an empty message, it knows to set the error flag.
   We count on the fact that collectives that exchange data (as
   opposed to barrier) will never send an empty message.  The barrier
   collective will not communicate failure information this way, but
   this is OK since there is no data that can be received corrupted. */

#undef FUNCNAME
#define FUNCNAME MPIC_Send_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Send_ft(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SEND_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SEND_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (*errflag && MPIR_PARAM_ENABLE_COLL_FT_RET)
        mpi_errno = MPIC_Send(buf, count, datatype, dest, MPIR_ERROR_TAG, comm);
    else
        mpi_errno = MPIC_Send(buf, count, datatype, dest, tag, comm);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SEND_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Recv_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Recv_ft(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                 MPI_Comm comm, MPI_Status *status, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_RECV_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_RECV_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (!MPIR_PARAM_ENABLE_COLL_FT_RET) {
            mpi_errno = MPIC_Recv(buf, count, datatype, source, tag, comm, status);
            goto fn_exit;
    }
    
    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;
    
    mpi_errno = MPIC_Recv(buf, count, datatype, source, MPI_ANY_TAG, comm, status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (*errflag)
        goto fn_exit;

    if (source != MPI_PROC_NULL) {
        if (status->MPI_TAG == MPIR_ERROR_TAG)
            *errflag = TRUE;
        else {
            MPIU_Assert(status->MPI_TAG == tag);
        }
    }

 fn_exit:
    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "OUT: errflag = %s", *errflag?"TRUE":"FALSE");
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_RECV_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Ssend_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Ssend_ft(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SSEND_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SSEND_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");
    
    if (*errflag && MPIR_PARAM_ENABLE_COLL_FT_RET)
        mpi_errno = MPIC_Ssend(buf, count, datatype, dest, MPIR_ERROR_TAG, comm);
    else
        mpi_errno = MPIC_Ssend(buf, count, datatype, dest, tag, comm);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SSEND_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Sendrecv_ft(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     int dest, int sendtag, void *recvbuf, int recvcount,
                     MPI_Datatype recvtype, int source, int recvtag,
                     MPI_Comm comm, MPI_Status *status, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SENDRECV_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (!MPIR_PARAM_ENABLE_COLL_FT_RET) {
        mpi_errno = MPIC_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                                  recvbuf, recvcount, recvtype, source, recvtag,
                                  comm, status);
        goto fn_exit;
    }
    
    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;
    
    if (*errflag) {
        mpi_errno = MPIC_Sendrecv(sendbuf, sendcount, sendtype, dest, MPIR_ERROR_TAG,
                                  recvbuf, recvcount, recvtype, source, MPI_ANY_TAG,
                                  comm, status);
        goto fn_exit;
    }
    
    mpi_errno = MPIC_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                              recvbuf, recvcount, recvtype, source, MPI_ANY_TAG,
                              comm, status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (source != MPI_PROC_NULL) {
        if (status->MPI_TAG == MPIR_ERROR_TAG)
            *errflag = TRUE;
        else {
            MPIU_Assert(status->MPI_TAG == recvtag);
        }
    }
    
 fn_exit:
    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "OUT: errflag = %s", *errflag?"TRUE":"FALSE");

    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SENDRECV_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv_replace_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Sendrecv_replace_ft(void *buf, int count, MPI_Datatype datatype,
                             int dest, int sendtag,
                             int source, int recvtag,
                             MPI_Comm comm, MPI_Status *status, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV_REPLACE_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SENDRECV_REPLACE_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (!MPIR_PARAM_ENABLE_COLL_FT_RET) {
        mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                          dest, sendtag,
                                          source, recvtag,
                                          comm, status);
        goto fn_exit;
    }

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;
    
    if (*errflag) {
        mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                          dest, MPIR_ERROR_TAG,
                                          source, MPI_ANY_TAG,
                                          comm, status);
        goto fn_exit;
    }
    
    mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                      dest, sendtag,
                                      source, MPI_ANY_TAG,
                                      comm, status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    if (source != MPI_PROC_NULL) {
        if (status->MPI_TAG == MPIR_ERROR_TAG)
            *errflag = TRUE;
        else {
            MPIU_Assert(status->MPI_TAG == recvtag);
        }
    }

 fn_exit:
    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "OUT: errflag = %s", *errflag?"TRUE":"FALSE");
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SENDRECV_REPLACE_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Isend_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Isend_ft(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, MPI_Request *request, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_ISEND_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_ISEND_FT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (*errflag && MPIR_PARAM_ENABLE_COLL_FT_RET)
        mpi_errno = MPIC_Isend(buf, count, datatype, dest, MPIR_ERROR_TAG, comm, request);
    else
        mpi_errno = MPIC_Isend(buf, count, datatype, dest, tag, comm, request);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_ISEND_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Irecv_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Irecv_ft(void *buf, int count, MPI_Datatype datatype, int source,
                  int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_IRECV_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_IRECV_FT);

    if (MPIR_PARAM_ENABLE_COLL_FT_RET)
        mpi_errno = MPIC_Irecv(buf, count, datatype, source, MPI_ANY_TAG, comm, request);
    else
        mpi_errno = MPIC_Irecv(buf, count, datatype, source, tag, comm, request);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_IRECV_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_Waitall_ft
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIC_Waitall_ft(int numreq, MPI_Request requests[], MPI_Status statuses[], int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_WAITALL_FT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_WAITALL_FT);

    MPIU_Assert(statuses != MPI_STATUSES_IGNORE);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    mpi_errno = MPIR_Waitall_impl(numreq, requests, statuses);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (*errflag || !MPIR_PARAM_ENABLE_COLL_FT_RET)
        goto fn_exit;

    for (i = 0; i < numreq; ++i) {
        if (statuses[i].MPI_TAG == MPIR_ERROR_TAG) {
            *errflag = TRUE;
            break;
        }
    }

 fn_exit:
    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "OUT: errflag = %s", *errflag?"TRUE":"FALSE");
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_WAITALL_FT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

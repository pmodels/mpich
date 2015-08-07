/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

#define COPY_BUFFER_SZ 16384
#if !defined(MPIC_REQUEST_PTR_ARRAY_SIZE)
#define MPIC_REQUEST_PTR_ARRAY_SIZE 64
#endif

/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPID_CONTEXT_INTRA_COLL or MPID_CONTEXT_INTER_COLL. */

#undef FUNCNAME
#define FUNCNAME MPIC_Probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
#define FUNCNAME MPIR_Localcopy
#undef FCNAME
#define FCNAME "MPIR_Localcopy"
int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype)
{
    int mpi_errno = MPI_SUCCESS;
    int sendtype_iscontig, recvtype_iscontig;
    MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz, copy_sz;
    MPI_Aint true_extent, sendtype_true_lb, recvtype_true_lb;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_LOCALCOPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_LOCALCOPY);

    MPID_Datatype_get_size_macro(sendtype, sendsize);
    MPID_Datatype_get_size_macro(recvtype, recvsize);

    sdata_sz = sendsize * sendcount;
    rdata_sz = recvsize * recvcount;

    /* if there is no data to copy, bail out */
    if (!sdata_sz || !rdata_sz)
        goto fn_exit;

#if defined(HAVE_ERROR_CHECKING)
    if (sdata_sz > rdata_sz) {
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz);
        copy_sz = rdata_sz;
    }
    else
#endif /* HAVE_ERROR_CHECKING */
        copy_sz = sdata_sz;

    /* Builtin types is the common case; optimize for it */
    if ((HANDLE_GET_KIND(sendtype) == HANDLE_KIND_BUILTIN) &&
        HANDLE_GET_KIND(recvtype) == HANDLE_KIND_BUILTIN) {
        MPIU_Memcpy(recvbuf, sendbuf, copy_sz);
        goto fn_exit;
    }

    MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_iscontig(recvtype, &recvtype_iscontig);

    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &true_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &true_extent);

    if (sendtype_iscontig && recvtype_iscontig)
    {
#if defined(HAVE_ERROR_CHECKING)
        MPIR_ERR_CHKMEMCPYANDJUMP(mpi_errno,
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
        MPIR_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
    }
    else if (recvtype_iscontig)
    {
        MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(sendbuf, sendcount, sendtype, &seg, 0);
	last = copy_sz;
	MPID_Segment_pack(&seg, 0, &last, (char*)recvbuf + recvtype_true_lb);
        MPIR_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
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
            MPIR_ERR_CHKANDJUMP(sfirst == copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");        

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


/* FIXME: For the brief-global and finer-grain control, we must ensure that
   the global lock is *not* held when this routine is called. (unless we change
   progress_start/end to grab the lock, in which case we must *still* make
   sure that the lock is not held when this routine is called). */
#undef FUNCNAME
#define FUNCNAME MPIC_Wait
#undef FCNAME
#define FCNAME "MPIC_Wait"
int MPIC_Wait(MPID_Request * request_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_WAIT);

    MPIDI_PT2PT_FUNC_ENTER(MPID_STATE_MPIC_WAIT);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (request_ptr->kind == MPID_REQUEST_SEND)
        request_ptr->status.MPI_TAG = 0;

    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	}
	MPID_Progress_end(&progress_state);
    }

    if (request_ptr->kind == MPID_REQUEST_RECV)
        MPIR_Process_status(&request_ptr->status, errflag);

    MPIR_TAG_CLEAR_ERROR_BITS(request_ptr->status.MPI_TAG);

 fn_exit:
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_WAIT);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_exit;
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
#define FUNCNAME MPIC_Send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                 MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPID_Request *request_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SEND);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr,
                          context_id, &request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_Request_release(request_ptr);
    }

 fn_exit:
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SEND);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) MPID_Request_release(request_ptr);
    if (mpi_errno && !*errflag) {
        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(mpi_errno)) {
            *errflag = MPIR_ERR_PROC_FAILED;
        } else {
            *errflag = MPIR_ERR_OTHER;
        }
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
                 MPID_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPI_Status mystatus;
    MPID_Request *request_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_RECV);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, status, &request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        *status = request_ptr->status;
        mpi_errno = status->MPI_ERROR;
        MPID_Request_release(request_ptr);
    } else {
        MPIR_Process_status(status, errflag);

        MPIR_TAG_CLEAR_ERROR_BITS(status->MPI_TAG);
    }

    if (MPI_SUCCESS == MPIR_ERR_GET_CLASS(status->MPI_ERROR)) {
        MPIU_Assert(status->MPI_TAG == tag);
    }

 fn_exit:
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_RECV);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) MPID_Request_release(request_ptr);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                  MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPID_Request *request_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SSEND);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
            "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    mpi_errno = MPID_Ssend(buf, count, datatype, dest, tag, comm_ptr,
                           context_id, &request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_Request_release(request_ptr);
    }

 fn_exit:
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SSEND);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr) MPID_Request_release(request_ptr);
    if (mpi_errno && !*errflag) {
        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(mpi_errno)) {
            *errflag = MPIR_ERR_PROC_FAILED;
        } else {
            *errflag = MPIR_ERR_OTHER;
        }
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                     int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                     MPI_Datatype recvtype, int source, int recvtag,
                     MPID_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPI_Status mystatus;
    MPID_Request *recv_req_ptr = NULL, *send_req_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SENDRECV);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    MPIR_ERR_CHKANDJUMP1((sendcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", sendcount);
    MPIR_ERR_CHKANDJUMP1((recvcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", recvcount);

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE) status = &mystatus;
    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(sendtag);
        default:
            MPIR_TAG_SET_ERROR_BIT(sendtag);
    }

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                           comm_ptr, context_id, &recv_req_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag,
                           comm_ptr, context_id, &send_req_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIC_Wait(send_req_ptr, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIC_Wait(recv_req_ptr, errflag);
    if (mpi_errno) MPIR_ERR_POPFATAL(mpi_errno);

    *status = recv_req_ptr->status;

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = recv_req_ptr->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = send_req_ptr->status.MPI_ERROR;
        }
    }

    MPID_Request_release(send_req_ptr);
    MPID_Request_release(recv_req_ptr);

 fn_exit:
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SENDRECV);
    return mpi_errno;
 fn_fail:
    if (send_req_ptr)
        MPID_Request_release(send_req_ptr);
    if (recv_req_ptr)
        MPID_Request_release(recv_req_ptr);
    goto fn_exit;
}

/* NOTE: for regular collectives (as opposed to irregular collectives) calling
 * this function repeatedly will almost always be slower than performing the
 * equivalent inline because of the overhead of the repeated malloc/free */
#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                             int dest, int sendtag,
                             int source, int recvtag,
                             MPID_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    MPIU_Context_id_t context_id_offset;
    MPID_Request *sreq = NULL;
    MPID_Request *rreq = NULL;
    void *tmpbuf = NULL;
    MPI_Aint tmpbuf_size = 0;
    MPI_Aint tmpbuf_count = 0;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIC_SENDRECV_REPLACE);
#ifdef MPID_LOG_ARROWS
    /* The logging macros log sendcount and recvcount */
    int sendcount = count, recvcount = count;
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_SENDRECV_REPLACE);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    if (status == MPI_STATUS_IGNORE) status = &mystatus;
    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(sendtag);
        default:
            MPIR_TAG_SET_ERROR_BIT(sendtag);
    }

    context_id_offset = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    if (count > 0 && dest != MPI_PROC_NULL) {
        MPIR_Pack_size_impl(count, datatype, &tmpbuf_size);
        MPIU_CHKLMEM_MALLOC(tmpbuf, void *, tmpbuf_size, mpi_errno, "temporary send buffer");

        mpi_errno = MPIR_Pack_impl(buf, count, datatype, tmpbuf, tmpbuf_size, &tmpbuf_count);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag,
                           comm_ptr, context_id_offset, &rreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_Isend(tmpbuf, tmpbuf_count, MPI_PACKED, dest,
                           sendtag, comm_ptr, context_id_offset, &sreq);
    if (mpi_errno != MPI_SUCCESS) {
        /* --BEGIN ERROR HANDLING-- */
        /* FIXME: should we cancel the pending (possibly completed) receive
         * request or wait for it to complete? */
        MPID_Request_release(rreq);
        MPIR_ERR_POP(mpi_errno);
        /* --END ERROR HANDLING-- */
    }

    mpi_errno = MPIC_Wait(sreq, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIC_Wait(rreq, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    *status = rreq->status;

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = rreq->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
        }
    }

    MPID_Request_release(sreq);
    MPID_Request_release(rreq);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_SENDRECV_REPLACE);
    return mpi_errno;
 fn_fail:
     if (sreq)
         MPID_Request_release(sreq);
     if (rreq)
         MPID_Request_release(rreq);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPID_Comm *comm_ptr, MPID_Request **request_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_ISEND);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr,
            context_id, request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_ISEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPID_Comm *comm_ptr, MPID_Request **request_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_ISSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_ISSEND);

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "IN: errflag = %d", *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    switch(*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Issend(buf, count, datatype, dest, tag, comm_ptr,
            context_id, request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_ISSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
                  int tag, MPID_Comm *comm_ptr, MPID_Request **request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_IRECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_IRECV);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr,
            context_id, request_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_IRECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_Waitall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Waitall(int numreq, MPID_Request *requests[], MPI_Status statuses[], MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Request request_ptr_array[MPIC_REQUEST_PTR_ARRAY_SIZE];
    MPI_Request *request_ptrs = request_ptr_array;
    MPI_Status status_static_array[MPIC_REQUEST_PTR_ARRAY_SIZE];
    MPI_Status *status_array = statuses;
    MPIDI_STATE_DECL(MPID_STATE_MPIC_WAITALL);
    MPIU_CHKLMEM_DECL(2);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIC_WAITALL);

    MPIU_DBG_MSG_S(PT2PT, TYPICAL, "IN: errflag = %s", *errflag?"TRUE":"FALSE");

    if (statuses == MPI_STATUSES_IGNORE) {
        status_array = status_static_array;
    }

    if (numreq > MPIC_REQUEST_PTR_ARRAY_SIZE) {
        MPIU_CHKLMEM_MALLOC(request_ptrs, MPI_Request *, numreq * sizeof(MPI_Request), mpi_errno, "request pointers");
        MPIU_CHKLMEM_MALLOC(status_array, MPI_Status *, numreq * sizeof(MPI_Status), mpi_errno, "status objects");
    }

    for (i = 0; i < numreq; ++i) {
        /* The MPI_TAG field is not set for send operations, so if we want
        to check for the error bit in the tag below, we should initialize all
        tag fields here. */
        status_array[i].MPI_TAG = 0;

        /* Convert the MPID_Request objects to MPI_Request objects */
        request_ptrs[i] = requests[i]->handle;
    }

    mpi_errno = MPIR_Waitall_impl(numreq, request_ptrs, status_array);

    /* The errflag value here is for all requests, not just a single one.  If
     * in the future, this function is used for multiple collectives at a
     * single time, we may have to change that. */
    for (i = 0; i < numreq; ++i) {
        MPIR_Process_status(&status_array[i], errflag);

        MPIR_TAG_CLEAR_ERROR_BITS(status_array[i].MPI_TAG);
    }

 fn_exit:
    if (numreq > MPIC_REQUEST_PTR_ARRAY_SIZE)
        MPIU_CHKLMEM_FREEALL();

    MPIU_DBG_MSG_D(PT2PT, TYPICAL, "OUT: errflag = %d", *errflag);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIC_WAITALL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

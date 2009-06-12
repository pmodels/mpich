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
    if (request_ptr) {
        MPID_Request_release(request_ptr);
    }
    goto fn_exit;
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
    if (request_ptr) { 
	MPID_Request_release(request_ptr);
    }
    goto fn_exit;
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
    if (request_ptr) {
        MPID_Request_release(request_ptr);
    }
    goto fn_exit;
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
    MPIDI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV);
    return mpi_errno;
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
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_LOCALCOPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_LOCALCOPY);

    MPIU_THREADPRIV_GET;

    MPIR_Nest_incr();
    
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

    mpi_errno = NMPI_Type_get_true_extent(sendtype, &sendtype_true_lb, &true_extent);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = NMPI_Type_get_true_extent(recvtype, &recvtype_true_lb, &true_extent);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    if (sendtype_iscontig && recvtype_iscontig)
    {    
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
    MPIR_Nest_decr();
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
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_ISEND);
    return mpi_errno;
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
    MPIDI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_IRECV);
    return mpi_errno;
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
    if ((*(request_ptr)->cc_ptr) != 0)
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while((*(request_ptr)->cc_ptr) != 0)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	}
	MPID_Progress_end(&progress_state);
    }

 fn_fail:
    MPIDI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_WAIT);
    return mpi_errno;
}

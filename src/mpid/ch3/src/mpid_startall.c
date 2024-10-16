/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* FIXME: Consider using function pointers for invoking persistent requests;
   if we made those part of the public request structure, the top-level routine
   could implement startall unless the device wanted to study the requests
   and reorder them */

/* FIXME: Where is the memory registration call in the init routines, 
   in case the channel wishes to take special action (such as pinning for DMA)
   the memory? This was part of the design. */

/* This macro initializes all of the fields in a persistent request */
#define MPIDI_Request_create_psreq(sreq_, mpi_errno_, FAIL_)		\
{									\
    (sreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_SEND);                  \
    if ((sreq_) == NULL)						\
    {									\
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"send request allocation failed");\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;				\
	FAIL_;								\
    }									\
									\
    MPIR_Object_set_ref((sreq_), 1);					\
    MPIR_cc_set(&(sreq_)->cc, 0);                                       \
    (sreq_)->comm = comm;						\
    MPIR_Comm_add_ref(comm);						\
    (sreq_)->dev.match.parts.rank = rank;				\
    (sreq_)->dev.match.parts.tag = tag;					\
    (sreq_)->dev.match.parts.context_id = comm->context_id + context_offset;	\
    (sreq_)->dev.user_buf = (void *) buf;				\
    (sreq_)->dev.user_count = count;					\
    (sreq_)->dev.datatype = datatype;					\
    (sreq_)->u.persist.real_request = NULL;                             \
    MPIR_Comm_save_inactive_request(comm, sreq_);                       \
}

	
/*
 * MPID_Startall()
 */
int MPID_Startall(int count, MPIR_Request * requests[])
{
    int i;
    int rc;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    for (i = 0; i < count; i++)
    {
	MPIR_Request * const preq = requests[i];
        MPIR_Request_start(preq);

        if (preq->kind == MPIR_REQUEST_KIND__PREQUEST_COLL) {
            mpi_errno = MPIR_Persist_coll_start(preq);
            MPIR_ERR_CHECK(mpi_errno);
            continue;
        }

        if (preq->kind == MPIR_REQUEST_KIND__CONTINUE) {
            mpi_errno = MPIR_Continue_start(preq);
            MPIR_ERR_CHECK(mpi_errno);
            continue;
        }

        /* only pt2pt requests should reach here */
        MPIR_Assert(preq->kind == MPIR_REQUEST_KIND__PREQUEST_SEND ||
                    preq->kind == MPIR_REQUEST_KIND__PREQUEST_RECV);

        /* continue if the source/dest is MPI_PROC_NULL */
        if (preq->dev.match.parts.rank == MPI_PROC_NULL)
            continue;


	/* FIXME: The odd 7th arg (match.context_id - comm->context_id) 
	   is probably to get the context offset.  Do we really need the
	   context offset? Is there any case where the offset isn't zero? */
	switch (MPIDI_Request_get_type(preq))
	{
	    case MPIDI_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.parts.rank,
		    preq->dev.match.parts.tag, preq->comm, preq->dev.match.parts.context_id - preq->comm->recvcontext_id,
		    &preq->u.persist.real_request);
		break;
	    }
	    
	    case MPIDI_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.parts.rank,
		    preq->dev.match.parts.tag, preq->comm, preq->dev.match.parts.context_id - preq->comm->context_id,
		    &preq->u.persist.real_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.parts.rank,
		    preq->dev.match.parts.tag, preq->comm, preq->dev.match.parts.context_id - preq->comm->context_id,
		    &preq->u.persist.real_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.parts.rank,
		    preq->dev.match.parts.tag, preq->comm, preq->dev.match.parts.context_id - preq->comm->context_id,
		    &preq->u.persist.real_request);
		break;
	    }

	    case MPIDI_REQUEST_TYPE_BSEND:
	    {
                rc = MPIR_Bsend_isend(preq->dev.user_buf, preq->dev.user_count,
                                      preq->dev.datatype, preq->dev.match.parts.rank,
                                      preq->dev.match.parts.tag, preq->comm,
                                      &preq->u.persist.real_request);
                if (rc == MPI_SUCCESS) {
                    preq->status.MPI_ERROR = MPI_SUCCESS;
                    preq->cc_ptr = &preq->cc;
                    /* bsend is local-complete */
                    MPIR_cc_set(preq->cc_ptr, 0);
                    goto fn_exit;
                }
		break;
	    }

	    default:
	    {
		/* --BEGIN ERROR HANDLING-- */
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __func__, __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
					  "**ch3|badreqtype %d", MPIDI_Request_get_type(preq));
		/* --END ERROR HANDLING-- */
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->status.MPI_ERROR = MPI_SUCCESS;
	    preq->cc_ptr = &preq->u.persist.real_request->cc;
	}
	/* --BEGIN ERROR HANDLING-- */
	else
	{
	    /* If a failure occurs attempting to start the request, then we 
	       assume that partner request was not created, and stuff
	       the error code in the persistent request.  The wait and test
	       routines will look at the error code in the persistent
	       request if a partner request is not present. */
	    preq->u.persist.real_request = NULL;
	    preq->status.MPI_ERROR = rc;
	    preq->cc_ptr = &preq->cc;
            MPIR_cc_set(&preq->cc, 0);
	}
	/* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* FIXME:
   Move the routines that initialize the persistent requests into this file,
   since startall must be used with all of them */

/*
 * MPID_Send_init()
 */
int MPID_Send_init(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int attr,
		   MPIR_Request ** request)
{
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    if (!HANDLE_IS_BUILTIN(datatype))
    {
	MPIR_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
    MPIR_Datatype_ptr_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/*
 * MPID_Ssend_init()
 */
int MPID_Ssend_init(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int attr,
		    MPIR_Request ** request)
{
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    *request = sreq;

  fn_exit:    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/*
 * MPID_Rsend_init()
 */
int MPID_Rsend_init(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int attr,
		    MPIR_Request ** request)
{
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
    if (!HANDLE_IS_BUILTIN(datatype))
    {
	MPIR_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
    MPIR_Datatype_ptr_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/*
 * MPID_Bsend_init()
 */
int MPID_Bsend_init(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int attr,
		    MPIR_Request ** request)
{
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_BSEND);
    if (!HANDLE_IS_BUILTIN(datatype))
    {
	MPIR_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
    MPIR_Datatype_ptr_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* 
 * FIXME: The ch3 implementation of the persistent routines should
 * be very simple and use common code as much as possible.  All
 * persistent routine should be in the same file, along with 
 * startall.  Consider using function pointers to specify the 
 * start functions, as if these were generalized requests, 
 * rather than having MPID_Startall look at the request type.
 */
/*
 * MPID_Recv_init()
 */
int MPID_Recv_init(void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int context_offset,
		   MPIR_Request ** request)
{
    MPIR_Request * rreq;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    
    rreq = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_RECV);
    if (rreq == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __func__, __LINE__, MPI_ERR_OTHER, "**nomemreq", 0);
	/* --END ERROR HANDLING-- */
	goto fn_exit;
    }
    
    MPIR_Object_set_ref(rreq, 1);
    rreq->comm = comm;
    MPIR_cc_set(&rreq->cc, 0);
    MPIR_Comm_add_ref(comm);
    rreq->dev.match.parts.rank = rank;
    rreq->dev.match.parts.tag = tag;
    rreq->dev.match.parts.context_id = comm->recvcontext_id + context_offset;
    rreq->dev.user_buf = (void *) buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;
    rreq->u.persist.real_request = NULL;
    MPIR_Comm_save_inactive_request(comm, rreq);
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_RECV);
    if (!HANDLE_IS_BUILTIN(datatype))
    {
	MPIR_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
    MPIR_Datatype_ptr_add_ref(rreq->dev.datatype_ptr);
    }
    *request = rreq;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Bcast_init(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                    MPIR_Comm *comm_ptr, MPIR_Info* info_ptr, MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Bcast_init_impl(buffer, count, datatype, root, comm_ptr, info_ptr, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Allreduce_init(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Info * info_ptr,
                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allreduce_init_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, info_ptr,
                                         request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Reduce_init(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                     MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Info* info_ptr,
                     MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_init_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      info_ptr, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Alltoall_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                       MPIR_Comm *comm_ptr, MPIR_Info* info_ptr, MPIR_Request** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoall_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, info_ptr, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Alltoallv_init(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                        MPI_Datatype sendtype, void *recvbuf, const MPI_Aint recvcounts[],
                        const MPI_Aint rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoallv_init_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                         recvcounts, rdispls, recvtype, comm_ptr, info_ptr,
                                         request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Alltoallw_init(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                        const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Aint recvcounts[],
                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                        MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoallw_init_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                         recvcounts, rdispls, recvtypes, comm_ptr, info_ptr,
                                         request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Allgather_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Info* info_ptr, MPIR_Request** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allgather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, info_ptr, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Allgatherv_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                         void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                         MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Info* info_ptr,
                         MPIR_Request** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allgatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, comm_ptr, info_ptr, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                   MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_scatter_block_init_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                                    info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Reduce_scatter_init(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm, MPIR_Info * info,
                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_scatter_init_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                              info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Scan_init(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scan_init_impl(sendbuf, recvbuf, count, datatype, op, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Gather_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype, void *recvbuf,
                     MPI_Aint recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                     MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Gather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Gatherv_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype, void *recvbuf,
                      const MPI_Aint recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype,
                      int root, MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Gatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                       recvtype, root, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Scatter_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype, void *recvbuf,
                      MPI_Aint recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                      MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scatter_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       root, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Scatterv_init(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint displs[],
                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                       MPI_Datatype recvtype, int root, MPIR_Comm * comm, MPIR_Info * info,
                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scatterv_init_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Barrier_init(MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Barrier_init_impl(comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Exscan_init(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                     MPI_Op op, MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Exscan_init_impl(sendbuf, recvbuf, count, datatype, op, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Neighbor_allgather_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_allgather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Neighbor_allgatherv_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint
                                  displs[], MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Info *
                                  info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_allgatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcounts, displs, recvtype, comm, info,
                                                   request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Neighbor_alltoall_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoall_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, comm, info, request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Neighbor_alltoallv_init(const void *sendbuf, const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                 const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                 MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Info * info,
                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoallv_init_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                  recvcounts, rdispls, recvtype, comm, info,
                                                  request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Neighbor_alltoallw_init(const void *sendbuf, const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                 void *recvbuf, const MPI_Aint recvcounts[],
                                 const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                 MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoallw_init_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                  recvcounts, rdispls, recvtypes, comm, info,
                                                  request);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_Request_set_type(*request, MPIDI_REQUEST_TYPE_PERSISTENT_COLL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

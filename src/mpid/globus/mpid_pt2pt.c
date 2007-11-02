/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


/*
 * MPID_Startall()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Startall
int MPID_Startall(int count, MPID_Request * requests[])
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int i;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_STARTALL);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: count=%d", count));

    for (i = 0; i < count; i++)
    {
	int rank;
	int tag;
	int ctx;
	void * buf;
	int cnt;
	MPI_Datatype dt;
	MPID_Request * const preq = requests[i];

	mpig_request_get_envelope(preq, &rank, &tag, &ctx);
	mpig_request_get_buffer(preq, &buf, &cnt, &dt);

	switch (mpig_request_get_type(preq))
	{
	    case MPIG_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
	    
	    case MPIG_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }

	    case MPIG_REQUEST_TYPE_BSEND:
	    {
		MPI_Request sreq_handle;
		
		MPIR_Nest_incr();
		{
		    rc = NMPI_Ibsend(buf, cnt, dt, rank, tag, preq->comm->handle, &sreq_handle);
		    if (rc == MPI_SUCCESS)
		    {
			MPID_Request_get_ptr(sreq_handle, preq->partner_request);
		    }
		}
		MPIR_Nest_decr();
		
		break;
	    }		

	    default:
	    {
		/* --BEGIN ERROR HANDLING-- */
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_INTERN, "**globus|badreqtype",
		    "**globus|badreqtype %d", mpig_request_get_type(preq));
		/* --END ERROR HANDLING-- */
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->status.MPI_ERROR = MPI_SUCCESS;
	    preq->cc_ptr = &preq->partner_request->cc;
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    /* If a failure occurs attempting to start the request, then we assume that partner request was not created, and stuff
	       the error code in the persistent request.  The wait and test routines will look at the error code in the persistent
	       request if a partner request is not present. */
	    preq->partner_request = NULL;
	    preq->status.MPI_ERROR = rc;
	    preq->cc_ptr = &preq->cc;
	    preq->cc = 0;
	    /* --END ERROR HANDLING-- */
	}
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
}
/* MPID_Startall() */


/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
int MPID_Recv_init(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RECV_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RECV_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_request_create_prreq(1, 0, buf, cnt, dt, rank, tag, ctx, comm, rreqp);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "persistent Recv request allocated, handle=" MPIG_HANDLE_FMT ", ptr=%p",
	(*rreqp)->handle, *rreqp));
    
  fn_return:    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), MPIG_PTR_CAST(*rreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Recv_init() */


/*
 * MPID_Send_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send_init
int MPID_Send_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
    MPID_Request ** sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SEND_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SEND_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
	", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_SEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "persistent Send request allocated, handle=" MPIG_HANDLE_FMT ", ptr=%p",
	(*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_SEND_INIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Send_init() */


/*
 * MPID_Rsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rsend_init
int MPID_Rsend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
    MPID_Request ** sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RSEND_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
	", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_RSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "persistent Rsend request allocated, handle=" MPIG_HANDLE_FMT ", ptr=%p",
	(*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_RSEND_INIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Rsend_init() */


/*
 * MPID_Ssend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
int MPID_Ssend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
    MPID_Request ** sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SSEND_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
	", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_SSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "persistent Ssend request allocated, handle="MPIG_HANDLE_FMT ", ptr=%p",
	(*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Ssend_init() */


/*
 * MPID_Bsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Bsend_init
int MPID_Bsend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
    MPID_Request ** sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_BSEND_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    MPIU_Assertp(ctxoff == 0 &&
	"persistent Bsend operations do not support context offsets other than zero");
    
    mpig_request_create_psreq(MPIG_REQUEST_TYPE_BSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "persistent Bsend request allocated, handle="MPIG_HANDLE_FMT ", ptr=%p",
	(*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_BSEND_INIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Bsend_init() */


/*
 * mpig_adi3_cancel_recv()
 */
#undef FUNCNAME
#define FUNCNAME mpig_adi3_cancel_recv
int mpig_adi3_cancel_recv(MPID_Request * rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t cancelled;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_adi3_cancel_recv);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_adi3_cancel_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

    mpi_errno = mpig_recvq_cancel_posted_rreq(rreq, &cancelled);
    if (cancelled)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "cancel receive request succeeded immediately: rreq=" MPIG_HANDLE_FMT
	    ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

	/* if the request was posted as a receive any source, then inform the PE that the op has completed */
	if (mpig_request_get_rank(rreq) == MPI_ANY_SOURCE)
	{
	    mpig_pe_end_ras_op();
	}

	rreq->status.cancelled = TRUE;
	rreq->status.count = 0;
	mpig_request_complete(rreq);
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "cancel receive request pending or failed: rreq=" MPIG_HANDLE_FMT
	    ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_adi3_cancel_recv);
    return mpi_errno;
}
/* mpig_adi3_cancel_recv() */

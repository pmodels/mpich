/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "../../../mpi/pt2pt/bsendutil.h"
/*
 * MPID_Startall()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Startall(int count, MPID_Request * requests[])
{
    int i;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_STARTALL);

    for (i = 0; i < count; i++)
    {
	MPID_Request * const preq = requests[i];

	switch (MPIDI_Request_get_type(preq))
	{
	    case MPIDI_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
	    
	    case MPIDI_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }

	    case MPIDI_REQUEST_TYPE_BSEND:
	    {
		MPID_Request * sreq;
		
		sreq = MPIDI_CH3_Request_create();
		if (sreq != NULL)
		{
		    MPIU_Object_set_ref(sreq, 1);
		    sreq->kind = MPID_REQUEST_SEND;
		    sreq->cc   = 0;
		    sreq->comm = preq->comm;
		    MPIR_Comm_add_ref(sreq->comm);
		    
		    rc = MPIR_Bsend_isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
					  preq->dev.match.tag, preq->comm, BSEND_INIT, &preq->partner_request);

		    sreq->status.MPI_ERROR = rc;
		    preq->partner_request = sreq;
		    rc = MPI_SUCCESS;
		}
		else
		{
		    rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		}
		
		break;
	    }

	    default:
	    {
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
					  "**ch3|badreqtype %d", MPIDI_Request_get_type(preq));
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->status.MPI_ERROR = MPI_SUCCESS;
	    preq->cc_ptr = &preq->partner_request->cc;
	}
	else
	{
	    /* If a failure occurs attempting to start the request, then we assume that partner request was not created, and stuff
	       the error code in the persistent request.  The wait and test routines will look at the error code in the persistent
	       request if a partner request is not present. */
	    preq->partner_request = NULL;
	    preq->status.MPI_ERROR = rc;
	    preq->cc_ptr = &preq->cc;
	    preq->cc = 0;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
}

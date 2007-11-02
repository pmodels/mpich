/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Recv_init(void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
    MPID_Request * rreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RECV_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RECV_INIT);
    
    rreq = MPIDI_CH3_Request_create();
    if (rreq == NULL)
    {
	goto fn_exit;
    }
    
    MPIU_Object_set_ref(rreq, 1);
    rreq->kind = MPID_PREQUEST_RECV;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    rreq->dev.match.rank = rank;
    rreq->dev.match.tag = tag;
    rreq->dev.match.context_id = comm->context_id + context_offset;
    rreq->dev.user_buf = (void *) buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;
    rreq->partner_request = NULL;
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_RECV);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
    }
	
    *request = rreq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Ssend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Ssend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SSEND_INIT);

    MPIDI_CH3M_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;
}

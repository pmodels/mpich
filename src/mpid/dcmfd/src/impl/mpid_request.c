/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/impl/mpid_request.c
 * \brief Accessors and actors for MPID Requests
 */
#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

/**
 * \defgroup MPID_REQUEST MPID Request object management
 *
 * Accessors and actors for MPID Requests
 */


/* these are referenced by src/mpi/pt2pt/wait.c in PMPI_Wait! */
MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem =
  {
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request),
    MPID_Request_direct,
    MPID_REQUEST_PREALLOC
  };


/**
 * \brief Create and initialize a new request
 */

MPID_Request * MPID_Request_create()
{
  MPID_Request * req;

  req = MPIU_Handle_obj_alloc(&MPID_Request_mem);
  if (req == NULL)
    MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1, "Cannot allocate Request");

  MPID_assert (HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);
  MPIU_Object_set_ref(req, 1);
  req->cc                = 1;
  req->cc_ptr            = & req->cc;
  req->status.MPI_SOURCE = MPI_UNDEFINED;
  req->status.MPI_TAG    = MPI_UNDEFINED;
  req->status.MPI_ERROR  = MPI_SUCCESS;
  req->status.count      = 0;
  req->status.cancelled  = FALSE;
  req->comm              = NULL;

  struct MPIDI_DCMF_Request* dcmf = &req->dcmf;
  if (DCQuad_sizeof(MPIDI_DCMF_MsgInfo) == 1)
    {
      DCQuad* q = dcmf->envelope.envelope.msginfo.quad;
      q->w0 = 0;
      q->w1 = 0;
      q->w2 = 0;
      q->w3 = 0;
    }
  else
    memset(&dcmf->envelope.envelope.msginfo, 0x00, sizeof(MPIDI_DCMF_MsgInfo));

  dcmf->envelope.envelope.offset = 0;
  dcmf->envelope.envelope.length = 0;
  dcmf->userbuf                  = NULL;
  dcmf->uebuf                    = NULL;
  dcmf->datatype_ptr             = NULL;
  dcmf->cancel_pending           = FALSE;
  dcmf->state                    = MPIDI_DCMF_INITIALIZED;
  MPID_Request_setCA  (req, MPIDI_DCMF_CA_COMPLETE);
  MPID_Request_setSelf(req, 0);
  MPID_Request_setType(req, MPIDI_DCMF_REQUEST_TYPE_RECV);

  return req;
}

static inline void MPIDI_Request_try_free(MPID_Request *req)
{
  if ( (req->ref_count == 0) && (MPID_Request_get_cc(req) == 0) )
    {
      if (req->comm)              MPIR_Comm_release(req->comm, 0);
      if (req->dcmf.datatype_ptr) MPID_Datatype_release(req->dcmf.datatype_ptr);
      MPIU_Handle_obj_free(&MPID_Request_mem, req);
    }
}


/* *********************************************************************** */
/*           destroy a request                                             */
/* *********************************************************************** */

void MPID_Request_release (MPID_Request *req)
{
  int ref_count;
  MPID_assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);
  MPIU_Object_release_ref(req, &ref_count);
  MPID_assert(req->ref_count >= 0);
  MPIDI_Request_try_free(req);
}

/* *********************************************************************** */
/*            Dealing with completion counts                               */
/* *********************************************************************** */

void MPID_Request_complete (MPID_Request *req)
{
  int cc;
  MPID_Request_decrement_cc(req, &cc);
  MPID_assert(cc >= 0);
  if (cc == 0) /* decrement completion count; if 0, signal progress engine */
    {
      MPIDI_Request_try_free(req);
      MPID_Progress_signal();
    }
}

void MPID_Request_set_completed (MPID_Request *req)
{
  *(req)->cc_ptr = 0; /* force completion count to 0 */
  MPIDI_Request_try_free(req);
  MPID_Progress_signal();
}

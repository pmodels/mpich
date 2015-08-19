/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/pt2pt/mpid_cancel.c
 * \brief Device interface for canceling an MPI Recv
 */
#include <mpidimpl.h>


int
MPID_Cancel_recv(MPID_Request * rreq)
{
  MPID_assert(rreq->kind == MPID_REQUEST_RECV);
  if (MPIDI_Recvq_FDPR(rreq))
    {
      MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
      MPIR_STATUS_SET_COUNT(rreq->status, 0);
      MPIDI_Request_complete(rreq);
    }
  /* This is successful, even if the recv isn't cancelled */
  return MPI_SUCCESS;
}


/**
 * \brief Cancel an MPI_Isend()
 *
 * \param[in] req The request element to cancel
 *
 * \return The same as MPIDI_CtrlSend()
 */
static inline pami_result_t
MPIDI_CancelReq_post(pami_context_t context, void * _req)
{
  MPID_Request * req = (MPID_Request*)_req;
  MPID_assert(req != NULL);

  /* ------------------------------------------------- */
  /* Check if we already have a cancel request pending */
  /* ------------------------------------------------- */
  if (MPIDI_Request_cancel_pending(req))
    {
      MPIDI_Request_complete(req);
      return PAMI_SUCCESS;
    }

  MPIDI_MsgInfo cancel = {
    .MPItag  = MPIDI_Request_getMatchTag(req),
    .MPIrank = MPIDI_Request_getMatchRank(req),
    .MPIctxt = MPIDI_Request_getMatchCtxt(req),
    .req     = req->handle,
  };
  cancel.control = MPIDI_CONTROL_CANCEL_REQUEST;

  pami_endpoint_t dest;
  MPIDI_Context_endpoint(req, &dest);

  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_Cancel,
    .dest     = dest,
    .header   = {
      .iov_base= &cancel,
      .iov_len= sizeof(MPIDI_MsgInfo),
    },
  };

  TRACE_ERR("Running posted cancel for request=%p  local=%u  remote=%u\n",
            req, MPIR_Process.comm_world->rank, MPIDI_Request_getPeerRank_pami(req));
  pami_result_t rc;
  rc = PAMI_Send_immediate(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);

  return PAMI_SUCCESS;
}


int
MPID_Cancel_send(MPID_Request * sreq)
{
  MPID_assert(sreq != NULL);

  if(!sreq->comm)
    return MPI_SUCCESS;

  MPIDI_Request_uncomplete(sreq);
  /* TRACE_ERR("Posting cancel for request=%p   cc(curr)=%d ref(curr)=%d\n", sreq, val+1, MPIU_Object_get_ref(sreq)); */

  pami_context_t context = MPIDI_Context_local(sreq);

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  if (likely(MPIDI_Process.perobj.context_post.active > 0))
    {
      /* This leaks intentionally.  At this time, the amount of work
       * required to avoid a leak here just isn't worth it.
       * Hopefully people aren't cancelling sends too much.
       */
      pami_work_t  * work    = MPIU_Malloc(sizeof(pami_work_t));
      PAMI_Context_post(context, work, MPIDI_CancelReq_post, sreq);
    }
  else
    {
      /* Lock access to the context. For more information see discussion of the
       * simplifying assumption that the "per object" mpich lock mode does not
       * expect a completely single threaded run environment in the file
       * src/mpid/pamid/src/mpid_progress.h
       */
       PAMI_Context_lock(context);
       MPIDI_CancelReq_post(context, sreq);
       PAMI_Context_unlock(context);
    }
#else /* (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT) */
  /*
   * It is not necessary to lock the context before access in the "global" mpich
   * lock mode because all application threads must first acquire the global
   * mpich lock upon entry into the library, and any active async threads must
   * first acquire the global mpich lock before accessing internal mpich data
   * structures or accessing pami objects. This makes the context lock redundant.
   */
  MPIDI_CancelReq_post(context, sreq);
#endif

  return MPI_SUCCESS;
}

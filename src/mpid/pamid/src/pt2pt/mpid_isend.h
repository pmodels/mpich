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
 * \file src/pt2pt/mpid_isend.h
 * \brief ???
 */

#ifndef __src_pt2pt_mpid_isend_h__
#define __src_pt2pt_mpid_isend_h__


#define MPID_Isend          MPID_Isend_inline


static inline unsigned
MPIDI_Context_hash(pami_task_t rank, unsigned ctxt, unsigned bias, unsigned ncontexts)
{
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  return (( rank + ctxt + bias ) & (ncontexts-1));
#else
  /* The 'global' mpich lock mode only supports a single context. See
   * discussion in mpich/src/mpid/pamid/src/mpid_init.c for more information.
   */
  return 0;
#endif
}
static inline void
MPIDI_Context_endpoint(MPID_Request * req, pami_endpoint_t * e)
{
  pami_task_t remote = MPIDI_Request_getPeerRank_pami(req);
  pami_task_t local  = MPIR_Process.comm_world->rank;
  unsigned    rctxt  = MPIDI_Context_hash(local, req->comm->context_id, MPIDI_Process.avail_contexts>>1, MPIDI_Process.avail_contexts);

  pami_result_t rc;
  rc = PAMI_Endpoint_create(MPIDI_Client, remote, rctxt, e);
  MPID_assert(rc == PAMI_SUCCESS);
}
static inline pami_context_t
MPIDI_Context_local(MPID_Request * req)
{
  pami_task_t remote = MPIDI_Request_getPeerRank_comm(req);
  unsigned    lctxt  = MPIDI_Context_hash(remote, req->comm->context_id, 0, MPIDI_Process.avail_contexts);
  MPID_assert(lctxt < MPIDI_Process.avail_contexts);
  return MPIDI_Context[lctxt];
}


/**
 * \brief ADI level implemenation of MPI_Isend()
 *
 * \param[in]  buf            The buffer to send
 * \param[in]  count          Number of elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The destination rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 *
 * This is a slight variation on mpid_send.c - basically, we *always*
 * want to return a send request even if the request is already
 * complete (as is in the case of sending to a NULL rank).
 */
static inline int
MPID_Isend_inline(const void    * buf,
                  MPI_Aint        count,
                  MPI_Datatype    datatype,
                  int             rank,
                  int             tag,
                  MPID_Comm     * comm,
                  int             context_offset,
                  MPID_Request ** request)
{
  /* ---------------------------------------------------- */
  /* special case: PROC null handled by handoff function  */
  /* ---------------------------------------------------- */

  /* --------------------- */
  /* create a send request */
  /* --------------------- */
  MPID_Request * sreq = *request = MPIDI_Request_create2_fast();

  unsigned context_id = comm->context_id;
  /* match info */
  MPIDI_Request_setMatch(sreq, tag, comm->rank, context_id+context_offset);

  /* data buffer info */
  sreq->mpid.userbuf        = (void*)buf;
  sreq->mpid.userbufcount   = count;
  sreq->mpid.datatype       = datatype;

  /* Enable passing in MPI_PROC_NULL, do the translation in the
     handoff function */
  MPIDI_Request_setPeerRank_comm(sreq, rank);

  unsigned ncontexts = MPIDI_Process.avail_contexts;
  /* communicator & destination info */
  sreq->comm = comm;
  sreq->kind = MPID_REQUEST_SEND;
  MPIR_Comm_add_ref(comm);

  pami_context_t context = MPIDI_Context[MPIDI_Context_hash(rank, context_id, 0, ncontexts)];
  if (context_offset == 0)
    MPIDI_Context_post(context, &sreq->mpid.post_request, MPIDI_Isend_handoff, sreq);
  else
    MPIDI_Context_post(context, &sreq->mpid.post_request, MPIDI_Isend_handoff_internal, sreq);

  return MPI_SUCCESS;
}


#endif

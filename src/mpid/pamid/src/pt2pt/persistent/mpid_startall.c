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
 * \file src/pt2pt/persistent/mpid_startall.c
 * \brief ???
 */
#include <mpidimpl.h>
#include <../mpi/pt2pt/bsendutil.h>

int MPID_Startall(int count, MPID_Request * requests[])
{
  int rc=MPI_SUCCESS, i;
  for (i = 0; i < count; i++)
    {
      MPID_Request * const preq = requests[i];
      switch(MPIDI_Request_getPType(preq))
        {
        case MPIDI_REQUEST_PTYPE_RECV:
          {
            rc = MPID_Irecv(preq->mpid.userbuf,
                            preq->mpid.userbufcount,
                            preq->mpid.datatype,
                            MPIDI_Request_getMatchRank(preq),
                            MPIDI_Request_getMatchTag(preq),
                            preq->comm,
                            MPIDI_Request_getMatchCtxt(preq) - preq->comm->recvcontext_id,
                            &preq->partner_request);
            break;
          }
        case MPIDI_REQUEST_PTYPE_SEND:
          {
            rc = MPID_Isend(preq->mpid.userbuf,
                            preq->mpid.userbufcount,
                            preq->mpid.datatype,
                            MPIDI_Request_getMatchRank(preq),
                            MPIDI_Request_getMatchTag(preq),
                            preq->comm,
                            MPIDI_Request_getMatchCtxt(preq) - preq->comm->context_id,
                            &preq->partner_request);
            break;
          }
        case MPIDI_REQUEST_PTYPE_SSEND:
          {
            rc = MPID_Issend(preq->mpid.userbuf,
                             preq->mpid.userbufcount,
                             preq->mpid.datatype,
                             MPIDI_Request_getMatchRank(preq),
                             MPIDI_Request_getMatchTag(preq),
                             preq->comm,
                             MPIDI_Request_getMatchCtxt(preq) - preq->comm->context_id,
                             &preq->partner_request);
            break;
          }
        case MPIDI_REQUEST_PTYPE_BSEND:
          {
            rc = MPIR_Bsend_isend(preq->mpid.userbuf,
                                  preq->mpid.userbufcount,
                                  preq->mpid.datatype,
                                  MPIDI_Request_getMatchRank(preq),
                                  MPIDI_Request_getMatchTag(preq),
                                  preq->comm,
                                  BSEND_INIT,
                                  &preq->partner_request);
            /*
             * MPICH maintains an independant reference to the child,
             * but doesn't refcount it.  Since they actually call
             * MPI_Test() on the child request (which will release a
             * ref iff the request is complete), we have to increment
             * the ref_count so that it doesn't get freed from under
             * us.
             */
            if (preq->partner_request != NULL)
              MPIU_Object_add_ref(preq->partner_request);
            break;
          }

        default:
          {
            rc = MPIR_Err_create_code(MPI_SUCCESS,
                                      MPIR_ERR_FATAL,
                                      __FUNCTION__,
                                      __LINE__,
                                      MPI_ERR_INTERN,
                                      "**ch3|badreqtype",
                                      "**ch3|badreqtype %d",
                                      MPIDI_Request_getPType(preq));
          }

        } /* switch should end here, bug fixed. */

      if (rc == MPI_SUCCESS)
      {
        preq->status.MPI_ERROR = MPI_SUCCESS;
        if (MPIDI_Request_getPType(preq) == MPIDI_REQUEST_PTYPE_BSEND)
          {
            /*
             * Complete a persistent Bsend immediately.
             *
             * Because the child of a persistent Bsend is just a
             * normal Isend on a temp buffer, we don't need to wait on
             * the child when the user calls MPI_Wait on the parent.
             * Therefore, disconnect the cc_ptr link to the child and
             * mark the parent complete.
             */
            preq->cc_ptr = &preq->cc;
            MPID_Request_set_completed(preq);
          }
        else
          preq->cc_ptr = &preq->partner_request->cc;
      }
      else
      {
        /* If a failure occurs attempting to start the request,
          then we assume that partner request was not created,
          and stuff the error code in the persistent request.
          The wait and test routines will look at the error code
          in the persistent request if a partner request is not present. */
        preq->partner_request = NULL;
        preq->status.MPI_ERROR = rc;
        preq->cc_ptr = &preq->cc;
        MPID_Request_set_completed(preq);
      }
  } /* for */
  return rc;
}

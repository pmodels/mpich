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
 * \file src/pt2pt/mpidi_recv.h
 * \brief ADI level implemenation of MPI_Irecv()
 */

#ifndef __src_pt2pt_mpidi_recv_h__
#define __src_pt2pt_mpidi_recv_h__

#include <mpidimpl.h>
#include "../mpid_recvq.h"
#include "mpid_datatype.h"
/*#ifdef MPIDI_STATISTICS
  #include "../../include/mpidi_datatypes.h"
#endif*/


/**
 * \brief ADI level implemenation of MPI_(I)Recv()
 *
 * \param[in]  buf            The buffer to receive into
 * \param[in]  count          Number of expected elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The sending rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 * \param[out] status         Update the status structure
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 */
static inline int
MPIDI_Recv(void          * buf,
           int             count,
           MPI_Datatype    datatype,
           int             rank,
           int             tag,
           MPID_Comm     * comm,
           int             context_offset,
           unsigned        is_blocking,
           MPI_Status    * status,
           MPID_Request ** request)
{
  MPID_Request * rreq;
  int found;
  int mpi_errno = MPI_SUCCESS;

  /* ---------------------------------------- */
  /* NULL rank means empty request            */
  /* ---------------------------------------- */
  if (unlikely(rank == MPI_PROC_NULL))
    {
      MPIDI_RecvMsg_procnull(comm, is_blocking, status, request);
      return MPI_SUCCESS;
    }
#if (MPIDI_STATISTICS)
  MPID_NSTAT(mpid_statp->recvs);
#endif
  MPIR_Comm_add_ref(comm);

  /* ---------------------------------------- */
  /* find our request in the unexpected queue */
  /* or allocate one in the posted queue      */
  /* ---------------------------------------- */
  MPID_Request *newreq = MPIDI_Request_create2();
  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_FDU_or_AEP(newreq, rank,
                                tag,
                                comm->recvcontext_id + context_offset,
                                &found);
#ifdef MPIDI_TRACE
{
  size_t ll;
  ll = count * MPID_Datatype_get_basic_size(datatype);
  SET_REC_PR(rreq,buf,count,ll,datatype,pami_source,rank,tag,comm,is_blocking);
}
#endif
#else
  int pami_source;
  if(rank != MPI_ANY_SOURCE) {
    pami_source = MPID_VCR_GET_LPID(comm->vcr, rank);
  } else {
    pami_source = MPI_ANY_SOURCE;
  }
  if ((pami_source != MPI_ANY_SOURCE) && (MPIDI_In_cntr[pami_source].n_OutOfOrderMsgs>0))  {
        /* returns unlock    */
        MPIDI_Recvq_process_out_of_order_msgs(pami_source, MPIDI_Context[0]);
  }
  rreq = MPIDI_Recvq_FDU_or_AEP(newreq, rank,
                                pami_source,
                                tag,
                                comm->recvcontext_id + context_offset,
                                &found);
#endif

  /* ----------------------------------------------------------------- */
  /* populate request with our data                                    */
  /* We can do this because this is not a multithreaded implementation */
  /* ----------------------------------------------------------------- */

  rreq->comm              = comm;
  rreq->mpid.userbuf      = buf;
  rreq->mpid.userbufcount = count;
  rreq->mpid.datatype     = datatype;
  /* We don't need this because MPIDI_CA_COMPLETE is the initialized default */
  /* MPIDI_Request_setCA(rreq, MPIDI_CA_COMPLETE); */

  if (unlikely(found))
    {
      MPIDI_RecvMsg_Unexp(rreq, buf, count, datatype);
      mpi_errno = rreq->status.MPI_ERROR;
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      MPID_Request_discard(newreq);
    }
  else
    {
      /* ----------------------------------------------------------- */
      /* request not found in unexpected queue, allocated and posted */
      /* ----------------------------------------------------------- */
      if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
        {
          MPID_Datatype_get_ptr(datatype, rreq->mpid.datatype_ptr);
          MPID_Datatype_add_ref(rreq->mpid.datatype_ptr);
        }
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }

  /* mutex has been dropped... */
  *request = rreq;
  if (status != MPI_STATUS_IGNORE)
    *status = rreq->status;
  #ifdef MPIDI_STATISTICS
    if (!(MPID_cc_is_complete(&rreq->cc))) {
        MPID_NSTAT(mpid_statp->recvWaitsComplete);
    }
  #endif

  return mpi_errno;
}

#endif

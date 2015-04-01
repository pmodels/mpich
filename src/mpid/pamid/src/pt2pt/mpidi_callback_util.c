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
 * \file src/pt2pt/mpidi_callback_util.c
 * \brief Utility functions to help uncommon cases in the new-message callbacks
 */
#include <mpidimpl.h>


/* MSGQUEUE lock must be held by caller */
void
MPIDI_Callback_process_unexp(MPID_Request *newreq,
			     pami_context_t        context,
                             const MPIDI_MsgInfo * msginfo,
                             size_t                sndlen,
                             pami_endpoint_t       sender,
                             const void          * sndbuf,
                             pami_recv_t         * recv,
                             unsigned              isSync)
{
  MPID_Request *rreq = NULL;

  /* ---------------------------------------------------- */
  /*  Fallback position:                                  */
  /*     + Request was not posted, or                     */
  /*     + Request was long & not contiguous.             */
  /*  We must allocate enough space to hold the message.  */
  /*  The temporary buffer will be unpacked later.        */
  /* ---------------------------------------------------- */
  unsigned rank       = msginfo->MPIrank;
  unsigned tag        = msginfo->MPItag;
  unsigned context_id = msginfo->MPIctxt;
#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_AEU(newreq, rank, tag, context_id);
#else
  unsigned msg_seqno  = msginfo->MPIseqno;
  rreq = MPIDI_Recvq_AEU(newreq, rank, PAMIX_Endpoint_query(sender), tag, context_id, msg_seqno);
#endif
  /* ---------------------- */
  /*  Copy in information.  */
  /* ---------------------- */
  rreq->status.MPI_SOURCE = rank;
  rreq->status.MPI_TAG    = tag;
  MPIR_STATUS_SET_COUNT(rreq->status, sndlen);
  MPIDI_Request_setCA          (rreq, MPIDI_CA_COMPLETE);
  MPIDI_Request_cpyPeerRequestH(rreq, msginfo);
  MPIDI_Request_setSync        (rreq, isSync);

  /* Set the rank of the sender if a sync msg. */
#ifndef OUT_OF_ORDER_HANDLING
  if (isSync)
    {
#endif
      MPIDI_Request_setPeerRank_comm(rreq, rank);
      MPIDI_Request_setPeerRank_pami(rreq, PAMIX_Endpoint_query(sender));
#ifndef OUT_OF_ORDER_HANDLING
    }
#endif

  MPID_assert(!sndlen || rreq->mpid.uebuf != NULL);
  TRACE_MEMSET_R(PAMIX_Endpoint_query(sender),msg_seqno,recv_status);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),msgid,msginfo->MPIseqno);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),rtag,tag);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),rctx,msginfo->MPIctxt);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),rlen,sndlen);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),fl.f.sync,isSync);
  TRACE_SET_R_VAL(PAMIX_Endpoint_query(sender),(msginfo->MPIseqno & SEQMASK),rsource,PAMIX_Endpoint_query(sender));
  TRACE_SET_REQ_VAL(rreq->mpid.idx,(msginfo->MPIseqno & SEQMASK));

  if (recv != NULL)
    {
      recv->local_fn = MPIDI_RecvDoneCB_mutexed;
      recv->cookie   = rreq;
      /* -------------------------------------------------- */
      /*  Let PAMI know where to put the rest of the data.  */
      /* -------------------------------------------------- */
      recv->addr = rreq->mpid.uebuf;
    }
  else
    {
      /* ------------------------------------------------- */
      /*  We have the data; copy it and complete the msg.  */
      /* ------------------------------------------------- */
      memcpy(rreq->mpid.uebuf, sndbuf,   sndlen);
      MPIDI_RecvDoneCB(context, rreq, PAMI_SUCCESS);
      /* caller must release rreq, after unlocking MSGQUEUE */
    }
}


/* MSGQUEUE lock is not held */
void
MPIDI_Callback_process_trunc(pami_context_t  context,
                             MPID_Request   *rreq,
                             pami_recv_t    *recv,
                             const void     *sndbuf)
{
  rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

  /* -------------------------------------------------------------- */
  /*  The data is already available, so we can just unpack it now.  */
  /* -------------------------------------------------------------- */
  if (recv)
    {
      MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
      rreq->mpid.uebuflen = MPIR_STATUS_GET_COUNT(rreq->status);
      rreq->mpid.uebuf    = MPIU_Malloc(MPIR_STATUS_GET_COUNT(rreq->status));
      MPID_assert(rreq->mpid.uebuf != NULL);
      rreq->mpid.uebuf_malloc = mpiuMalloc;

      recv->addr = rreq->mpid.uebuf;
    }
  else
    {
      MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
      rreq->mpid.uebuflen = MPIR_STATUS_GET_COUNT(rreq->status);
      rreq->mpid.uebuf    = (void*)sndbuf;
      MPIDI_RecvDoneCB(context, rreq, PAMI_SUCCESS);
      MPID_Request_release(rreq);
    }
}


/* MSGQUEUE lock is not held */
void
MPIDI_Callback_process_userdefined_dt(pami_context_t      context,
                                      const void        * sndbuf,
                                      size_t              sndlen,
                                      MPID_Request      * rreq)
{
  unsigned dt_contig, dt_size;
  MPID_Datatype *dt_ptr;
  MPI_Aint dt_true_lb;
  MPIDI_Datatype_get_info(rreq->mpid.userbufcount,
                          rreq->mpid.datatype,
                          dt_contig,
                          dt_size,
                          dt_ptr,
                          dt_true_lb);

  /* ----------------------------- */
  /*  Test for truncated message.  */
  /* ----------------------------- */
  if (unlikely(sndlen > dt_size))
    {
#if ASSERT_LEVEL > 0
      MPIDI_Callback_process_trunc(context, rreq, NULL, sndbuf);
      return;
#else
      sndlen = dt_size;
#endif
    }

  /*
   * This is to test that the fields don't need to be
   * initialized.  Remove after this doesn't fail for a while.
   */
  if (likely (dt_contig))
    {
      MPID_assert(rreq->mpid.uebuf    == NULL);
      MPID_assert(rreq->mpid.uebuflen == 0);
      void* rcvbuf = rreq->mpid.userbuf +  dt_true_lb;;
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on && MPIDI_cuda_is_device_buf(rcvbuf))
    {
      cudaError_t cudaerr = CudaMemcpy(rcvbuf, sndbuf, (size_t)sndlen, cudaMemcpyHostToDevice);
    }
    else
#endif
      memcpy(rcvbuf, sndbuf, sndlen);
      MPIDI_Request_complete(rreq);
      return;
    }

  MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
  rreq->mpid.uebuflen = sndlen;
  rreq->mpid.uebuf    = (void*)sndbuf;
  MPIDI_RecvDoneCB(context, rreq, PAMI_SUCCESS);
  MPID_Request_release(rreq);
}

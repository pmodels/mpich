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
 * \file src/pt2pt/mpidi_rendezvous.c
 * \brief Provide for a flow-control rendezvous-based send
 */
#include <mpidimpl.h>

inline void
MPIDI_RendezvousTransfer_use_pami_rget(pami_context_t   context,
                                       pami_endpoint_t  dest,
                                       MPID_Request     *rreq)
__attribute__((__always_inline__));
#ifdef RDMA_FAILOVER
inline void
MPIDI_RendezvousTransfer_use_pami_get(pami_context_t   context,
                                      pami_endpoint_t  dest,
	                              void             *rcvbuf,
                                      MPID_Request     *rreq)
__attribute__((__always_inline__));
#endif


pami_result_t
MPIDI_RendezvousTransfer(pami_context_t   context,
                         void           * _rreq)
{
  MPID_Request * rreq = (MPID_Request*) _rreq;

  void *rcvbuf;
  size_t rcvlen;

  /* -------------------------------------- */
  /* calculate message length for reception */
  /* calculate receive message "count"      */
  /* -------------------------------------- */
  unsigned dt_contig;
  size_t dt_size;
  MPID_Datatype *dt_ptr;
  MPI_Aint dt_true_lb;
  MPIDI_Datatype_get_info(rreq->mpid.userbufcount,
                          rreq->mpid.datatype,
                          dt_contig,
                          dt_size,
                          dt_ptr,
                          dt_true_lb);

  /* -------------------------------------- */
  /* test for truncated message.            */
  /* -------------------------------------- */
  if (rreq->mpid.envelope.length > dt_size)
    {
      rcvlen = dt_size;
      rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
      MPIR_STATUS_SET_COUNT(rreq->status,  rcvlen);
    }
  else
    {
      rcvlen = rreq->mpid.envelope.length;
    }

  /* -------------------------------------- */
  /* if buffer is contiguous ...            */
  /* -------------------------------------- */
  if (dt_contig)
    {
      MPIDI_Request_setCA(rreq, MPIDI_CA_COMPLETE);
      rreq->mpid.uebuf = NULL;
      rreq->mpid.uebuflen = 0;
      rcvbuf = rreq->mpid.userbuf + dt_true_lb;
    }

  /* --------------------------------------------- */
  /* buffer is non-contiguous. we need to allocate */
  /* a temporary buffer, and unpack later.         */
  /* --------------------------------------------- */
  else
    {
      MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
      rcvbuf = MPIU_Malloc(rcvlen);
      MPID_assert(rcvbuf != NULL);
      rreq->mpid.uebuf    = rcvbuf;
      rreq->mpid.uebuflen = rcvlen;
      rreq->mpid.uebuf_malloc = mpiuMalloc;
    }

  /* ---------------------------------------------------------------- */
  /* Get the data from the origin node.                               */
  /* ---------------------------------------------------------------- */

  pami_result_t rc;
  pami_endpoint_t dest;
  MPIDI_Context_endpoint(rreq, &dest);

#if CUDA_AWARE_SUPPORT
  if(MPIDI_Process.cuda_aware_support_on && MPIDI_cuda_is_device_buf(rcvbuf))
  {
    MPIDI_RendezvousTransfer_use_pami_get(context,dest,rcvbuf,rreq);
  }
  else
  {
#endif


#ifdef USE_PAMI_RDMA
  size_t rcvlen_out;
  rc = PAMI_Memregion_create(context,
			     rcvbuf,
			     rcvlen,
			     &rcvlen_out,
			     &rreq->mpid.memregion);
  MPID_assert(rc == PAMI_SUCCESS);
  MPID_assert(rcvlen == rcvlen_out);

  TRACE_ERR("RZV Xfer for req=%p addr=%p *addr[0]=%#016llx *addr[1]=%#016llx\n",
	    rreq,
	    rcvbuf,
	    *(((unsigned long long*)rcvbuf)+0),
	    *(((unsigned long long*)rcvbuf)+1));

  MPIDI_RendezvousTransfer_use_pami_rget(context,dest,rreq);
#else
  rreq->mpid.memregion_used=0;
  if( (!MPIDI_Process.mp_s_use_pami_get) && (rreq->mpid.envelope.memregion_used) )
    {
      size_t rcvlen_out;
      rc = PAMI_Memregion_create(context,
				 rcvbuf,
				 rcvlen,
				 &rcvlen_out,
				 &rreq->mpid.memregion);
      if (rc == PAMI_SUCCESS)
	{
	  rreq->mpid.memregion_used=1;
	  MPID_assert(rcvlen == rcvlen_out);

	  TRACE_ERR("RZV Xfer for req=%p addr=%p *addr[0]=%#016llx *addr[1]=%#016llx\n",
		    rreq,
		    rcvbuf,
		    *(((unsigned long long*)rcvbuf)+0),
		    *(((unsigned long long*)rcvbuf)+1));
          MPIDI_RendezvousTransfer_use_pami_rget(context,dest,rreq);
	} else {
          MPIDI_RendezvousTransfer_use_pami_get(context,dest,rcvbuf,rreq);
	}
    } else {
      MPIDI_RendezvousTransfer_use_pami_get(context,dest,rcvbuf,rreq);
    }
#endif

#if CUDA_AWARE_SUPPORT
  }
#endif

  return PAMI_SUCCESS;
}


inline void
MPIDI_RendezvousTransfer_use_pami_rget(pami_context_t   context,
                                       pami_endpoint_t  dest,
                                       MPID_Request     *rreq)
{
  pami_result_t rc;
  pami_rget_simple_t params = {
    .rma  = {
      .dest    = dest,
      .hints   = {
	.buffer_registered     = PAMI_HINT_ENABLE,
	.use_rdma              = PAMI_HINT_ENABLE,
        .remote_async_progress = PAMI_HINT_DEFAULT, 
        .use_shmem             = PAMI_HINT_DEFAULT,
      },
      .bytes   = rreq->mpid.envelope.length,
      .cookie  = rreq,
      .done_fn = MPIDI_RecvRzvDoneCB,
    },
    .rdma = {
      .local  = {
	.mr     = &rreq->mpid.memregion,
	.offset = 0,
      },
      .remote = {
	.mr     = &rreq->mpid.envelope.memregion,
	.offset = 0,
      },
    },
  };

  rc = PAMI_Rget(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
}


#ifdef RDMA_FAILOVER
inline void
MPIDI_RendezvousTransfer_use_pami_get(pami_context_t   context,
                                      pami_endpoint_t  dest,
	                              void             *rcvbuf,
	                              MPID_Request     *rreq)
{
  pami_result_t rc;
  int val=0;

  if (MPIDI_Process.mp_s_use_pami_get) 
      val=PAMI_HINT_DEFAULT;
  else 
      val=PAMI_HINT_DISABLE;
  pami_get_simple_t params = {
    .rma  = {
      .dest    = dest,
      .hints   = {
	.buffer_registered     = PAMI_HINT_DEFAULT,
	.use_rdma              = val, 
        .remote_async_progress = PAMI_HINT_DEFAULT, 
        .use_shmem             = PAMI_HINT_DEFAULT,
#ifndef OUT_OF_ORDER_HANDLING
	.no_long_header= 1,
#endif
      },
      .bytes   = rreq->mpid.envelope.length,
      .cookie  = rreq,
      .done_fn = MPIDI_RecvRzvDoneCB,
    },
    .addr = {
      .local   = rcvbuf,
      .remote  = rreq->mpid.envelope.data,
    },
  };

  rc = PAMI_Get(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
}
#endif

pami_result_t MPIDI_RendezvousTransfer_SyncAck (pami_context_t context, void * _rreq)
{
  MPID_Request *rreq = (MPID_Request*)_rreq;

  // Do the sync ack transfer here.
  MPIDI_SyncAck_post (context, rreq, MPIDI_Request_getPeerRank_pami(rreq));

  // Continue on to the rendezvous transfer part.
  return MPIDI_RendezvousTransfer(context, _rreq);
}

pami_result_t MPIDI_RendezvousTransfer_zerobyte (pami_context_t context, void * _rreq)
{
  MPIDI_RecvRzvDoneCB_zerobyte (context, _rreq, PAMI_SUCCESS);
  return PAMI_SUCCESS;
}

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
 * \file src/pt2pt/mpidi_sendmsg.c
 * \brief Funnel point for starting all MPI messages
 */
#include <mpidimpl.h>
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if TOKEN_FLOW_CONTROL
#define MPIDI_Piggy_back_tokens    MPIDI_Piggy_back_tokens_inline
static inline void *
MPIDI_Piggy_back_tokens_inline(int dest,MPID_Request *shd,size_t len)
  {
         int rettoks=0;
         if (MPIDI_Token_cntr[dest].rettoks)
         {
             rettoks=min(MPIDI_Token_cntr[dest].rettoks, TOKENS_BITMASK);
             MPIDI_Token_cntr[dest].rettoks -= rettoks;
             shd->mpid.envelope.msginfo.tokens = rettoks;
          }
  }
#endif

static inline void
MPIDI_SendMsg_short(pami_context_t    context,
                    MPID_Request    * sreq,
                    pami_endpoint_t   dest,
                    void            * sndbuf,
                    unsigned          sndlen,
                    unsigned          isSync)
{
  MPIDI_MsgInfo * msginfo = &sreq->mpid.envelope.msginfo;

  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_Short,
    .dest     = dest,
    .header   = {
      .iov_base = msginfo,
      .iov_len  = sizeof(MPIDI_MsgInfo),
    },
    .data     = {
      .iov_base = sndbuf,
      .iov_len  = sndlen,
    },
  };
  if (isSync)
    params.dispatch = MPIDI_Protocols_ShortSync;

  pami_result_t rc;
  rc = PAMI_Send_immediate(context, &params);
#ifdef TRACE_ON
  if (rc)
    {
      TRACE_ERR("sizeof(msginfo)=%zu sizeof(data)=%u\n", sizeof(MPIDI_MsgInfo), sndlen);
    }
#endif
  MPID_assert(rc == PAMI_SUCCESS);
 TRACE_SET_S_VAL(dest,(sreq->mpid.envelope.msginfo.MPIseqno & SEQMASK),mode,params.dispatch);
 if (!isSync) {
     TRACE_SET_S_BIT(dest,(sreq->mpid.envelope.msginfo.MPIseqno & SEQMASK),fl.f.NoComp);
     TRACE_SET_S_BIT(dest,(sreq->mpid.envelope.msginfo.MPIseqno & SEQMASK),fl.f.sendShort);
 } else {
     TRACE_SET_S_BIT(dest,(sreq->mpid.envelope.msginfo.MPIseqno & SEQMASK),fl.f.sendEnvelop);
 }

  MPIDI_SendDoneCB_inline(context, sreq, PAMI_SUCCESS);
#if (MPIDI_STATISTICS)
  MPID_NSTAT(mpid_statp->sendsComplete);
#endif
}

static void
MPIDI_SendMsg_eager(pami_context_t    context,
                    MPID_Request    * sreq,
                    pami_endpoint_t   dest,
                    void            * sndbuf,
                    unsigned          sndlen)
  __attribute__((__noinline__));
static void
MPIDI_SendMsg_eager(pami_context_t    context,
                    MPID_Request    * sreq,
                    pami_endpoint_t   dest,
                    void            * sndbuf,
                    unsigned          sndlen)
{
  MPIDI_MsgInfo * msginfo = &sreq->mpid.envelope.msginfo;

  pami_send_t params = {
    .send   = {
      .dispatch = MPIDI_Protocols_Eager,
      .dest     = dest,
      .header   = {
        .iov_base = msginfo,
        .iov_len  = sizeof(MPIDI_MsgInfo),
      },
      .data     = {
        .iov_base = sndbuf,
        .iov_len  = sndlen,
      },
    },
    .events = {
      .cookie   = sreq,
      .local_fn = MPIDI_SendDoneCB,
      .remote_fn= NULL,
    },
  };

  pami_result_t rc;
  rc = PAMI_Send(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),mode,MPIDI_Protocols_Eager);
  TRACE_SET_S_BIT(dest,(sreq->mpid.idx),fl.f.sendEager);
}


static void
MPIDI_SendMsg_rzv(pami_context_t    context,
                  MPID_Request    * sreq,
                  pami_endpoint_t   dest,
                  void            * sndbuf,
                  size_t            sndlen)
  __attribute__((__noinline__));
static void
MPIDI_SendMsg_rzv(pami_context_t    context,
                  MPID_Request    * sreq,
                  pami_endpoint_t   dest,
                  void            * sndbuf,
                  size_t            sndlen)
{
  pami_result_t rc;

  /* Set the isRzv bit in the SEND request. This is important for
   * canceling requests.
   */
  MPIDI_Request_setRzv(sreq, 1);

  /* The rendezvous information, such as the origin/local/sender
   * node's send buffer and the number of bytes the origin node wishes
   * to send, is sent as the payload of the request-to-send (RTS)
   * message.
   */
#ifdef USE_PAMI_RDMA
  size_t sndlen_out;
  rc = PAMI_Memregion_create(context,
                             sndbuf,
			     sndlen,
			     &sndlen_out,
			     &sreq->mpid.envelope.memregion);
  MPID_assert(rc == PAMI_SUCCESS);
  MPID_assert(sndlen == sndlen_out);
  TRACE_ERR("RZV send for mr=%#llx addr=%p *addr[0]=%#016llx *addr[1]=%#016llx bytes=%zu\n",
            *(unsigned long long*)&sreq->mpid.envelope.memregion,
            sndbuf,
            *(((unsigned long long*)sndbuf)+0),
            *(((unsigned long long*)sndbuf)+1),
            sndlen);
#else
  sreq->mpid.envelope.memregion_used = 0;
#ifdef OUT_OF_ORDER_HANDLING
  if ((!MPIDI_Process.mp_s_use_pami_get) && (!sreq->mpid.envelope.msginfo.noRDMA))
#else
  if (!MPIDI_Process.mp_s_use_pami_get)
#endif
    {
      size_t sndlen_out;
      rc = PAMI_Memregion_create(context,
				 sndbuf,
				 sndlen,
				 &sndlen_out,
				 &sreq->mpid.envelope.memregion);
      if(rc == PAMI_SUCCESS)
	{
	  MPID_assert(sndlen == sndlen_out);
	  TRACE_ERR("RZV send for mr=%#llx addr=%p *addr[0]=%#016llx *addr[1]=%#016llx bytes=%zu\n",
		    *(unsigned long long*)&sreq->mpid.envelope.memregion,
		    sndbuf,
		    *(((unsigned long long*)sndbuf)+0),
		    *(((unsigned long long*)sndbuf)+1),
		    sndlen);
	  sreq->mpid.envelope.memregion_used = 1;
	}
        sreq->mpid.envelope.data   = sndbuf;
    }
    else
    {
      TRACE_ERR("RZV send (failed registration for sreq=%p addr=%p *addr[0]=%#016llx *addr[1]=%#016llx bytes=%zu\n",
		sreq,sndbuf,
		*(((unsigned long long*)sndbuf)+0),
		*(((unsigned long long*)sndbuf)+1),
		sndlen);
      sreq->mpid.envelope.data   = sndbuf;
    }
#endif
  sreq->mpid.envelope.length = sndlen;

  /* Do not specify a callback function to be invoked when the RTS
   * message has been sent. The MPI_Send is completed only when the
   * target/remote/receiver node has completed an PAMI_Get from the
   * origin node and has then sent a rendezvous acknowledgement (ACK)
   * to the origin node to signify the end of the transfer.  When the
   * ACK message is received by the origin node the same callback
   * function is used to complete the MPI_Send as the non-rendezvous
   * case.
   */
  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_RVZ,
    .dest     = dest,
    .header   = {
      .iov_base = &sreq->mpid.envelope,
      .iov_len  = sizeof(MPIDI_MsgEnvelope),
    },
    .data     = {
      .iov_base = NULL,
      .iov_len  = 0,
    },
  };

  rc = PAMI_Send_immediate(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),bufaddr,sreq->mpid.envelope.data);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),mode,MPIDI_Protocols_RVZ);
  TRACE_SET_S_BIT(dest,(sreq->mpid.idx),fl.f.sendRzv);
  TRACE_SET_S_BIT(dest,(sreq->mpid.idx),fl.f.sendEnvelop);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),fl.f.memRegion,sreq->mpid.envelope.memregion_used);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),fl.f.use_pami_get,MPIDI_Process.mp_s_use_pami_get);
}


static void
MPIDI_SendMsg_rzv_zerobyte(pami_context_t    context,
                           MPID_Request    * sreq,
                           pami_endpoint_t   dest)
  __attribute__((__noinline__));
static void
MPIDI_SendMsg_rzv_zerobyte(pami_context_t    context,
                           MPID_Request    * sreq,
                           pami_endpoint_t   dest)
{
  pami_result_t rc;

  /* Set the isRzv bit in the SEND request. This is important for
   * canceling requests.
   */
  MPIDI_Request_setRzv(sreq, 1);

  /* The rendezvous information, such as the origin/local/sender
   * node's send buffer and the number of bytes the origin node wishes
   * to send, is sent as the payload of the request-to-send (RTS)
   * message.
   */

  sreq->mpid.envelope.data = NULL;
  sreq->mpid.envelope.length = 0;

  /* Do not specify a callback function to be invoked when the RTS
   * message has been sent. The MPI_Send is completed only when the
   * target/remote/receiver node has matched the receive  and has then
   * sent a rendezvous acknowledgement (ACK) to the origin node to
   * signify the end of the transfer.  When the ACK message is received
   * by the origin node the same callback function is used to complete
   * the MPI_Send as the non-rendezvous case.
   */
  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_RVZ_zerobyte,
    .dest     = dest,
    .header   = {
      .iov_base = &sreq->mpid.envelope,
      .iov_len  = sizeof(MPIDI_MsgEnvelope),
    },
    .data     = {
      .iov_base = NULL,
      .iov_len  = 0,
    },
  };

  rc = PAMI_Send_immediate(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),bufaddr,sreq->mpid.envelope.data);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),mode,MPIDI_Protocols_RVZ_zerobyte);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),fl.f.memRegion,sreq->mpid.envelope.memregion_used);
  TRACE_SET_S_VAL(dest,(sreq->mpid.idx),fl.f.use_pami_get,MPIDI_Process.mp_s_use_pami_get);
  TRACE_SET_S_BIT(dest,(sreq->mpid.idx),fl.f.sendRzv);
  TRACE_SET_S_BIT(dest,(sreq->mpid.idx),fl.f.sendEnvelop);
}



static void
MPIDI_SendMsg_process_userdefined_dt(MPID_Request      * sreq,
                                     void             ** sndbuf,
                                     size_t            * data_sz)
  __attribute__((__noinline__));
static void
MPIDI_SendMsg_process_userdefined_dt(MPID_Request      * sreq,
                                     void             ** _sndbuf,
                                     size_t            * _data_sz)
{
  size_t          data_sz;
  int             dt_contig;
  MPI_Aint        dt_true_lb;
  MPID_Datatype * dt_ptr;
  void          * sndbuf;

  /*
   * Get the datatype info
   */
  MPIDI_Datatype_get_info(sreq->mpid.userbufcount,
                          sreq->mpid.datatype,
                          dt_contig,
                          data_sz,
                          dt_ptr,
                          dt_true_lb);

  MPID_assert(sreq->mpid.uebuf == NULL);

  /*
   * Contiguous data type
   */
  if (likely(dt_contig))
    {
      sndbuf = sreq->mpid.userbuf + dt_true_lb;
    }

  /*
   * Non-contiguous data type; allocate and populate temporary send
   * buffer
   */
  else
    {
      char *buf = NULL;
#if CUDA_AWARE_SUPPORT
      // This will need to be done in two steps:
      // 1 - Allocate a temp buffer which is the same size as user buffer and copy in it.
      // 2 - Pack data into ue buffer from temp buffer.
      int on_device =  MPIDI_cuda_is_device_buf(sreq->mpid.userbuf);
      if(MPIDI_Process.cuda_aware_support_on && on_device)
      {
        MPI_Aint dt_extent;
        MPID_Datatype_get_extent_macro(sreq->mpid.datatype, dt_extent);
        buf =  MPIU_Malloc(dt_extent * sreq->mpid.userbufcount);

        cudaError_t cudaerr = CudaMemcpy(buf, sreq->mpid.userbuf, dt_extent * sreq->mpid.userbufcount, cudaMemcpyDeviceToHost);
        if (cudaSuccess != cudaerr) {
          fprintf(stderr, "cudaMalloc failed: %s\n", CudaGetErrorString(cudaerr));
        }

      }
#endif

      MPID_Segment segment;

      if(data_sz != 0) {
        sreq->mpid.uebuf = sndbuf = MPIU_Malloc(data_sz);
        if (unlikely(sndbuf == NULL))
          {
            sreq->status.MPI_ERROR = MPI_ERR_NO_SPACE;
            MPIR_STATUS_SET_COUNT(sreq->status, 0);
            MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                       "Unable to allocate non-contiguous buffer");
          }
        sreq->mpid.uebuf_malloc = mpiuMalloc;

        DLOOP_Offset last = data_sz;
#if CUDA_AWARE_SUPPORT
        if(!MPIDI_Process.cuda_aware_support_on || !on_device)
          buf = sreq->mpid.userbuf;
#else
        buf = sreq->mpid.userbuf;
#endif
        MPID_assert(buf != NULL);

        MPID_Segment_init(buf,
                          sreq->mpid.userbufcount,
                          sreq->mpid.datatype,
                          &segment,
                          0);
        MPID_Segment_pack(&segment, 0, &last, sndbuf);
        MPID_assert(last == data_sz);
#if CUDA_AWARE_SUPPORT
        if(MPIDI_Process.cuda_aware_support_on && on_device)
          MPIU_Free(buf);
#endif
      } else {
	sndbuf = NULL;
      }
    }

  *_sndbuf = sndbuf;
  *_data_sz = data_sz;
}


static inline void
MPIDI_SendMsg(pami_context_t   context,
              MPID_Request   * sreq,
              unsigned         isSync,
              const unsigned   isInternal)
{
  /* ------------------------------ */
  /* special case: NULL destination */
  /* ------------------------------ */
  int rank = MPIDI_Request_getPeerRank_comm(sreq);
  if (unlikely(rank == MPI_PROC_NULL))
    {
      if (isSync)
        MPIDI_Request_complete(sreq);
      MPIDI_Request_complete(sreq);
      return;
    }
  else
    {
      MPIDI_Request_setPeerRank_pami(sreq, MPID_VCR_GET_LPID(sreq->comm->vcr, rank));
    }

  MPIDI_Request_setSync(sreq, isSync);
  MPIDI_Request_setPeerRequestH(sreq);

  /*
   * Create the destination endpoint
   */
  pami_endpoint_t dest;
  MPIDI_Context_endpoint(sreq, &dest);
  pami_task_t  dest_tid;
  dest_tid=sreq->comm->vcr[rank]->taskid;
#if (MPIDI_STATISTICS)
  MPID_NSTAT(mpid_statp->sends);
#endif
#ifdef OUT_OF_ORDER_HANDLING
  MPIDI_Out_cntr_t *out_cntr;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  out_cntr = &MPIDI_Out_cntr[dest_tid];
  out_cntr->nMsgs++;
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
  MPIDI_Request_setMatchSeq(sreq, out_cntr->nMsgs);
#endif
if (!TOKEN_FLOW_CONTROL_ON) {
  size_t   data_sz;
  void   * sndbuf;
  if (likely(HANDLE_GET_KIND(sreq->mpid.datatype) == HANDLE_KIND_BUILTIN))
    {
      sndbuf   = sreq->mpid.userbuf;
      data_sz  = sreq->mpid.userbufcount * MPID_Datatype_get_basic_size(sreq->mpid.datatype);
    }
  else
    {
      MPIDI_SendMsg_process_userdefined_dt(sreq, &sndbuf, &data_sz);
    }
  MPIDI_GET_S_REC(dest_tid,sreq,context,isSync,data_sz);

#ifdef OUT_OF_ORDER_HANDLING
  sreq->mpid.envelope.msginfo.noRDMA=0;
#endif

  const unsigned isLocal = PAMIX_Task_is_local(dest_tid);
  const size_t data_sz_limit = isSync?ULONG_MAX:data_sz;

  /*
   * Always use the short protocol when data_sz is small.
   */
  if (likely(data_sz < MPIDI_PT2PT_SHORT_LIMIT(isInternal,isLocal)))
    {
      TRACE_ERR("Sending(short%s%s) bytes=%zu (short_limit=%u)\n", isInternal==1?",internal":"", isLocal==1?",intranode":"", data_sz, MPIDI_PT2PT_SHORT_LIMIT(isInternal,isLocal));
      MPIDI_SendMsg_short(context,
                          sreq,
                          dest,
                          sndbuf,
                          data_sz,
                          isSync);
    }
  /*
   * Use the eager protocol when data_sz is less than the eager limit.
   */
  else if (data_sz_limit < MPIDI_PT2PT_EAGER_LIMIT(isInternal,isLocal))
    {
      TRACE_ERR("Sending(eager%s%s) bytes=%zu (eager_limit=%u)\n", isInternal==1?",internal":"", isLocal==1?",intranode":"", data_sz, MPIDI_PT2PT_EAGER_LIMIT(isInternal,isLocal));
      MPIDI_SendMsg_eager(context,
                          sreq,
                          dest,
                          sndbuf,
                          data_sz);
#ifdef MPIDI_STATISTICS
      if (!isLocal && MPID_cc_is_complete(&sreq->cc))
        {
          MPID_NSTAT(mpid_statp->sendsComplete);
        }
#endif
    }
  /*
   * Use the default rendezvous protocol implementation that guarantees
   * no unexpected data and does not complete the send until the remote
   * receive is posted.
   */
  else
    {
      TRACE_ERR("Sending(rendezvous%s%s) bytes=%zu (eager_limit=%u)\n", isInternal==1?",internal":"", isLocal==1?",intranode":"", data_sz, MPIDI_PT2PT_EAGER_LIMIT(isInternal,isLocal));
#ifdef OUT_OF_ORDER_HANDLING
      sreq->mpid.envelope.msginfo.noRDMA=isLocal;
#endif
      if (likely(data_sz > 0))
        {
          MPIDI_SendMsg_rzv(context,
                            sreq,
                            dest,
                            sndbuf,
                            data_sz);
        }
      else
        {
          MPIDI_SendMsg_rzv_zerobyte(context, sreq, dest);
        }

#ifdef MPIDI_STATISTICS
      if (!isLocal && MPID_cc_is_complete(&sreq->cc))
        {
          MPID_NSTAT(mpid_statp->sendsComplete);
        }
#endif
    }
    }
  else
    {  /* TOKEN_FLOW_CONTROL_ON  */
    #if TOKEN_FLOW_CONTROL
    if (!(sreq->mpid.userbufcount))
       {
        MPIDI_GET_S_REC(dest_tid,sreq,context,isSync,0);
        TRACE_ERR("Sending(short,intranode) bytes=%u (short_limit=%u)\n", data_sz, MPIDI_Process.short_limit);
        MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
        MPIDI_Piggy_back_tokens(dest,sreq,0);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        MPIDI_SendMsg_short(context,
                            sreq,
                            dest,
                            sreq->mpid.userbuf,
                            0,
                            isSync);
       }
     else
       {
       size_t   data_sz;
       void   * sndbuf;
       int      noRDMA=0;
       if (likely(HANDLE_GET_KIND(sreq->mpid.datatype) == HANDLE_KIND_BUILTIN))
         {
           sndbuf   = sreq->mpid.userbuf;
           data_sz  = sreq->mpid.userbufcount * MPID_Datatype_get_basic_size(sreq->mpid.datatype);
         }
       else
        {
          MPIDI_SendMsg_process_userdefined_dt(sreq, &sndbuf, &data_sz);
         }
       MPIDI_GET_S_REC(dest_tid,sreq,context,isSync,data_sz);
       if (unlikely(PAMIX_Task_is_local(dest_tid) != 0))  noRDMA=1;

       MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
       if ((!isSync) && MPIDI_Token_cntr[dest].tokens >= 1)
        {
          if (data_sz <= MPIDI_Process.pt2pt.limits.application.immediate.remote)
             {
             TRACE_ERR("Sending(short,intranode) bytes=%u (short_limit=%u)\n", data_sz,MPIDI_Process.pt2pt.limits.application.immediate.remote);
             --MPIDI_Token_cntr[dest].tokens;
             MPIDI_Piggy_back_tokens(dest,sreq,data_sz);
             MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
             MPIDI_SendMsg_short(context,
                                 sreq,
                                 dest,
                                 sndbuf,
                                 data_sz,
                                 isSync);
             }
           else if (data_sz <= MPIDI_Process.pt2pt.limits.application.eager.remote)
             {
              TRACE_ERR("Sending(eager) bytes=%u (eager_limit=%u)\n", data_sz, MPIDI_Process.pt2pt.limits.application.eager.remote);
              --MPIDI_Token_cntr[dest].tokens;
              MPIDI_Piggy_back_tokens(dest,sreq,data_sz);
              MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
              MPIDI_SendMsg_eager(context,
                                 sreq,
                                 dest,
                                 sndbuf,
                                 data_sz);
#ifdef MPIDI_STATISTICS
                    if (MPID_cc_is_complete(&sreq->cc)) {
                        MPID_NSTAT(mpid_statp->sendsComplete);
                    }
#endif

              }
            else   /* rendezvous message  */
              {
                TRACE_ERR("Sending(RZV) bytes=%u (eager_limit=%u)\n", data_sz, MPIDI_Process.pt2pt.limits.application.eager.remote);
                MPIDI_Piggy_back_tokens(dest,sreq,data_sz);
                MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
                sreq->mpid.envelope.msginfo.noRDMA=noRDMA;
                MPIDI_SendMsg_rzv(context,
                                  sreq,
                                  dest,
                                  sndbuf,
                                  data_sz);
#ifdef MPIDI_STATISTICS
                       if (MPID_cc_is_complete(&sreq->cc))
                       {
                          MPID_NSTAT(mpid_statp->sendsComplete);
                       }
#endif
              }
       }
     else
      {   /* no tokens, all messages use rendezvous protocol */
        if ((data_sz <= MPIDI_Process.pt2pt.limits.application.eager.remote) && (!isSync)) {
             ++MPIDI_Token_cntr[dest].n_tokenStarved;
              sreq->mpid.envelope.msginfo.noRDMA=1;
        }
        else sreq->mpid.envelope.msginfo.noRDMA=noRDMA;
        MPIDI_Piggy_back_tokens(dest,sreq,data_sz);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        TRACE_ERR("Sending(RZV) bytes=%u (eager_limit=%u)\n", data_sz, MPIDI_Process.pt2pt.limits.application.eager.remote);
        if (likely(data_sz > 0))
           {
             MPIDI_SendMsg_rzv(context,
                                  sreq,
                                  dest,
                                sndbuf,
                              data_sz);
           }
          else
            {
              MPIDI_SendMsg_rzv_zerobyte(context, sreq, dest);
            }
#ifdef MPIDI_STATISTICS
               if (MPID_cc_is_complete(&sreq->cc))
                {
                   MPID_NSTAT(mpid_statp->sendsComplete);
                }
#endif
    }
  }
    #else
    MPID_assert_always(0);
    #endif /* TOKEN_FLOW_CONTROL */
 }
}


/*
 * \brief Central function for all low-level sends.
 *
 * This is assumed to have been posted to a context, and is now being
 * called from inside advance.  This has (unspecified) locking
 * implications.
 *
 * Prerequisites:
 *    + Not sending to a NULL rank
 *    + Request already allocated
 *
 * \param[in]     context The PAMI context on which to do the send operation
 * \param[in,out] sreq    Structure containing all relevant info about the message.
 */
pami_result_t
MPIDI_Send_handoff(pami_context_t   context,
                   void           * _sreq)
{
  MPID_Request * sreq = (MPID_Request*)_sreq;
  MPID_assert(sreq != NULL);

  MPIDI_SendMsg(context, sreq, 0, 0);
  return PAMI_SUCCESS;
}


pami_result_t
MPIDI_Ssend_handoff(pami_context_t   context,
                   void           * _sreq)
{
  MPID_Request * sreq = (MPID_Request*)_sreq;
  MPID_assert(sreq != NULL);

  MPIDI_SendMsg(context, sreq, 1, 0);
  return PAMI_SUCCESS;
}


/*
 * \brief Central function for all low-level sends.
 *
 * This is assumed to have been posted to a context, and is now being
 * called from inside advance.  This has (unspecified) locking
 * implications.
 *
 * Prerequisites:
 *    + Not sending to a NULL rank
 *    + Request already allocated
 *
 * \param[in]     context The PAMI context on which to do the send operation
 * \param[in,out] sreq    Structure containing all relevant info about the message.
 */
pami_result_t
MPIDI_Isend_handoff(pami_context_t   context,
                    void           * _sreq)
{
  MPID_Request * sreq = (MPID_Request*)_sreq;
  MPID_assert(sreq != NULL);

  /* This initializes all the fields not set in MPI_Isend() */
  MPIDI_Request_initialize(sreq);

  /* Since this is only called from MPI_Isend(), it is not synchronous */
  MPIDI_SendMsg(context, sreq, 0, 0);
  return PAMI_SUCCESS;
}

pami_result_t
MPIDI_Isend_handoff_internal(pami_context_t   context,
                             void           * _sreq)
{
  MPID_Request * sreq = (MPID_Request*)_sreq;
  MPID_assert(sreq != NULL);

  /* This initializes all the fields not set in MPI_Isend() */
  MPIDI_Request_initialize(sreq);

  /* Since this is only called from MPI_Isend(), it is not synchronous */
  MPIDI_SendMsg(context, sreq, 0, 1);
  return PAMI_SUCCESS;
}

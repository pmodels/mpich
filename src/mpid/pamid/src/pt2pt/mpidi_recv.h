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


#if TOKEN_FLOW_CONTROL
extern MPIDI_Out_cntr_t *MPIDI_Out_cntr;
extern int MPIDI_tfctrl_hwmark;
extern void *MPIDI_mm_alloc(size_t);
extern void  MPIDI_mm_free(void *, size_t);
extern int tfctrl_enabled;
extern char *EagerLimit;
#define MPIDI_Return_tokens        MPIDI_Return_tokens_inline
#define MPIDI_Receive_tokens       MPIDI_Receive_tokens_inline
#define MPIDI_Update_rettoks       MPIDI_Update_rettoks_inline
#define MPIDI_Alloc_lock           MPIDI_Alloc_lock_inline
#define MPIDI_Must_return_tokens   MPIDI_Must_return_tokens_inline

static inline void *
MPIDI_Return_tokens_inline(pami_context_t context, int dest, int tokens)
{
   MPIDI_MsgInfo  tokenInfo;
   if (tokens) {
       memset(&tokenInfo,0, sizeof(MPIDI_MsgInfo));
       tokenInfo.control=MPIDI_CONTROL_RETURN_TOKENS;
       tokenInfo.alltokens=tokens;
       pami_send_immediate_t params = {
           .dispatch = MPIDI_Protocols_Control,
           .dest     = dest,
           .header   = {
              .iov_base = &tokenInfo,
              .iov_len  = sizeof(MPIDI_MsgInfo),
           },
           .data     = {
             .iov_base = NULL,
             .iov_len  = 0,
           },
         };
         pami_result_t rc;
         rc = PAMI_Send_immediate(context, &params);
         MPID_assert(rc == PAMI_SUCCESS);
     }
}


static inline void *
MPIDI_Must_return_tokens_inline(pami_context_t context,int dest) 
{
  int rettoks=0;  
   
  if  (MPIDI_Token_cntr[dest].rettoks
       && (MPIDI_Token_cntr[dest].rettoks + MPIDI_Token_cntr[dest].unmatched
       >= MPIDI_tfctrl_hwmark))
  {
       rettoks=MPIDI_Token_cntr[dest].rettoks;
       MPIDI_Token_cntr[dest].rettoks=0;
       MPIDI_Return_tokens_inline(context,dest,rettoks);

  }
}

static inline void *
MPIDI_Receive_tokens_inline(const MPIDI_MsgInfo *m, int dest)
    {
      if ((m)->tokens)
      {
          MPIDI_Token_cntr[dest].tokens += (m)->tokens;
      }
    }

static inline void *
MPIDI_Update_rettoks_inline(int source) 
 {
     MPIDI_Token_cntr[source].rettoks++;
 }

static inline void *
MPIDI_Alloc_lock_inline(void **buf,size_t size)
 {
       MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
       (*buf) = (void *) MPIDI_mm_alloc(size);
       MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
 }

#else
#define MPIDI_Return_tokens(x,y,z)
#define MPIDI_Receive_tokens(x,y)
#define MPIDI_Update_rettoks(x)
#define MPIDI_Must_return_tokens(x,y) (0)
#define MPIDI_Alloc_lock(x,y)
#endif

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
           MPI_Aint        count,
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
  MPIDI_SET_PR_REC(rreq,buf,count,datatype,pami_source,rank,tag,comm,is_blocking);

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
      TRACE_SET_R_VALX(pami_source,rreq,len,rreq->mpid.uebuflen);
      MPIDI_RecvMsg_Unexp(rreq, buf, count, datatype);
      mpi_errno = rreq->status.MPI_ERROR;
      if (TOKEN_FLOW_CONTROL_ON) {
         #if TOKEN_FLOW_CONTROL
         if ((rreq->mpid.uebuflen) && (!(rreq->mpid.envelope.msginfo.isRzv))) {
           MPIDI_Token_cntr[(rreq->mpid.peer_pami)].unmatched--;
           MPIDI_Update_rettoks(rreq->mpid.peer_pami);
         }
         MPIDI_Must_return_tokens(MPIDI_Context[0],(rreq->mpid.peer_pami));
         #else
         MPID_assert_always(0);
         #endif
      }
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      MPID_Request_discard(newreq);
#ifdef OUT_OF_ORDER_HANDLING
      if ((MPIDI_In_cntr[rreq->mpid.peer_pami].n_OutOfOrderMsgs>0))
          MPIDI_Recvq_process_out_of_order_msgs(rreq->mpid.peer_pami, MPIDI_Context[0]);
#endif
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
    if (!(MPID_cc_is_complete(&rreq->cc)))
    {
        MPID_NSTAT(mpid_statp->recvWaitsComplete);
    }
#endif

  return mpi_errno;
}

#endif

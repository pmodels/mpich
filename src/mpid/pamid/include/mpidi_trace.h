/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file include/mpidi_trace.h
 * \brief record trace info. for pt2pt comm.
 */
/*
 *
 *
 */


#ifndef __include_mpidi_trace_h__
#define __include_mpidi_trace_h__

#include <sys/time.h>
#include <sys/param.h>

#ifdef MPIDI_TRACE
#define N_MSGS    1024
#define SEQMASK   N_MSGS-1
typedef struct {
   void  *req;            /* address of request                      */
   void  *bufadd;         /* user's receive buffer address           */
   uint  msgid;           /* msg seqno.                              */
   unsigned short ctx;    /* mpi context id                          */
   unsigned short dummy;  /* reserved                                */
   uint nMsgs;            /* highest msg seqno that arrived in order */
   int        tag;
   int        len;
   int        rsource;    /* source of the message arrived           */
   int        rtag;       /* tag of a received message               */
   int        rlen;       /* len of a received message               */
   int        rctx;       /* context of a received message           */
   void *     matchedHandle; /* a message with multiple handles      */
   union {
   uint       flags;
   struct {
#ifdef __BIG_ENDIAN__
   uint       posted:1;   /* has the receive posted                  */
   uint       rzv:1;      /* rendezvous message ?                    */
   uint       sync:1;     /* synchronous message?                    */
   uint       sendAck:1;  /* send ack?                               */
   uint       sendFin:1;  /* send complete info?                     */
   uint       HH:1;       /* header handler                          */
   uint       ool:1;      /* the msg arrived out of order            */
   uint       matchedInOOL:1;/* found a match in out of order list   */
   uint       comp_in_HH:4;  /* the msg completed in header handler  */
   uint       comp_in_HHV_noMatch:1;/* no matched in header handler EA */
   uint       sync_com_in_HH:1; /* sync msg completed in header handler*/
   uint       matchedInHH:1; /* found a match in header haldner      */
   uint       matchedInComp:1;/* found a match in completion handler */
   uint       matchedInUQ:2; /* found a match in unexpected queue    */
   uint       matchedInUQ2:2;/* found a match in unexpected queue    */
   uint       matchedInWait:1;/* found a match in MPI_Wait() etc.    */
   uint       ReadySend:1;   /* a ready send messsage                */
   uint       persist:1;     /* persist communication                */
   uint       reserve:1;
   uint       reserve1:8;
#else
   uint      reserve1:8;
   uint      reserve:1;
   uint      persist:1;     /* persist communication                */
   uint      ReadySend:1;   /* a ready send messsage                */
   uint      matchedInWait:1;/* found a match in MPI_Wait() etc.    */
   uint      matchedInUQ2:2;/* found a match in unexpected queue    */
   uint      matchedInUQ:2; /* found a match in unexpected queue    */
   uint      matchedInComp:1;/* found a match in completion handler */
   uint      matchedInHH:1; /* found a match in header haldner      */
   uint      sync_com_in_HH:1; /* sync msg completed in header handler*/
   uint      comp_in_HHV_noMatch:1;/* no matched in header handler EA */
   uint      comp_in_HH:4;  /* the msg completed in header handler  */
   uint      matchedInOOL:1;/* found a match in out of order list   */
   uint      ool:1;      /* the msg arrived out of order            */
   uint      HH:1;       /* header handler                          */
   uint      sendFin:1;  /* send complete info?                     */
   uint      sendAck:1;  /* send ack?                               */
   uint      sync:1;     /* synchronous message?                    */
   uint      rzv:1;      /* rendezvous message ?                    */
   uint      posted:1;   /* has the receive posted                  */
#endif
   }f;
   }fl;
} recv_status;

typedef struct {
   void       *req;          /* address of request                   */
   void       *bufaddr;      /* address of user's send buffer        */
   int        dest;          /* destination of a message             */
   int        rank;          /* rank in a communicator               */
   int        mode;          /* protocol used                        */
   uint       msgid;         /* message sequence no.                 */
   unsigned short  sctx;     /* context id                           */
   unsigned short dummy;
   int        tag;           /* tag of a message                     */
   int        len;           /* lengh of a message                   */
   union {
   uint       flags;
   struct {
#ifdef __BIG_ENDIAN__
   uint       blocking:1;    /* blocking send ?                      */
   uint       sync:1;        /* sync message                         */
   uint       sendEnvelop:1; /* envelop send?                        */
   uint       sendShort:1;   /* send immediate                       */
   uint       sendEager:1;   /* eager send                           */
   uint       sendRzv:1;     /* send via renzdvous protocol          */
   uint       memRegion:1;   /* memory is registered                 */
   uint       use_pami_get:1;/* use only PAMI_Get()                  */
   uint       NoComp:4;      /* no completion handler                */
   uint       sendComp:1;    /* send complete                        */
   uint       recvAck:1;     /* recv an ack from the receiver        */
   uint       recvFin:1;     /* recv complete information            */
   uint       complSync:1;   /* complete sync                        */
   uint       ReadySend:1;   /* ready send                           */
   uint       reqXfer:1;     /* request message transfer             */
   uint       persist:1;     /* persistent communiation              */
   uint       reserve:5;
   uint       reserve1:8;
#else
   uint       reserve1:8;
   uint       reserve:5;
   uint       persist:1;     /* persistent communiation              */
   uint       reqXfer:1;     /* request message transfer             */
   uint       ReadySend:1;   /* ready send                           */
   uint       complSync:1;   /* complete sync                        */
   uint       recvFin:1;     /* recv complete information            */
   uint       recvAck:1;     /* recv an ack from the receiver        */
   uint       sendComp:1;    /* send complete                        */
   uint       NoComp:4;      /* no completion handler                */
   uint       use_pami_get:1;/* use only PAMI_Get()                  */
   uint       memRegion:1;   /* memory is registered                 */
   uint       sendRzv:1;     /* send via renzdvous protocol          */
   uint       sendEager:1;   /* eager send                           */
   uint       sendShort:1;   /* send immediate                       */
   uint       sendEnvelop:1; /* envelop send?                        */
   uint       sync:1;        /* sync message                         */
   uint       blocking:1;    /* blocking send ?                      */
#endif
   }f;
   }fl;
} send_status;

typedef struct {
   void  *req;         /* address of a request                 */
   void  *bufadd;      /* address of user receive buffer       */
   int    src_task;    /* source PAMI task id                  */
   int    rank;        /* rank in a communicator               */
   int    tag;         /* tag of a posted recv                 */
   int    count;       /* count of a specified datattype       */
   int    datatype;
   int    len;         /* length of a receive message          */
   uint  nMsgs;        /* no. of messages have been received   */
   uint  msgid;        /* msg seqno of the matched message     */
#ifdef __BIG_ENDIAN__
   uint  sendCtx:16;   /* context of incoming msg              */
   uint  recvCtx:16;   /* context of a posted receive          */
#else
   uint  recvCtx:16;   /* context of a posted receive          */
   uint  sendCtx:16;   /* context of incoming msg              */
#endif
   union {
   uint       flags;
   struct {
#ifdef __BIG_ENDIAN__
   uint  lw:4;         /* use lw protocol immediate send       */
   uint  persist:4;    /* persistent communication             */
   uint  blocking:2;   /* blocking receive                     */
   uint  reserve:6;  
   uint  reserve1:16;
#else
   uint  reserve1:16;
   uint  reserve:6;  
   uint  blocking:2;   /* blocking receive                     */
   uint  persist:4;    /* persistent communication             */
   uint  lw:4;         /* use lw protocol immediate send       */
#endif
   }f;
   }fl;
} posted_recv;


typedef struct MPIDI_Trace_buf {
    recv_status *R;     /* record incoming messages    */
    posted_recv *PR;    /* record posted receive       */
    send_status *S;     /* send messages               */
    int  totPR;         /* total no. of poste receive  */
} MPIDI_Trace_buf_t;

MPIDI_Trace_buf_t  *MPIDI_Trace_buf;



#define MPIDI_SET_PR_REC(rreq,buf,ct,dt,pami_id,rank,tag,comm,is_blk) { \
        int idx,src,seqNo,x;                                      \
        if (pami_id != MPI_ANY_SOURCE)                            \
            src=pami_id;                                          \
        else {                                                    \
            src= MPIR_Process.comm_world->rank;                   \
        }                                                         \
        MPIDI_Trace_buf[src].totPR++ ;                            \
        seqNo=MPIDI_Trace_buf[src].totPR;                         \
        idx = (seqNo & SEQMASK);                                  \
        memset(&MPIDI_Trace_buf[src].PR[idx],0,sizeof(posted_recv)); \
        MPIDI_Trace_buf[src].PR[idx].src_task= pami_id;           \
        MPIDI_Trace_buf[src].PR[idx].rank   = rank;               \
        MPIDI_Trace_buf[src].PR[idx].bufadd = buf;                \
        MPIDI_Trace_buf[src].PR[idx].msgid = seqNo;               \
        MPIDI_Trace_buf[src].PR[idx].count = ct;                  \
        MPIDI_Trace_buf[src].PR[idx].datatype = dt;               \
        MPIDI_Trace_buf[src].PR[idx].tag=tag;                     \
        MPIDI_Trace_buf[src].PR[idx].sendCtx=comm->context_id;    \
        MPIDI_Trace_buf[src].PR[idx].recvCtx=comm->recvcontext_id;\
        MPIDI_Trace_buf[src].PR[idx].fl.f.blocking=is_blk;             \
        rreq->mpid.PR_idx=idx;                                    \
}

#define MPIDI_GET_S_REC(dd,sreq,ctx,isSync,dataSize) {        \
        send_status *sstatus;                                   \
        int seqNo=sreq->mpid.envelope.msginfo.MPIseqno;         \
        int idx = (seqNo & SEQMASK);                            \
        sreq->mpid.partner_id=dd;                               \
        memset(&MPIDI_Trace_buf[dd].S[idx],0,sizeof(send_status));\
        sstatus=&MPIDI_Trace_buf[dd].S[idx];                    \
        sstatus->req    = (void *)sreq;                         \
        sstatus->tag    = sreq->mpid.envelope.msginfo.MPItag;   \
        sstatus->dest   = sreq->mpid.peer_pami;                 \
        sstatus->rank   = sreq->mpid.peer_comm;                 \
        sstatus->msgid = seqNo;                                 \
        sstatus->fl.f.sync = isSync;                            \
        sstatus->sctx = ctx;                                    \
        sstatus->tag = sreq->mpid.envelope.msginfo.MPItag;      \
        sstatus->len= dataSize;                                 \
        sreq->mpid.idx=idx;                                     \
}


#define TRACE_SET_S_BIT(dd,ii,mbr) MPIDI_Trace_buf[(dd)].S[(ii)].mbr=1;
#define TRACE_SET_R_BIT(dd,ii,mbr) MPIDI_Trace_buf[(dd)].R[(ii)].mbr=1;
#define TRACE_SET_S_VAL(dd,ii,mbr,val) MPIDI_Trace_buf[(dd)].S[(ii)].mbr=val;
#define TRACE_SET_R_VALX(dd,rr,mbr,val) {                        \
    pami_task_t dd1;                                             \
    if (dd < 0)                                                  \
       dd1=rr->mpid.partner_id;                                  \
    else                                                         \
       dd1=dd;                                                   \
    MPIDI_Trace_buf[(dd1)].R[(rr->mpid.PR_idx)].mbr=val;         \
}
#define TRACE_SET_R_VAL(dd,ii,mbr,val) MPIDI_Trace_buf[(dd)].R[(ii)].mbr=val;
#define TRACE_SET_REQ_VAL(ww,val1) ww=val1;
#define TRACE_MEMSET_R(tt,nbr,str)  (memset(&MPIDI_Trace_buf[tt].R[(nbr & SEQMASK)],0,sizeof(str)));
#define TRACE_MEMSET_S(tt,nbr,str)  (memset(&MPIDI_Trace_buf[tt].S[(nbr & SEQMASK)],0,sizeof(str)));
#else 
int recv_status;
int send_status;
int posted_recv;
#define MPIDI_SET_PR_REC(rreq,buf,ct,dt,pami_id,rank,tag,comm,is_blk)
#define MPIDI_GET_S_REC(dest,sreq,ctx,isSync,dataSize)
#define TRACE_SET_S_BIT(dd,ii,mbr)
#define TRACE_SET_R_BIT(dd,ii,mbr)
#define TRACE_SET_S_VAL(dd,ii,mbr,val)
#define TRACE_SET_R_VALX(dd,rr,mbr,val)
#define TRACE_SET_R_VAL(dd,ii,mbr,val)
#define TRACE_SET_REQ_VAL(ww,val1)
#define TRACE_MEMSET_R(tt,nbr,str)

#endif  /* MPIDI_TRACE             */
#endif   /* include_mpidi_trace_h  */

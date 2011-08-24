/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */



#ifndef MX_MODULE_IMPL_H
#define MX_MODULE_IMPL_H
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif
#include <myriexpress.h>
#include "mx_extensions.h"
#include "mpid_nem_impl.h"

/* #define USE_CTXT_AS_MARK  */
/* #define DEBUG_IOV */
/* #define ONDEMAND */

int MPID_nem_mx_init (MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mx_finalize (void);
int MPID_nem_mx_poll(int in_blocking_progress);
int MPID_nem_mx_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mx_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_mx_vc_init (MPIDI_VC_t *vc);
int MPID_nem_mx_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_mx_vc_terminate (MPIDI_VC_t *vc);
int MPID_nem_mx_get_from_bc(const char *business_card, uint32_t *remote_endpoint_id, uint64_t *remote_nic_id);
 
/* alternate interface */
int MPID_nem_mx_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, 
			    void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_mx_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, 
				MPIDI_msg_sz_t data_sz, MPID_Request **sreq_ptr);
int MPID_nem_mx_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz);

/* Direct Routines */
int  MPID_nem_mx_directSend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int dest, int tag, 
			    MPID_Comm * comm, int context_offset, MPID_Request **sreq_p);
int  MPID_nem_mx_directSsend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int dest, int tag, 
			    MPID_Comm * comm, int context_offset,MPID_Request **sreq_p);
int MPID_nem_mx_directRecv(MPIDI_VC_t *vc, MPID_Request *rreq);
int MPID_nem_mx_cancel_send(MPIDI_VC_t *vc, MPID_Request *sreq);
int MPID_nem_mx_cancel_recv(MPIDI_VC_t *vc, MPID_Request *rreq);
int MPID_nem_mx_probe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, MPI_Status *status);
int MPID_nem_mx_iprobe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status);

int MPID_nem_mx_anysource_iprobe(int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status);

/* Callback routine for unex msgs in MX */
mx_unexp_handler_action_t MPID_nem_mx_get_adi_msg(void *context,mx_endpoint_addr_t source,
						  uint64_t match_info,uint32_t length,void *data);
/* Any source management */
void MPID_nem_mx_anysource_posted(MPID_Request *rreq);
int MPID_nem_mx_anysource_matched(MPID_Request *rreq);

/* Dtype management */
int MPID_nem_mx_process_sdtype(MPID_Request **sreq_p,  MPI_Datatype datatype,  MPID_Datatype * dt_ptr, const void *buf, 
			       int count, MPIDI_msg_sz_t data_sz, mx_segment_t *mx_iov, uint32_t  *num_seg,int first_free_slot);
int MPID_nem_mx_process_rdtype(MPID_Request **rreq_p, MPID_Datatype * dt_ptr, MPIDI_msg_sz_t data_sz, mx_segment_t *mx_iov, 
			       uint32_t  *num_seg);

/* Connection management*/
int MPID_nem_mx_send_conn_info (MPIDI_VC_t *vc);

extern mx_endpoint_t MPID_nem_mx_local_endpoint;
extern int           MPID_nem_mx_pending_send_req;
extern uint32_t      MPID_NEM_MX_FILTER;
extern uint64_t      MPID_nem_mx_local_nic_id;
extern uint32_t      MPID_nem_mx_local_endpoint_id;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    /* The following 2 are used to set-up the connection */
    uint32_t           remote_endpoint_id; 
    uint64_t           remote_nic_id;     
    uint16_t           local_connected;
    uint16_t           remote_connected;
    /* The following is used to actually send messages */
    mx_endpoint_addr_t remote_endpoint_addr;
    /* Poster recv pointer for anysource management*/
    int             (* recv_posted)(MPID_Request *req, void *vc);
} MPID_nem_mx_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vcp, field) (((MPID_nem_mx_vc_area *)VC_CH(((vcp)))->netmod_area.padding)->field)

/* The req provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the req structure
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    mx_request_t mx_request; 
} MPID_nem_mx_req_area;

/* accessor macro to private fields in REQ */
#define REQ_FIELD(reqp, field) (((MPID_nem_mx_req_area *)((reqp)->ch.netmod_area.padding))->field)

/* The begining of this structure is the same as MPID_Request */
struct MPID_nem_mx_internal_req 
{
   MPIU_OBJECT_HEADER; /* adds (unused) handle and ref_count fields */
   MPID_Request_kind_t    kind;       /* used   */
   MPIDI_CH3_PktGeneric_t pending_pkt;
   MPIDI_VC_t            *vc;
   void                  *tmpbuf;
   MPIDI_msg_sz_t         tmpbuf_sz;
   struct  MPID_nem_mx_internal_req *next;
} ;

typedef struct MPID_nem_mx_internal_req MPID_nem_mx_internal_req_t;

typedef union 
{
   MPID_nem_mx_internal_req_t nem_mx_req;
   MPID_Request               mpi_req; 
}
MPID_nem_mx_unified_req_t ;

int MPID_nem_mx_internal_req_queue_init(void);
int MPID_nem_mx_internal_req_queue_destroy(void);
int MPID_nem_mx_internal_req_dequeue(MPID_nem_mx_internal_req_t **req);
int MPID_nem_mx_internal_req_enqueue(MPID_nem_mx_internal_req_t *req);

#if CH3_RANK_BITS == 16
#ifdef USE_CTXT_AS_MARK
#define NBITS_TAG  32
#else /* USE_CTXT_AS_MARK */
#define NBITS_TAG  31
#endif /* USE_CTXT_AS_MARK */
typedef int32_t Mx_Nem_tag_t;
#elif CH3_RANK_BITS == 32
#ifdef USE_CTXT_AS_MARK
#define NBITS_TAG  16
#else /* USE_CTXT_AS_MARK */
#define NBITS_TAG  15
#endif /* USE_CTXT_AS_MARK */
typedef int16_t Mx_Nem_tag_t;
#endif /* CH3_RANK_BITS */

#ifdef USE_CTXT_AS_MARK
#define NBITS_TYPE 0
#else /* USE_CTXT_AS_MARK */
#define NBITS_TYPE 1
#endif /* USE_CTXT_AS_MARK */
#define NBITS_RANK CH3_RANK_BITS
#define NBITS_CTXT 16
#define NBITS_PGRANK (sizeof(int)*8)

#define NEM_MX_MATCHING_BITS (NBITS_TYPE+NBITS_TAG+NBITS_RANK+NBITS_CTXT)
#define SHIFT_TYPE           (NBITS_TAG+NBITS_RANK+NBITS_CTXT)
#define SHIFT_TAG            (NBITS_RANK+NBITS_CTXT)
#define SHIFT_RANK           (NBITS_CTXT)
#define SHIFT_PGRANK         (NBITS_CTXT)
#define SHIFT_CTXT           (0)

#define NEM_MX_MAX_TYPE      ((UINT64_C(1)<<NBITS_TYPE)  -1)
#define NEM_MX_MAX_TAG       ((UINT64_C(1)<<NBITS_TAG)   -1)
#define NEM_MX_MAX_RANK      ((UINT64_C(1)<<NBITS_RANK)  -1)
#define NEM_MX_MAX_CTXT      ((UINT64_C(1)<<NBITS_CTXT)  -1)
#define NEM_MX_MAX_PGRANK    ((UINT64_C(1)<<NBITS_PGRANK)-1)

#define NEM_MX_TYPE_MASK     (NEM_MX_MAX_TYPE<<SHIFT_TYPE)
#define NEM_MX_TAG_MASK      (NEM_MX_MAX_TAG <<SHIFT_TAG ) 
#define NEM_MX_RANK_MASK     (NEM_MX_MAX_RANK<<SHIFT_RANK)
#define NEM_MX_CTXT_MASK     (NEM_MX_MAX_CTXT<<SHIFT_CTXT)
#define NEM_MX_PGRANK_MASK   (NEM_MX_MAX_PGRANK<<SHIFT_PGRANK)

#define NEM_MX_MATCH_INTRA      (UINT64_C(0x8000000000000000))
#define NEM_MX_MATCH_DIRECT     (UINT64_C(0x0000000000000000))
#define NEM_MX_MATCH_FULL_MASK  (UINT64_C(0xffffffffffffffff))
#define NEM_MX_MATCH_EMPTY_MASK (UINT64_C(0x0000000000000000))
#define NEM_MX_MASK             (UINT64_C(0x8000000000000000))

#define NEM_MX_SET_TAG(_match, _tag)  do {                                  \
	MPIU_Assert((_tag >= 0)&&(_tag <= (NEM_MX_MAX_TAG)));		    \
        ((_match) |= (((uint64_t)((_tag)&(NEM_MX_MAX_TAG))) << SHIFT_TAG)); \
}while(0)
#define NEM_MX_SET_SRC(_match, _src) do {                      \
        MPIU_Assert((_src >= 0)&&(_src<=(NEM_MX_MAX_RANK)));   \
        ((_match) |= (((uint64_t)(_src)) << SHIFT_RANK));      \
}while(0)
#define NEM_MX_SET_CTXT(_match, _ctxt) do {                     \
        MPIU_Assert((_ctxt >= 0)&&(_ctxt<=(NEM_MX_MAX_CTXT)));  \
        ((_match) |= (((uint64_t)(_ctxt)) << SHIFT_CTXT));      \
}while(0)
#define NEM_MX_SET_PGRANK(_match, _pg_rank)  do {               \
	((_match) |= (((uint64_t)(_pg_rank)) << SHIFT_PGRANK));	\
}while(0)
#define NEM_MX_SET_ANYSRC(_match) do{   \
	((_match) &= ~NEM_MX_RANK_MASK);\
}while(0)
#define NEM_MX_SET_ANYTAG(_match) do{   \
	((_match) &= ~NEM_MX_TAG_MASK); \
}while(0)

#define NEM_MX_MATCH_GET_TYPE(_match, _type) do{                                 \
	((_type) = ((int16_t)(((_match) & NEM_MX_TYPE_MASK) >> SHIFT_TYPE)));    \
}while(0)
#define NEM_MX_MATCH_GET_TAG(_match, _tag)   do{                                 \
        ((_tag)  = ((Mx_Nem_tag_t)(((_match) & NEM_MX_TAG_MASK)  >> SHIFT_TAG)));\
}while(0)
#define NEM_MX_MATCH_GET_RANK(_match, _rank) do{                                 \
	((_rank) = ((MPIR_Rank_t)(((_match) & NEM_MX_RANK_MASK) >> SHIFT_RANK)));\
}while(0)
#define NEM_MX_MATCH_GET_CTXT(_match, _ctxt) do{                                 \
((_ctxt) = ((MPIR_Context_id_t)(((_match) & NEM_MX_CTXT_MASK) >> SHIFT_CTXT)));  \
}while(0)
#define NEM_MX_MATCH_GET_PGRANK(_match, _pg_rank) do{                            \
	((_pg_rank) = ((int)(((_match) & NEM_MX_PGRANK_MASK) >> SHIFT_PGRANK))); \
}while(0)

#ifdef USE_CTXT_AS_MARK
#define NEM_MX_INTRA_CTXT (0x0000000c)
#define NEM_MX_SET_MATCH(_match,_tag,_rank,_context ) do{              \
	   MPIU_Assert((_tag >= 0)&&(_tag <= (NEM_MX_MAX_TAG)));       \
           MPIU_Assert((_rank >= 0)&&(_rank<=(NEM_MX_MAX_RANK)));      \
	   MPIU_Assert((_context >= 0)&&(_context<=(NEM_MX_MAX_CTXT)));\
           (_match)=((((uint64_t)(_tag))     << SHIFT_TAG)	       \
                    |(((uint64_t)(_rank))    << SHIFT_RANK)            \
                    |(((uint64_t)(_context)) << SHIFT_CTXT));          \
}while(0) 
#define NEM_MX_DIRECT_MATCH(_match,_tag,_rank,_context)  NEM_MX_SET_MATCH(_match,_tag,_rank,_context)
#define NEM_MX_ADI_MATCH(_match)                         NEM_MX_SET_MATCH(_match,0,0,NEM_MX_INTRA_CTXT)
#else /* USE_CTXT_AS_MARK */
#define NEM_MX_DIRECT_TYPE (0x0)
#define NEM_MX_INTRA_TYPE  (0x1)
#define NEM_MX_SET_MATCH(_match,_type,_tag,_rank,_context ) do{            \
	   MPIU_Assert((_tag >= 0)&&(_tag <= (NEM_MX_MAX_TAG)));	   \
           MPIU_Assert((_rank >= 0)&&(_rank<=(NEM_MX_MAX_RANK)));	   \
	   MPIU_Assert((_context >= 0)&&(_context<=(NEM_MX_MAX_CTXT)));    \
           (_match)=((((uint64_t) (_type))                 << SHIFT_TYPE)  \
                    |(((uint64_t)((_tag)&(NEM_MX_MAX_TAG)))<< SHIFT_TAG)   \
                    |(((uint64_t) (_rank))                 << SHIFT_RANK)  \
                    |(((uint64_t) (_context))              << SHIFT_CTXT));\
}while(0) 
#define NEM_MX_DIRECT_MATCH(_match,_tag,_rank,_context)  NEM_MX_SET_MATCH(_match,NEM_MX_DIRECT_TYPE,_tag,_rank,_context)
#define NEM_MX_ADI_MATCH(_match)                         NEM_MX_SET_MATCH(_match,NEM_MX_INTRA_TYPE,0,0,0)
#endif /* USE_CTXT_AS_MARK */

#endif 


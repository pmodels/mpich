/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#include "newmad_impl.h"
#include "newmad_extended_interface.h"
#include "my_papi_defs.h"
#include "../mx/uthash.h"

typedef struct mpid_nem_nmad_hash_struct {
    MPID_Request    *mpid_req_ptr;
    nm_sr_request_t *nmad_req_ptr;
    UT_hash_handle  hh;
}mpid_nem_nmad_hash_t;

static mpid_nem_nmad_hash_t *mpid_nem_nmad_asreqs = NULL;
#define MPID_MEM_NMAD_ADD_REQ_IN_HASH(_mpi_req,_nmad_req) do{		            \
	mpid_nem_nmad_hash_t *s;	       				            \
	s = MPIU_Malloc(sizeof(mpid_nem_nmad_hash_t));			            \
	s->mpid_req_ptr = (_mpi_req);					            \
	s->nmad_req_ptr = (_nmad_req);					            \
	HASH_ADD(hh, mpid_nem_nmad_asreqs, mpid_req_ptr, sizeof(MPID_Request*), s); \
    }while(0)
#define MPID_NEM_NMAD_GET_REQ_FROM_HASH(_mpi_req_ptr,_nmad_req) do{		                                    \
	mpid_nem_nmad_hash_t *s;						                                    \
	HASH_FIND(hh, mpid_nem_nmad_asreqs, &(_mpi_req_ptr), sizeof(MPID_Request*), s);                             \
	if(s){HASH_DELETE(hh, mpid_nem_nmad_asreqs, s); (_nmad_req) = s->nmad_req_ptr; MPIU_Free(s);} else {(_nmad_req) = NULL;} \
    }while(0)

static int  MPID_nem_newmad_handle_rreq(MPID_Request *req, nm_tag_t match_info, size_t size);
static void MPID_nem_newmad_handle_sreq(MPID_Request *req);

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_get_adi_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void 
MPID_nem_newmad_get_adi_msg(nm_sr_event_t event, const nm_sr_event_info_t*info)
{
    nm_tag_t          match_info = info->recv_unexpected.tag;
    MPIR_Context_id_t ctxt;

#ifdef DEBUG
    fprintf(stdout,"===> Getting ADI MSG match is %lx \n",match_info);
#endif

    NEM_NMAD_MATCH_GET_CTXT(match_info, ctxt);
    if(ctxt == NEM_NMAD_INTRA_CTXT)
    {
        MPID_nem_newmad_internal_req_t *rreq;
	mpid_nem_newmad_p_gate_t        from   = info->recv_unexpected.p_gate;
	int                             length = info->recv_unexpected.len; 
	void                           *data;

        MPID_nem_newmad_internal_req_dequeue(&rreq);
        rreq->kind = MPID_REQUEST_RECV;   
        rreq->vc = nm_gate_ref_get(from);
       
        if(length <=  sizeof(MPIDI_CH3_PktGeneric_t)) 
	{
	  data = (char*)&(rreq->pending_pkt);
	}
       else
       {
	  rreq->tmpbuf = MPIU_Malloc(length);
	  MPIU_Assert(rreq->tmpbuf);
	  rreq->tmpbuf_sz = length;                   
	  data = (char*)(rreq->tmpbuf);
       }
	
	nm_sr_irecv_with_ref_tagged(mpid_nem_newmad_session, from, match_info, 
				    NEM_NMAD_MATCH_FULL_MASK, data,length, &(rreq->newmad_req),(void *)rreq);	
    }
    return;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_directRecv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_directRecv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_DIRECTRECV);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_DIRECTRECV);    
    
    if (!VC_CH(vc)->is_local)
    {
	nm_tag_t          match_info = 0; 
	nm_tag_t          match_mask = NEM_NMAD_MATCH_FULL_MASK; 	    
	MPIR_Rank_t       source     = rreq->dev.match.parts.rank;
	MPIR_Context_id_t context    = rreq->dev.match.parts.context_id;
	Nmad_Nem_tag_t    tag        = rreq->dev.match.parts.tag;
	int               ret;
	MPIDI_msg_sz_t    data_sz;
	int               dt_contig;
	MPI_Aint          dt_true_lb;
	MPID_Datatype    *dt_ptr;
	
	NEM_NMAD_DIRECT_MATCH(match_info,0,source,context);
	if (tag != MPI_ANY_TAG)
	{
	    NEM_NMAD_SET_TAG(match_info,tag);
	}
	else
	{
	    NEM_NMAD_SET_ANYTAG(match_info);
	    NEM_NMAD_SET_ANYTAG(match_mask);
	}

#ifdef DEBUG
	fprintf(stdout,"========> Posting Recv req  %p (match is %lx) \n",rreq,match_info);
#endif
	MPIDI_Datatype_get_info(rreq->dev.user_count,rreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);
	rreq->dev.OnDataAvail = NULL;

	if (dt_contig)
	{
	    ret = nm_sr_irecv_with_ref_tagged(mpid_nem_newmad_session,VC_FIELD(vc,p_gate),match_info,match_mask,
					      (char*)(rreq->dev.user_buf) + dt_true_lb,data_sz,
					      &(REQ_FIELD(rreq,newmad_req)),(void*)rreq);
	    REQ_FIELD(rreq,iov) = NULL;
	}
	else
	{
	    int           num_seg        = 0;
	    struct iovec *newmad_iov     = (struct iovec *)MPIU_Malloc(NMAD_IOV_MAX_DEPTH*sizeof(struct iovec));	    
	    struct iovec *newmad_iov_ptr = &(newmad_iov[0]); 
	    MPID_nem_newmad_process_rdtype(&rreq,dt_ptr,data_sz,&newmad_iov_ptr,&num_seg);
	    MPIU_Assert(num_seg <= NMAD_IOV_MAX_DEPTH);
#ifdef DEBUG
	    {
		int index;
		for(index = 0; index < num_seg ; index++)
		    {
			fprintf(stdout,"======================\n");
			fprintf(stdout,"RECV nmad_iov[%i]: [base %p][len %i]\n",index,
				newmad_iov[index].iov_base,newmad_iov[index].iov_len);
		    }
	    }
#endif
	    ret = nm_sr_irecv_iov_with_ref_tagged(mpid_nem_newmad_session,VC_FIELD(vc,p_gate),match_info,match_mask,
						  newmad_iov,num_seg,&(REQ_FIELD(rreq,newmad_req)),(void*)rreq);	
	    REQ_FIELD(rreq,iov) = newmad_iov;
	}
    }
    else
    {
	/* Fixme : this might not work in the case of multiple netmods */ 
	memset((&(REQ_FIELD(rreq,newmad_req))),0,sizeof(nm_sr_request_t));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTRECV);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_newmad_poll(int in_blocking_poll)
{
   nm_sr_request_t *p_request = NULL;
   nm_tag_t         match_info = 0;
   int mpi_errno = MPI_SUCCESS;   
   
   nm_sr_send_success(mpid_nem_newmad_session, &p_request);
   if (p_request != NULL)
   {
      MPID_nem_newmad_unified_req_t *ref;
      MPID_Request                  *req;
      MPID_Request_kind_t            kind;      
      MPIR_Context_id_t              ctxt;
      
      nm_sr_get_stag(mpid_nem_newmad_session,p_request, &match_info);
      NEM_NMAD_MATCH_GET_CTXT(match_info, ctxt);
            
      nm_sr_get_ref(mpid_nem_newmad_session,p_request,(void *)&ref);
      req = &(ref->mpi_req);
      MPIU_Assert(req != NULL);
      kind = req->kind;
	
      if(ctxt == NEM_NMAD_INTRA_CTXT)
      {
	 if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
	 {
	    MPID_nem_newmad_handle_sreq(req);	    
	 }
      }
      else
      {
	 if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
	 {
	    MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);                  
	    MPID_nem_newmad_handle_sreq(req);	    
	 }
      }   
   }
   
   nm_sr_recv_success(mpid_nem_newmad_session, &p_request);
   if (p_request != NULL)
   {
      MPID_nem_newmad_unified_req_t *ref;
      MPID_Request                  *req;
      MPID_Request_kind_t            kind;      
      MPIR_Context_id_t              ctxt;
      size_t                         size;
            
      nm_sr_get_ref(mpid_nem_newmad_session,p_request,(void *)&ref);
      req = &(ref->mpi_req);
      MPIU_Assert(req != NULL);
      kind = req->kind;
      nm_sr_get_size(mpid_nem_newmad_session, p_request, &size);
      nm_sr_get_rtag(mpid_nem_newmad_session,p_request, &match_info);
      NEM_NMAD_MATCH_GET_CTXT(match_info, ctxt);
	
      if(ctxt == NEM_NMAD_INTRA_CTXT)
	  {
	 MPID_nem_newmad_internal_req_t *adi_req = &(ref->nem_newmad_req);
	 if (kind == MPID_REQUEST_RECV)
	 {
	    if (size <= sizeof(MPIDI_CH3_PktGeneric_t))
	    {
	       MPID_nem_handle_pkt(adi_req->vc,(char *)&(adi_req->pending_pkt),(MPIDI_msg_sz_t)(size));
	    }
	    else
	    {
	       MPID_nem_handle_pkt(adi_req->vc,(char *)(adi_req->tmpbuf),(MPIDI_msg_sz_t)(adi_req->tmpbuf_sz));
	       MPIU_Free(adi_req->tmpbuf);
	    }
	    nm_core_disable_progression(mpid_nem_newmad_session->p_core);
	    MPID_nem_newmad_internal_req_enqueue(adi_req);
	    nm_core_enable_progression(mpid_nem_newmad_session->p_core);
	 }
	 else
	 {
	    MPIU_Assert(0);
	 }	 
      }
      else
      {
	 if ((kind == MPID_REQUEST_RECV) || (kind == MPID_PREQUEST_RECV))
	 {
	    int found = FALSE;
	    nm_sr_request_t *nmad_request = NULL;	       
	    MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);
	    MPIU_THREAD_CS_ENTER(MSGQUEUE,req);
	    MPID_NEM_NMAD_GET_REQ_FROM_HASH(req,nmad_request);
	    if(nmad_request != NULL)
	    {
	       MPIU_Assert(req->dev.match.parts.rank == MPI_ANY_SOURCE);
	       MPIU_Free(nmad_request);
	    }
	    found = MPIDI_CH3U_Recvq_DP(req);
	    if(found){
		MPID_nem_newmad_handle_rreq(req,match_info,size);
	    }
	    MPIU_THREAD_CS_EXIT(MSGQUEUE,req);
	 }
	 else
	 {
	    fprintf(stdout, ">>>>>>>>>>>>> ERROR: Wrong req type : %i (%p)\n",(int)kind,req);
	    MPIU_Assert(0);
	 }
      }   
   }

 fn_exit:
   return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
   goto fn_exit;   
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_handle_sreq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_newmad_handle_sreq(MPID_Request *req)
{
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
#ifdef DEBUG
    fprintf(stdout,"========> Completing Send req  %p \n",req);
#endif
    reqFn = req->dev.OnDataAvail;
    if (!reqFn){
	MPIDI_CH3U_Request_complete(req);
	MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }
    else{
	MPIDI_VC_t *vc = req->ch.vc;
	int complete   = 0;
	reqFn(vc, req, &complete);
	if(!complete)
        {   
	   MPIU_Assert(complete == TRUE);
	}
    }
    if (REQ_FIELD(req,iov) != NULL)
	MPIU_Free((REQ_FIELD(req,iov)));
    mpid_nem_newmad_pending_send_req--;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_new_handle_rreq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
MPID_nem_newmad_handle_rreq(MPID_Request *req, nm_tag_t match_info, size_t size)
{
    int            mpi_errno = MPI_SUCCESS;
    int            complete = FALSE;
    int            dt_contig;
    MPI_Aint       dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype *dt_ptr;
    MPIDI_msg_sz_t data_sz;
    MPIDI_VC_t    *vc = NULL;

#ifdef DEBUG
   fprintf(stdout,"========> Completing Recv req  %p (match is %lx) \n",req,match_info);
#endif

    NEM_NMAD_MATCH_GET_RANK(match_info,req->status.MPI_SOURCE);
    NEM_NMAD_MATCH_GET_TAG(match_info,req->status.MPI_TAG);
    req->status.count = size;
    req->dev.recv_data_sz = size;

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, userbuf_sz, dt_ptr, dt_true_lb);

    if (size <=  userbuf_sz) {
	data_sz = req->dev.recv_data_sz;
    }
    else
    {
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
					    "receive buffer too small; message truncated, msg_sz="
					    MPIDI_MSG_SZ_FMT ", userbuf_sz="
					    MPIDI_MSG_SZ_FMT,
					    req->dev.recv_data_sz, userbuf_sz));
	req->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS,
						     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
						     "**truncate", "**truncate %d %d %d %d",
						     req->status.MPI_SOURCE, req->status.MPI_TAG,
						     req->dev.recv_data_sz, userbuf_sz );
	req->status.count = userbuf_sz;
	data_sz = userbuf_sz;
    }
    
    if ((!dt_contig)&&(req->dev.tmpbuf != NULL))
    {
	MPIDI_msg_sz_t last;
	last = req->dev.recv_data_sz;
	MPID_Segment_unpack( req->dev.segment_ptr, 0, &last, req->dev.tmpbuf);
	MPIU_Free(req->dev.tmpbuf);
	if (last != data_sz) {
	    req->status.count = (int)last;
	    if (req->dev.recv_data_sz <= userbuf_sz) {
		MPIU_ERR_SETSIMPLE(req->status.MPI_ERROR,MPI_ERR_TYPE,"**dtypemismatch");
	    }
	}
    }

    if (REQ_FIELD(req,iov) != NULL)
	MPIU_Free(REQ_FIELD(req,iov));	

    MPIDI_Comm_get_vc_set_active(req->comm, req->status.MPI_SOURCE, &vc);
    MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
    MPIU_Assert(complete == TRUE);


#ifdef DEBUG
   fprintf(stdout,"========> Completing Recv req  %p done \n",req);
#endif

 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
	goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_anysource_posted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_newmad_anysource_posted(MPID_Request *rreq)
{
    /* This function is called whenever an anyource request has been
       posted to the posted receive queue.  */
    MPIR_Context_id_t context;
    Nmad_Nem_tag_t    tag;
    nm_tag_t          match_info  = 0;
    nm_tag_t          match_mask  = NEM_NMAD_MATCH_FULL_MASK; 
    nm_sr_request_t  *newmad_req  = MPIU_Malloc(sizeof(nm_sr_request_t));
    int               num_seg     = 1;
    int               ret;
    MPIDI_msg_sz_t    data_sz;
    int               dt_contig;
    MPI_Aint          dt_true_lb;
    MPID_Datatype    *dt_ptr;               
    struct iovec     *newmad_iov  = (struct iovec *)MPIU_Malloc(NMAD_IOV_MAX_DEPTH*sizeof(struct iovec));
  
    tag     = rreq->dev.match.parts.tag;
    context = rreq->dev.match.parts.context_id;                       
    NEM_NMAD_DIRECT_MATCH(match_info,0,0,context);
    if (tag != MPI_ANY_TAG)
    {
	NEM_NMAD_SET_TAG(match_info,tag);	
    }
    else
    {
	NEM_NMAD_SET_ANYTAG(match_info);
	NEM_NMAD_SET_ANYTAG(match_mask); 
    }
    NEM_NMAD_SET_ANYSRC(match_info);
    NEM_NMAD_SET_ANYSRC(match_mask);

#ifdef DEBUG
    fprintf(stdout,"========> Any Source : Posting Recv req  %p (nmad req is %p) (match is %lx) (mask is %lx) \n",
	    rreq,newmad_req,match_info,match_mask);
#endif

    MPIDI_Datatype_get_info(rreq->dev.user_count,rreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);
    rreq->dev.OnDataAvail = NULL;
    
    if (dt_contig)
    {
	newmad_iov[0].iov_base = (char*)(rreq->dev.user_buf) + dt_true_lb;
	newmad_iov[0].iov_len  = data_sz;
    }
    else
    {
	struct iovec *newmad_iov_ptr = &(newmad_iov[0]); 
	MPID_nem_newmad_process_rdtype(&rreq,dt_ptr,data_sz,&newmad_iov_ptr,&num_seg);
    }

    ret = nm_sr_irecv_iov_with_ref_tagged(mpid_nem_newmad_session,NM_ANY_GATE,match_info,match_mask,
					  newmad_iov,num_seg,newmad_req,(void*)rreq);	
    REQ_FIELD(rreq,iov) = newmad_iov;    
    MPID_MEM_NMAD_ADD_REQ_IN_HASH(rreq,newmad_req);  
    /*
      #ifdef DEBUG
      fprintf(stdout,"========> Any Source : callback end \n");
      #endif
    */
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_anysource_matched
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_anysource_matched(MPID_Request *rreq)
{
    /* This function is called when an anysource request in the posted
       receive queue is matched and dequeued */
    nm_sr_request_t *nmad_request = NULL;
    int ret;
    int matched = FALSE;

#ifdef DEBUG
    fprintf(stdout,"========> Any Source : MPID_nem_newmad_anysource_matched , req is %p\n",rreq);
#endif

    MPID_NEM_NMAD_GET_REQ_FROM_HASH(rreq,nmad_request);

    if(nmad_request != NULL)
    {	

#ifdef DEBUG
	fprintf(stdout,"========> Any Source nmad req found :%p \n",nmad_request);
#endif
	ret = nm_sr_rcancel(mpid_nem_newmad_session,nmad_request);
	if (ret !=  NM_ESUCCESS)
	{

#ifdef DEBUG
	    fprintf(stdout,"========> Any Source nmad req (%p) not cancelled \n",nmad_request);
#endif
	    size_t size;
	    nm_tag_t match_info;
	    MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);                  	
	    ret = nm_sr_rwait(mpid_nem_newmad_session,nmad_request);
	    MPIU_Assert(ret ==  NM_ESUCCESS);
	    nm_sr_request_unset_completion_queue(mpid_nem_newmad_session,nmad_request);
	    nm_sr_get_rtag(mpid_nem_newmad_session,nmad_request,&match_info);
	    nm_sr_get_size(mpid_nem_newmad_session,nmad_request,&size);
	    MPID_nem_newmad_handle_rreq(rreq,match_info, size);
	    matched = TRUE;
	}
	else
	{
	    MPID_Segment_free(rreq->dev.segment_ptr);
	    if (REQ_FIELD(rreq,iov) != NULL)
	      MPIU_Free(REQ_FIELD(rreq,iov));	
	}    
	MPIU_Free(nmad_request);
    }    
    return matched;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_process_rdtype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_process_rdtype(MPID_Request **rreq_p, MPID_Datatype * dt_ptr, MPIDI_msg_sz_t data_sz, struct iovec *newmad_iov[], int *num_iov)
{
    MPID_Request  *rreq      = *rreq_p;
    MPIDI_msg_sz_t last;
    MPID_IOV      *iov;
    int            n_iov     = 0;
    int            mpi_errno = MPI_SUCCESS;
    int            index;
    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_RDTYPE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_RDTYPE);

    if (rreq->dev.segment_ptr == NULL)
    {
	rreq->dev.segment_ptr = MPID_Segment_alloc( );
	MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    }
    MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = data_sz;
    last = rreq->dev.segment_size;

    MPID_Segment_count_contig_blocks(rreq->dev.segment_ptr,rreq->dev.segment_first,&last,&n_iov);
    MPIU_Assert(n_iov > 0);
    iov = MPIU_Malloc(n_iov*sizeof(MPID_IOV));

    MPID_Segment_unpack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last,iov, &n_iov);
    MPIU_Assert(last == rreq->dev.segment_size);

#ifdef DEBUG
    for(index = 0; index < n_iov ; index++)
	{
	    fprintf(stdout,"======================\n");
	    fprintf(stdout,"RECV iov[%i]: [base %p][len %i]\n",index,
		    iov[index].MPID_IOV_BUF,iov[index].MPID_IOV_LEN);
	}
#endif

    if(n_iov <= NMAD_IOV_MAX_DEPTH) 
    {
	for(index=0; index < n_iov ; index++)
	{
	    (*newmad_iov)[index].iov_base = iov[index].MPID_IOV_BUF;
	    (*newmad_iov)[index].iov_len  = iov[index].MPID_IOV_LEN;
	}
	rreq->dev.tmpbuf = NULL;
	*num_iov = n_iov;
    }
    else
    {
	int packsize = 0;
	MPIR_Pack_size_impl(rreq->dev.user_count, rreq->dev.datatype, &packsize);
	rreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
	MPIU_Assert(rreq->dev.tmpbuf);
	rreq->dev.tmpbuf_sz = packsize;
	(*newmad_iov)[0].iov_base = (char *)  rreq->dev.tmpbuf;
	(*newmad_iov)[0].iov_len  = (uint32_t) packsize;
	*num_iov = 1 ;
    }
    MPIU_Free(iov);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_RDTYPE);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}




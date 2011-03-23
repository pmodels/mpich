/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */



#include "mx_impl.h"
#include "my_papi_defs.h"
#include "uthash.h"

typedef struct mpid_nem_mx_hash_struct {
  MPID_Request   *mpid_req_ptr;  
  MPIDI_VC_t     *vc;  
  mx_request_t   *mx_req_ptr;   
  UT_hash_handle  hh1,hh2;
}mpid_nem_mx_hash_t;

static mpid_nem_mx_hash_t *mpid_nem_mx_asreqs = NULL;
#define MPID_MEM_MX_ADD_REQ_IN_HASH(_mpi_req,_mx_req) do{		       \
    mpid_nem_mx_hash_t *s;						       \
    s = MPIU_Malloc(sizeof(mpid_nem_mx_hash_t));			       \
    s->mpid_req_ptr = (_mpi_req);					       \
    s->mx_req_ptr = (_mx_req);						       \
    HASH_ADD(hh1, mpid_nem_mx_asreqs, mpid_req_ptr, sizeof(MPID_Request*), s); \
}while(0)
#define MPID_NEM_MX_GET_REQ_FROM_HASH(_mpi_req_ptr,_mx_req) do{		                                              \
    mpid_nem_mx_hash_t *s;						                                              \
    HASH_FIND(hh1, mpid_nem_mx_asreqs, &(_mpi_req_ptr), sizeof(MPID_Request*), s);                                    \
    if(s){HASH_DELETE(hh1, mpid_nem_mx_asreqs, s); (_mx_req) = s->mx_req_ptr; MPIU_Free(s);} else {(_mx_req) = NULL;} \
}while(0)

static mpid_nem_mx_hash_t *mpid_nem_mx_connreqs ATTRIBUTE((unused, used))= NULL; 
#define MPID_MEM_MX_ADD_VC_IN_HASH(_vc,_mx_req) do{      		       \
    mpid_nem_mx_hash_t *s;						       \
    s = MPIU_Malloc(sizeof(mpid_nem_mx_hash_t));			       \
    s->vc = (_vc);               					       \
    s->mx_req_ptr = (_mx_req);				   		       \
    HASH_ADD(hh2, mpid_nem_mx_connreqs, mpid_req_ptr, sizeof(MPID_Request*), s); \
}while(0)
#define MPID_NEM_MX_REM_VC_FROM_HASH(_vc,_mx_req) do{		                                           \
    mpid_nem_mx_hash_t *s;						                                   \
    HASH_FIND(hh2, mpid_nem_mx_connreqs, &(_mpi_req_ptr), sizeof(MPID_Request*), s);                       \
    if(s){HASH_DELETE(hh2, mpid_nem_mx_connreqs, s); (_mx_req) = s->mx_req_ptr; } else {(_mx_req) = NULL;} \
}while(0)
#define MPID_NEM_MX_IS_VC_IN_HASH(_vc,_ret) do{          				\
    mpid_nem_mx_hash_t *s;				                             \
    HASH_FIND(hh2, mpid_nem_mx_connreqs, &(_mpi_req_ptr), sizeof(MPID_Request*), s); \
    if(s){ (_ret) = 1; } else {(_ret) = 0;}                                   \
}while(0)

static int MPID_nem_mx_handle_sreq(MPID_Request *sreq);
static int MPID_nem_mx_handle_rreq(MPID_Request *rreq, mx_status_t status);

/* This routine cannot manipulate MPI message queues or request queues */
#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_get_adi_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
mx_unexp_handler_action_t MPID_nem_mx_get_adi_msg(void *context, mx_endpoint_addr_t source, uint64_t match_info,
                                                  uint32_t length, void *data)
{
  MPIDI_VC_t *vc;
#ifdef USE_CTXT_AS_MARK
  MPIR_Context_id_t ctxt;
  NEM_MX_MATCH_GET_CTXT(match_info, ctxt);
#else
  int16_t type;	    
  NEM_MX_MATCH_GET_TYPE(match_info,type);    
#endif

#ifdef ONDEMAND
  mx_get_endpoint_addr_context(source,(void **)(&vc));	    
  
  if (vc == NULL)
  {
#ifdef USE_CTXT_AS_MARK	
    if(ctxt == NEM_MX_INTRA_CTXT) 
#else
    if(type == NEM_MX_INTRA_TYPE)
#endif
    {
      mx_return_t   ret;
      uint32_t      result;
      uint64_t      remote_nic_id;
      uint32_t      remote_endpoint_id;
      int           pg_rank;
      char          pg_id[MPID_NEM_MAX_NETMOD_STRING_LEN];
      MPIDI_PG_t   *pg;
      mx_request_t  mx_request;
      mx_status_t   status;
      
      remote_nic_id = *((uint64_t *)data);
      remote_endpoint_id = *((uint32_t *)((char *)data+sizeof(uint64_t)));
      MPIU_Memcpy(pg_id,(char *)data+sizeof(uint64_t)+sizeof(uint32_t),length-sizeof(uint64_t)-sizeof(uint32_t));
      
      MPIDI_PG_Find (pg_id, &pg);
      NEM_MX_MATCH_GET_PGRANK(match_info,pg_rank);
      MPIDI_PG_Get_vc_set_active(pg, pg_rank, &vc);	    
      
      fprintf(stdout,"[%i]=== NULL VC : Receiver Unex Got infos (%lx) from  Sender (in data) ..%li %i %s %p\n", 
	      MPID_nem_mem_region.rank,match_info,remote_nic_id,remote_endpoint_id,pg_id,vc);
      ret = mx_iconnect(MPID_nem_mx_local_endpoint, remote_nic_id,remote_endpoint_id ,
			  MPID_NEM_MX_FILTER,match_info,NULL,&mx_request);
      MPIU_Assert(ret == MX_SUCCESS);
      
      /*fprintf(stdout,"[%i]=== NULL VC : Receiver IConnect posted  \n", MPID_nem_mem_region.rank); */

      do{
	ret = mx_test(MPID_nem_mx_local_endpoint,&mx_request,&status,&result);
      }while((result == 0) && (ret == MX_SUCCESS));
      MPIU_Assert(ret == MX_SUCCESS);

      VC_FIELD(vc, remote_connected) = 1;
      VC_FIELD(vc, local_connected) = 1;
      VC_FIELD(vc, remote_endpoint_addr) = status.source;
      mx_set_endpoint_addr_context(VC_FIELD(vc, remote_endpoint_addr),(void *)vc);
      fprintf(stdout,"[%i]=== NULL VC : Receiver IConnect done \n", MPID_nem_mem_region.rank);


      return MX_RECV_FINISHED;
    }
    else
#ifndef  USE_CTXT_AS_MARK
if(type == NEM_MX_DIRECT_TYPE)
#endif
    {
      fprintf(stdout,"[%i]=== NULL VC : callback for direct %lx  \n", MPID_nem_mem_region.rank,match_info);
      return MX_RECV_CONTINUE;
    }
#ifndef  USE_CTXT_AS_MARK 
    else
    {	    
      fprintf(stdout,"Unknown Message Type :%i, aborting ...\n",type);
      MPIU_Assert(0);
      abort();
    }
#endif
  }
  else {
#endif
#ifdef USE_CTXT_AS_MARK	
    if(ctxt == NEM_MX_INTRA_CTXT) 
#else
    if (type == NEM_MX_INTRA_TYPE)
#endif
    {
      MPID_nem_mx_internal_req_t *rreq;
      mx_request_t                mx_request;
      mx_segment_t                iov;
      mx_return_t                 ret;			    
      
#ifdef ONDEMAND
      if (VC_FIELD(vc, remote_connected) == 0)
      {
	mx_return_t  ret;
	uint64_t     remote_nic_id;
	uint32_t     remote_endpoint_id;
	int          pg_rank;
	char         pg_id[MPID_NEM_MAX_NETMOD_STRING_LEN];
	MPIDI_PG_t  *pg;
        uint32_t     result;
	
	remote_nic_id = *((uint64_t *)data);
	remote_endpoint_id = *((uint32_t *)((char *)data+sizeof(uint64_t)));
	MPIU_Memcpy(pg_id,(char *)data+sizeof(uint64_t)+sizeof(uint32_t),length-sizeof(uint64_t)-sizeof(uint32_t));   
	
	fprintf(stdout,"[%i]=== NOT NULL VC : Receiver Unex Got infos from  Sender (in data) ..%li %i %s vc is %p\n", 
		MPID_nem_mem_region.rank,remote_nic_id,remote_endpoint_id,pg_id,vc);
	
	MPIDI_PG_Find (pg_id, &pg);
	NEM_MX_MATCH_GET_PGRANK(match_info,pg_rank);
	MPIDI_PG_Get_vc_set_active(pg, pg_rank, &vc);
	
	if(VC_FIELD(vc, local_connected) == 0)
	{			
	  mx_request_t mx_request;
	  mx_status_t  status;
	  ret = mx_iconnect(MPID_nem_mx_local_endpoint,remote_nic_id,remote_endpoint_id,
			    MPID_NEM_MX_FILTER,match_info,NULL,&mx_request);
	  MPIU_Assert(ret == MX_SUCCESS);
	  do{
	    ret = mx_test(MPID_nem_mx_local_endpoint,&mx_request,&status,&result);
	  }while((result == 0) && (ret == MX_SUCCESS));
	  MPIU_Assert(ret == MX_SUCCESS);
	  
	  VC_FIELD(vc, remote_endpoint_addr) = status.source;
	  mx_set_endpoint_addr_context(VC_FIELD(vc, remote_endpoint_addr),(void *)vc);
	  VC_FIELD(vc, local_connected) = 1;
	}
	VC_FIELD(vc, remote_connected) = 1;
	fprintf(stdout,"[%i]=== NOT NULL VC : Receiver FULLY Connected with peer \n", MPID_nem_mem_region.rank);
	
	return MX_RECV_FINISHED;		
      }
#endif

      MPID_nem_mx_internal_req_dequeue(&rreq);
      rreq->kind = MPID_REQUEST_RECV;	
      mx_get_endpoint_addr_context(source,(void **)(&vc));	    
      rreq->vc = vc;

      if(length <=  sizeof(MPIDI_CH3_PktGeneric_t)) {
	iov.segment_ptr = (char*)&(rreq->pending_pkt);
      }
      else{
	rreq->tmpbuf = MPIU_Malloc(length);
	MPIU_Assert(rreq->tmpbuf);
	rreq->tmpbuf_sz = length;		    
	iov.segment_ptr = (char*)(rreq->tmpbuf);
      }
      iov.segment_length = length;

      ret = mx_irecv(MPID_nem_mx_local_endpoint,&iov,1,match_info,NEM_MX_MATCH_FULL_MASK,(void *)rreq,&mx_request);
      MPIU_Assert(ret == MX_SUCCESS);
      
      return MX_RECV_CONTINUE;
    }
    else 
#ifndef  USE_CTXT_AS_MARK
if (type == NEM_MX_DIRECT_TYPE)
#endif
    {
      /* Do nothing:                              */
      /* This shall be eventually matched in recv */	
      return MX_RECV_CONTINUE;
    }
#ifndef  USE_CTXT_AS_MARK
    else
    {
      fprintf(stdout,"Unknown Message Type :%i, aborting ...\n",type);
      MPIU_Assert(0);
      abort();
    }
#endif
#ifdef ONDEMAND
  }
#endif
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_directRecv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_directRecv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
  int mpi_errno = MPI_SUCCESS;
    
  MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_DIRECTRECV);    
  MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_DIRECTRECV);    
  
  if (!VC_CH(vc)->is_local)
  {
      uint64_t          match_info = NEM_MX_MATCH_DIRECT;
      uint64_t          match_mask = NEM_MX_MATCH_FULL_MASK;
      MPIR_Rank_t       source     = rreq->dev.match.parts.rank;       
      MPIR_Context_id_t context    = rreq->dev.match.parts.context_id;		    
      Mx_Nem_tag_t      tag        = rreq->dev.match.parts.tag;
      mx_segment_t      mx_iov[MX_MAX_SEGMENTS];
      mx_return_t       ret;	    
      uint32_t          num_seg = 1;
      MPIDI_msg_sz_t    data_sz;
      int               dt_contig;
      MPI_Aint          dt_true_lb;
      MPID_Datatype    *dt_ptr;   
      /*int            threshold = (vc->eager_max_msg_sz - sizeof(MPIDI_CH3_PktGeneric_t));*/
      
      NEM_MX_DIRECT_MATCH(match_info,0,source,context);
      if (tag == MPI_ANY_TAG)
      {
	NEM_MX_SET_ANYTAG(match_info);
	NEM_MX_SET_ANYTAG(match_mask);
      }
      else
	NEM_MX_SET_TAG(match_info,tag); 		    
      
      MPIDI_Datatype_get_info(rreq->dev.user_count,rreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);	    
      rreq->dev.OnDataAvail = NULL;	    
      
      if (dt_contig)
      {
          mx_iov[0].segment_ptr    = (char *)(rreq->dev.user_buf) + dt_true_lb;
          mx_iov[0].segment_length = data_sz;
      }
      else
	MPID_nem_mx_process_rdtype(&rreq,dt_ptr,data_sz,mx_iov,&num_seg);
      ret = mx_irecv(MPID_nem_mx_local_endpoint,mx_iov,num_seg,match_info,match_mask,(void *)rreq, &(REQ_FIELD(rreq,mx_request)));
      MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_irecv", "**mx_irecv %s", mx_strerror (ret));
  }
  else
  {
    /* Fixme : this might not work in the case of multiple netmods */ 
    memset((&(REQ_FIELD(rreq,mx_request))),0,sizeof(mx_request_t));
  }
   
 fn_exit:
  MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_DIRECTRECV);
  return mpi_errno;
 fn_fail:
  goto fn_exit;
}


#ifndef USE_CTXT_AS_MARK	
#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_poll(int in_blocking_poll)
{
   int           mpi_errno = MPI_SUCCESS;
   mx_status_t   status;
   mx_return_t   ret;
   uint32_t      result;

   /* first check ADI msgs */
   ret = mx_test_any(MPID_nem_mx_local_endpoint,NEM_MX_MATCH_INTRA,NEM_MX_MASK,&status,&result);
   MPIU_Assert(ret == MX_SUCCESS);
   if ((ret == MX_SUCCESS) && (result > 0))
   {
     MPID_nem_mx_unified_req_t *myreq = (MPID_nem_mx_unified_req_t *)(status.context);
     MPID_Request              *req   = &(myreq->mpi_req);
     MPID_Request_kind_t        kind  = req->kind;

     if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
     {	   
       MPID_nem_mx_handle_sreq(req);
     }
     else if (kind == MPID_REQUEST_RECV)	       
     {
       MPID_nem_mx_internal_req_t *adi_req = &(myreq->nem_mx_req);	
       MPIU_Assert(status.code != MX_STATUS_TRUNCATED);	       	
       if(adi_req->vc == NULL)
	  mx_get_endpoint_addr_context(status.source,(void **)(&(adi_req->vc))); 
       if (status.msg_length <= sizeof(MPIDI_CH3_PktGeneric_t))
       {
	 MPID_nem_handle_pkt(adi_req->vc,(char *)&(adi_req->pending_pkt),(MPIDI_msg_sz_t)(status.msg_length));
       }
       else
       {
	 MPID_nem_handle_pkt(adi_req->vc,(char *)(adi_req->tmpbuf),(MPIDI_msg_sz_t)(adi_req->tmpbuf_sz));
	 MPIU_Free(adi_req->tmpbuf);
       }
       mx_disable_progression(MPID_nem_mx_local_endpoint);
       MPID_nem_mx_internal_req_enqueue(adi_req);
       mx_reenable_progression(MPID_nem_mx_local_endpoint);
     }
     else
     {
         /* Error : unknown REQ type */
         MPIU_ERR_CHKINTERNAL(TRUE, mpi_errno, "unknown REQ type");
     }
   }
   
   /* Then check Direct MPI msgs */
   ret = mx_test_any(MPID_nem_mx_local_endpoint,NEM_MX_MATCH_DIRECT,NEM_MX_MASK,&status,&result);
   MPIU_Assert(ret == MX_SUCCESS);
   if ((ret == MX_SUCCESS) && (result > 0))
   {
     MPID_nem_mx_unified_req_t *myreq = (MPID_nem_mx_unified_req_t *)(status.context);
     MPID_Request              *req   = &(myreq->mpi_req);
     MPID_Request_kind_t        kind  = req->kind;
     
     if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
     {	   
       MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);		   
       mpi_errno = MPID_nem_mx_handle_sreq(req);
       if (mpi_errno) MPIU_ERR_POP(mpi_errno);
     }
     else if ((kind == MPID_REQUEST_RECV) || (kind == MPID_PREQUEST_RECV))
     {
       int found = FALSE;
       mx_request_t *mx_request = NULL;
       MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);		   
       MPIU_THREAD_CS_ENTER(MSGQUEUE,req);	   	 
       MPID_NEM_MX_GET_REQ_FROM_HASH(req,mx_request);
       if(mx_request != NULL)
       {
	 MPIU_Assert(req->dev.match.parts.rank == MPI_ANY_SOURCE);
	 MPIU_Free(mx_request);
       }
       found = MPIDI_CH3U_Recvq_DP(req);
       if(found){
	 mpi_errno = MPID_nem_mx_handle_rreq(req, status);
	  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
       }
       MPIU_THREAD_CS_EXIT(MSGQUEUE,req);
     }
     else
     {
         /* Error : unknown REQ type */
         MPIU_ERR_CHKINTERNAL(TRUE, mpi_errno, "unknown REQ type");
     }
   }   
 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;   
}

#else /*USE_CTXT_AS_MARK	 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_poll(int in_blocking_progress)
{
   int           mpi_errno = MPI_SUCCESS;
   mx_status_t   status;
   mx_return_t   ret;
   uint32_t      result;

   ret = mx_test_any(MPID_nem_mx_local_endpoint,NEM_MX_MATCH_EMPTY_MASK,NEM_MX_MATCH_EMPTY_MASK,&status,&result);
   MPIU_Assert(ret == MX_SUCCESS);
   if ((ret == MX_SUCCESS) && (result > 0))
   {
     MPID_nem_mx_unified_req_t *myreq = (MPID_nem_mx_unified_req_t *)(status.context);
     MPID_Request              *req   = &(myreq->mpi_req);
     MPID_Request_kind_t        kind  = req->kind;
     MPIR_Context_id_t          ctxt;

     NEM_MX_MATCH_GET_CTXT(status.match_info, ctxt);
     
     if(ctxt == NEM_MX_INTRA_CTXT)     
     { 
       if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
       {
	 
	 MPID_nem_mx_handle_sreq(req);
       }
       else if (kind == MPID_REQUEST_RECV)	       
      {
      	 MPID_nem_mx_internal_req_t *adi_req = &(myreq->nem_mx_req);
	 MPIU_Assert(status.code != MX_STATUS_TRUNCATED);	       	
	 if(adi_req->vc == NULL)
	   mx_get_endpoint_addr_context(status.source,(void **)(&(adi_req->vc))); 
	 if (status.msg_length <= sizeof(MPIDI_CH3_PktGeneric_t))
	 {
	   MPID_nem_handle_pkt(adi_req->vc,(char *)&(adi_req->pending_pkt),(MPIDI_msg_sz_t)(status.msg_length));
	 }
	 else
	 {
	   MPID_nem_handle_pkt(adi_req->vc,(char *)(adi_req->tmpbuf),(MPIDI_msg_sz_t)(adi_req->tmpbuf_sz));
	   MPIU_Free(adi_req->tmpbuf);
	 }
	 mx_disable_progression(MPID_nem_mx_local_endpoint);
	 MPID_nem_mx_internal_req_enqueue(adi_req);
	 mx_reenable_progression(MPID_nem_mx_local_endpoint);
       }
       else
       {
	 MPIU_Assert(0);
       }
     }
     else
     {
       if ((kind == MPID_REQUEST_SEND) || (kind == MPID_PREQUEST_SEND))
       {	   
	 MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);		   
	 mpi_errno = MPID_nem_mx_handle_sreq(req);
	 if (mpi_errno) MPIU_ERR_POP(mpi_errno);
       }
       else if ((kind == MPID_REQUEST_RECV) || (kind == MPID_PREQUEST_RECV))
       {
	 int found = FALSE;
	 mx_request_t *mx_request = NULL;
	 MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);		   
	 MPIU_THREAD_CS_ENTER(MSGQUEUE,req);	   	 
	 MPID_NEM_MX_GET_REQ_FROM_HASH(req,mx_request);
	 if(mx_request != NULL)
	 {
	   MPIU_Assert(req->dev.match.parts.rank == MPI_ANY_SOURCE);
	   MPIU_Free(mx_request);
	 }
	 found = MPIDI_CH3U_Recvq_DP(req);
	 if(found){
	   mpi_errno = MPID_nem_mx_handle_rreq(req, status);
	   if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	 }
	 MPIU_THREAD_CS_EXIT(MSGQUEUE,req);
       }
       else
       {
	 MPIU_Assert(0);
       }
     }
   } 
 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;   
}
#endif /*USE_CTXT_AS_MARK */

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_handle_sreq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
MPID_nem_mx_handle_sreq(MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    if ((req->dev.datatype_ptr != NULL) && (req->dev.tmpbuf != NULL))
    {
      MPIU_Free(req->dev.tmpbuf);
    }	   
    reqFn = req->dev.OnDataAvail;
    if (!reqFn){
      MPIDI_CH3U_Request_complete(req);
      MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }
    else{
      MPIDI_VC_t *vc = req->ch.vc;
      int complete   = 0;
      mpi_errno = reqFn(vc, req, &complete);
      if (mpi_errno) MPIU_ERR_POP(mpi_errno);
      if(!complete)
      {	
	 /* FIXME */
	 /* GM: enqueue the not complete sreq somewhere and test it again later */
	 MPIU_Assert(complete == TRUE);
      }
    }
    MPID_nem_mx_pending_send_req--;
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_handle_rreq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
MPID_nem_mx_handle_rreq(MPID_Request *req, mx_status_t status)
{
  int            mpi_errno = MPI_SUCCESS;
  uint64_t       match_info = status.match_info;
  int            complete = FALSE;    
  int            dt_contig;
  MPI_Aint       dt_true_lb;
  MPIDI_msg_sz_t userbuf_sz;
  MPID_Datatype *dt_ptr;
  MPIDI_msg_sz_t data_sz;
  MPIDI_VC_t    *vc;
  
  NEM_MX_MATCH_GET_RANK(match_info,req->status.MPI_SOURCE);
  NEM_MX_MATCH_GET_TAG(match_info,req->status.MPI_TAG);	   
  req->status.count = status.xfer_length;	
  req->dev.recv_data_sz = status.xfer_length;
  
  MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
  /*fprintf(stdout," ===> userbuf_size is %i, msg_length is %i, xfer_length is %i\n",userbuf_sz,status.msg_length,status.xfer_length); */
  
  if (status.msg_length <=  userbuf_sz) 
  {
     data_sz = status.xfer_length;
     /* the sent message was truncated */
     if (status.msg_length != status.xfer_length )
     {
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
					    "message truncated on receiver side, real_msg_sz=" 
					    MPIDI_MSG_SZ_FMT ", expected_msg_sz="
					    MPIDI_MSG_SZ_FMT,
					    status.xfer_length, status.msg_length));
	req->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
						     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
						     "**truncate", "**truncate %d %d %d %d", 
						     req->status.MPI_SOURCE, req->status.MPI_TAG, 
						     req->dev.recv_data_sz, userbuf_sz );
     }
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
    MPID_Segment_unpack( req->dev.segment_ptr, 0, &last, req->dev.tmpbuf );
    MPIU_Free(req->dev.tmpbuf);       
    if (last != data_sz) {
      req->status.count = (int)last;
      if (req->dev.recv_data_sz <= userbuf_sz) {
	MPIU_ERR_SETSIMPLE(req->status.MPI_ERROR,MPI_ERR_TYPE,"**dtypemismatch");
      }
    }
  }
  
  mx_get_endpoint_addr_context(status.source,(void **)(&vc));
#ifdef ONDEMAND    
  if( vc == NULL)
  {
    char business_card[MPID_NEM_MAX_NETMOD_STRING_LEN];
    mx_return_t ret;   
    mx_request_t mx_request;
    mx_status_t  status;
    uint32_t     result;
    
    MPIDI_Comm_get_vc_set_active(req->comm, req->status.MPI_SOURCE, &vc);   
    mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card, MPID_NEM_MAX_NETMOD_STRING_LEN, vc->pg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_mx_get_from_bc (business_card, &VC_FIELD(vc, remote_endpoint_id), &VC_FIELD(vc, remote_nic_id));
    if (mpi_errno)    MPIU_ERR_POP (mpi_errno);
    
    ret = mx_iconnect(MPID_nem_mx_local_endpoint,VC_FIELD(vc, remote_nic_id),
		      VC_FIELD(vc, remote_endpoint_id),MPID_NEM_MX_FILTER,match_info,NULL,&mx_request);
    MPIU_Assert(ret == MX_SUCCESS);
    do{
      ret = mx_test(MPID_nem_mx_local_endpoint,&mx_request,&status,&result);
    }while((result == 0) && (ret == MX_SUCCESS));
    MPIU_Assert(ret == MX_SUCCESS);
    
    fprintf(stdout,"[%i]=== Connected on recv  with %i ... %p \n", MPID_nem_mem_region.rank,vc->lpid,vc);		
    VC_FIELD(vc, remote_endpoint_addr) = status.source;
    VC_FIELD(vc, local_connected) = 1;
    VC_FIELD(vc, remote_connected) = 1;
    mx_set_endpoint_addr_context(VC_FIELD(vc, remote_endpoint_addr),(void *)vc);
    fprintf(stdout,"[%i]=== Connected 2  on recv  with %i ... %p \n", MPID_nem_mem_region.rank,vc->lpid,vc);
  }
#endif
   
  MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
  MPIU_Assert(complete == TRUE);	       	    
 fn_exit:
  return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
      goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_anysource_posted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_mx_anysource_posted(MPID_Request *rreq)
{
  /* This function is called whenever an anyource request has been
     posted to the posted receive queue.  */
  MPIR_Context_id_t context;
  Mx_Nem_tag_t      tag;
  uint64_t          match_info = 0;
  uint64_t          match_mask = NEM_MX_MATCH_FULL_MASK;
  mx_request_t     *mx_request = MPIU_Malloc(sizeof(mx_request_t));
  mx_segment_t      mx_iov[MX_MAX_SEGMENTS];
  uint32_t          num_seg = 1;
  mx_return_t       ret;
  MPIDI_msg_sz_t    data_sz;
  int               dt_contig;
  MPI_Aint          dt_true_lb;
  MPID_Datatype    *dt_ptr;               
  
  MPIDI_Datatype_get_info(rreq->dev.user_count,rreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);	    	

  tag     = rreq->dev.match.parts.tag;
  context = rreq->dev.match.parts.context_id;		    	    
  NEM_MX_DIRECT_MATCH(match_info,0,0,context);
  if (tag == MPI_ANY_TAG)
  {
    NEM_MX_SET_ANYTAG(match_info);
    NEM_MX_SET_ANYTAG(match_mask);
  }
  else
    NEM_MX_SET_TAG(match_info,tag);
  NEM_MX_SET_ANYSRC(match_info);
  NEM_MX_SET_ANYSRC(match_mask);

  if (dt_contig)
  {
      mx_iov[0].segment_ptr = (char *)(rreq->dev.user_buf) + dt_true_lb;
      mx_iov[0].segment_length = data_sz;
  }
  else
    MPID_nem_mx_process_rdtype(&rreq,dt_ptr,data_sz,mx_iov,&num_seg);
  ret = mx_irecv(MPID_nem_mx_local_endpoint,mx_iov,num_seg,match_info,match_mask,(void *)rreq,mx_request);
  /* FIXME: this function can't return an error because it's called
     from a recvq function that doesn't check for errors.  For now,
     I'm replacing the chkandjump with an assertp. */
  /* MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_irecv", "**mx_irecv %s", mx_strerror (ret)); */
  MPIU_Assertp(ret == MX_SUCCESS);
  
  MPID_MEM_MX_ADD_REQ_IN_HASH(rreq,mx_request);  
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_anysource_matched
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_anysource_matched(MPID_Request *rreq)
{
  /* This function is called when an anysource request in the posted
     receive queue is matched and dequeued.  It returns 0 if the req
     was not matched by mx; non-zero otherwise. */
  mx_request_t *mx_request = NULL;
  mx_return_t ret;
  uint32_t    result;
  int matched   = FALSE;
  int mpi_errno = MPI_SUCCESS;
   
  MPID_NEM_MX_GET_REQ_FROM_HASH(rreq,mx_request);
  if(mx_request != NULL)
  {
    ret = mx_cancel(MPID_nem_mx_local_endpoint,mx_request,&result);
    if (ret == MX_SUCCESS)
    {
      if (result != 1)
      {
	mx_status_t status;
	MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);		   
	do{
	  ret = mx_test(MPID_nem_mx_local_endpoint,mx_request,&status,&result);
	}while((result == 0) && (ret == MX_SUCCESS));
	MPIU_Assert(ret == MX_SUCCESS);
	mpi_errno = MPID_nem_mx_handle_rreq(rreq, status);
	/* FIXME : how can we report MPI_ERR_TRUNC in this case?*/ 
	matched = TRUE;
      }
      else
      {
	MPID_Segment_free(rreq->dev.segment_ptr);
      }	   
      MPIU_Free(mx_request);
    }
  }    
  return matched;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_process_rdtype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_process_rdtype(MPID_Request **rreq_p, MPID_Datatype * dt_ptr, MPIDI_msg_sz_t data_sz, mx_segment_t *mx_iov, uint32_t  *num_seg)
{
  MPID_Request *rreq =*rreq_p;
  MPID_IOV  *iov;
  MPIDI_msg_sz_t last;
  int num_entries = MX_MAX_SEGMENTS;
  int n_iov       = 0;
  int mpi_errno   = MPI_SUCCESS;
  int index;
  
  MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_PROCESS_RDTYPE);
  MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_PROCESS_RDTYPE);
  
  if (rreq->dev.segment_ptr == NULL)
  {
    rreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_allo\
c");
  }
  MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, rreq->dev.segment_ptr, 0);
  rreq->dev.segment_first = 0;
  rreq->dev.segment_size = data_sz;
  last = rreq->dev.segment_size;
  MPID_Segment_count_contig_blocks(rreq->dev.segment_ptr,rreq->dev.segment_first,&last,&n_iov);
  MPIU_Assert(n_iov > 0);
  iov = MPIU_Malloc(n_iov*sizeof(MPID_IOV));

  MPID_Segment_unpack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, iov, &n_iov);
  MPIU_Assert(last == rreq->dev.segment_size);
  
#ifdef DEBUG_IOV
  fprintf(stdout,"=============== %i entries (free slots : %i)\n",n_iov,num_entries);
  for(index = 0; index < n_iov; index++)
    fprintf(stdout,"[%i]======= Recv iov[%i] = ptr : %p, len : %i \n",
	    MPID_nem_mem_region.rank,index,iov[index].MPID_IOV_BUF,iov[index].MPID_IOV_LEN);
#endif
  if(n_iov <= num_entries)
  {
    for(index = 0; index < n_iov ; index++)
    {
      (mx_iov)[index].segment_ptr    = iov[index].MPID_IOV_BUF;
      (mx_iov)[index].segment_length = iov[index].MPID_IOV_LEN;
    }
    rreq->dev.tmpbuf = NULL;
    *num_seg = n_iov;
  }
  else
  {
    int packsize = 0;
    MPIR_Pack_size_impl(rreq->dev.user_count, rreq->dev.datatype, &packsize);
    rreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
    MPIU_Assert(rreq->dev.tmpbuf);
    rreq->dev.tmpbuf_sz = packsize;
    mx_iov[0].segment_ptr = (char *)  rreq->dev.tmpbuf;
    mx_iov[0].segment_length = (uint32_t) packsize;
    *num_seg = 1 ;
  }
  MPIU_Free(iov);
 fn_exit:
  MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_PROCESS_RDTYPE);
  return mpi_errno;
 fn_fail:
  goto fn_exit;
}



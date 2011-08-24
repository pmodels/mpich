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

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    mx_request_t  mx_request; 
    mx_segment_t  mx_iov[2];
    uint32_t      num_seg = 1;
    mx_return_t   ret;
    uint64_t      match_info = 0;        
    /*MPIDI_CH3_Pkt_type_t type = ((MPIDI_CH3_Pkt_t *)(hdr))->type; */

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_ISENDCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_ISENDCONTIGMSG);    
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "mx_iSendContig");    
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

#ifdef ONDEMAND
    if( VC_FIELD(vc, local_connected) == 0)
    {	
	MPID_nem_mx_send_conn_info(vc);
    }
#endif

    NEM_MX_ADI_MATCH(match_info); 
    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    mx_iov[0].segment_ptr     = (char *)&(sreq->dev.pending_pkt);
    mx_iov[0].segment_length  = sizeof(MPIDI_CH3_PktGeneric_t);
    num_seg = 1;
    if(data_sz)
    {
	mx_iov[1].segment_ptr     = data;
	mx_iov[1].segment_length  = data_sz;
	num_seg += 1;
    }
   
    ret = mx_isend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),match_info,(void*)sreq,&mx_request);
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));
    MPID_nem_mx_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_ISENDCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz, MPID_Request **sreq_ptr)
{
    MPID_Request *sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    mx_request_t  mx_request;
    mx_segment_t  mx_iov[2]; 
    uint32_t      num_seg = 1;
    mx_return_t   ret;
    uint64_t      match_info = 0;        

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_ISTARTCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_ISTARTCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));   
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "mx_iSendContig");    
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

#ifdef ONDEMAND
    if( VC_FIELD(vc, local_connected) == 0)
    {	
	MPID_nem_mx_send_conn_info(vc);
    }
#endif
    /* create a request */    
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2); 
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = 0;
    
    NEM_MX_ADI_MATCH(match_info); 
    /*fprintf(stdout,"[%i]=== Startcontigmsg  sending  (%lx) to %i... \n",MPID_nem_mem_region.rank,match_info,vc->lpid); */
    
    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    mx_iov[0].segment_ptr     = (char *)&(sreq->dev.pending_pkt);
    mx_iov[0].segment_length  = sizeof(MPIDI_CH3_PktGeneric_t);    
    num_seg = 1;
    if (data_sz)
    {
	mx_iov[1].segment_ptr     = (char *)data;
	mx_iov[1].segment_length  = data_sz;
	num_seg += 1;
    }	    
   
    ret = mx_isend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),match_info,(void *)sreq,&mx_request);
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));
    MPID_nem_mx_pending_send_req++;
    sreq->ch.vc = vc;    
    
 fn_exit:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    mx_segment_t   mx_iov[2];
    uint32_t       num_seg = 1;
    int            mpi_errno = MPI_SUCCESS;
    mx_request_t   mx_request;
    mx_return_t    ret;
    uint64_t       match_info;
    MPIDI_msg_sz_t last;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_SENDNONCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_SENDNONCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));   
    MPIU_Assert(MPID_IOV_LIMIT < MX_MAX_SEGMENTS);
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "MPID_nem_mx_iSendNoncontig");    

#ifdef ONDEMAND
    if( VC_FIELD(vc, local_connected) == 0)
    {	
	MPID_nem_mx_send_conn_info(vc);
    }
#endif

    NEM_MX_ADI_MATCH(match_info); 
    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)header,sizeof(MPIDI_CH3_PktGeneric_t));
    mx_iov[0].segment_ptr     = (char *)&(sreq->dev.pending_pkt);
    mx_iov[0].segment_length  = sizeof(MPIDI_CH3_PktGeneric_t);
    num_seg = 1;

    MPIU_Assert(sreq->dev.segment_first == 0);
    last = sreq->dev.segment_size;
    if (last > 0)
    {
        sreq->dev.tmpbuf = MPIU_Malloc((size_t)sreq->dev.segment_size);
        MPID_Segment_pack(sreq->dev.segment_ptr,sreq->dev.segment_first, &last,(char *)(sreq->dev.tmpbuf));
        MPIU_Assert(last == sreq->dev.segment_size);
        mx_iov[1].segment_ptr = (char *)(sreq->dev.tmpbuf);
        mx_iov[1].segment_length = (uint32_t)last;
        num_seg++;
    }
   
    /*
    MPIDI_Datatype_get_info(sreq->dev.user_count,sreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);
    if(data_sz)
    {
	if( data_sz <= vc->eager_max_msg_sz)
	{
	    MPID_nem_mx_process_sdtype(&sreq,sreq->dev.datatype,dt_ptr,sreq->dev.user_buf,sreq->dev.user_count,
				       data_sz, mx_iov,&num_seg,1);   
	}
	else
	{
	    int packsize = 0;
	    MPI_Aint last;
	    MPIU_Assert(sreq->dev.segment_ptr == NULL);
	    sreq->dev.segment_ptr = MPID_Segment_alloc( );
	    MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
	    MPIR_Pack_size_impl(sreq->dev.user_count, sreq->dev.datatype, &packsize);
	    sreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
	    MPIU_Assert(sreq->dev.tmpbuf);	
	    MPID_Segment_init(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype, sreq->dev.segment_ptr, 0);
	    last = data_sz;
	    MPID_Segment_pack(sreq->dev.segment_ptr, 0, &last, sreq->dev.tmpbuf);	
	    mx_iov[1].segment_ptr = (char *)  sreq->dev.tmpbuf;
	    mx_iov[1].segment_length = (uint32_t) last;
	    num_seg++;
	}
    }
   */
   
    MPIU_Assert(num_seg <= MX_MAX_SEGMENTS);    
    ret = mx_isend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),match_info,(void *)sreq,&mx_request);
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));

    MPID_nem_mx_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_SENDNONCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_directSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPID_nem_mx_directSend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,MPID_Request **request)
{
    MPID_Request  *sreq = NULL;
    mx_segment_t   mx_iov[MX_MAX_SEGMENTS];
    uint32_t       num_seg = 1;
    mx_return_t    ret;
    uint64_t       mx_matching;
    int            mpi_errno = MPI_SUCCESS;
    MPID_Datatype *dt_ptr;
    int            dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint       dt_true_lb;
    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_DIRECTSEND);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_DIRECTSEND);
        
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    sreq->partner_request = NULL;    

#ifdef ONDEMAND
    if( VC_FIELD(vc, local_connected) == 0)
    {
	MPID_nem_mx_send_conn_info(vc);
	fprintf(stdout,"[%i]=== DirectSend info Sender  FULLY connected with %i %p \n", MPID_nem_mem_region.rank,vc->lpid,vc);
    }
#endif

    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = NULL;       
    
    NEM_MX_DIRECT_MATCH(mx_matching,tag,comm->rank,comm->context_id + context_offset);  
    if(data_sz)
    {
	if (dt_contig)
	{
	    mx_iov[0].segment_ptr = (char*)buf + dt_true_lb;
	    mx_iov[0].segment_length  = data_sz;
	}
	else
	{
	    if( data_sz <= vc->eager_max_msg_sz)
	    {
		MPID_nem_mx_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,mx_iov,&num_seg,0);   
#ifdef DEBUG_IOV
                {
                    int index;
                    fprintf(stdout,"==========================\n");
                    for(index = 0; index < num_seg; index++)
                        fprintf(stdout,"[%i]======= MX iov[%i] = ptr : %p, len : %i \n",
                                MPID_nem_mem_region.rank,index,mx_iov[index].segment_ptr,mx_iov[index].segment_length);
                }
#endif
	    }
	    else
	    {
		int packsize = 0;
		MPI_Aint last;
		sreq->dev.segment_ptr = MPID_Segment_alloc( );
		MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
		MPIR_Pack_size_impl(count, datatype, &packsize);
		sreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
		MPIU_Assert(sreq->dev.tmpbuf);	
		MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
		last = data_sz;
		MPID_Segment_pack(sreq->dev.segment_ptr, 0, &last, sreq->dev.tmpbuf);
		mx_iov[0].segment_ptr = (char *)  sreq->dev.tmpbuf;
		mx_iov[0].segment_length = (uint32_t) last;
	    }
	}
    }
    else
    {
	mx_iov[0].segment_ptr = NULL;
	mx_iov[0].segment_length  = 0;
    }
    
    ret = mx_isend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),
		   mx_matching,(void *)sreq,&(REQ_FIELD(sreq,mx_request)));
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));
    MPID_nem_mx_pending_send_req++;
 fn_exit:
    *request = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_DIRECTSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_directSsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPID_nem_mx_directSsend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,MPID_Request **request)
{
    MPID_Request *sreq = NULL;    
    uint32_t      num_seg = 1;
    mx_segment_t  mx_iov[MX_MAX_SEGMENTS];
    mx_return_t   ret;
    uint64_t      mx_matching;
    int           mpi_errno = MPI_SUCCESS;
    MPID_Datatype *dt_ptr;
    int            dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint       dt_true_lb;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_DIRECTSSEND);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_DIRECTSSEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    sreq->partner_request = NULL;        

#ifdef ONDEMAND
    if( VC_FIELD(vc, local_connected) == 0)
    {
	MPID_nem_mx_send_conn_info(vc);
	fprintf(stdout,"[%i]=== DirectSsend info Sender  FULLY connected with %i %p \n", MPID_nem_mem_region.rank,vc->lpid,vc);
    }
#endif
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    sreq->ch.vc = vc;   
    sreq->dev.OnDataAvail = NULL;    	
    
    NEM_MX_DIRECT_MATCH(mx_matching,tag,comm->rank,comm->context_id + context_offset);  

    if(data_sz)
    {
	if (dt_contig)
	{
	    mx_iov[0].segment_ptr = (char*)(buf) + dt_true_lb;
	    mx_iov[0].segment_length  = data_sz;
	}
	else	
	{
	    if( data_sz <= vc->eager_max_msg_sz)
	    {
		MPID_nem_mx_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,mx_iov,&num_seg,0);   
	    }
	    else
	    {
		int packsize = 0;
		MPI_Aint last;
		sreq->dev.segment_ptr = MPID_Segment_alloc( );
		MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
		MPIR_Pack_size_impl(count, datatype, &packsize);
		sreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
		MPIU_Assert(sreq->dev.tmpbuf);	
		MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
		last = data_sz;
		MPID_Segment_pack(sreq->dev.segment_ptr, 0, &last, sreq->dev.tmpbuf);
		mx_iov[0].segment_ptr = (char *) sreq->dev.tmpbuf;
		mx_iov[0].segment_length = (uint32_t) last;
	    }
	}
    }
    else
    {
	mx_iov[0].segment_ptr = NULL;
	mx_iov[0].segment_length  = 0;
    }

    ret = mx_issend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),
		    mx_matching,(void *)sreq,&(REQ_FIELD(sreq,mx_request)));
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));	
    MPID_nem_mx_pending_send_req++;

 fn_exit:
   *request = sreq;
   MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_DIRECTSSEND);
   return mpi_errno;
 fn_fail:
   goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_process_sdtype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_process_sdtype(MPID_Request **sreq_p,  MPI_Datatype datatype,  MPID_Datatype * dt_ptr, const void *buf, int count, MPIDI_msg_sz_t data_sz, mx_segment_t *mx_iov, uint32_t  *num_seg,int first_free_slot)
{
    MPID_Request *sreq =*sreq_p;
    MPID_IOV  *iov;
    MPIDI_msg_sz_t last;
    int num_entries = MX_MAX_SEGMENTS - first_free_slot;
    int n_iov       = 0;
    int mpi_errno   = MPI_SUCCESS;
    int index;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_PROCESS_SDTYPE);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_PROCESS_SDTYPE);
    
    sreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;
    last = sreq->dev.segment_size;
    
    MPID_Segment_count_contig_blocks(sreq->dev.segment_ptr,sreq->dev.segment_first,&last,&n_iov);
    MPIU_Assert(n_iov > 0);
    iov = MPIU_Malloc(n_iov*sizeof(MPID_IOV));    
    
    MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last, iov, &n_iov);
    MPIU_Assert(last == sreq->dev.segment_size);    
    
#ifdef DEBUG_IOV
    fprintf(stdout,"=============== %i entries (free slots : %i)\n",n_iov,num_entries);
    for(index = 0; index < n_iov; index++)
	fprintf(stdout,"[%i]======= Send iov[%i] = ptr : %p, len : %i \n",
		MPID_nem_mem_region.rank,index,iov[index].MPID_IOV_BUF,iov[index].MPID_IOV_LEN);
#endif

    if(n_iov <= num_entries)
    {
	for(index = 0; index < n_iov ; index++)
	{
	    (mx_iov)[first_free_slot+index].segment_ptr    = iov[index].MPID_IOV_BUF;
	    (mx_iov)[first_free_slot+index].segment_length = iov[index].MPID_IOV_LEN;
	}
	*num_seg = n_iov;
    }   	
    else
    {
	int size_to_copy = 0;
	int offset = 0;
	int last_entry = num_entries - 1;
	for(index = 0; index < n_iov ; index++)
	{
	    if (index <= (last_entry-1))
	    {
		(mx_iov)[first_free_slot+index].segment_ptr    = iov[index].MPID_IOV_BUF;
		(mx_iov)[first_free_slot+index].segment_length = iov[index].MPID_IOV_LEN;	 
	    }
	    else
	    {
		size_to_copy += iov[index].MPID_IOV_LEN;
	    }
	}
	sreq->dev.tmpbuf = MPIU_Malloc(size_to_copy);
	MPIU_Assert(sreq->dev.tmpbuf);
	for(index = last_entry; index < n_iov; index++)
	{
	    MPIU_Memcpy((char *)(sreq->dev.tmpbuf) + offset, iov[index].MPID_IOV_BUF, iov[index].MPID_IOV_LEN);
	    offset += iov[index].MPID_IOV_LEN;	    
	}	    
	(mx_iov)[MX_MAX_SEGMENTS-1].segment_ptr = sreq->dev.tmpbuf;
	(mx_iov)[MX_MAX_SEGMENTS-1].segment_length = size_to_copy;				
	*num_seg = MX_MAX_SEGMENTS ;
    }
    MPIU_Free(iov);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_PROCESS_SDTYPE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_send_conn_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_mx_send_conn_info (MPIDI_VC_t *vc)
{
   char business_card[MPID_NEM_MAX_NETMOD_STRING_LEN];
   uint64_t     match_info;
   uint32_t     num_seg = 3;
   mx_segment_t mx_iov[3];
   mx_request_t mx_request;
   mx_return_t  ret;
   mx_status_t  status;
   uint32_t     result;
   int          mpi_errno = MPI_SUCCESS;

   MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MX_SEND_CONN_INFO);    
   MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MX_SEND_CONN_INFO);

   mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card, MPID_NEM_MAX_NETMOD_STRING_LEN, vc->pg);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);
   
   mpi_errno = MPID_nem_mx_get_from_bc (business_card, &VC_FIELD(vc, remote_endpoint_id), &VC_FIELD(vc, remote_nic_id));
   if (mpi_errno)    MPIU_ERR_POP (mpi_errno);

   fprintf(stdout,"[%i]=== Sender connecting  to  %i  \n", MPID_nem_mem_region.rank,vc->lpid);   

   /* FIXME: match_info is used uninitialized */
   ret = mx_iconnect(MPID_nem_mx_local_endpoint,VC_FIELD(vc, remote_nic_id),
		     VC_FIELD(vc, remote_endpoint_id),MPID_NEM_MX_FILTER,match_info,NULL,&mx_request);   
   MPIU_Assert(ret == MX_SUCCESS);
   do{
       ret = mx_test(MPID_nem_mx_local_endpoint,&mx_request,&status,&result);
   }while((result == 0) && (ret == MX_SUCCESS));
   MPIU_Assert(ret == MX_SUCCESS);
   VC_FIELD(vc, remote_endpoint_addr) = status.source;
   VC_FIELD(vc, local_connected) = 1;
   mx_set_endpoint_addr_context(VC_FIELD(vc, remote_endpoint_addr),(void *)vc);   

   fprintf(stdout,"[%i]=== Sending conn info connection to  %i  \n", MPID_nem_mem_region.rank,vc->lpid);   

   NEM_MX_ADI_MATCH(match_info);
   NEM_MX_SET_PGRANK(match_info,MPIDI_Process.my_pg_rank);
  
   mx_iov[0].segment_ptr = (char*)&MPID_nem_mx_local_nic_id;
   mx_iov[0].segment_length  = sizeof(uint64_t);
   mx_iov[1].segment_ptr = (char*)&MPID_nem_mx_local_endpoint_id;
   mx_iov[1].segment_length  = sizeof(uint32_t);
   mx_iov[2].segment_ptr = MPIDI_Process.my_pg->id;
   mx_iov[2].segment_length  =  strlen(MPIDI_Process.my_pg->id) + 1;
   
   ret = mx_isend(MPID_nem_mx_local_endpoint,mx_iov,num_seg,VC_FIELD(vc,remote_endpoint_addr),match_info,NULL,&mx_request);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));
   do{
       ret = mx_test(MPID_nem_mx_local_endpoint,&mx_request,&status,&result);
   }while((result == 0) && (ret == MX_SUCCESS));
   MPIU_Assert(ret == MX_SUCCESS);
   fprintf(stdout,"[%i]=== Send conn info to  %i (%Lx) (%Li %i %s) ... \n", 
	   MPID_nem_mem_region.rank,vc->lpid,match_info,MPID_nem_mx_local_nic_id,MPID_nem_mx_local_endpoint_id,(char *)MPIDI_Process.my_pg->id);
 fn_exit:
   MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MX_SEND_CONN_INFO);
   return mpi_errno;
 fn_fail:
   goto fn_exit;
}



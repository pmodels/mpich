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

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int           mpi_errno = MPI_SUCCESS;
    nm_tag_t      match_info = 0;
    struct iovec *newmad_iov = (struct iovec *)MPIU_Malloc(2*sizeof(struct iovec));
    int           num_iov = 1;
    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);    

    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newmad_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    NEM_NMAD_ADI_MATCH(match_info);

#ifdef DEBUG
    fprintf(stdout,"iSendContig ========> Sending ADI msg  for req %p (match is %lx) \n",sreq,match_info);
#endif

    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    newmad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    newmad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);
    if(data_sz)
    {
	newmad_iov[1].iov_base = data;
	newmad_iov[1].iov_len  = data_sz;
	num_iov += 1;
    }
    REQ_FIELD(sreq,iov) = newmad_iov;

    nm_sr_isend_iov_with_ref(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
			     newmad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)),(void *)sreq);    
    mpid_nem_newmad_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz, MPID_Request **sreq_ptr)
{
    MPID_Request *sreq = NULL;
    nm_tag_t      match_info = 0;
    struct iovec *newmad_iov = (struct iovec *)MPIU_Malloc(2*sizeof(struct iovec));
    int           num_iov = 1;
    int           mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newmad_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = 0;

    NEM_NMAD_ADI_MATCH(match_info);
#ifdef DEBUG
    fprintf(stdout,"StartcontigMsg ========> Sending ADI msg  for req %p (match is %lx) \n",sreq,match_info);
#endif

    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    newmad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    newmad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);
    if (data_sz)
    {
	newmad_iov[1].iov_base = (char *)data;
	newmad_iov[1].iov_len  = data_sz;
	num_iov += 1;
    }
    REQ_FIELD(sreq,iov) = newmad_iov;    

    nm_sr_isend_iov_with_ref(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
			     newmad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)),(void *)sreq);    
    mpid_nem_newmad_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int            mpi_errno = MPI_SUCCESS;
    nm_tag_t       match_info = 0;
    struct iovec   newmad_iov[2];
    int            num_iov = 1;
    MPIDI_msg_sz_t last;

    /*
    struct iovec  *newmad_iov = (struct iovec *)MPIU_Malloc(NMAD_IOV_MAX_DEPTH*sizeof(struct iovec));
    MPIDI_msg_sz_t data_sz;
    int            dt_contig;
    MPI_Aint       dt_true_lb;
    MPID_Datatype *dt_ptr;
    */

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));   
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "MPID_nem_newmad_iSendNoncontig");    
    
    NEM_NMAD_ADI_MATCH(match_info);

    MPIU_Memcpy(&(sreq->dev.pending_pkt),(char *)header,sizeof(MPIDI_CH3_PktGeneric_t));
    newmad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    newmad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);

    MPIU_Assert(sreq->dev.segment_first == 0);
    last = sreq->dev.segment_size;
    if (last > 0)
    {
	sreq->dev.tmpbuf = MPIU_Malloc((size_t)sreq->dev.segment_size);
	MPID_Segment_pack(sreq->dev.segment_ptr,sreq->dev.segment_first, &last,(char *)(sreq->dev.tmpbuf));
	MPIU_Assert(last == sreq->dev.segment_size);
	newmad_iov[1].iov_base = (char *)(sreq->dev.tmpbuf);
	newmad_iov[1].iov_len = (uint32_t)last;
	num_iov++;
    }

    REQ_FIELD(sreq,iov) = newmad_iov;        

    /*
    MPIDI_Datatype_get_info(sreq->dev.user_count,sreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);
    if(data_sz)
    {
	struct iovec *mad_iov_ptr = &(newmad_iov[0]);
	MPID_nem_newmad_process_sdtype(&sreq,sreq->dev.datatype,dt_ptr,sreq->dev.user_buf,
				       sreq->dev.user_count,data_sz, &mad_iov_ptr,&num_iov,1);
    }
#ifdef DEBUG
    fprintf(stdout,"SendNonContig ========> Sending ADI msg  for req %p (match is %lx) \n",sreq,match_info);
#endif
    */

    nm_sr_isend_iov_with_ref(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
			     newmad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)),(void*)sreq);    
    mpid_nem_newmad_pending_send_req++;    

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_directSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPID_nem_newmad_directSend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int rank, int tag,
				MPID_Comm * comm, int context_offset, MPID_Request **sreq_p)
{
    MPID_Request  *sreq = NULL;
    nm_tag_t       match_info = 0;
    int            num_iov = 0;    
    int            mpi_errno = MPI_SUCCESS;
    MPID_Datatype *dt_ptr;
    int            dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint       dt_true_lb;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSEND);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    sreq->partner_request = NULL;
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = NULL;
    NEM_NMAD_DIRECT_MATCH(match_info,tag,comm->rank,comm->context_id + context_offset);
#ifdef DEBUG
    fprintf(stdout,"Direct Send ========> Sending NON ADI msg for req %p (match is %lx) \n",sreq,match_info);
#endif

    if(data_sz)
    {
	struct iovec *newmad_iov = (struct iovec *)MPIU_Malloc(NMAD_IOV_MAX_DEPTH*sizeof(struct iovec));
	if (dt_contig)
	{
	    newmad_iov[0].iov_base = (char*)(buf + dt_true_lb);
	    newmad_iov[0].iov_len  = data_sz;
	    num_iov = 1;
	}
	else
	{
	    struct iovec  *mad_iov_ptr = &(newmad_iov[0]);
	    MPID_nem_newmad_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,&mad_iov_ptr,&num_iov,0);	    	    
	}
	MPIU_Assert(num_iov <= NMAD_IOV_MAX_DEPTH);
#ifdef DEBUG
	for(index = 0; index < num_iov ; index++)
	    {
		fprintf(stdout,"======================\n");
		fprintf(stdout,"SEND nmad_iov[%i]: [base %p][len %i]\n",index,
			newmad_iov[index].iov_base,newmad_iov[index].iov_len);
	    }
#endif
	nm_sr_isend_iov_with_ref(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
				 newmad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)),(void*)sreq);    
	REQ_FIELD(sreq,iov) = newmad_iov;        
    }
    else
    {
	nm_sr_isend_with_ref(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
			     NULL, 0, &(REQ_FIELD(sreq,newmad_req)),(void*)sreq);    
	REQ_FIELD(sreq,iov) = NULL;
    }
    mpid_nem_newmad_pending_send_req++;

 fn_exit:
    *sreq_p = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSEND);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_directSsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPID_nem_newmad_directSsend(MPIDI_VC_t *vc, const void * buf, int count, MPI_Datatype datatype, int rank, int tag,
				MPID_Comm * comm, int context_offset, MPID_Request **sreq_p)
{
    MPID_Request  *sreq = NULL;
    nm_tag_t       match_info = 0;
    int            num_iov = 0;    
    int            mpi_errno = MPI_SUCCESS;
    MPID_Datatype *dt_ptr;
    int            dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint       dt_true_lb;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSSEND);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSSEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    sreq->partner_request = NULL;
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = NULL;

    NEM_NMAD_DIRECT_MATCH(match_info,tag,comm->rank,comm->context_id + context_offset);
    if(data_sz)
    {
	struct iovec  *newmad_iov = (struct iovec *)MPIU_Malloc(NMAD_IOV_MAX_DEPTH*sizeof(struct iovec));
	if (dt_contig)
	{
	    newmad_iov[0].iov_base = (char*)(buf + dt_true_lb);
	    newmad_iov[0].iov_len  = data_sz;
	    num_iov += 1;
	}
	else
	{
	    struct iovec *mad_iov_ptr = &(newmad_iov[0]);
	    MPID_nem_newmad_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,&mad_iov_ptr,&num_iov,0);	    	    
	}
	nm_sr_issend_iov(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
			 newmad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    
	REQ_FIELD(sreq,iov) = newmad_iov;        
    }
    else
    {
	nm_sr_issend(mpid_nem_newmad_session, VC_FIELD(vc, p_gate), match_info, 
		     NULL, 0, &(REQ_FIELD(sreq,newmad_req)));    
	REQ_FIELD(sreq,iov) = NULL;
    }
    mpid_nem_newmad_pending_send_req++;

 fn_exit:
    *sreq_p = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSSEND);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_process_sdtype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_process_sdtype(MPID_Request **sreq_p,  MPI_Datatype datatype,  MPID_Datatype *dt_ptr, const void *buf, 
				   int count, MPIDI_msg_sz_t data_sz, struct iovec *mad_iov[], int  *num_iov, int first_taken)
{
    MPID_Request  *sreq        =*sreq_p;
    MPID_IOV      *iov;
    MPIDI_msg_sz_t last;
    int            n_iov       = 0; 
    int            num_entries = NMAD_IOV_MAX_DEPTH - first_taken;
    int            mpi_errno   = MPI_SUCCESS;
    int            index;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);

    sreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;
    last = sreq->dev.segment_size;

    MPID_Segment_count_contig_blocks(sreq->dev.segment_ptr,sreq->dev.segment_first,&last,&n_iov);
    MPIU_Assert(n_iov > 0);
    iov = MPIU_Malloc(n_iov*sizeof(MPID_IOV));
    
    MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last,iov, &n_iov);
    MPIU_Assert(last == sreq->dev.segment_size);

#ifdef DEBUG
    for(index = 0; index < n_iov ; index++)
	{
	    fprintf(stdout,"======================\n");
	    fprintf(stdout,"SEND: iov[%i]: [base %p][len %i]\n",index,
		    iov[index].MPID_IOV_BUF,iov[index].MPID_IOV_LEN);
	}
#endif

    if(n_iov <= num_entries) 
    {
	for(index = 0; index < n_iov ; index++)
	{
	    (*mad_iov)[first_taken+index].iov_base = iov[index].MPID_IOV_BUF;
	    (*mad_iov)[first_taken+index].iov_len  = iov[index].MPID_IOV_LEN;
	}
	*num_iov = n_iov + first_taken;
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
		(*mad_iov)[first_taken+index].iov_base = iov[index].MPID_IOV_BUF;
		(*mad_iov)[first_taken+index].iov_len  = iov[index].MPID_IOV_LEN;
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
        (*mad_iov)[NMAD_IOV_MAX_DEPTH-1].iov_base = sreq->dev.tmpbuf;
        (*mad_iov)[NMAD_IOV_MAX_DEPTH-1].iov_len  = size_to_copy;
        *num_iov = NMAD_IOV_MAX_DEPTH;
    }
    MPIU_Free(iov);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}



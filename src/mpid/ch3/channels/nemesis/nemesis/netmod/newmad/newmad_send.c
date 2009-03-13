/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newmad_impl.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int           mpi_errno = MPI_SUCCESS;
    nm_tag_t      match_info = 0;
    struct iovec  mad_iov[2];
    int           num_iov = 1;
    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);    

    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "mx_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    NEM_NMAD_ADI_MATCH(match_info);
    memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    mad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    mad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);
    if(data_sz)
    {
	mad_iov[1].iov_base = data;
	mad_iov[1].iov_len  = data_sz;
	num_iov += 1;
    }

    nm_sr_isend_iov(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
		    mad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    
    mpid_nem_newmad_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_ISENDCONTIG);
    return mpi_errno;
 fn_fail:
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
    struct iovec  mad_iov[2];
    int           num_iov = 1;
    int           mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "mx_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = 0;

    NEM_NMAD_ADI_MATCH(match_info);
    memcpy(&(sreq->dev.pending_pkt),(char *)hdr,sizeof(MPIDI_CH3_PktGeneric_t));
    mad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    mad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);
    if (data_sz)
    {
	mad_iov[1].iov_base = (char *)data;
	mad_iov[1].iov_len  = data_sz;
	num_iov += 1;
    }
    
    nm_sr_isend_iov(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
		    mad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    
    mpid_nem_newmad_pending_send_req++;
    sreq->ch.vc = vc;

 fn_exit:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
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
    struct iovec  *mad_iov;
    int            num_iov = 1;
    MPIDI_msg_sz_t data_sz;
    int            dt_contig;
    MPI_Aint       dt_true_lb;
    MPID_Datatype *dt_ptr;
    
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));   
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "MPID_nem_newmad_iSendNoncontig");    

    MPIDI_Datatype_get_info(sreq->dev.user_count,sreq->dev.datatype, dt_contig, data_sz, dt_ptr,dt_true_lb);

    if(data_sz)
    {
	MPID_nem_mx_process_sdtype(&sreq,sreq->dev.datatype,dt_ptr,sreq->dev.user_buf,
				   sreq->dev.user_count,data_sz, &mad_iov,&num_iov,1);
    }
    else
    {
	mad_iov = MPIU_Malloc(sizeof(struct iovec));
    }

    NEM_NMAD_ADI_MATCH(match_info);
    memcpy(&(sreq->dev.pending_pkt),(char *)header,sizeof(MPIDI_CH3_PktGeneric_t));
    mad_iov[0].iov_base = (char *)&(sreq->dev.pending_pkt);
    mad_iov[0].iov_len  = sizeof(MPIDI_CH3_PktGeneric_t);

    nm_sr_isend_iov(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
		    mad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    

    MPIU_Free(mad_iov); /* FIXME : is this safe ? */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_SENDNONCONTIGMSG);
    return mpi_errno;
 fn_fail:
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
    struct iovec  *mad_iov;
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

    if(data_sz)
    {
	if (dt_contig)
	{
	    mad_iov = MPIU_Malloc(sizeof(struct iovec));
	    mad_iov[0].iov_base = (char*)(buf + dt_true_lb);
	    mad_iov[0].iov_len  = data_sz;
	    num_iov += 1;
	}
	else
	{
	    MPID_nem_mx_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,&mad_iov,&num_iov,0);	    	    
	}
	nm_sr_isend_iov(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
			mad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    
    }
    else
    {
	nm_sr_isend(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
		    NULL, 0, &(REQ_FIELD(sreq,newmad_req)));    
    }
    mpid_nem_newmad_pending_send_req++;
    MPIU_Free(mad_iov);

 fn_exit:
    *sreq_p = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSEND);
    return mpi_errno;
 fn_fail:
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
    struct iovec  *mad_iov;
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
	if (dt_contig)
	{
	    mad_iov = MPIU_Malloc(sizeof(struct iovec));
	    mad_iov[0].iov_base = (char*)(buf + dt_true_lb);
	    mad_iov[0].iov_len  = data_sz;
	    num_iov += 1;
	}
	else
	{
	    MPID_nem_mx_process_sdtype(&sreq,datatype,dt_ptr,buf,count,data_sz,&mad_iov,&num_iov,0);	    	    
	}
	/* FIXME issend !*/
	nm_sr_isend_iov(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
			mad_iov, num_iov, &(REQ_FIELD(sreq,newmad_req)));    
    }
    else
    {
	/* FIXME issend !*/
	nm_sr_isend(mpid_nem_newmad_pcore, VC_FIELD(vc, p_gate), match_info, 
		    NULL, 0, &(REQ_FIELD(sreq,newmad_req)));    
    }
    mpid_nem_newmad_pending_send_req++;
    MPIU_Free(mad_iov);

 fn_exit:
    *sreq_p = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTSSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_process_sdtype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_process_sdtype(MPID_Request **sreq_p,  MPI_Datatype datatype,  MPID_Datatype * dt_ptr, const void *buf, 
				   int count, MPIDI_msg_sz_t data_sz, struct iovec **mad_iov, int  *num_iov, int first_taken)
{
    MPID_Request  *sreq =*sreq_p;
    MPIDI_msg_sz_t last;
    int iov_num_ub  = count * dt_ptr->max_contig_blocks;
    int n_iov       = iov_num_ub;
    int mpi_errno   = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);

    sreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;
    last = sreq->dev.segment_size;
    (*mad_iov) = MPIU_Malloc((iov_num_ub+first_taken)*sizeof(struct iovec));

    MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last,
			     (MPID_IOV  *)((*mad_iov)+(first_taken*sizeof(struct iovec))), &n_iov);
    MPIU_Assert(last == sreq->dev.segment_size);
    *num_iov = n_iov + first_taken;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_PROCESS_SDTYPE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_newmad_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
   int mpi_errno = MPI_SUCCESS;
   return mpi_errno;
}


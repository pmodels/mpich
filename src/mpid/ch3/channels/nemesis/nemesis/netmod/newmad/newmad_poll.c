/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newmad_impl.h"
#include "my_papi_defs.h"
#include "uthash.h"

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
	if(s){HASH_DELETE(hh, mpid_nem_nmad_asreqs, s); (_nmad_req) = s->nmad_req_ptr; } else {(_nmad_req) = NULL;} \
    }while(0)




#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_get_adi_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_newmad_get_adi_msg(nm_sr_event_t event, nm_sr_event_info_t*info)
{
    nm_tag_t match_info = info->recv_unexpected.tag;
    MPIR_Context_id_t ctxt;
    NEM_NMAD_MATCH_GET_CTXT(match_info, ctxt);
    if(ctxt == NEM_NMAD_INTRA_CTXT)
    {
	nm_gate_t     from = info->recv_unexpected.p_gate;
	MPID_Request *rreq;
	MPIDI_VC_t   *vc;
	struct iovec  mad_iov;
	int           num_iov = 1;
	int           length = 0; //=info->...

	rreq = MPID_Request_create();
	MPIU_Assert (rreq != NULL);
	MPIU_Object_set_ref (rreq, 1);
	rreq->kind = MPID_REQUEST_RECV;   

	//get vc from gate
	rreq->ch.vc = vc;
      
	if(length <=  sizeof(MPIDI_CH3_PktGeneric_t)) {
	    mad_iov.iov_base = (char*)&(rreq->dev.pending_pkt);
	}
	else{
	    rreq->dev.tmpbuf = MPIU_Malloc(length);
	    MPIU_Assert(rreq->dev.tmpbuf);
	    rreq->dev.tmpbuf_sz = length;               
	    mad_iov.iov_base = (char*)(rreq->dev.tmpbuf);
	}
	mad_iov.iov_len = length;
	
	nm_sr_irecv_with_ref(mpid_nem_newmad_pcore, from, match_info, mad_iov.iov_base,
			     length, &(REQ_FIELD(rreq,newmad_req)), (void *)&rreq);	
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
    




 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWMAD_DIRECTRECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_poll(MPID_nem_poll_dir_t in_or_out)
{
   int              mpi_errno = MPI_SUCCESS;   
   nm_sr_request_t *p_out_req = NULL;
   MPID_Request    *rreq = NULL;

   //   nm_sr_progress(mpid_nem_newmad_pcore);
   nm_sr_recv_success(mpid_nem_newmad_pcore, &p_out_req);


 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;   
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_handle_sreq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_newmad_handle_sreq(nm_sr_event_t event, nm_sr_event_info_t*info)
{

    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    nm_sr_request_t *p_request = info->send_completed.p_request;
    MPID_Request *req;
    nm_sr_get_ref(mpid_nem_newmad_pcore,p_request,(void *)&req);
    MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);                  
    reqFn = req->dev.OnDataAvail;
    if (!reqFn){
	MPIDI_CH3U_Request_complete(req);
	MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }
    else{
	MPIDI_VC_t *vc = req->ch.vc;
	int complete   = 0;
	reqFn(vc, req, &complete);
	if(complete)
        {            
	    MPIDI_CH3U_Request_complete(req);
	    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
	}
    }
    mpid_nem_newmad_pending_send_req--;
    return;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_anysource_posted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_anysource_posted(MPID_Request *rreq)
{
    /* This function is called whenever an anyource request has been
       posted to the posted receive queue.  */
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_anysource_matched
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_anysource_matched(MPID_Request *rreq)
{
    /* This function is called when an anysource request in the posted
       receive queue is matched and dequeued */
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


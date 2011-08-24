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

MPID_nem_netmod_funcs_t MPIDI_nem_newmad_funcs = {
    MPID_nem_newmad_init,
    MPID_nem_newmad_finalize,
    MPID_nem_newmad_poll,
    MPID_nem_newmad_get_business_card,
    MPID_nem_newmad_connect_to_root,
    MPID_nem_newmad_vc_init,
    MPID_nem_newmad_vc_destroy,
    MPID_nem_newmad_vc_terminate,
    MPID_nem_newmad_anysource_iprobe
};

static MPIDI_Comm_ops_t comm_ops = {
    MPID_nem_newmad_directRecv, /* recv_posted */

    MPID_nem_newmad_directSend, /* send */
    MPID_nem_newmad_directSend, /* rsend */
    MPID_nem_newmad_directSsend, /* ssend */
    MPID_nem_newmad_directSend, /* isend */
    MPID_nem_newmad_directSend, /* irsend */
    MPID_nem_newmad_directSsend, /* issend */

    NULL,                   /* send_init */
    NULL,                   /* bsend_init */
    NULL,                   /* rsend_init */
    NULL,                   /* ssend_init */
    NULL,                   /* startall */

    MPID_nem_newmad_cancel_send,/* cancel_send */
    MPID_nem_newmad_cancel_recv, /* cancel_recv */

    MPID_nem_newmad_probe, /* probe */
    MPID_nem_newmad_iprobe /* iprobe */
};

#define MPIDI_CH3I_URL_KEY "url_id"

static int         mpid_nem_newmad_myrank;
static const char *label="mpich2";
static const char *local_session_url = NULL;
nm_session_t       mpid_nem_newmad_session;
int                mpid_nem_newmad_pending_send_req = 0;

#undef FUNCNAME
#define FUNCNAME init_mad
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init_mad( MPIDI_PG_t *pg_p )
{
    int   mpi_errno = MPI_SUCCESS;
    char *dummy_argv[2] = {"mpich2",NULL};
    int   dummy_argc    = 1;
    int   ret;

    MPID_nem_newmad_internal_req_queue_init();

    ret = nm_session_create(&mpid_nem_newmad_session, label);
    MPIU_Assert( ret == NM_ESUCCESS);
    
    ret = nm_session_init(mpid_nem_newmad_session, &dummy_argc,dummy_argv, &local_session_url);
    MPIU_Assert( ret == NM_ESUCCESS);

    ret = nm_sr_init(mpid_nem_newmad_session);
    if(ret != NM_ESUCCESS) {
	fprintf(stdout,"nm_sr_init return err = %d\n", ret);
    }
   
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_init_completed
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_init_completed(void)
{
   
   int mpi_errno = MPI_SUCCESS ;
   int ret;
   
   ret = nm_sr_monitor(mpid_nem_newmad_session, NM_SR_EVENT_RECV_UNEXPECTED,
		       &MPID_nem_newmad_get_adi_msg);
   MPIU_Assert( ret == NM_ESUCCESS);
   
fn_exit:
       return mpi_errno;
fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_init (MPIDI_PG_t *pg_p, int pg_rank,
		      char **bc_val_p, int *val_max_sz_p)
{
   int mpi_errno = MPI_SUCCESS ;
   int index;
   
   /*
   fprintf(stdout,"Size of MPID_nem_mad_module_vc_area_internal_t : %i | size of nm_sr_request_t :%i | Size of req_area : %i\n",
         sizeof(MPID_nem_newmad_vc_area_internal_t),sizeof(nm_sr_request_t), sizeof(MPID_nem_newmad_req_area));
    */
   
   /*
   MPIU_Assert( sizeof(MPID_nem_newmad_vc_area_internal_t) <= MPID_NEM_VC_NETMOD_AREA_LEN);
   MPIU_Assert( sizeof(MPID_nem_newmad_req_area) <= MPID_NEM_REQ_NETMOD_AREA_LEN);
   */
   

   if (sizeof(MPID_nem_newmad_vc_area) > MPID_NEM_VC_NETMOD_AREA_LEN)
   {
       fprintf(stdout,"===========================================================\n");
       fprintf(stdout,"===  Error : Newmad data structure size is too long     ===\n");
       fprintf(stdout,"===  VC netmod area is %4i | Nmad struct size is %4i    ===\n", 
	       MPID_NEM_VC_NETMOD_AREA_LEN, sizeof(MPID_nem_newmad_vc_area));
       fprintf(stdout,"===========================================================\n");
       exit(0);
   }
   
   if (sizeof(MPID_nem_newmad_req_area) > MPID_NEM_REQ_NETMOD_AREA_LEN)
   {
       fprintf(stdout,"===========================================================\n");
       fprintf(stdout,"===  Error : Newmad data structure size is too long     ===\n");
       fprintf(stdout,"===  Req netmod area is %4i | Nmad struct size is %4i   ===\n", 
	       MPID_NEM_REQ_NETMOD_AREA_LEN, sizeof(MPID_nem_newmad_req_area));
       fprintf(stdout,"===========================================================\n");
       exit(0);
   }

   mpid_nem_newmad_myrank = pg_rank;
   init_mad(pg_p);

   mpi_errno = MPID_nem_newmad_get_business_card(pg_rank,bc_val_p, val_max_sz_p);
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   mpi_errno = MPIDI_CH3I_Register_anysource_notification(MPID_nem_newmad_anysource_posted, MPID_nem_newmad_anysource_matched);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);

   mpi_errno = MPID_nem_register_initcomp_cb(MPID_nem_newmad_init_completed);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);
   
   fn_exit:
       return mpi_errno;
   fn_fail: 
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    
    str_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_URL_KEY, local_session_url, strlen(local_session_url));
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_get_from_bc (const char *business_card, char *url)
{
   int mpi_errno = MPI_SUCCESS;
   int str_errno = MPIU_STR_SUCCESS;
   int len;
   
   str_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_URL_KEY, url,
					MPID_NEM_NMAD_MAX_SIZE, &len);
   if (str_errno != MPIU_STR_SUCCESS)
   {      
      /* FIXME: create a real error string for this */
      MPIU_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**argstr_hostd");
   }
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;   
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_vc_init (MPIDI_VC_t *vc)
{
   MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
   char          *business_card;
   int            mpi_errno = MPI_SUCCESS;   
   int            val_max_sz;
   int            ret;
   
#ifdef USE_PMI2_API
   val_max_sz = PMI2_MAX_VALLEN;
#else
   mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
#endif
   business_card = (char *)MPIU_Malloc(val_max_sz);   
   mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card,val_max_sz,vc->pg);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);
   
   /* Very important */
   memset(VC_FIELD(vc, url),0,MPID_NEM_NMAD_MAX_SIZE);
   
   mpi_errno = MPID_nem_newmad_get_from_bc (business_card, VC_FIELD(vc, url));
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   MPIU_Free(business_card);
   
   ret = nm_session_connect(mpid_nem_newmad_session, &(VC_FIELD(vc,p_gate)), VC_FIELD(vc, url));
   if (ret != NM_ESUCCESS) fprintf(stdout,"nm_session_connect returned ret = %d\n", ret);

   nm_gate_ref_set(VC_FIELD(vc, p_gate),(void*)vc);
   MPIDI_CHANGE_VC_STATE(vc, ACTIVE);
   
   vc->eager_max_msg_sz = 32768;
   vc->rndvSend_fn      = NULL;
   vc->sendNoncontig_fn = MPID_nem_newmad_SendNoncontig;
   vc->comm_ops         = &comm_ops;

   vc_ch->iStartContigMsg = MPID_nem_newmad_iStartContigMsg;
   vc_ch->iSendContig     = MPID_nem_newmad_iSendContig;
   
 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

 fn_exit:   
       return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_vc_terminate (MPIDI_VC_t *vc)
{
    /* FIXME: Check to make sure that it's OK to terminate the
       connection without making sure that all sends have been sent */
    return MPIDI_CH3U_Handle_connection (vc, MPIDI_VC_EVENT_TERMINATED);
}


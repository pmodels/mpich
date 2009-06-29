/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "newmad_impl.h"

#define MPIDI_CH3I_HOSTNAME_KEY  "hostname_id"

MPID_nem_netmod_funcs_t MPIDI_nem_newmad_funcs = {
    MPID_nem_newmad_init,
    MPID_nem_newmad_finalize,
    MPID_nem_newmad_ckpt_shutdown,
    MPID_nem_newmad_poll,
    MPID_nem_newmad_send,
    MPID_nem_newmad_get_business_card,
    MPID_nem_newmad_connect_to_root,
    MPID_nem_newmad_vc_init,
    MPID_nem_newmad_vc_destroy,
    MPID_nem_newmad_vc_terminate
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


static int         mpid_nem_newmad_myrank;
static int         num_recv_req = 0;
static int         num_send_req = 0;
static MPID_nem_newmad_init_req_t *init_reqs = NULL;
static nm_drv_id_t drv_id[MPID_NEM_NMAD_MAX_NETS];
static char       *url[MPID_NEM_NMAD_MAX_NETS];
static char        url_keys[MPID_NEM_NMAD_MAX_NETS][MPID_NEM_NMAD_MAX_SIZE] = {"url_id0","url_id1","url_id2","url_id3"};
static int         mpid_nem_newmad_num_rails = 1 ;
nm_core_t          mpid_nem_newmad_pcore;
int                mpid_nem_newmad_pending_send_req = 0;

#ifdef MPID_MAD_MODULE_MULTIRAIL
static puk_component_t *p_driver_load_array;
static void mpid_nem_newmad_rails(void)
{
    int index = 0;
    mpid_nem_newmad_num_rails = 0 ;
#if defined CONFIG_IBVERBS
    mpid_nem_newmad_num_rails++;
#endif
#if defined CONFIG_MX
    mpid_nem_newmad_num_rails++;
#endif
#if defined CONFIG_GM
    mpid_nem_newmad_num_rails++;
#endif
#if defined CONFIG_QSNET
    mpid_nem_newmad_num_rails++;
#endif
#if defined CONFIG_TCP
    mpid_nem_newmad_num_rails++;
#endif
    MPIU_Assert(mpid_nem_newmad_num_rails <= MPID_NEM_NMAD_MAX_NETS);

    p_driver_load_array = (puk_component_t *)MPIU_Malloc( mpid_nem_newmad_num_rails*sizeof(puk_component_t));

#if defined CONFIG_IBVERBS
    p_driver_load_array[index++] = nm_core_component_load("driver", "ibverbs");
#endif
#if defined CONFIG_MX
    p_driver_load_array[index++] = nm_core_component_load("driver", "mx");
#endif
#if defined CONFIG_GM
    p_driver_load_array[index++] = nm_core_component_load("driver", "gm");
#endif
#if defined CONFIG_QSNET
    p_driver_load_array[index++] = nm_core_component_load("driver", "qsnet");
#endif
#if defined CONFIG_TCP
    p_driver_load_array[index++] = nm_core_component_load("driver", "tcp");
#endif
}
#else /* MPID_MAD_MODULE_MULTIRAIL */
static puk_component_t  p_driver_load;
static void mpid_nem_newmad_rails(void)
{
# if defined CONFIG_IBVERBS
    p_driver_load = nm_core_component_load("driver", "ibverbs");
# elif defined CONFIG_MX
    p_driver_load = nm_core_component_load("driver", "mx");
# elif defined CONFIG_GM
    p_driver_load = nm_core_component_load("driver", "gm");
# elif defined CONFIG_QSNET
    p_driver_load = nm_core_component_load("driver", "qsnet");
# elif defined CONFIG_TCP
    p_driver_load = nm_core_component_load("driver", "tcp");
# endif
}
#endif /* MULTIRAIL */


#undef FUNCNAME
#define FUNCNAME init_mad
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init_mad( MPIDI_PG_t *pg_p )
{
#if 0
#ifdef CONFIG_TCP
#ifndef MPID_MAD_MODULE_MULTIRAIL
    char  hostname[16];
#endif /* TCP */
#endif /* MPID_MAD_MODULE_MULTIRAIL */
#endif
    int   index = 0;
    int   ret;
    int   mpi_errno = MPI_SUCCESS;
    char *dummy_argv[1] = {NULL};
    int   dummy_argc    = 1;
    
    ret = nm_core_init(&dummy_argc,dummy_argv, &mpid_nem_newmad_pcore);
    if (ret != NM_ESUCCESS){
        fprintf(stdout,"nm_core_init returned err = %d\n", ret);
    }

    mpid_nem_newmad_rails();
#ifdef MPID_MAD_MODULE_MULTIRAIL
    fprintf(stdout,"Number of rails : %i\n",nem_mad_num_rail);
#endif /* MPID_MAD_MODULE_MULTIRAIL */

    ret = nm_sr_init(mpid_nem_newmad_pcore);
    if(ret != NM_ESUCCESS) {
	fprintf(stdout,"nm_so_pack_interface_init return err = %d\n", ret);
    }
#ifdef MPID_MAD_MODULE_MULTIRAIL
#warning "========== MAD MODULE MULTIRAIL CODE ENABLED ============="
    ret = nm_core_driver_load_init_some(mpid_nem_newmad_pcore, mpid_nem_newmad_num_rails, 
					p_driver_load_array, drv_id, url);
#else /* MPID_MAD_MODULE_MULTIRAIL */
    ret = nm_core_driver_load_init(mpid_nem_newmad_pcore, 
				   p_driver_load, &drv_id[0], &url[0]);
#endif /* MPID_MAD_MODULE_MULTIRAIL */
    if (ret != NM_ESUCCESS) {
	fprintf(stdout,"nm_core_driver_init(some) returned ret = %d\n", ret);
    }
    
#if 0
#ifdef CONFIG_TCP
#ifndef MPID_MAD_MODULE_MULTIRAIL
    {
	gethostname(hostname, 16);
	strcat(hostname,":");
	strcat(hostname,url[0]);
	url[0] = (char *)MPIU_Malloc(strlen(hostname)+1);
	strcpy(url[0],hostname);
    }
#endif /* !MULTIRAIL  */
#endif /* TCP */                                                                                                                         
#endif  
    
   nm_ns_init(mpid_nem_newmad_pcore);
   
   init_reqs = (MPID_nem_newmad_init_req_t *)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(MPID_nem_newmad_init_req_t));
   for (index = 0; index < MPID_nem_mem_region.num_procs ; index ++)
   {
      memset(init_reqs + index,0,sizeof(MPID_nem_newmad_init_req_t));
      init_reqs[index].process_no  = -2;
      
      if ( !(MPID_NEM_IS_LOCAL(index)) && (index > mpid_nem_newmad_myrank))
	num_recv_req++;
      else if ( !(MPID_NEM_IS_LOCAL(index)) && (index < mpid_nem_newmad_myrank))
	num_send_req++;
   }
   
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}



/*
 int  
   MPID_nem_newmad_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
	          MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		  MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		      MPID_nem_queue_ptr_t proc_free_queue, 
		      MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
		      MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		      MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		      MPIDI_PG_t *pg_p, int pg_rank,
		      char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;
   int index;

   fprintf(stdout,"Size of MPID_nem_mad_module_vc_area_internal_t : %i | size of nm_sr_request_t :%i | Size of req_area : %i\n",
         sizeof(MPID_nem_newmad_vc_area_internal_t),sizeof(nm_sr_request_t), sizeof(MPID_nem_newmad_req_area));
   //MPIU_Assert( sizeof(MPID_nem_newmad_vc_area_internal_t) <= MPID_NEM_VC_NETMOD_AREA_LEN);
   MPIU_Assert( sizeof(MPID_nem_newmad_req_area) <= MPID_NEM_REQ_NETMOD_AREA_LEN);

   mpid_nem_newmad_myrank = pg_rank;
   for (index = 0; index <  MPID_NEM_NMAD_MAX_NETS ; index++)
   {
       drv_id[index] = -1;
       url[index]    = NULL;
   }

   init_mad(pg_p);

   mpi_errno = MPID_nem_newmad_get_business_card(pg_rank,bc_val_p, val_max_sz_p);
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   mpi_errno = MPIDI_CH3I_Register_anysource_notification(MPID_nem_newmad_anysource_posted, MPID_nem_newmad_anysource_matched);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);

   mpi_errno = MPID_nem_register_initcomp_cb(MPID_nem_newmad_post_init);
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
   char name[MPID_NEM_NMAD_MAX_SIZE];
   int index;
   
   gethostname(name,MPID_NEM_NMAD_MAX_SIZE);

   mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_HOSTNAME_KEY, name, strlen(name));
   if (mpi_errno != MPIU_STR_SUCCESS){
       if (mpi_errno == MPIU_STR_NOMEM){
	   MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
       }
       else{
	   MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
       }
       goto fn_exit;
   }
   
   for(index = 0 ; index < mpid_nem_newmad_num_rails ; index ++){
       mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, url_keys[index], url[index], strlen(url[index]));
       if (mpi_errno != MPIU_STR_SUCCESS){
	   if (mpi_errno == MPIU_STR_NOMEM){
	       MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	   }
	   else{
	       MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	   }
	   goto fn_exit;
       }
   }

   fn_exit:
       return mpi_errno;
   fn_fail: ATTRIBUTE((unused))
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_get_from_bc (const char *business_card, char *hostname, char *url, int index)
{
   int mpi_errno = MPI_SUCCESS;
   int len;
   
   mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_HOSTNAME_KEY, hostname, 
					MPID_NEM_NMAD_MAX_SIZE, &len);
   if ((mpi_errno != MPIU_STR_SUCCESS)){
       MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
   }

   mpi_errno = MPIU_Str_get_binary_arg (business_card, url_keys[index], url, 
					MPID_NEM_NMAD_MAX_SIZE, &len);
   if ((mpi_errno != MPIU_STR_SUCCESS)){
       MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
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
#define FUNCNAME MPID_nem_newmad_send_init_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_newmad_send_init_msg(nm_sr_event_t event, const nm_sr_event_info_t*info)
{
   --num_send_req;
   return;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_recv_init_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_newmad_recv_init_msg(nm_sr_event_t event, const nm_sr_event_info_t*info)
{
   nm_sr_request_t            *p_request = info->recv_completed.p_request;
   MPID_nem_newmad_init_req_t *init_req;
   MPIDI_PG_t                 *pg_p;
   int                         index;
   
   nm_sr_get_ref(mpid_nem_newmad_pcore, p_request, (void *)&init_req);
   MPIU_Assert(init_req != NULL);
   
   /* get pg for comm world */
   pg_p = MPIDI_Process.my_pg;
   for (index = 0; index < pg_p->size ; index++)
   {
      if((index != mpid_nem_newmad_myrank) && !(MPID_NEM_IS_LOCAL(index))) 
      {
	 MPIDI_VC_t *vc;
	 MPIDI_PG_Get_vc_set_active(pg_p, index, &vc);
	 if (vc->lpid == init_req->process_no)
	 {
	    VC_FIELD(vc, p_gate) = init_req->p_gate;
	    nm_gate_ref_set(VC_FIELD(vc, p_gate),(void*)vc);
	    break;
	 }
      }
   }
   --num_recv_req;
   return;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_vc_init (MPIDI_VC_t *vc)
{
    MPIDI_CH3I_VC           *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    char                     business_card[MPID_NEM_NMAD_MAX_SIZE];
    int                      mpi_errno = MPI_SUCCESS;   
    int                      ret;
    int                      index;

    mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card,MPID_NEM_NMAD_MAX_SIZE, vc->pg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
       
    (((MPID_nem_newmad_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->area) =
	(MPID_nem_newmad_vc_area_internal_t *)MPIU_Malloc(sizeof(MPID_nem_newmad_vc_area_internal_t));

    MPIU_Assert( (((MPID_nem_newmad_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->area) != NULL);

   ret = nm_core_gate_init(mpid_nem_newmad_pcore, &(init_reqs[vc->lpid].p_gate));
   if (ret != NM_ESUCCESS) fprintf(stdout,"nm_core_gate_init returned ret = %d\n", ret);
   
    for(index = 0 ; index < mpid_nem_newmad_num_rails ; index ++)
    {	
	mpi_errno = MPID_nem_newmad_get_from_bc (business_card, VC_FIELD(vc, hostname), VC_FIELD(vc, url[index]), index);
	if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	
	VC_FIELD(vc, drv_id[index]) = drv_id[index];
	
	if (vc->lpid > mpid_nem_newmad_myrank){
	    ret = nm_core_gate_accept(mpid_nem_newmad_pcore,init_reqs[vc->lpid].p_gate, drv_id[index], NULL);
	}
	else if (vc->lpid < mpid_nem_newmad_myrank){
	    ret = nm_core_gate_connect(mpid_nem_newmad_pcore,init_reqs[vc->lpid].p_gate, drv_id[index], VC_FIELD(vc, url[index]));
	}
    }

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
#define FUNCNAME MPID_nem_newmad_post_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_post_init(void)
{
   MPIDI_PG_t *pg_p;
   int         index;

   pg_p = MPIDI_Process.my_pg;
   for (index = 0; index < pg_p->size ; index++)
   {
      if((index != mpid_nem_newmad_myrank) && !(MPID_NEM_IS_LOCAL(index))) 
      {
	 MPIDI_VC_t *vc;
	 MPIDI_PG_Get_vc_set_active(pg_p, index, &vc);

	 if (vc->lpid > mpid_nem_newmad_myrank)
	 {
	    nm_sr_irecv_with_ref(mpid_nem_newmad_pcore,init_reqs[vc->lpid].p_gate,0xfffffffc,
				 &(init_reqs[vc->lpid].process_no),sizeof(int),&(init_reqs[vc->lpid].init_request),
				 (void *)&(init_reqs[vc->lpid]));
	      
	    nm_sr_request_monitor(mpid_nem_newmad_pcore,&(init_reqs[vc->lpid].init_request),NM_SR_EVENT_RECV_COMPLETED,
				  &MPID_nem_newmad_recv_init_msg);
	 }
	 else if (vc->lpid < mpid_nem_newmad_myrank)
	 {
	    nm_gate_ref_set(init_reqs[vc->lpid].p_gate,(void*)vc);
	    VC_FIELD(vc, p_gate) = init_reqs[vc->lpid].p_gate;
	    
	    nm_sr_isend(mpid_nem_newmad_pcore,init_reqs[vc->lpid].p_gate,0xfffffffc,&mpid_nem_newmad_myrank,sizeof(int),
			&(init_reqs[vc->lpid].init_request));
	    
	    nm_sr_request_monitor(mpid_nem_newmad_pcore,&(init_reqs[vc->lpid].init_request),NM_SR_EVENT_SEND_COMPLETED,
				  &MPID_nem_newmad_send_init_msg);
	 }
      }
   }
   
   while((num_recv_req > 0) || (num_send_req > 0))
     nm_schedule(mpid_nem_newmad_pcore);
   MPIU_Free(init_reqs);
   
   nm_sr_monitor(mpid_nem_newmad_pcore, NM_SR_EVENT_RECV_UNEXPECTED,&MPID_nem_newmad_get_adi_msg);
   nm_sr_monitor(mpid_nem_newmad_pcore, NM_SR_EVENT_RECV_COMPLETED, &MPID_nem_newmad_get_rreq);
   nm_sr_monitor(mpid_nem_newmad_pcore, NM_SR_EVENT_SEND_COMPLETED, &MPID_nem_newmad_handle_sreq);
   
   return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    MPIU_Free((((MPID_nem_newmad_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->area));

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
    return MPI_SUCCESS;
}


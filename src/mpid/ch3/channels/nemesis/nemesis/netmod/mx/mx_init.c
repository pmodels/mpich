/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */



#include "mpid_nem_impl.h"
#include "mx_impl.h"

MPID_nem_netmod_funcs_t MPIDI_nem_mx_funcs = {
    MPID_nem_mx_init,
    MPID_nem_mx_finalize,
    MPID_nem_mx_poll,
    MPID_nem_mx_get_business_card,
    MPID_nem_mx_connect_to_root,
    MPID_nem_mx_vc_init,
    MPID_nem_mx_vc_destroy,
    MPID_nem_mx_vc_terminate,
    MPID_nem_mx_anysource_iprobe
};

static MPIDI_Comm_ops_t comm_ops = {
    MPID_nem_mx_directRecv, /* recv_posted */
    
    MPID_nem_mx_directSend, /* send */
    MPID_nem_mx_directSend, /* rsend */
    MPID_nem_mx_directSsend, /* ssend */
    MPID_nem_mx_directSend, /* isend */
    MPID_nem_mx_directSend, /* irsend */
    MPID_nem_mx_directSsend, /* issend */
    
    NULL,                   /* send_init */
    NULL,                   /* bsend_init */
    NULL,                   /* rsend_init */
    NULL,                   /* ssend_init */
    NULL,                   /* startall */
    
    MPID_nem_mx_cancel_send,/* cancel_send */
    MPID_nem_mx_cancel_recv, /* cancel_recv */
    
    MPID_nem_mx_probe, /* probe */
    MPID_nem_mx_iprobe /* iprobe */
};


#define MPIDI_CH3I_ENDPOINT_KEY "endpoint_id"
#define MPIDI_CH3I_NIC_KEY      "nic_id"

int           MPID_nem_mx_pending_send_req = 0;
uint32_t      MPID_NEM_MX_FILTER = 0xabadbada;
uint64_t      MPID_nem_mx_local_nic_id;
uint32_t      MPID_nem_mx_local_endpoint_id;
mx_endpoint_t MPID_nem_mx_local_endpoint;

#undef FUNCNAME
#define FUNCNAME init_mx
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init_mx( MPIDI_PG_t *pg_p )
{
   mx_endpoint_addr_t local_endpoint_addr;
   mx_return_t        ret;
   mx_param_t         param;
   int                mpi_errno = MPI_SUCCESS;
   int                r;

   r = MPL_putenv("MX_DISABLE_SHARED=1");
   MPIU_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");
   r = MPL_putenv("MX_DISABLE_SELF=1");
   MPIU_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");

   ret = mx_init();
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_init", "**mx_init %s", mx_strerror (ret));
   
   mx_set_error_handler(MX_ERRORS_RETURN);

   /*
   ret = mx_get_info(NULL, MX_NIC_COUNT, NULL, 0, &nic_count, sizeof(int));
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_info", "**mx_get_info %s", mx_strerror (ret));
   
   count = ++nic_count;
   mx_nics = (uint64_t *)MPIU_Malloc(count*sizeof(uint64_t));
   ret = mx_get_info(NULL, MX_NIC_IDS, NULL, 0, mx_nics, count*sizeof(uint64_t));
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_info", "**mx_get_info %s", mx_strerror (ret));
    
    do{	     
      ret = mx_nic_id_to_board_number(mx_nics[index],&mx_board_num);
      index++;
   }while(ret != MX_SUCCESS);
   */
#ifndef USE_CTXT_AS_MARK
   param.key = MX_PARAM_CONTEXT_ID;
   param.val.context_id.bits  = NEM_MX_MATCHING_BITS - SHIFT_TYPE;
   param.val.context_id.shift = SHIFT_TYPE;
   ret = mx_open_endpoint(MX_ANY_NIC,MX_ANY_ENDPOINT,MPID_NEM_MX_FILTER,&param,1,&MPID_nem_mx_local_endpoint);
#else
   ret = mx_open_endpoint(MX_ANY_NIC,MX_ANY_ENDPOINT,MPID_NEM_MX_FILTER,NULL,0,&MPID_nem_mx_local_endpoint);
#endif
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_open_endpoint", "**mx_open_endpoint %s", mx_strerror (ret));
      
   ret = mx_get_endpoint_addr(MPID_nem_mx_local_endpoint,&local_endpoint_addr);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_endpoint_addr", "**mx_get_endpoint_addr %s", mx_strerror (ret));   
   
   ret = mx_decompose_endpoint_addr(local_endpoint_addr,&MPID_nem_mx_local_nic_id,&MPID_nem_mx_local_endpoint_id);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_decompose_endpoint_addr", "**mx_decompose_endpoint_addr %s", mx_strerror (ret));
   
 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;
}

/*
 int  
   MPID_nem_mx_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
	          MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		  MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
   OUT
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_init (MPIDI_PG_t *pg_p, int pg_rank,
		  char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;

   MPID_nem_mx_internal_req_queue_init();
   
   mpi_errno = init_mx(pg_p);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);

   mpi_errno = MPID_nem_mx_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   mx_register_unexp_handler(MPID_nem_mx_local_endpoint,MPID_nem_mx_get_adi_msg,NULL);

   mpi_errno = MPIDI_CH3I_Register_anysource_notification(MPID_nem_mx_anysource_posted, 
							  MPID_nem_mx_anysource_matched);
   if (mpi_errno) MPIU_ERR_POP(mpi_errno);

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;

    str_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ENDPOINT_KEY, MPID_nem_mx_local_endpoint_id);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }
    
    str_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_NIC_KEY, (char *)&MPID_nem_mx_local_nic_id, sizeof(uint64_t));
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
#define FUNCNAME MPID_nem_mx_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_get_from_bc(const char *business_card, uint32_t *remote_endpoint_id, uint64_t *remote_nic_id)
{
   int mpi_errno = MPI_SUCCESS;
   int str_errno = MPIU_STR_SUCCESS;
   int len;
   int tmp_endpoint_id;
   
   mpi_errno = MPIU_Str_get_int_arg(business_card, MPIDI_CH3I_ENDPOINT_KEY, &tmp_endpoint_id);
   /* FIXME: create a real error string for this */
   MPIU_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**argstr_hostd");
   *remote_endpoint_id = (uint32_t)tmp_endpoint_id;
   
   mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_NIC_KEY, (char *)remote_nic_id, sizeof(uint64_t), &len);
   /* FIXME: create a real error string for this */
   MPIU_ERR_CHKANDJUMP(str_errno || len != sizeof(uint64_t), mpi_errno, MPI_ERR_OTHER, "**argstr_hostd");
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_vc_init (MPIDI_VC_t *vc)
{
   uint32_t threshold;
   MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
   int mpi_errno = MPI_SUCCESS;

   /* first make sure that our private fields in the vc fit into the area provided  */
   MPIU_Assert(sizeof(MPID_nem_mx_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

#ifdef ONDEMAND
   VC_FIELD(vc, local_connected)  = 0;
   VC_FIELD(vc, remote_connected) = 0;
#else
   {
       char *business_card;
       int   val_max_sz;
       int   ret;
#ifdef USE_PMI2_API
       val_max_sz = PMI2_MAX_VALLEN;
#else
       mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
#endif 
       business_card = (char *)MPIU_Malloc(val_max_sz); 
       mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card,val_max_sz, vc->pg);
       if (mpi_errno) MPIU_ERR_POP(mpi_errno);
       
       mpi_errno = MPID_nem_mx_get_from_bc (business_card, &VC_FIELD(vc, remote_endpoint_id), &VC_FIELD(vc, remote_nic_id));
       if (mpi_errno)    MPIU_ERR_POP (mpi_errno);

       MPIU_Free(business_card);
       
       ret = mx_connect(MPID_nem_mx_local_endpoint,VC_FIELD(vc, remote_nic_id),VC_FIELD(vc, remote_endpoint_id),
			MPID_NEM_MX_FILTER,MX_INFINITE,&(VC_FIELD(vc, remote_endpoint_addr)));
       MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_connect", "**mx_connect %s", mx_strerror (ret));
       mx_set_endpoint_addr_context(VC_FIELD(vc, remote_endpoint_addr),(void *)vc);

       MPIDI_CHANGE_VC_STATE(vc, ACTIVE);
   }
#endif
   mx_get_info(MPID_nem_mx_local_endpoint, MX_COPY_SEND_MAX, NULL, 0, &threshold, sizeof(uint32_t));

   vc->eager_max_msg_sz = threshold;
   vc->rndvSend_fn      = NULL;
   vc->sendNoncontig_fn = MPID_nem_mx_SendNoncontig;
   vc->comm_ops         = &comm_ops;
 
   vc_ch->iStartContigMsg = MPID_nem_mx_iStartContigMsg;
   vc_ch->iSendContig     = MPID_nem_mx_iSendContig;

 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    /* free any resources associated with this VC here */

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_vc_terminate (MPIDI_VC_t *vc)
{
    /* FIXME: Check to make sure that it's OK to terminate the
       connection without making sure that all sends have been sent */
    return MPIDI_CH3U_Handle_connection (vc, MPIDI_VC_EVENT_TERMINATED);
}

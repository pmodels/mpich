/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

/* helper functions */
char *iba_error(int error, char *file, int line)
{
    return NULL;
}

char *iba_compstatus(int status, char *file, int line)
{
    return NULL;
}

/* minimal set of functions */

ib_int32_t ib_init_us(void)
{
    return IB_SUCCESS;
}

ib_int32_t ib_release_us(void)
{
    return IB_SUCCESS;
}

ib_int32_t ib_hca_open_us(ib_uint32_t index,
			  ib_hca_handle_t * handle_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_hca_query_us(ib_hca_handle_t hca_handle, 
			   ib_hca_attr_t * hca_attr_p, 
			   ib_uint32_t hca_attr_flags, // combination of Ib_hca_query_flag values
			   ib_uint32_t * attr_size_p	// IN/OUT size/required size of buffer hca_attr_p
			   )
{
    return IB_SUCCESS;
}

ib_int32_t ib_pd_allocate_us(ib_hca_handle_t hca_handle,
			     ib_pd_handle_t * pd_handle_p)
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_register_us(ib_hca_handle_t   hca_handle,
			     ib_uint8_t * va,
			     ib_uint32_t byte_num,
			     ib_pd_handle_t pd_handle,
			     ib_uint32_t access_control,
			     ib_mr_handle_t * mr_handle,
			     ib_uint32_t * l_key,
			     ib_uint32_t * r_key)
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_deregister_us(ib_hca_handle_t hca_handle,
			       ib_mr_handle_t  mr_handle)
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_create_us( ib_hca_handle_t hca_handle,
			   ib_pd_handle_t  pd_handle,
			   ib_qp_attr_list_t * attr_list,
			   ib_qp_handle_t * qp_handle,
			   ib_uint32_t * qpn,
			   void * user_context_async_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_modify_us( ib_hca_handle_t hca_handle,
			   ib_qp_handle_t qp_handle,
			   Ib_qp_state qp_state,
			   ib_qp_attr_list_t * attr_list )
{
    return IB_SUCCESS;
}

ib_int32_t ib_cqd_create_us (  ib_hca_handle_t hca_handle,
			     ib_cqd_handle_t * cqd_handle_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_cq_create_us( ib_hca_handle_t hca_handle,
			   ib_cqd_handle_t   cqd_handle,
			   ib_uint32_t * num_entries_p,
			   ib_cq_handle_t * cq_handle_p,
			   void * user_context_async_p)
{
    return IB_SUCCESS;
}

ib_int32_t ib_cq_destroy_us(ib_hca_handle_t hca_handle,
			    ib_cq_handle_t  cq_handle)
{
    return IB_SUCCESS;
}

ib_int32_t ib_post_send_req_us(ib_hca_handle_t hca_handle,
			       ib_qp_handle_t qp_handle,
			       ib_work_req_send_t * work_req)
{
    return IB_SUCCESS;
}

ib_int32_t ib_post_rcv_req_us(ib_hca_handle_t hca_handle,
			      ib_qp_handle_t qp_handle,
			      ib_work_req_rcv_t * work_req)
{
    return IB_SUCCESS;
}

ib_int32_t ib_completion_poll_us(ib_hca_handle_t hca_handle,
				 ib_cq_handle_t  cq_handle,
				 ib_work_completion_t * work_c)
{
    return IB_SUCCESS;
}

#if 0

/* complete set of functions */

/************ Init/Close **********************/
ib_int32_t ib_init_us(void)
{
    return IB_SUCCESS;
}

ib_int32_t ib_release_us(void)
{
    return IB_SUCCESS;
}

/************ General OS services ************/
/* return amount of registered HCAs */
ib_uint32_t ib_enum_hca_if_us(void)
{
    return IB_SUCCESS;
}

/* fill out the array of HCA structures */
ib_int32_t ib_get_hca_if_us( hca_name_t * hca_p, 
			    ib_uint32_t* size_p /* in-out parameter */ )
{
    return IB_SUCCESS;
}

ib_int32_t ib_get_hca_name_us( ib_hca_handle_t hca_handle, hca_name_t name )
{
    return IB_SUCCESS;
}

/************ HCA *************************/
ib_int32_t ib_hca_open_us( const char * hca_name,
			  ib_hca_handle_t * handle_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_hca_query_us(ib_hca_handle_t hca_handle, 
			   ib_hca_attr_t * hca_attr_p, 
			   ib_uint32_t hca_attr_flags )
{
    return IB_SUCCESS;
}

ib_int32_t ib_hca_close_us(ib_hca_handle_t hca_handle)
{
    return IB_SUCCESS;
}


ib_int32_t ib_pd_allocate_us(ib_hca_handle_t hca_handle,
			     ib_pd_handle_t * pd_handle_p)
{
    return IB_SUCCESS;
}

ib_int32_t ib_pd_deallocate_us(ib_hca_handle_t hca_handle,
			       ib_pd_handle_t  pd_handle)
{
    return IB_SUCCESS;
}

ib_int32_t ib_rdd_allocate_us( ib_hca_handle_t hca_handle,
			      ib_rdd_handle_t * rdd_handle_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_rdd_deallocate_us( ib_hca_handle_t hca_handle,
				ib_rdd_handle_t rdd_handle )
{
    return IB_SUCCESS;
}

/************ Address management ***********/
ib_int32_t ib_address_create_us(ib_hca_handle_t  hca_handle,
				ib_pd_handle_t pd_handle,
				ib_address_vector_t * ad_vec,
				ib_address_handle_t * ad_handle)
{
    return IB_SUCCESS;
}

ib_int32_t ib_address_modify_us(ib_hca_handle_t hca_handle,
				ib_address_handle_t ad_handle,
				ib_address_vector_t * ad_vec)
{
    return IB_SUCCESS;
}

ib_int32_t ib_query_address_handle_us( ib_hca_handle_t hca_handle,
				      ib_address_handle_t ad_handle,
				      ib_pd_handle_t * pd_handle,
				      ib_address_vector_t * ad_vec )
{
    return IB_SUCCESS;
}

ib_int32_t ib_address_destroy_us(ib_hca_handle_t  hca_handle,
				 ib_address_handle_t  ad_handle)
{
    return IB_SUCCESS;
}

/************ Queue Pair *******************/
ib_int32_t ib_qp_create_us( ib_hca_handle_t hca_handle,
			   ib_pd_handle_t  pd_handle,
			   ib_qp_attr_list_t * attr_list,
			   ib_qp_handle_t * qp_handle,
			   ib_uint32_t * qpn,
			   void * user_context_async_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_modify_us( ib_hca_handle_t hca_handle,
			   ib_qp_handle_t qp_handle,
			   Ib_qp_state qp_state,
			   ib_qp_attr_list_t * attr_list )
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_query_us( ib_hca_handle_t hca_handle,
			  ib_qp_handle_t qp_handle,
			  ib_qp_attr_list_t * attr_list )
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_destroy_us( ib_hca_handle_t hca_handle,
			    ib_qp_handle_t qp_handle )
{
    return IB_SUCCESS;
}

ib_int32_t ib_qp_get_special ( ib_hca_handle_t hca_handle,
			      ib_uint8_t port_num,
			      Ib_sp_qp_type qp_type,
			      ib_pd_handle_t pd_handle,
			      ib_qp_attr_list_t * attr_list,
			      ib_qp_handle_t * qp_handle)
{
    return IB_SUCCESS;
}

/************ Completion Domains *******************/
ib_int32_t ib_cqd_create_us (  ib_hca_handle_t hca_handle,
			     ib_cqd_handle_t * cqd_handle_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_cqd_destroy_us (  ib_hca_handle_t hca_handle,
			      ib_cqd_handle_t cqd_handle_p )
{
    return IB_SUCCESS;
}

/************ Completion Queue *******************/
ib_int32_t ib_cq_create_us( ib_hca_handle_t hca_handle,
			   ib_cqd_handle_t   cqd_handle,
			   ib_uint32_t * num_entries_p,
			   ib_cq_handle_t * cq_handle_p,
			   void * user_context_async_p)
{
    return IB_SUCCESS;
}

ib_int32_t ib_cq_query_us( ib_hca_handle_t hca_handle,
			  ib_cq_handle_t cq_handle,
			  ib_uint32_t * no_of_entries )
{
    return IB_SUCCESS;
}

ib_int32_t ib_cq_resize_us( ib_hca_handle_t hca_handle,
			   ib_cq_handle_t cq_handle,
			   ib_uint32_t * no_of_entries )
{
    return IB_SUCCESS;
}

ib_int32_t ib_cq_destroy_us(ib_hca_handle_t hca_handle,
			    ib_cq_handle_t  cq_handle)
{
    return IB_SUCCESS;
}

/************ EE context *************************/
ib_int32_t ib_eec_create_us (ib_hca_handle_t hca_handle,
			     ib_rdd_handle_t rdd_handle, /* IN */
			     ib_eec_handle_t * eec_handle,/* OUT */
			     void * user_context_async_p)
{
    return IB_SUCCESS;
}

ib_int32_t ib_eec_modify_us( ib_hca_handle_t hca_handle,
			    ib_eec_handle_t eec_handle, /* IN */
			    Ib_ee_state eec_state, 
			    ib_eec_attr_list_t * eec_attr )/* IN */
{
    return IB_SUCCESS;
}

ib_int32_t ib_eec_query_us( ib_hca_handle_t hca_handle,
			   ib_eec_handle_t  eec_handle, /* IN */
			   /* IN-OUT:attributes type is filled by requestor */
			   /* attributes value is filled by the verbs */
			   ib_eec_attr_list_t * eec_attr )
{
    return IB_SUCCESS;
}

ib_int32_t ib_eec_destroy_us( ib_hca_handle_t hca_handle,
			     ib_eec_handle_t eec_handle )
{
    return IB_SUCCESS;
}

/************ Memory management ******************/
ib_int32_t ib_mr_register_us(ib_hca_handle_t   hca_handle,
			     ib_uint8_t * va,
			     ib_uint32_t byte_num,
			     ib_pd_handle_t pd_handle,
			     ib_uint32_t access_control,
			     ib_mr_handle_t * mr_handle,
			     ib_uint32_t * l_key,
			     ib_uint32_t * r_key)
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_phy_register_us( ib_hca_handle_t hca_handle,   /* in - hca handle */
				 ib_uint32_t phy_buf_num,   /* in - total number of Physical Buffers in the list */
				 ib_uint8_t ** phy_addr,   /* in - list of Physical Buffers. Each buffer must begin and end on an HCA-supported page boundary. */
				 ib_uint8_t ** iova,   /* in-out - I/O Virtual Address for the first ib_uint8_t of the region. Initially the value requested by the consumer; actually assigned value on return. */
				 ib_uint32_t byte_num,   /* in - length of region to be registered in bytes */
				 ib_uint32_t offset,   /* in - offset of Region's starting IOVA within the first physical buffer. */
				 ib_pd_handle_t pd_handle,   /* in - Protection Domain to be assigned to the registered region. */
				 ib_uint32_t access_control,   /* in - access control:compute by or of the ib_access_control( see "ib_defs.h" ). */
				 ib_mr_handle_t * mr_handle,   /* out - Memory Region Handle */
				 ib_uint32_t * l_key,   /* out - L_Key */
				 ib_uint32_t * r_key)   /* out - R_Key */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_query_us( ib_hca_handle_t hca_handle,
			  ib_mr_handle_t mr_handle,
			  ib_uint32_t * l_key,
			  ib_uint32_t * r_key,
			  ib_uint8_t ** va_local_access,   /* Page aligned protection bounds */
			  ib_uint32_t * byte_num_local,      
			  ib_uint8_t ** va_remote_access,  /* Byte aligned protection bounds */
			  ib_uint32_t * byte_num_remote,     /* as requested by the consumer */
			  ib_pd_handle_t * pd_handle,
			  ib_uint32_t * access_control )
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_deregister_us(ib_hca_handle_t hca_handle,
			       ib_mr_handle_t  mr_handle)
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_reregister_us( ib_hca_handle_t hca_handle,
			       ib_mr_handle_t * mr_handle,  /* IN-OUT: could be changed */
			       ib_uint32_t * l_key,        /* OUT */
			       ib_uint32_t * r_key,        /* OUT: if remote access needed */
			       ib_mr_reregister_req_t * reregister_req )/* IN */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_phy_reregister_us( ib_hca_handle_t hca_handle,
				   ib_mr_handle_t * mr_handle,  /* IN-OUT: could be changed */
				   ib_uint8_t ** iova,       /* OUT */
				   ib_uint32_t * l_key,        /* OUT */
				   ib_uint32_t * r_key,        /* OUT */
				   ib_mr_reregister_req_t * reregsiter_req ) /* IN */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mr_shared_register_us( ib_hca_handle_t hca_handle,
				    ib_mr_handle_t *    mr_handle, /* IN-OUT: could be changed */
				    ib_pd_handle_t     pd_handle, /* IN */
				    ib_uint32_t access_control, /* IN */
				    ib_uint32_t * l_key,         /* OUT */
				    ib_uint32_t * r_key,         /* OUT: if remote access needed */
				    ib_uint8_t ** va )         /* IN-OUT: virt. addr. to register */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mw_allocate_us( ib_hca_handle_t hca_handle,   /* in - hca handle */
			     ib_pd_handle_t pd_handle,   /* in - Protection Domain handle */
			     ib_mw_handle_t * mw_handle,   /* out - Memory Window Handle */
			     ib_uint32_t * r_key)   /* out - R_Key */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mw_query_us( ib_hca_handle_t hca_handle,   /* in - hca handle */
			  ib_mw_handle_t mw_handle,   /* in - Memory Window Handle */
			  ib_pd_handle_t * pd_handle,   /* out - Protection Domain handle */
			  ib_uint32_t * r_key)   /* out - R_Key */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mw_bind_us( ib_hca_handle_t hca_handle,   /* in - hca handle */
			 ib_qp_handle_t qp_handle,   /* in - qp handle */
			 ib_mw_handle_t mw_handle,   /* in - Memory Window Handle */
			 ib_mr_handle_t mr_handle,   /* in - Memory Region Handle */
			 ib_uint64_t wr_id,
			 ib_uint32_t * r_key,   /* in-out - R_Key */
			 ib_uint32_t l_key,   /* in - L_Key */
			 ib_uint8_t * va,   /* in - the address of the first ib_uint8_t of the region to be registered */
			 ib_uint32_t byte_num,   /* in - length of region to be registered */
			 ib_uint32_t access_control,   /* in - access control */
			 ib_bool_t signaled_f)   /* completion notification indicator (if the Send Queue was set up for selectable signaling) */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mw_deallocate_us( ib_hca_handle_t hca_handle,   /* in - hca handle */
			       ib_mw_handle_t mw_handle )   /* in - Memory Window Handle */
{
    return IB_SUCCESS;
}

/********** Multicast groups *********/

ib_int32_t ib_mg_qp_attach_us( ib_hca_handle_t hca_handle,
			      LID   mg_dlid,            /* IN */
			      GID   mg_address,         /* IN */
			      ib_qp_handle_t qp_handle ) /* IN */
{
    return IB_SUCCESS;
}

ib_int32_t ib_mg_qp_detach_us( ib_hca_handle_t hca_handle,
			      LID mg_dlid,              /* IN */
			      GID mg_address,           /* IN */
			      ib_qp_handle_t qp_handle ) /* IN */
{
    return IB_SUCCESS;
}

/************ Work request processing ******************/
ib_int32_t ib_post_send_req_us(ib_hca_handle_t hca_handle,
			       ib_qp_handle_t qp_handle,
			       ib_work_req_send_t * work_req)
{
    return IB_SUCCESS;
}

ib_int32_t ib_post_rcv_req_us(ib_hca_handle_t hca_handle,
			      ib_qp_handle_t qp_handle,
			      ib_work_req_rcv_t * work_req)
{
    return IB_SUCCESS;
}

ib_int32_t ib_completion_poll_us(ib_hca_handle_t hca_handle,
				 ib_cq_handle_t  cq_handle,
				 ib_work_completion_t * work_c)
{
    return IB_SUCCESS;
}

ib_int32_t ib_completion_notify_us(ib_hca_handle_t hca_handle,
				   ib_cq_handle_t  cq_handle,
				   Ib_notification_type type)
{
    return IB_SUCCESS;
}

/************ Event handling ******************/
ib_int32_t ib_set_completion_eh_us ( ib_hca_handle_t hca_handle,
				    ib_cq_handle_t cq_handle,
				    ib_cq_comp_handler_t handler,
				    void * user_context_cq_p )
{
    return IB_SUCCESS;
}

ib_int32_t ib_set_cq_async_error_eh_us ( ib_hca_handle_t hca_handle,
					ib_cq_async_handler_t handler )
{
    return IB_SUCCESS;
}

ib_int32_t ib_set_qp_async_error_eh_us ( ib_hca_handle_t hca_handle,
					ib_qp_async_handler_t handler )
{
    return IB_SUCCESS;
}

ib_int32_t ib_set_ee_async_error_eh_us (ib_hca_handle_t	hca_handle,
					ib_ee_async_handler_t handler )
{
    return IB_SUCCESS;
}

ib_int32_t ib_set_un_async_error_eh_us ( ib_hca_handle_t hca_handle,
					ib_un_async_handler_t handler,
					void * user_context_un_async_p )
{
    return IB_SUCCESS;
}

#endif

#endif

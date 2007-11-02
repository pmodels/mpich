/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MX_MODULE_IMPL_H
#define MX_MODULE_IMPL_H
#include <myriexpress.h>
#include "mpid_nem_impl.h"

#define MPID_NEM_CELL_LEN_MX          (32*1024)
#define MPID_NEM_CELL_PAYLOAD_LEN_MX  (MPID_NEM_CELL_LEN_MX - sizeof(void *))

extern uint32_t               MPID_nem_module_mx_filter;
extern mx_endpoint_t          MPID_nem_module_mx_local_endpoint;
extern mx_endpoint_addr_t    *MPID_nem_module_mx_endpoints_addr;

extern int                    MPID_nem_module_mx_send_outstanding_request_num;
extern int                    MPID_nem_module_mx_pendings_sends;

extern int                    MPID_nem_module_mx_recv_outstanding_request_num;
extern int                    MPID_nem_module_mx_pendings_recvs;
extern int                   *MPID_nem_module_mx_pendings_recvs_array;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    unsigned int       remote_endpoint_id; /* uint32_t equivalent */
    unsigned long long remote_nic_id;      /* uint64_t equivalent */
} MPID_nem_mx_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_mx_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

typedef struct MPID_nem_mx_cell
{   
   struct MPID_nem_mx_cell *next;
   mx_request_t             mx_request;    
} MPID_nem_mx_cell_t, *MPID_nem_mx_cell_ptr_t;


typedef struct MPID_nem_mx_req_queue
{   
   MPID_nem_mx_cell_ptr_t head;
   MPID_nem_mx_cell_ptr_t tail;
} MPID_nem_mx_req_queue_t, *MPID_nem_mx_req_queue_ptr_t;


#define MPID_NEM_MX_CELL_TO_REQUEST(cellp) (&((cellp)->mx_request))
#define MPID_NEM_MX_REQ                    64
#define MPID_NEM_MX_MATCH                  (UINT64_C(0x666))
#define MPID_NEM_MX_MASK                   (UINT64_C(0xffffffffffffffff))

static inline int
MPID_nem_mx_req_queue_empty ( MPID_nem_mx_req_queue_ptr_t  qhead )
{   
   return qhead->head == NULL;
}

static inline void 
MPID_nem_mx_req_queue_enqueue (MPID_nem_mx_req_queue_ptr_t qhead, MPID_nem_mx_cell_ptr_t element)
{   
   MPID_nem_mx_cell_ptr_t prev = qhead->tail;   
   if (prev == NULL)
     {	
	qhead->head = element;
     }   
   else
     {	
	prev->next = element;
     }   
   qhead->tail = element;
}

static inline void
MPID_nem_mx_req_queue_dequeue (MPID_nem_mx_req_queue_ptr_t qhead, MPID_nem_mx_cell_ptr_t *e)
{   
   register MPID_nem_mx_cell_ptr_t _e = qhead->head;   
   if(_e == NULL)
     {	
	*e = NULL;
     }   
   else
     {	
	qhead->head  = _e->next;
	if(qhead->head == NULL)
	  {	     
	     qhead->tail = NULL;
	  }	
	_e->next = NULL;
	*e = (MPID_nem_mx_cell_ptr_t)_e;
     }   
}

extern MPID_nem_mx_cell_ptr_t      MPID_nem_module_mx_send_outstanding_request;
extern MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_free_req_queue;
extern MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_pending_req_queue;

extern MPID_nem_mx_cell_ptr_t      MPID_nem_module_mx_recv_outstanding_request;
extern MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_free_req_queue;
extern MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_pending_req_queue;

extern MPID_nem_queue_ptr_t MPID_nem_module_mx_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

#endif //MX_MODULE_IMPL_H

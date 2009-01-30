/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MX_IMPL_H
#define MX_IMPL_H
#include <myriexpress.h>
#include "mpid_nem_impl.h"

int MPID_nem_mx_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements,
		    int num_proc_elements, MPID_nem_cell_ptr_t module_elements, int num_module_elements,
		    MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		    MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mx_finalize (void);
int MPID_nem_mx_ckpt_shutdown (void);
int MPID_nem_mx_poll(MPID_nem_poll_dir_t in_or_out);
int MPID_nem_mx_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_mx_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mx_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_mx_vc_init (MPIDI_VC_t *vc);
int MPID_nem_mx_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_mx_vc_terminate (MPIDI_VC_t *vc);

int  MPID_nem_mx_test (void);

int  MPID_mem_mx_register_mem (void *p, int len);
int  MPID_nem_mx_deregister_mem (void *p, int len);

/* completion counter is atomically decremented when operation completes */
int  MPID_nem_mx_get (void *target_p, void *source_p, int len, MPIDI_VC_t *source_vc, int *completion_ctr);
int  MPID_nem_mx_put (void *target_p, void *source_p, int len, MPIDI_VC_t *target_vc, int *completion_ctr);

/* large message transfer functions */
int  MPID_nem_mx_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie);
int  MPID_nem_mx_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie);
int  MPID_nem_mx_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_mx_lmt_start_recv (MPIDI_VC_t *src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_mx_lmt_send_post (struct iovec cookie);
int  MPID_nem_mx_lmt_recv_post (struct iovec cookie);

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
} MPID_nem_mx_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_mx_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

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

#endif 

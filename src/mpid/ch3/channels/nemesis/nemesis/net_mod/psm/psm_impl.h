/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PSM_IMPL_H
#define PSM_IMPL_H

#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <psm.h>
#include <psm_mq.h>
#include "mpid_nem_impl.h"

int MPID_nem_psm_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements,
		    int num_proc_elements, MPID_nem_cell_ptr_t module_elements, int num_module_elements,
		    MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		    MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_psm_finalize (void);
int MPID_nem_psm_ckpt_shutdown (void);
int MPID_nem_psm_poll(MPID_nem_poll_dir_t in_or_out);
int MPID_nem_psm_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_psm_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_psm_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_psm_vc_init (MPIDI_VC_t *vc);
int MPID_nem_psm_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_psm_vc_terminate (MPIDI_VC_t *vc);

int  MPID_nem_psm_test (void);

int  MPID_mem_psm_register_mem (void *p, int len);
int  MPID_nem_psm_deregister_mem (void *p, int len);

/* completion counter is atomically decremented when operation completes */
int  MPID_nem_psm_get (void *target_p, void *source_p, int len, MPIDI_VC_t *source_vc, int *completion_ctr);
int  MPID_nem_psm_put (void *target_p, void *source_p, int len, MPIDI_VC_t *target_vc, int *completion_ctr);

/* large message transfer functions */
int  MPID_nem_psm_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie);
int  MPID_nem_psm_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie);
int  MPID_nem_psm_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_psm_lmt_start_recv (MPIDI_VC_t *src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_psm_lmt_send_post (struct iovec cookie);
int  MPID_nem_psm_lmt_recv_post (struct iovec cookie);

#define SEC_IN_NS   1000000000ULL

/* #define MPID_NEM_CELL_LEN_PSM          (32*1024) */
/* #define MPID_NEM_CELL_PAYLOAD_LEN_PSM  (MPID_NEM_CELL_LEN_PSM - sizeof(void *)) */

#define MQ_TAG	       1ULL
#define MQ_TAGSEL_ALL  0ULL
#define MQ_FLAGS_NONE  0
#define MQ_NO_CONTEXT_PTR   ((void *)NULL)


extern psm_epid_t       MPID_nem_module_psm_local_endpoint_id;
extern psm_ep_t         MPID_nem_module_psm_local_endpoint;

extern psm_epaddr_t    *MPID_nem_module_psm_endpoint_addrs;
extern psm_epid_t      *MPID_nem_module_psm_endpoint_ids;

extern psm_uuid_t       MPID_nem_module_psm_uuid;
extern psm_mq_t         MPID_nem_module_psm_mq;

extern int              MPID_nem_module_psm_connected;


extern int                    MPID_nem_module_psm_send_outstanding_request_num;
extern int                    MPID_nem_module_psm_pendings_sends;

extern int                    MPID_nem_module_psm_recv_outstanding_request_num;
extern int                    MPID_nem_module_psm_pendings_recvs;
extern int                   *MPID_nem_module_psm_pendings_recvs_array;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    psm_epid_t       remote_endpoint_id; 
} MPID_nem_psm_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_psm_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

typedef struct MPID_nem_psm_cell
{
   struct MPID_nem_psm_cell *next;
   psm_mq_req_t              psm_request;
} MPID_nem_psm_cell_t, *MPID_nem_psm_cell_ptr_t;


typedef struct MPID_nem_psm_req_queue
{
   MPID_nem_psm_cell_ptr_t head;
   MPID_nem_psm_cell_ptr_t tail;
} MPID_nem_psm_req_queue_t, *MPID_nem_psm_req_queue_ptr_t;


#define MPID_NEM_PSM_CELL_TO_REQUEST(cellp) (&((cellp)->psm_request))
#define MPID_NEM_PSM_REQ                    64
#define MPID_NEM_PSM_MATCH                  (UINT64_C(0x666))
#define MPID_NEM_PSM_MASK                   (UINT64_C(0xffffffffffffffff))

static inline int
MPID_nem_psm_req_queue_empty ( MPID_nem_psm_req_queue_ptr_t  qhead )
{
   return qhead->head == NULL;
}

static inline void
MPID_nem_psm_req_queue_enqueue (MPID_nem_psm_req_queue_ptr_t qhead, MPID_nem_psm_cell_ptr_t element)
{
   MPID_nem_psm_cell_ptr_t prev = qhead->tail;
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
MPID_nem_psm_req_queue_dequeue (MPID_nem_psm_req_queue_ptr_t qhead, MPID_nem_psm_cell_ptr_t *e)
{
   register MPID_nem_psm_cell_ptr_t _e = qhead->head;
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
	*e = (MPID_nem_psm_cell_ptr_t)_e;
     }
}

extern MPID_nem_psm_cell_ptr_t      MPID_nem_module_psm_send_outstanding_request;
extern MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_send_free_req_queue;
extern MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_send_pending_req_queue;

extern MPID_nem_psm_cell_ptr_t      MPID_nem_module_psm_recv_outstanding_request;
extern MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_recv_free_req_queue;
extern MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_recv_pending_req_queue;

extern MPID_nem_queue_ptr_t MPID_nem_module_psm_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

#endif 

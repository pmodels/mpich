/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PSM_MODULE_IMPL_H
#define PSM_MODULE_IMPL_H

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

#define SEC_IN_NS   1000000000ULL

/* #define MPID_NEM_CELL_LEN_PSM          (32*1024) */
/* #define MPID_NEM_CELL_PAYLOAD_LEN_PSM  (MPID_NEM_CELL_LEN_PSM - sizeof(void *)) */

//extern uint32_t               MPID_nem_module_psm_filter;

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
} MPID_nem_psm_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_psm_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

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

#endif //PSM_MODULE_IMPL_H

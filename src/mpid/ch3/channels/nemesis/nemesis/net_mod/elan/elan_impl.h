/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ELAN_IMPL_H
#define ELAN_IMPL_H

#include <elan/elan.h>
#include "mpid_nem_impl.h"

int MPID_nem_elan_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements,
                               int num_proc_elements, MPID_nem_cell_ptr_t module_elements, int num_module_elements,
                               MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
                               MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_elan_finalize (void);
int MPID_nem_elan_ckpt_shutdown (void);
int MPID_nem_elan_poll(MPID_nem_poll_dir_t in_or_out);
int MPID_nem_elan_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_elan_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_elan_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_elan_vc_init (MPIDI_VC_t *vc);
int MPID_nem_elan_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_elan_vc_terminate (MPIDI_VC_t *vc);

int  MPID_nem_elan_test (void);

int  MPID_mem_elan_register_mem (void *p, int len);
int  MPID_nem_elan_deregister_mem (void *p, int len);

/* completion counter is atomically decremented when operation completes */
int  MPID_nem_elan_get (void *target_p, void *source_p, int len, MPIDI_VC_t *source_vc, int *completion_ctr);
int  MPID_nem_elan_put (void *target_p, void *source_p, int len, MPIDI_VC_t *target_vc, int *completion_ctr);

/* large message transfer functions */
int  MPID_nem_elan_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie);
int  MPID_nem_elan_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie);
int  MPID_nem_elan_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_elan_lmt_start_recv (MPIDI_VC_t *src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_elan_lmt_send_post (struct iovec cookie);
int  MPID_nem_elan_lmt_recv_post (struct iovec cookie);


#define MPID_NEM_ELAN_SLOT_SIZE     MPID_NEM_CELL_PAYLOAD_LEN
#define MPID_NEM_ELAN_NUM_SLOTS     MPID_NEM_NUM_CELLS
#define MPID_NEM_ELAN_MAX_NUM_SLOTS 1024
#define MPID_NEM_ELAN_LOOPS_SEND    0
#define MPID_NEM_ELAN_LOOPS_RECV    0
#define MPID_NEM_ELAN_RAIL_NUM      0

extern int             MPID_nem_elan_freq;
extern int             MPID_nem_module_elan_pendings_sends;
extern int            *MPID_nem_elan_vpids; 
extern ELAN_QUEUE_TX **rxq_ptr_array;
extern ELAN_QUEUE_TX  *mpid_nem_elan_recv_queue_ptr;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    void *rxq_ptr_array; 
    int   vpid;
} MPID_nem_elan_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_elan_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

typedef struct MPID_nem_elan_cell
{   
   struct MPID_nem_elan_cell *next;
   ELAN_EVENT                *elan_event;
   MPID_nem_cell_ptr_t        cell_ptr;
   int                        to_proceed;
}
MPID_nem_elan_cell_t, *MPID_nem_elan_cell_ptr_t;

typedef struct MPID_nem_elan_event_queue
{   
   MPID_nem_elan_cell_ptr_t head;
   MPID_nem_elan_cell_ptr_t tail;
}
MPID_nem_elan_event_queue_t, *MPID_nem_elan_event_queue_ptr_t;

#define MPID_NEM_ELAN_SET_CELL(event_cell_ptr,event_ptr,cell,proceed) \
   (event_cell_ptr)->elan_event = (event_ptr); \
   (event_cell_ptr)->cell_ptr   = (cell);      \
   (event_cell_ptr)->to_proceed = (proceed) ; 

#define MPID_NEM_ELAN_RESET_CELL(event_cell_ptr) \
   MPID_NEM_ELAN_SET_CELL(event_cell_ptr,NULL,NULL,0)

static inline int
MPID_nem_elan_event_queue_empty ( MPID_nem_elan_event_queue_ptr_t qhead )
{   
   return (qhead->head == NULL);
}

static inline void 
MPID_nem_elan_event_queue_enqueue (MPID_nem_elan_event_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t element)
{
   MPID_nem_elan_cell_ptr_t prev = qhead->tail;
   
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
MPID_nem_elan_event_queue_dequeue (MPID_nem_elan_event_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t *e)
{       
   register MPID_nem_elan_cell_ptr_t _e = qhead->head;
   
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
	*e = (MPID_nem_elan_cell_ptr_t)_e;
     }   
}
    
extern MPID_nem_elan_cell_ptr_t        MPID_nem_module_elan_cells;
extern MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_free_event_queue;
extern MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_pending_event_queue;

extern MPID_nem_queue_ptr_t MPID_nem_module_elan_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

#endif /*ELAN_IMPL_H */

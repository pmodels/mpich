/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef GM_MODULE_IMPL_H
#define GM_MODULE_IMPL_H
#include <gm.h>
#include "mpid_nem_impl.h"
#include "gm_module.h"

#define UNIQUE_ID_LEN 6 /* why doesn't GM define this? */

/* typedef struct nodes_struct  */
/* { */
/*     unsigned port_id; */
/*     unsigned node_id; */
/*     unsigned char unique_id[UNIQUE_ID_LEN]; */
/* } node_t; */

/* extern node_t *nodes; */

#define PACKET_SIZE (gm_min_size_for_length (MPID_NEM_MAX_PACKET_LEN))

extern int MPID_nem_module_gm_num_send_tokens;

extern struct gm_port *MPID_nem_module_gm_port;

extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

int MPID_nem_gm_module_recv_poll();
inline int MPID_nem_gm_module_recv();
inline int MPID_nem_send_from_queue();

int MPID_nem_gm_module_recv_init();

int MPID_nem_gm_module_lmt_init();
int MPID_nem_gm_module_lmt_finalize();
int MPID_nem_gm_module_lmt_do_get (int node_id, int port_id, struct iovec **r_iov, int *r_n_iov, int *r_offset, struct iovec **s_iov, int *s_n_iov,
			  int *s_offset, int *compl_ctr);

/* these perform the gm_put or gm_get call, there must be at least one send token  */
/* called by MPID_nem_gm_module_put and _get and by polling functions */
int MPID_nem_gm_module_do_put (void *target_p, void *source_p, int len, int node_id, int port_id, int *completion_ctr);
int MPID_nem_gm_module_do_get (void *target_p, void *source_p, int len, int node_id, int port_id, int *completion_ctr);

/* gm module packet */
#define PKT_HEADER_LEN (sizeof(packet_t) - SENDPKT_DATALEN)
#define SENDPKT_DATALEN (64*1024)
typedef struct packet
{
    MPI_Aint source_id;
    char buf[SENDPKT_DATALEN];
} packet_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    unsigned gm_port_id;
    unsigned gm_node_id; 
    unsigned source_id; 
    unsigned char gm_unique_id[6]; /* GM unique id length is 6 bytes.  GM doesn't define a constant. */
} MPID_nem_gm_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_gm_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)


/* lmt queues */

typedef struct MPID_nem_gm_module_lmt_queue
{
    int node_id;
    int port_id;
    struct iovec *r_iov;
    int r_n_iov;
    int r_offset;
    struct iovec *s_iov;
    int s_n_iov;
    int s_offset;
    int *compl_ctr;
    struct MPID_nem_gm_module_lmt_queue *next;
}
MPID_nem_gm_module_lmt_queue_t;

typedef struct MPID_nem_gm_module_lmt_queue_head
{
    MPID_nem_gm_module_lmt_queue_t *head;
    MPID_nem_gm_module_lmt_queue_t *tail;
}
MPID_nem_gm_module_lmt_queue_head_t;

extern MPID_nem_gm_module_lmt_queue_head_t MPID_nem_gm_module_lmt_queue;
extern MPID_nem_gm_module_lmt_queue_t *MPID_nem_gm_module_lmt_free_queue;



typedef struct MPID_nem_gm_module_rdma_desc
{
    enum {RDMA_TYPE_PUT, RDMA_TYPE_GET} type;
    void *target_p;
    void *source_p;
    int len;
    int *completion_ctr;
}
MPID_nem_gm_module_rdma_desc_t;

typedef struct MPID_nem_gm_module_send_queue
{
    struct MPID_nem_gm_module_send_queue *next;
    int node_id;
    int port_id;
    enum {SEND_TYPE_RDMA, SEND_TYPE_CELL} type;
    union 
    {
	MPID_nem_cell_t *cell;
	MPID_nem_gm_module_rdma_desc_t rdma;
    } u;
}
MPID_nem_gm_module_send_queue_t;

typedef struct MPID_nem_gm_module_send_queue_head
{
    MPID_nem_gm_module_send_queue_t *head;
    MPID_nem_gm_module_send_queue_t *tail;
}
MPID_nem_gm_module_send_queue_head_t;

extern MPID_nem_gm_module_send_queue_head_t MPID_nem_gm_module_send_queue;
extern MPID_nem_gm_module_send_queue_t *MPID_nem_gm_module_send_free_queue;



#define MPID_nem_gm_module_queue_empty(queue) (MPID_nem_gm_module_##queue##_queue.head == NULL)
#define MPID_nem_gm_module_queue_head(queue) (MPID_nem_gm_module_##queue##_queue.head)
#define MPID_nem_gm_module_queue_dequeue(queue, e) do {					\
    *(e) = MPID_nem_gm_module_##queue##_queue.head;					\
    if (*(e))									\
    {										\
	MPID_nem_gm_module_##queue##_queue.head = MPID_nem_gm_module_##queue##_queue.head->next;	\
	if (MPID_nem_gm_module_##queue##_queue.head == NULL)				\
	    MPID_nem_gm_module_##queue##_queue.tail = NULL;				\
    }										\
} while (0)

#define MPID_nem_gm_module_queue_enqueue(queue, e) do {		\
    if (MPID_nem_gm_module_##queue##_queue.tail == NULL)		\
	MPID_nem_gm_module_##queue##_queue.head = e;		\
    else						\
	MPID_nem_gm_module_##queue##_queue.tail->next = e;	\
    MPID_nem_gm_module_##queue##_queue.tail = e;			\
    (e)->next = NULL;					\
} while (0)

#define MPID_nem_gm_module_queue_free(queue, e) /*free (e)*/ do {	\
    (e)->next = MPID_nem_gm_module_##queue##_free_queue;			\
    MPID_nem_gm_module_##queue##_free_queue = e;				\
} while (0)

#define MPID_nem_gm_module_queue_alloc(queue) ({								\
    MPID_nem_gm_module_##queue##_queue_t *e; 								\
    if (MPID_nem_gm_module_##queue##_free_queue)								\
    {												\
	e = MPID_nem_gm_module_##queue##_free_queue;							\
	MPID_nem_gm_module_##queue##_free_queue = MPID_nem_gm_module_##queue##_free_queue->next;			\
    }												\
    else											\
    {												\
	e = (MPID_nem_gm_module_##queue##_queue_t *)MPIU_Malloc (sizeof (MPID_nem_gm_module_##queue##_queue_t));	\
    }												\
    e;												\
})


#endif

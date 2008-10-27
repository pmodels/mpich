/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SCTP_MODULE_IMPL_H
#define SCTP_MODULE_IMPL_H
#include "mpid_nem_impl.h"
#include "sctp_module_impl.h"
#include "all_hash.h"
#include "sctp_module_queue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <fcntl.h>  /* new for SCTP */
#include <errno.h>
#include <netdb.h> /* needed for gethostbyname */
#include <arpa/inet.h> /* needed for inet_pton  */

#define SCTP_POLL_FREQ_MULTI 5 
#define SCTP_POLL_FREQ_ALONE 1
#define SCTP_POLL_FREQ_NO   -1
#define SCTP_END_STRING "NEM_SCTP_MOD_FINALIZE"


#define MPIDI_CH3_HAS_CHANNEL_CLOSE
#define MPIDI_CH3_CHANNEL_AVOIDS_SELECT

    /* TODO make all of these _ and make all of these adjustable using env var */
#define MPICH_SCTP_NUM_STREAMS 1
#define _MPICH_SCTP_SOCKET_BUFSZ 233016  /* _ because not to confuse with env var */
    /* TODO add port and no_nagle */

    /* stream table */
#define HAVE_NOT_SENT_PG_ID 0
#define HAVE_SENT_PG_ID 1
#define HAVE_NOT_RECV_PG_ID 0
#define HAVE_RECV_PG_ID 1

typedef struct MPID_nem_sctp_stream {
    char have_sent_pg_id;
    char have_recv_pg_id;
} MPID_nem_sctp_stream_t;

/* typedefs */

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    int fd;
    MPID_nem_sctp_stream_t stream_table[MPICH_SCTP_NUM_STREAMS];
    struct sockaddr_in to_address;
    void * conn_pkt;
    struct
    {
        struct MPID_nem_sctp_module_send_q_element *head;
        struct MPID_nem_sctp_module_send_q_element *tail;
    } send_queue;
} MPID_nem_sctp_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_sctp_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

/*   sendq */
typedef struct MPID_nem_sctp_module_send_q_element
{
    struct MPID_nem_sctp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to sent */
    char *start;                       /* pointer to next byte to send */
    int stream;
    char buf[MPID_NEM_MAX_PACKET_LEN]; /* data to be sent */
} MPID_nem_sctp_module_send_q_element_t;

/*   hash table entry */
typedef struct hash_entry
{
    sctp_assoc_t assoc_id;
    MPIDI_VC_t * vc;
} MPID_nem_sctp_hash_entry;

/* globals */

/*   queues */
extern MPID_nem_queue_ptr_t MPID_nem_sctp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

/*   ints */
extern int MPID_nem_sctp_onetomany_fd; /* needed for all communication */
extern int MPID_nem_sctp_port; /* needed for init and bizcards */

/*   hash table for association ID -> VC */
extern HASH* MPID_nem_sctp_assocID_table;

/*   for tmp connection with dynamic process (and no select()) */
extern MPIDI_VC_t * MPIDI_CH3I_dynamic_tmp_vc;
extern int MPIDI_CH3I_dynamic_tmp_fd;

/* functions */

int MPID_nem_sctp_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
                              MPID_nem_queue_ptr_t proc_free_queue, 
                              MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
                              MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
                              MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
                              MPIDI_PG_t *pg_p, int pg_rank,
                              char **bc_val_p, int *val_max_sz_p);
int MPID_nem_sctp_module_finalize (void);
int MPID_nem_sctp_module_ckpt_shutdown (void);
int MPID_nem_sctp_module_poll (MPID_nem_poll_dir_t in_or_out);
int MPID_nem_sctp_module_poll_send (void);
int MPID_nem_sctp_module_poll_recv (void);
int MPID_nem_sctp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_sctp_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_sctp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_sctp_module_vc_init (MPIDI_VC_t *vc);
int MPID_nem_sctp_module_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_sctp_module_vc_terminate (MPIDI_VC_t *vc);

/* completion counter is atomically decremented when operation completes */
int MPID_nem_sctp_module_get (void *target_p, void *source_p, int source_node, int len, int *completion_ctr);
int MPID_nem_sctp_module_put (void *target_p, int target_node, void *source_p, int len, int *completion_ctr);


/* determine the stream # of a req. copied from UBC sctp channel so could be irrelevant */
/*    we want to use context so that collectives don't
 *    cause head-of-line blocking for p2p...
 */
 /* the following keeps stream s.t. 0 <= stream < MPICH_SCTP_NUM_STREAMS */
#define Req_Stream_from_match(match) (abs((match.tag) + (match.context_id))% MPICH_SCTP_NUM_STREAMS)
#define REQ_Stream(req) Req_Stream_from_match(req->dev.match)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq);


int MPID_nem_sctp_module_send_progress();
int MPID_nem_sctp_module_send_init();
int MPID_nem_sctp_module_send_finalize();


/* Send queue macros */
#define Q_EMPTY(q) GENERIC_Q_EMPTY (q)
#define Q_HEAD(q) GENERIC_Q_HEAD (q)
#define Q_ENQUEUE_EMPTY(qp, ep) GENERIC_Q_ENQUEUE_EMPTY (qp, ep, next)
#define Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, next)
#define Q_ENQUEUE_EMPTY_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE (qp, ep0, ep1, next)
#define Q_ENQUEUE_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_MULTIPLE (qp, ep0, ep1, next)
#define Q_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, next)
#define Q_REMOVE_ELEMENTS(qp, ep0, ep1) GENERIC_Q_REMOVE_ELEMENTS (qp, ep0, ep1, next)

/* VC list macros */
#define VC_L_EMPTY(q) GENERIC_L_EMPTY (q)
#define VC_L_HEAD(q) GENERIC_L_HEAD (q)
#define VC_L_ADD_EMPTY(qp, ep) GENERIC_L_ADD_EMPTY (qp, ep, ch.next, ch.prev)
#define VC_L_ADD(qp, ep) GENERIC_L_ADD (qp, ep, ch.next, ch.prev)
#define VC_L_REMOVE(qp, ep) GENERIC_L_REMOVE (qp, ep, ch.next, ch.prev)


#endif /* SCTP_MODULE_IMPL_H */

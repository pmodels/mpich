/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

extern int MPIDI_CH3I_my_rank;

#include "mpidi_ch3i_gasnet_conf.h"
#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

extern int MPIDI_CH3I_inside_handler;
extern gasnet_token_t MPIDI_CH3I_gasnet_token;

extern int MPIDI_CH3_packet_len;
extern void *MPIDI_CH3_packet_buffer;

#define CH3_NORMAL_QUEUE 0
#define CH3_RNDV_QUEUE   1
#define CH3_NUM_QUEUES   2

extern struct MPID_Request *MPIDI_CH3I_sendq_head[CH3_NUM_QUEUES];
extern struct MPID_Request *MPIDI_CH3I_sendq_tail[CH3_NUM_QUEUES];
extern struct MPID_Request *MPIDI_CH3I_active_send[CH3_NUM_QUEUES];

#define MPIDI_CH3I_SendQ_enqueue(req, queue)					\
{										\
    /* MT - not thread safe! */							\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue req=0x%08x", req->handle));	\
    req->dev.next = NULL;							\
    if (MPIDI_CH3I_sendq_tail[queue] != NULL)					\
    {										\
	MPIDI_CH3I_sendq_tail[queue]->dev.next = req;				\
    }										\
    else									\
    {										\
	MPIDI_CH3I_sendq_head[queue] = req;					\
    }										\
    MPIDI_CH3I_sendq_tail[queue] = req;						\
}

#define MPIDI_CH3I_SendQ_enqueue_head(req, queue)			\
{									\
    /* MT - not thread safe! */						\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head req=0x%08x",	\
		      req->handle));					\
    req->dev.next = MPIDI_CH3I_sendq_tail[queue];			\
    if (MPIDI_CH3I_sendq_tail[queue] == NULL)				\
    {									\
	MPIDI_CH3I_sendq_tail[queue] = req;				\
    }									\
    MPIDI_CH3I_sendq_head[queue] = req;					\
}

#define MPIDI_CH3I_SendQ_dequeue(queue)						\
{										\
    /* MT - not thread safe! */							\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue req=0x%08x",			\
                      MPIDI_CH3I_sendq_head[queue]->handle));			\
    MPIDI_CH3I_sendq_head[queue] = MPIDI_CH3I_sendq_head[queue]->dev.next;	\
    if (MPIDI_CH3I_sendq_head[queue] == NULL)					\
    {										\
	MPIDI_CH3I_sendq_tail[queue] = NULL;					\
    }										\
}

#define MPIDI_CH3I_SendQ_head(queue) (MPIDI_CH3I_sendq_head[queue])

#define MPIDI_CH3I_SendQ_empty(queue) (MPIDI_CH3I_sendq_head[queue] == NULL)

#ifndef MPIDI_CH3_GASNET_TAKES_IOV
int gasnet_AMRequestMediumv0(gasnet_node_t dest, gasnet_handler_t handler,
			     struct iovec *iov, size_t n_iov);
#endif
int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
short MPIDI_CH3I_Listener_get_port(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *);
int MPIDI_CH3I_VC_post_read(MPIDI_VC_t *, MPID_Request *);
int MPIDI_CH3I_VC_post_write(MPIDI_VC_t *, MPID_Request *);
int MPIDI_CH3I_sock_errno_to_mpi_errno(char * fcname, int sock_errno);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */

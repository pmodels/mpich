/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_ib_conf.h"
#include "mpidimpl.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

typedef struct MPIDI_CH3I_Process_s
{
    MPIDI_PG_t * pg;
    ibu_set_t set;
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#define MPIDI_CH3I_SendQ_enqueue(vc, req)				\
{									\
    /* MT - not thread safe! */						\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x",	\
	              vc, req->handle));				\
    req->dev.next = NULL;						\
    if (vc->ch.sendq_tail != NULL)					\
    {									\
	vc->ch.sendq_tail->dev.next = req;				\
    }									\
    else								\
    {									\
	vc->ch.sendq_head = req;					\
    }									\
    vc->ch.sendq_tail = req;						\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vc, req)				     \
{									     \
    /* MT - not thread safe! */						     \
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", \
	              vc, req->handle));				     \
    req->dev.next = vc->ch.sendq_head;					     \
    if (vc->ch.sendq_tail == NULL)					     \
    {									     \
	vc->ch.sendq_tail = req;					     \
    }									     \
    vc->ch.sendq_head = req;						     \
}

#define MPIDI_CH3I_SendQ_dequeue(vc)					\
{									\
    /* MT - not thread safe! */						\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x",	\
	              vc, vc->ch.sendq_head));				\
    vc->ch.sendq_head = vc->ch.sendq_head->dev.next;			\
    if (vc->ch.sendq_head == NULL)					\
    {									\
	vc->ch.sendq_tail = NULL;					\
    }									\
}

#define MPIDI_CH3I_SendQ_head(vc) (vc->ch.sendq_head)

#define MPIDI_CH3I_SendQ_empty(vc) (vc->ch.sendq_head == NULL)

int post_pkt_recv(MPIDI_VC_t *recv_vc_ptr);

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Request_adjust_iov(MPID_Request *, MPIDI_msg_sz_t);
int MPIDI_CH3I_Setup_connections(MPIDI_PG_t *pg, int pg_rank);
int MPIDI_CH3I_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq);
int MPIDI_CH3I_rdma_writev(MPIDI_VC_t *vc, MPID_Request *sreq);
int MPIDI_CH3I_Switch_rndv_to_eager(MPIDI_VC_t * vc, MPID_Request * sreq, MPIDI_CH3_Pkt_t* pkt);
MPIDI_CH3I_Alloc_mem_list_t * MPIDI_CH3I_Get_mem_list_head();

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */

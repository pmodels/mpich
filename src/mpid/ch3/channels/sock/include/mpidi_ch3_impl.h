/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidimpl.h"
#include "ch3usock.h"

/* This is all socket connection definitions */

    /* MT - not thread safe! */

/* The SendQ will add a reference to the request in order to prevent
 * manipulating the dev.next field after the request has been destroyed.  This
 * ref will be dropped after dequeueing.  (tt#1038) */

#define MPIDI_CH3I_SendQ_enqueue(vcch, req)				\
{									\
    MPIU_DBG_MSG(CH3_MSG,TYPICAL,"Enqueuing this request");\
    MPIR_Request_add_ref(req);                                          \
    req->dev.next = NULL;						\
    if (vcch->sendq_tail != NULL)					\
    {									\
	vcch->sendq_tail->dev.next = req;				\
    }									\
    else								\
    {									\
	vcch->sendq_head = req;					\
    }									\
    vcch->sendq_tail = req;						\
}

    /* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_enqueue_head(vcch, req)			\
{									\
    MPIU_DBG_MSG(CH3_MSG,TYPICAL,"Enqueuing this request at head");\
    MPIR_Request_add_ref(req);                                          \
    req->dev.next = vcch->sendq_head;					\
    if (vcch->sendq_tail == NULL)					\
    {									\
	vcch->sendq_tail = req;					\
    }									\
    vcch->sendq_head = req;						\
}

    /* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_dequeue(vcch)					\
{									\
    MPID_Request *req_ = vcch->sendq_head;                              \
    MPIU_DBG_MSG(CH3_MSG,TYPICAL,"Dequeuing this request");\
    vcch->sendq_head = vcch->sendq_head->dev.next;			\
    if (vcch->sendq_head == NULL)					\
    {									\
	vcch->sendq_tail = NULL;					\
    }									\
    MPID_Request_release(req_);                                         \
}


#define MPIDI_CH3I_SendQ_head(vcch) (vcch->sendq_head)

#define MPIDI_CH3I_SendQ_empty(vcch) (vcch->sendq_head == NULL)

/* End of connection-related macros */

/* FIXME: Any of these used in the ch3->channel interface should be
   defined in a header file in ch3/include that defines the 
   channel interface */
int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Progress_register_hook(int (*progress_fn)(int*), int *id);
int MPIDI_CH3I_Progress_deregister_hook(int id);
int MPIDI_CH3I_Progress_activate_hook(int id);
int MPIDI_CH3I_Progress_deactivate_hook(int id);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *);

/* Shared memory window atomic/accumulate mutex implementation */

#define MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr)
#define MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr)
#define MPIDI_CH3I_SHM_MUTEX_INIT(win_ptr)
#define MPIDI_CH3I_SHM_MUTEX_DESTROY(win_ptr)

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */

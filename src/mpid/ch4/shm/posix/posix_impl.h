/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_IMPL_H_INCLUDED
#define POSIX_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "mpidch4r.h"

#include "pmi.h"

#include "mpidu_shm.h"

/* ---------------------------------------------------- */
/* temp headers                                         */
/* ---------------------------------------------------- */
#include "posix_datatypes.h"    /* MPID_nem datatypes like cell, fastbox defined here */
#include "posix_defs.h" /* MPID_nem objects like shared memory region defined here */
#include "posix_queue.h"        /* MPIDI_POSIX_queue functions defined here */

/* ---------------------------------------------------- */
/* constants                                            */
/* ---------------------------------------------------- */
#define MPIDI_POSIX_EAGER_THRESHOLD MPIDI_POSIX_DATA_LEN
#define MPIDI_POSIX_TYPESTANDARD    0
#define MPIDI_POSIX_TYPEEAGER       1
#define MPIDI_POSIX_TYPELMT         2
#define MPIDI_POSIX_TYPESYNC        3
#define MPIDI_POSIX_TYPEBUFFERED    4
#define MPIDI_POSIX_TYPEREADY       5
#define MPIDI_POSIX_TYPEACK         6
#define MPIDI_POSIX_REQUEST(req)    (&(req)->dev.ch4.shm.posix)

/* ---------------------------------------------------- */
/* shm specific object data                             */
/* ---------------------------------------------------- */
/* VCR Table Data */
typedef struct {
    unsigned int avt_rank;
} MPIDI_POSIX_vcr_t;

struct MPIDI_POSIX_vcrt_t {
    MPIR_OBJECT_HEADER;
    unsigned size;                            /**< Number of entries in the table */
    MPIDI_POSIX_vcr_t vcr_table[0]; /**< Array of virtual connection references */
};
/* ---------------------------------------------------- */
/* general send/recv queue types, macros and objects    */
/* ---------------------------------------------------- */
typedef struct {
    MPIR_Request *head;
    MPIR_Request *tail;
} MPIDI_POSIX_request_queue_t;

#define MPIDI_POSIX_REQUEST_COMPLETE(req_)    \
{ \
    int incomplete__; \
    MPIR_cc_decr((req_)->cc_ptr, &incomplete__); \
    dtype_release_if_not_builtin(MPIDI_POSIX_REQUEST(req_)->datatype); \
    if (!incomplete__) \
        MPIR_Request_free(req_);    \
}

#define MPIDI_POSIX_REQUEST_ENQUEUE(req,queue) \
{ \
    if ((queue).tail != NULL) \
        MPIDI_POSIX_REQUEST((queue).tail)->next = req; \
    else \
        (queue).head = req; \
    (queue).tail = req; \
}

#define MPIDI_POSIX_REQUEST_DEQUEUE(req_p,prev_req,queue) \
{ \
    MPIR_Request *next = MPIDI_POSIX_REQUEST(*(req_p))->next; \
    if ((queue).head == *(req_p)) \
        (queue).head = next; \
    else \
        MPIDI_POSIX_REQUEST(prev_req)->next = next; \
    if ((queue).tail == *(req_p)) \
        (queue).tail = prev_req; \
    MPIDI_POSIX_REQUEST(*(req_p))->next = NULL; \
}

#define MPIDI_POSIX_REQUEST_DEQUEUE_AND_SET_ERROR(req_p,prev_req,queue,err) \
{ \
    MPIR_Request *next = MPIDI_POSIX_REQUEST(*(req_p))->next; \
    if ((queue).head == *(req_p)) \
        (queue).head = next; \
    else \
        MPIDI_POSIX_REQUEST(prev_req)->next = next; \
    if ((queue).tail == *(req_p)) \
        (queue).tail = prev_req; \
    (*(req_p))->status.MPI_ERROR = err; \
    MPIDI_POSIX_REQUEST_COMPLETE(*(req_p)); \
    *(req_p) = next; \
}

#define MPIDI_POSIX_REQUEST_CREATE_SREQ(sreq_)	\
{								\
    (sreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);             \
    MPIR_Request_add_ref((sreq_));                                      \
    (sreq_)->u.persist.real_request   = NULL;                          \
}

#define MPIDI_POSIX_REQUEST_CREATE_RREQ(rreq_)	\
{								\
    (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);             \
    MPIR_Request_add_ref((rreq_));                                      \
    (rreq_)->u.persist.real_request   = NULL;                          \
}

/* ---------------------------------------------------- */
/* matching macros                                      */
/* ---------------------------------------------------- */
#define MPIDI_POSIX_ENVELOPE_SET(ptr_,rank_,tag_,context_id_) \
{ \
    (ptr_)->rank = rank_; \
    (ptr_)->tag = tag_; \
    (ptr_)->context_id = context_id_; \
}

#define MPIDI_POSIX_ENVELOPE_GET(ptr_,rank_,tag_,context_id_) \
{ \
    rank_ = (ptr_)->rank; \
    tag_ = (ptr_)->tag; \
    context_id_ = (ptr_)->context_id; \
}

#define MPIDI_POSIX_ENVELOPE_MATCH(ptr_,rank_,tag_,context_id_) \
    (((ptr_)->rank == (rank_) || (rank_) == MPI_ANY_SOURCE) && \
     ((ptr_)->tag == (tag_) || (tag_) == MPI_ANY_TAG) && \
     (ptr_)->context_id == (context_id_))

/*
 * Helper routines and macros for request completion
 */
#define DECL_FUNC(FUNCNAME)  MPL_QUOTE(FUNCNAME)

#undef FUNCNAME
#define FUNCNAME nothing
#define BEGIN_FUNC(FUNCNAME)                    \
  MPIR_FUNC_VERBOSE_STATE_DECL(FUNCNAME);                   \
  MPIR_FUNC_VERBOSE_ENTER(FUNCNAME);
#define END_FUNC(FUNCNAME)                      \
  MPIR_FUNC_VERBOSE_EXIT(FUNCNAME);
#define END_FUNC_RC(FUNCNAME) \
  fn_exit:                    \
  MPIR_FUNC_VERBOSE_EXIT(FUNCNAME);  \
  return mpi_errno;           \
fn_fail:                      \
  goto fn_exit;

#define __SHORT_FILE__                          \
  (strrchr(__FILE__,'/')                        \
   ? strrchr(__FILE__,'/')+1                    \
   : __FILE__                                   \
)

int MPIDI_POSIX_barrier_vars_init(MPIDI_POSIX_barrier_vars_t * barrier_region);
extern MPIDI_POSIX_request_queue_t MPIDI_POSIX_sendq;
extern MPIDI_POSIX_request_queue_t MPIDI_POSIX_recvq_unexpected;
extern MPIDI_POSIX_request_queue_t MPIDI_POSIX_recvq_posted;



#endif /* POSIX_IMPL_H_INCLUDED */

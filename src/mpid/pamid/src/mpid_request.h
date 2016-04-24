/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_request.h
 * \brief ???
 */

#ifndef __src_mpid_request_h__
#define __src_mpid_request_h__

#include "mpidu_datatype.h"

/**
 * \addtogroup MPIR_REQUEST
 * \{
 */

#define MPIR_Request_create    MPID_Request_create_inline
#define MPIR_Request_free   MPID_Request_free_inline
#define MPIDI_Request_complete MPIDI_Request_complete_inline
#define MPIDI_Request_complete_norelease MPIDI_Request_complete_norelease_inline
#define MPID_Request_discard   MPID_Request_discard_inline


extern MPIR_Object_alloc_t MPIR_Request_mem;
#if TOKEN_FLOW_CONTROL
extern void MPIDI_mm_free(void *,size_t);
#endif
typedef enum {mpiuMalloc=1,mpidiBufMM} MPIDI_mallocType;

void    MPIDI_Request_uncomplete(MPIR_Request *req);
#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
void    MPIDI_Request_allocate_pool();
#endif

#define MPIDI_Request_getCA(_req)                ({ (_req)->mpid.ca;                                })
#define MPIDI_Request_getPeerRank_pami(_req)     ({ (_req)->mpid.peer_pami;                         })
#define MPIDI_Request_getPeerRank_comm(_req)     ({ (_req)->mpid.peer_comm;                         })
#define MPIDI_Request_getPType(_req)             ({ (_req)->mpid.ptype;                             })
#define MPIDI_Request_getControl(_req)           ({ (_req)->mpid.envelope.msginfo.control;          })
#define MPIDI_Request_isSync(_req)               ({ (_req)->mpid.envelope.msginfo.isSync;           })
#define MPIDI_Request_isRzv(_req)                ({ (_req)->mpid.envelope.msginfo.isRzv;            })
#define MPIDI_Request_getMatchTag(_req)          ({ (_req)->mpid.envelope.msginfo.MPItag;           })
#define MPIDI_Request_getMatchRank(_req)         ({ (_req)->mpid.envelope.msginfo.MPIrank;          })
#define MPIDI_Request_getMatchCtxt(_req)         ({ (_req)->mpid.envelope.msginfo.MPIctxt;          })

#define MPIDI_Request_setCA(_req, _ca)           ({ (_req)->mpid.ca                        = (_ca); })
#define MPIDI_Request_setPeerRank_pami(_req,_r)  ({ (_req)->mpid.peer_pami                 = (_r);  })
#define MPIDI_Request_setPeerRank_comm(_req,_r)  ({ (_req)->mpid.peer_comm                 = (_r);  })
#define MPIDI_Request_setPType(_req,_t)          ({ (_req)->mpid.ptype                     = (_t);  })
#define MPIDI_Request_setControl(_req,_t)        ({ (_req)->mpid.envelope.msginfo.control  = (_t);  })
#define MPIDI_Request_setSync(_req,_t)           ({ (_req)->mpid.envelope.msginfo.isSync   = (_t);  })
#define MPIDI_Request_setRzv(_req,_t)            ({ (_req)->mpid.envelope.msginfo.isRzv    = (_t);  })
#ifdef OUT_OF_ORDER_HANDLING
#define MPIDI_Request_getMatchSeq(_req)          ({ (_req)->mpid.envelope.msginfo.MPIseqno;           })
#define MPIDI_Request_setMatchSeq(_req,_sq)      ({ (_req)->mpid.envelope.msginfo.MPIseqno = (_sq);  })
#endif
#define MPIDI_Request_setMatch(_req,_tag,_rank,_ctxtid) \
({                                                      \
  (_req)->mpid.envelope.msginfo.MPItag=(_tag);          \
  (_req)->mpid.envelope.msginfo.MPIrank=(_rank);        \
  (_req)->mpid.envelope.msginfo.MPIctxt=(_ctxtid);      \
})

#define MPIDI_Msginfo_getPeerRequest(_msg)       ({ MPIR_Request *req=NULL; MPIR_Request_get_ptr((_msg)->req, req); MPID_assert(req != NULL); req; })
#define MPIDI_Msginfo_getPeerRequestH(_msg)      ({                       (_msg)->req;                               })
#define MPIDI_Msginfo_cpyPeerRequestH(_dst,_src) ({                       (_dst)->req = (_src)->req;    MPI_SUCCESS; })
#define MPIDI_Request_getPeerRequest(_req)       MPIDI_Msginfo_getPeerRequest(&(_req)->mpid.envelope.msginfo)
#define MPIDI_Request_getPeerRequestH(_req)      ({ (_req)->mpid.envelope.msginfo.req;                               })
#define MPIDI_Request_setPeerRequestH(_req)      ({ (_req)->mpid.envelope.msginfo.req = (_req)->handle; MPI_SUCCESS; })
#define MPIDI_Request_cpyPeerRequestH(_dst,_src) MPIDI_Msginfo_cpyPeerRequestH(&(_dst)->mpid.envelope.msginfo,_src)


#define MPIU_HANDLE_ALLOCATION_MUTEX         0
#define MPIU_HANDLE_ALLOCATION_THREAD_LOCAL  1

/* XXX DJG for TLS hack */
#define MPIR_REQUEST_TLS_MAX 128

#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)

#  define MPIDI_Request_tls_alloc(req)                                  \
({                                                                      \
  size_t tid = MPIDI_THREAD_ID();                                       \
  MPIDI_RequestHandle_t *rh = &MPIDI_Process.request_handles[tid];      \
  if (unlikely(rh->head == NULL))                                       \
    MPIDI_Request_allocate_pool();                                      \
  (req) = rh->head;                                                     \
  rh->head = req->mpid.next;                                            \
  rh->count --;                                                         \
})

#  define MPIDI_Request_tls_free(req)                                   \
({                                                                      \
  size_t tid = MPIDI_THREAD_ID();                                       \
  MPIDI_RequestHandle_t *rh = &MPIDI_Process.request_handles[tid];      \
  if (likely(rh->count < MPIR_REQUEST_TLS_MAX))				\
    {                                                                   \
      /* push request onto the top of the stack */                      \
      req->mpid.next = rh->head;                                        \
      rh->head = req;                                                   \
      rh->count ++;                                                     \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      MPIR_Handle_obj_free(&MPIR_Request_mem, req);                     \
    }                                                                   \
})

#else

#  define MPIDI_Request_tls_alloc(req)                                  \
({                                                                      \
  (req) = MPIR_Handle_obj_alloc(&MPIR_Request_mem);                     \
  if (req == NULL)                                                      \
    MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1, "Cannot allocate Request");  \
})

#  define MPIDI_Request_tls_free(req) MPIR_Handle_obj_free(&MPIR_Request_mem, (req))

#endif

#ifdef HAVE_DEBUGGER_SUPPORT
#define MPIDI_Request_clear_dbg(req_) ((req_)->u.send.dbg_next = NULL)
#else
#define MPIDI_Request_clear_dbg(req_)
#endif

/**
 * \brief Create a very generic request
 * \note  This should only ever be called by more specific allocators
 */
static inline MPIR_Request *
MPIDI_Request_create_basic()
{
  MPIR_Request * req = NULL;

  MPIDI_Request_tls_alloc(req);
  MPID_assert(req != NULL);
  MPID_assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST);
  MPIR_cc_set(&req->cc, 1);
  req->cc_ptr = &req->cc;

#if 0
  /* This will destroy the MPID part of the request.  Use this to
     check for fields that are not being correctly initialized. */
  memset(&req->mpid, 0xFFFFFFFF, sizeof(struct MPIDI_Request));
#endif
  req->mpid.next = NULL;
  MPIDI_Request_clear_dbg(req);

  return req;
}


/**
 * \brief Create new request without initalizing
 */
static inline MPIR_Request *
MPIDI_Request_create2_fast()
{
  MPIR_Request * req;
  req = MPIDI_Request_create_basic();
  MPIR_Object_set_ref(req, 2);

  return req;
}


/**
 * \brief Create and initialize a new request
 */
static inline void
MPIDI_Request_initialize(MPIR_Request * req)
{
  req->greq_fns          = NULL;

  MPIR_STATUS_SET_COUNT(req->status, 0);
  MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);
  req->status.MPI_SOURCE = MPI_UNDEFINED;
  req->status.MPI_TAG    = MPI_UNDEFINED;
  req->status.MPI_ERROR  = MPI_SUCCESS;

  struct MPIDI_Request* mpid = &req->mpid;
  mpid->envelope.msginfo.flags = 0;
  mpid->cancel_pending   = FALSE;
  mpid->datatype_ptr     = NULL;
  mpid->uebuf            = NULL;
  mpid->uebuflen         = 0;
  mpid->uebuf_malloc     = 0;
#ifdef OUT_OF_ORDER_HANDLING
  mpid->prev             = NULL;
  mpid->prevR            = NULL;
  mpid->nextR            = NULL;
  mpid->oo_peer          = NULL;
#endif
  mpid->win_req          = NULL;
  MPIDI_Request_setCA(req, MPIDI_CA_COMPLETE);
}


/**
 * \brief Create and initialize a new request
 */
static inline MPIR_Request *
MPID_Request_create_inline()
{
  MPIR_Request * req;
  req = MPIDI_Request_create_basic();
  MPIR_Object_set_ref(req, 1);

  MPIDI_Request_initialize(req);
  req->comm=NULL;

  return req;
}


/**
 * \brief Create and initialize a new request
 */
static inline MPIR_Request *
MPIDI_Request_create2()
{
  MPIR_Request * req;
  req = MPIR_Request_create();
  MPIR_Object_set_ref(req, 2);

  return req;
}

static inline MPIR_Request *
MPIDI_Request_create1()
{
  MPIR_Request * req;
  req = MPIR_Request_create();
  MPIR_Object_set_ref(req, 1);

  return req;
}

/**
 * \brief Mark a request as cancel-pending
 * \param[in]  _req  The request to cancel
 * \return           The previous state
 */
#define MPIDI_Request_cancel_pending(_req)      \
({                                              \
  int _flag = (_req)->mpid.cancel_pending;      \
  (_req)->mpid.cancel_pending = TRUE;           \
  _flag;                                        \
})


static inline void
MPID_Request_free_inline(MPIR_Request *req)
{
  int count;
  MPID_assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST);
  MPIR_Object_release_ref(req, &count);
  MPID_assert(count >= 0);


  if (count == 0)
  {
    MPID_assert(MPIR_cc_is_complete(&req->cc));

    if (req->comm)              MPIR_Comm_release(req->comm, 0);
    if (req->greq_fns)          MPL_free(req->greq_fns);
    if (req->mpid.datatype_ptr) MPIDU_Datatype_release(req->mpid.datatype_ptr);
    if (req->mpid.uebuf_malloc== mpiuMalloc) {
        MPL_free(req->mpid.uebuf);
    }
    if(req->mpid.win_req)       MPL_free(req->mpid.win_req);
#if TOKEN_FLOW_CONTROL
    else if (req->mpid.uebuf_malloc == mpidiBufMM) {
        MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
        MPIDI_mm_free(req->mpid.uebuf,req->mpid.uebuflen);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }
#endif
    MPIDI_Request_tls_free(req);
  }
}


/* This request was never used, at most had uebuf allocated. */
static inline void
MPID_Request_discard_inline(MPIR_Request *req)
{
    if (req->mpid.uebuf_malloc == mpiuMalloc) {
        MPL_free(req->mpid.uebuf);
    }
#if TOKEN_FLOW_CONTROL
    else if (req->mpid.uebuf_malloc == mpidiBufMM) {
        MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
        MPIDI_mm_free(req->mpid.uebuf,req->mpid.uebuflen);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }
#endif
    MPIDI_Request_tls_free(req);
}

#define MPIR_REQUEST_SET_COMPLETED(req_) \
  MPIDI_Request_complete_norelease_inline(req_)

static inline void
MPIDI_Request_complete_inline(MPIR_Request *req)
{
    int count;
    MPIR_cc_decr(req->cc_ptr, &count);
    MPID_assert(count >= 0);

    MPIR_Request_free(req);
    if (count == 0) /* decrement completion count; if 0, signal progress engine */
    {
      MPIDI_Progress_signal();
    }
}


static inline void
MPIDI_Request_complete_norelease_inline(MPIR_Request *req)
{
    int count;
    MPIR_cc_decr(req->cc_ptr, &count);
    MPID_assert(count >= 0);

    if (count == 0) /* decrement completion count; if 0, signal progress engine */
    {
      MPIDI_Progress_signal();
    }
}


/** \} */


#endif

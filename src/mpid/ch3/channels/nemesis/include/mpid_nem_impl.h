/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_IMPL_H_INCLUDED
#define MPID_NEM_IMPL_H_INCLUDED

#include "my_papi_defs.h"
#include "mpidi_ch3_impl.h"
#include "mpir_mem.h"
#include "mpid_nem_net_module_defs.h"
#include "mpid_nem_atomics.h"
#include "mpid_nem_defs.h"
#include "mpid_nem_fbox.h"
#include "mpid_nem_nets.h"
#include "mpid_nem_queue.h"

#define MPID_NEM__BYPASS_Q_MAX_VAL  ((MPID_NEM_MPICH_DATA_LEN) - (sizeof(MPIDI_CH3_Pkt_t)))

int MPID_nem_mpich_init(void);
int MPID_nem_coll_init (void);
int MPID_nem_send_iov(MPIDI_VC_t *vc, MPIR_Request **sreq_ptr, MPL_IOV *iov, int n_iov);
int MPID_nem_lmt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize);
int MPID_nem_register_initcomp_cb(int (* callback)(void));
int MPID_nem_choose_netmod(void);
int MPIDI_CH3I_comm_create(MPIR_Comm *comm, void *param);
int MPIDI_CH3I_comm_destroy(MPIR_Comm *comm, void *param);

/* rendezvous hooks */
int MPID_nem_lmt_RndvSend(MPIR_Request **sreq_p, const void * buf, MPI_Aint count, MPI_Datatype datatype, int dt_contig,
                          intptr_t data_sz, MPI_Aint dt_true_lb, int rank, int tag, MPIR_Comm * comm, int context_offset);
int MPID_nem_lmt_RndvRecv(struct MPIDI_VC *vc, MPIR_Request *rreq);

#define MPID_nem_mpich_release_fbox(cell)                               \
    do {                                                                \
        OPA_store_release_int(&MPID_nem_mem_region.mailboxes.in[(cell)->pkt.header.source]->mpich.flag.value, 0); \
    } while (0)

/* initialize shared-memory MPI_Barrier variables */
int MPID_nem_barrier_vars_init (MPID_nem_barrier_vars_t *barrier_region);

/* assumes value!=0 means the fbox is full.  Contains acquire barrier to
 * ensure that later operations that are dependent on this check don't
 * escape earlier than this check. */
#define MPID_nem_fbox_is_full(pbox_) (OPA_load_acquire_int(&(pbox_)->flag.value))

typedef struct MPID_nem_pkt_lmt_rts
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
    intptr_t data_sz;
    intptr_t cookie_len;
}
MPID_nem_pkt_lmt_rts_t;

typedef struct MPID_nem_pkt_lmt_cts
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    MPI_Request receiver_req_id;
    intptr_t data_sz;
    intptr_t cookie_len;
}
MPID_nem_pkt_lmt_cts_t;

typedef struct MPID_nem_pkt_lmt_done
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request req_id;
}
MPID_nem_pkt_lmt_done_t;

typedef struct MPID_nem_pkt_lmt_cookie
{
    MPIDI_CH3_Pkt_type_t type;
    int from_sender;
    MPI_Request sender_req_id;
    MPI_Request receiver_req_id;
    intptr_t cookie_len;
}
MPID_nem_pkt_lmt_cookie_t;

typedef struct MPID_nem_pkt_ckpt_marker
{
    MPIDI_CH3_Pkt_type_t type;
    int wave; /* used for debugging */
}
MPID_nem_pkt_ckpt_marker_t;

typedef struct MPID_nem_pkt_netmod
{
    MPIDI_CH3_Pkt_type_t type;
    unsigned subtype;
}
MPID_nem_pkt_netmod_t;

typedef union MPIDI_CH3_nem_pkt
{
    MPID_nem_pkt_lmt_rts_t lmt_rts;
    MPID_nem_pkt_lmt_cts_t lmt_cts;
    MPID_nem_pkt_lmt_done_t lmt_done;
    MPID_nem_pkt_lmt_cookie_t lmt_cookie;
    MPID_nem_pkt_ckpt_marker_t ckpt_marker;
    MPID_nem_pkt_netmod_t netmod;
} MPIDI_CH3_nem_pkt_t;


/* MPID_PKT_DECL_CAST(pkt_u_var, s_pkt_type, s_pkt_p_var)
   To avoid strict aliasing warnings when doing something like:
       pkt_t upkt;
       rts_pkt_t * const rts_pkt = (rts_pkt_t *)&upkt;
   this macro does the same but through a union.

   pkt_u_var -- variable name of the packet union
   s_pkt_type -- type of the sub-packet
   s_pkt_p_var -- variable name of the sub-packet pointer
 */
#define MPID_PKT_DECL_CAST(pkt_u_var, s_pkt_type, s_pkt_p_var)  \
    union                                                       \
    {                                                           \
        MPIDI_CH3_Pkt_t p;                                      \
        s_pkt_type s;                                           \
    } pkt_u_var;                                                \
    s_pkt_type * const s_pkt_p_var = &pkt_u_var.s

    

/*  Macros for sending lmt messages from inside network modules.
    These assume mpi_errno is declared and the fn_exit and fn_fail
    labels are defined.
*/

#define MPID_nem_lmt_send_RTS(vc, rts_pkt, s_cookie_buf, s_cookie_len) do {                             \
        MPIR_Request *_rts_req;                                                                         \
        MPL_IOV _iov[2];                                                                               \
                                                                                                        \
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending rndv RTS packet");                                      \
        (rts_pkt)->cookie_len = (s_cookie_len);                                                         \
                                                                                                        \
        _iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(rts_pkt);                                            \
        _iov[0].MPL_IOV_LEN = sizeof(*(rts_pkt));                                                      \
        _iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(s_cookie_buf);                                       \
        _iov[1].MPL_IOV_LEN = (s_cookie_len);                                                          \
                                                                                                        \
        MPL_DBG_MSGPKT((vc), (rts_pkt)->match.parts.tag, (rts_pkt)->match.parts.context_id, (rts_pkt)->match.parts.rank, \
                        (rts_pkt)->data_sz, "Rndv");                                                    \
                                                                                                        \
        mpi_errno = MPIDI_CH3_iStartMsgv((vc), _iov, ((s_cookie_len)) ? 2 : 1, &_rts_req);              \
        /* --BEGIN ERROR HANDLING-- */                                                                  \
        if (mpi_errno != MPI_SUCCESS)                                                                   \
        {                                                                                               \
            if (NULL != _rts_req) {                                                                     \
                /* error case: drop both the ch3 and nemesis                                            \
                 * references, so the request can be cleanly freed */                                   \
                MPIR_Request_free(_rts_req);                                                         \
                MPIR_Request_free(_rts_req);                                                         \
            }                                                                                           \
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rtspkt");                                  \
        }                                                                                               \
        /* --END ERROR HANDLING-- */                                                                    \
        if (_rts_req != NULL)                                                                           \
        {                                                                                               \
            if (_rts_req->status.MPI_ERROR != MPI_SUCCESS)                                              \
            {                                                                                           \
                mpi_errno = _rts_req->status.MPI_ERROR;                                                 \
                /* error case: drop both the ch3 and nemesis                                            \
                 * references, so the request can be cleanly freed */                                   \
                MPIR_Request_free(_rts_req);                                                         \
                MPIR_Request_free(_rts_req);                                                         \
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rtspkt");                              \
            }                                                                                           \
            MPIR_Request_free(_rts_req);                                                             \
        }                                                                                               \
    } while (0)

#define MPID_nem_lmt_send_CTS(vc, rreq, r_cookie_buf, r_cookie_len) do {                                \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_pkt_lmt_cts_t, _cts_pkt);                                    \
        MPIR_Request *_cts_req;                                                                         \
        MPL_IOV _iov[2];                                                                               \
                                                                                                        \
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending rndv CTS packet");                                      \
        MPIDI_Pkt_init(_cts_pkt, MPIDI_NEM_PKT_LMT_CTS);                                                \
        _cts_pkt->sender_req_id = (rreq)->ch.lmt_req_id;                                                \
        _cts_pkt->receiver_req_id = (rreq)->handle;                                                     \
        _cts_pkt->cookie_len = (r_cookie_len);                                                          \
        _cts_pkt->data_sz = (rreq)->ch.lmt_data_sz;                                                     \
                                                                                                        \
        _iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)_cts_pkt;                                             \
        _iov[0].MPL_IOV_LEN = sizeof(*_cts_pkt);                                                       \
        _iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(r_cookie_buf);                                       \
        _iov[1].MPL_IOV_LEN = (r_cookie_len);                                                          \
                                                                                                        \
        mpi_errno = MPIDI_CH3_iStartMsgv((vc), _iov, (r_cookie_len) ? 2 : 1, &_cts_req);                \
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ctspkt");                           \
        if (_cts_req != NULL)                                                                           \
        {                                                                                               \
            MPIR_ERR_CHKANDJUMP(_cts_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**ctspkt");      \
            MPIR_Request_free(_cts_req);                                                             \
        }                                                                                               \
    } while (0)
        
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_send_COOKIE
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_nem_lmt_send_COOKIE(MPIDI_VC_t *vc, MPIR_Request *req,
                                           void *cookie_buf, MPI_Aint cookie_len)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_PKT_DECL_CAST(_upkt, MPID_nem_pkt_lmt_cookie_t, cookie_pkt);
    MPIR_Request *cookie_req;
    MPL_IOV iov[2];

    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending rndv COOKIE packet");
    MPIDI_Pkt_init(cookie_pkt, MPIDI_NEM_PKT_LMT_COOKIE);
    cookie_pkt->cookie_len = (cookie_len);

    /* We need to send both handle IDs in case there was
       no CTS message in this protocol. */
    switch (MPIDI_Request_get_type(req)) {
        case MPIDI_REQUEST_TYPE_RECV:
            cookie_pkt->from_sender = FALSE;
            cookie_pkt->sender_req_id = (req)->ch.lmt_req_id;
            cookie_pkt->receiver_req_id = (req)->handle;
            break;
        case MPIDI_REQUEST_TYPE_SEND:
        case MPIDI_REQUEST_TYPE_RSEND:
        case MPIDI_REQUEST_TYPE_SSEND:
        case MPIDI_REQUEST_TYPE_BSEND:
            cookie_pkt->from_sender = TRUE;
            cookie_pkt->sender_req_id = (req)->handle;
            cookie_pkt->receiver_req_id = (req)->ch.lmt_req_id;
            break;
        default:
            MPIR_ERR_INTERNALANDJUMP(mpi_errno, "unexpected request type");
            break;
    }

    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) cookie_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*cookie_pkt);
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) cookie_buf;
    iov[1].MPL_IOV_LEN = cookie_len;

    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, (cookie_len ? 2 : 1), &cookie_req);
    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**cookiepkt");
    if (cookie_req != NULL)
    {
        MPIR_ERR_CHKANDJUMP(cookie_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**cookiepkt");
        MPIR_Request_free(cookie_req);
    }

fn_fail:
    return mpi_errno;
}
        
#define MPID_nem_lmt_send_DONE(vc, rreq) do {                                                                   \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_pkt_lmt_done_t, _done_pkt);                                          \
        MPIR_Request *_done_req;                                                                                \
                                                                                                                \
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending rndv DONE packet");                                             \
        MPIDI_Pkt_init(_done_pkt, MPIDI_NEM_PKT_LMT_DONE);                                                      \
        _done_pkt->req_id = (rreq)->ch.lmt_req_id;                                                              \
                                                                                                                \
        mpi_errno = MPIDI_CH3_iStartMsg((vc), _done_pkt, sizeof(*_done_pkt), &_done_req);                       \
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**donepkt");                                  \
        if (_done_req != NULL)                                                                                  \
        {                                                                                                       \
            MPIR_ERR_CHKANDJUMP(_done_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**donepkt");            \
            MPIR_Request_free(_done_req);                                                                    \
        }                                                                                                       \
    } while (0)   


#endif /* MPID_NEM_IMPL_H_INCLUDED */

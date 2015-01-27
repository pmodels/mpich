/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_IMPL_H
#define OFI_IMPL_H

#include "mpid_nem_impl.h"
#include "mpihandlemem.h"
#include "pmi.h"
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_cm.h>
#include <netdb.h>

/* ************************************************************************** */
/* Type Definitions                                                           */
/* ************************************************************************** */
typedef struct iovec iovec_t;
typedef struct fi_info info_t;
typedef struct fi_cq_attr cq_attr_t;
typedef struct fi_av_attr av_attr_t;
typedef struct fi_domain_attr domain_attr_t;
typedef struct fi_tx_attr tx_attr_t;
typedef struct fi_cq_tagged_entry cq_tagged_entry_t;
typedef struct fi_cq_err_entry cq_err_entry_t;
typedef struct fi_context context_t;
typedef int (*event_callback_fn) (cq_tagged_entry_t * wc, MPID_Request *);
typedef int (*req_fn) (MPIDI_VC_t *, MPID_Request *, int *);

/* ******************************** */
/* Global Object for state tracking */
/* ******************************** */
typedef struct {
    char bound_addr[128];       /* This ranks bound address    */
    fi_addr_t any_addr;         /* Specifies any source        */
    size_t bound_addrlen;       /* length of the bound address */
    struct fid_fabric *fabric;  /* fabric object               */
    struct fid_domain *domain;  /* domain object               */
    struct fid_ep *endpoint;    /* endpoint object             */
    struct fid_cq *cq;          /* completion queue            */
    struct fid_av *av;          /* address vector              */
    struct fid_mr *mr;          /* memory region               */
    MPIDI_PG_t *pg_p;           /* MPI Process group           */
    MPIDI_VC_t *cm_vcs;         /* temporary VC's              */
    MPID_Request *persistent_req;       /* Unexpected request queue    */
    MPID_Request *conn_req;     /* Connection request          */
    MPIDI_Comm_ops_t comm_ops;
} MPID_nem_ofi_global_t;

/* ******************************** */
/* Device channel specific data     */
/* This is per destination          */
/* ******************************** */
typedef struct {
    fi_addr_t direct_addr;      /* Remote OFI address */
    int ready;                  /* VC ready state     */
    int is_cmvc;                /* Cleanup VC         */
    MPIDI_VC_t *next;           /* VC queue           */
} MPID_nem_ofi_vc_t;
#define VC_OFI(vc) ((MPID_nem_ofi_vc_t *)vc->ch.netmod_area.padding)

/* ******************************** */
/* Per request object data          */
/* OFI/Netmod specific              */
/* ******************************** */
typedef struct {
    context_t ofi_context;      /* Context Object              */
    void *addr;                 /* OFI Address                 */
    event_callback_fn event_callback;   /* Callback Event              */
    char *pack_buffer;          /* MPI Pack Buffer             */
    int pack_buffer_size;       /* Pack buffer size            */
    int match_state;            /* State of the match          */
    int req_started;            /* Request state               */
    MPIDI_VC_t *vc;             /* VC paired with this request */
    uint64_t tag;               /* 64 bit tag request          */
    MPID_Request *parent;       /* Parent request              */
} MPID_nem_ofi_req_t;
#define REQ_OFI(req) ((MPID_nem_ofi_req_t *)((req)->ch.netmod_area.padding))

/* ******************************** */
/* Logging and function macros      */
/* ******************************** */
#undef FUNCNAME
#define FUNCNAME nothing
#define BEGIN_FUNC(FUNCNAME)                    \
  MPIDI_STATE_DECL(FUNCNAME);                   \
  MPIDI_FUNC_ENTER(FUNCNAME);
#define END_FUNC(FUNCNAME)                      \
  MPIDI_FUNC_EXIT(FUNCNAME);
#define END_FUNC_RC(FUNCNAME) \
  fn_exit:                    \
  MPIDI_FUNC_EXIT(FUNCNAME);  \
  return mpi_errno;           \
fn_fail:                      \
  goto fn_exit;

#define __SHORT_FILE__                          \
  (strrchr(__FILE__,'/')                        \
   ? strrchr(__FILE__,'/')+1                    \
   : __FILE__                                   \
)
#define DECL_FUNC(FUNCNAME)  MPIU_QUOTE(FUNCNAME)
#define OFI_COMPILE_TIME_ASSERT(expr_)                                  \
  do { switch(0) { case 0: case (expr_): default: break; } } while (0)

#define FI_RC(FUNC,STR)                                         \
  do                                                            \
    {                                                           \
      ssize_t _ret = FUNC;                                      \
      MPIU_ERR_##CHKANDJUMP4(_ret<0,                            \
                           mpi_errno,                           \
                           MPI_ERR_OTHER,                       \
                           "**ofi_"#STR,                        \
                           "**ofi_"#STR" %s %d %s %s",          \
                           __SHORT_FILE__,                      \
                           __LINE__,                            \
                           FCNAME,                              \
                           fi_strerror(-_ret));                 \
    } while (0)

#define PMI_RC(FUNC,STR)                                        \
  do                                                            \
    {                                                           \
      pmi_errno  = FUNC;                                        \
      MPIU_ERR_##CHKANDJUMP4(pmi_errno!=PMI_SUCCESS,            \
                           mpi_errno,                           \
                           MPI_ERR_OTHER,                       \
                           "**ofi_"#STR,                        \
                           "**ofi_"#STR" %s %d %s %s",          \
                           __SHORT_FILE__,                      \
                           __LINE__,                            \
                           FCNAME,                              \
                           #STR);                               \
    } while (0)

#define MPI_RC(FUNC)                                        \
  do                                                        \
    {                                                       \
      mpi_errno  = FUNC;                                    \
      if (mpi_errno) MPIU_ERR_POP(mpi_errno);               \
    } while (0);

#define VC_READY_CHECK(vc)                      \
({                                              \
  if (1 != VC_OFI(vc)->ready) {                 \
    MPI_RC(MPID_nem_ofi_vc_connect(vc));        \
  }                                             \
})

#define OFI_ADDR_INIT(src, vc, remote_proc) \
({                                          \
  if (MPI_ANY_SOURCE != src) {              \
    MPIU_Assert(vc != NULL);                \
    VC_READY_CHECK(vc);                     \
    remote_proc = VC_OFI(vc)->direct_addr;  \
  } else {                                  \
    MPIU_Assert(vc == NULL);                \
    remote_proc = gl_data.any_addr;         \
  }                                         \
})


#define NO_PGID 0

/* **************************************************************************
 *  match/ignore bit manipulation
 * **************************************************************************
 * 0123 4567 01234567 0123 4567 01234567 0123 4567 01234567 01234567 01234567
 *     |                  |                  |
 * ^   |    context id    |       source     |       message tag
 * |   |                  |                  |
 * +---- protocol
 * ************************************************************************** */
#define MPID_PROTOCOL_MASK       (0xF000000000000000ULL)
#define MPID_CONTEXT_MASK        (0x0FFFF00000000000ULL)
#define MPID_SOURCE_MASK         (0x00000FFFF0000000ULL)
#define MPID_TAG_MASK            (0x000000000FFFFFFFULL)
#define MPID_PGID_MASK           (0x00000000FFFFFFFFULL)
#define MPID_PSOURCE_MASK        (0x0000FFFF00000000ULL)
#define MPID_PORT_NAME_MASK      (0x0FFF000000000000ULL)
#define MPID_SYNC_SEND           (0x1000000000000000ULL)
#define MPID_SYNC_SEND_ACK       (0x2000000000000000ULL)
#define MPID_MSG_RTS             (0x3000000000000000ULL)
#define MPID_MSG_CTS             (0x4000000000000000ULL)
#define MPID_MSG_DATA            (0x5000000000000000ULL)
#define MPID_CONN_REQ            (0x6000000000000000ULL)
#define MPID_SOURCE_SHIFT        (16)
#define MPID_TAG_SHIFT           (28)
#define MPID_PSOURCE_SHIFT       (16)
#define MPID_PORT_SHIFT          (32)
#define OFI_KVSAPPSTRLEN         1024

/* ******************************** */
/* Request manipulation inlines     */
/* ******************************** */
static inline void MPID_nem_ofi_init_req(MPID_Request * req)
{
    memset(REQ_OFI(req), 0, sizeof(MPID_nem_ofi_req_t));
}

static inline int MPID_nem_ofi_create_req(MPID_Request ** request, int refcnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req;
    req = MPID_Request_create();
    MPIU_Assert(req);
    MPIU_Object_set_ref(req, refcnt);
    MPID_nem_ofi_init_req(req);
    *request = req;
    return mpi_errno;
}

/* ******************************** */
/* Tag Manipulation inlines         */
/* ******************************** */
static inline uint64_t init_sendtag(MPIR_Context_id_t contextid, int source, int tag, uint64_t type)
{
    uint64_t match_bits;
    match_bits = contextid;
    match_bits = (match_bits << MPID_SOURCE_SHIFT);
    match_bits |= source;
    match_bits = (match_bits << MPID_TAG_SHIFT);
    match_bits |= (MPID_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
static inline uint64_t init_recvtag(uint64_t * mask_bits,
                                    MPIR_Context_id_t contextid, int source, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPID_SYNC_SEND;
    match_bits = contextid;
    match_bits = (match_bits << MPID_SOURCE_SHIFT);
    if (MPI_ANY_SOURCE == source) {
        match_bits = (match_bits << MPID_TAG_SHIFT);
        *mask_bits |= MPID_SOURCE_MASK;
    }
    else {
        match_bits |= source;
        match_bits = (match_bits << MPID_TAG_SHIFT);
    }
    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPID_TAG_MASK;
    else
        match_bits |= (MPID_TAG_MASK & tag);

    return match_bits;
}

static inline int get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPID_TAG_MASK));
}

static inline int get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_SOURCE_MASK) >> (MPID_TAG_SHIFT)));
}

static inline int get_psource(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_PSOURCE_MASK) >> (MPID_PORT_SHIFT)));
}

static inline int get_pgid(uint64_t match_bits)
{
    return ((int) (match_bits & MPID_PGID_MASK));
}

static inline int get_port(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_PORT_NAME_MASK) >> MPID_TAG_SHIFT));
}

/* ************************************************************************** */
/* MPICH Comm Override and Netmod functions                                   */
/* ************************************************************************** */
int MPID_nem_ofi_recv_posted(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_ofi_send(struct MPIDI_VC *vc, const void *buf, int count,
                      MPI_Datatype datatype, int dest, int tag, MPID_Comm * comm,
                      int context_offset, struct MPID_Request **request);
int MPID_nem_ofi_isend(struct MPIDI_VC *vc, const void *buf, int count,
                       MPI_Datatype datatype, int dest, int tag, MPID_Comm * comm,
                       int context_offset, struct MPID_Request **request);
int MPID_nem_ofi_ssend(struct MPIDI_VC *vc, const void *buf, int count,
                       MPI_Datatype datatype, int dest, int tag, MPID_Comm * comm,
                       int context_offset, struct MPID_Request **request);
int MPID_nem_ofi_issend(struct MPIDI_VC *vc, const void *buf, int count,
                        MPI_Datatype datatype, int dest, int tag, MPID_Comm * comm,
                        int context_offset, struct MPID_Request **request);
int MPID_nem_ofi_cancel_send(struct MPIDI_VC *vc, struct MPID_Request *sreq);
int MPID_nem_ofi_cancel_recv(struct MPIDI_VC *vc, struct MPID_Request *rreq);
int MPID_nem_ofi_iprobe(struct MPIDI_VC *vc, int source, int tag, MPID_Comm * comm,
                        int context_offset, int *flag, MPI_Status * status);
int MPID_nem_ofi_improbe(struct MPIDI_VC *vc, int source, int tag, MPID_Comm * comm,
                         int context_offset, int *flag, MPID_Request ** message,
                         MPI_Status * status);
int MPID_nem_ofi_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset,
                                  int *flag, MPI_Status * status);
int MPID_nem_ofi_anysource_improbe(int tag, MPID_Comm * comm, int context_offset,
                                   int *flag, MPID_Request ** message, MPI_Status * status);
void MPID_nem_ofi_anysource_posted(MPID_Request * rreq);
int MPID_nem_ofi_anysource_matched(MPID_Request * rreq);
int MPID_nem_ofi_send_data(cq_tagged_entry_t * wc, MPID_Request * sreq);
int MPID_nem_ofi_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq,
                               void *hdr, MPIDI_msg_sz_t hdr_sz);
int MPID_nem_ofi_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                 void *data, MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr);
int MPID_nem_ofi_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                             MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz);

/* ************************************************************************** */
/* OFI utility functions : not exposed as a netmod public API                 */
/* ************************************************************************** */
#define MPID_NONBLOCKING_POLL 0
#define MPID_BLOCKING_POLL 1
int MPID_nem_ofi_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_ofi_finalize(void);
int MPID_nem_ofi_vc_init(MPIDI_VC_t * vc);
int MPID_nem_ofi_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_ofi_poll(int in_blocking_poll);
int MPID_nem_ofi_vc_terminate(MPIDI_VC_t * vc);
int MPID_nem_ofi_vc_connect(MPIDI_VC_t * vc);
int MPID_nem_ofi_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc);
int MPID_nem_ofi_vc_destroy(MPIDI_VC_t * vc);
int MPID_nem_ofi_cm_init(MPIDI_PG_t * pg_p, int pg_rank);
int MPID_nem_ofi_cm_finalize();

extern MPID_nem_ofi_global_t gl_data;
extern MPIDI_Comm_ops_t _g_comm_ops;

#endif

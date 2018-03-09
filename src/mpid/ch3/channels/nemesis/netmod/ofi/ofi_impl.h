/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_IMPL_H_INCLUDED
#define OFI_IMPL_H_INCLUDED

#include "mpid_nem_impl.h"
#include "mpir_objects.h"
#include "pmi.h"
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_cm.h>
#include <netdb.h>

#include "ofi_tag_layout.h"

/*
 * Port name consists of tag and business card fields
 * and has a limit in length up to 256 (MPI_MAX_PORT_NAME).
 *
 * Port name has the following format:
 * <tag><business card>.
 *
 * <tag> has the following format:
 * tag<delim>tag_value<separ>\0
 * where "tag" is constant string (3 chars),
 * <delim>  and <separ> are one-char delimeters (2 chars),
 * tag_value is integer value (12 chars are enough for max value).
 *
 * <business card> has the following format:
 * OFI<delim>address<separ>
 * where "OFI" is constant string (3 chars),
 * <delim>  and <separ> are one-char delimeters (2 chars).
 * address is variable string.
 *
 * Address is provider specific and have encoded form
 * (that increase the original address length in 2 times).
 * Hence taking in account MPI_MAX_PORT_NAME
 * the original address length has a limit on top (OFI_MAX_ADDR_LEN).
 */
#define OFI_MAX_ADDR_LEN (((MPI_MAX_PORT_NAME) - 23)/2)

/* ************************************************************************** */
/* Type Definitions                                                           */
/* ************************************************************************** */
typedef struct iovec iovec_t;
typedef struct fi_info info_t;
typedef struct fi_cq_attr cq_attr_t;
typedef struct fi_av_attr av_attr_t;
typedef struct fi_domain_attr domain_attr_t;
typedef struct fi_ep_attr ep_attr_t;
typedef struct fi_tx_attr tx_attr_t;
typedef struct fi_cq_tagged_entry cq_tagged_entry_t;
typedef struct fi_cq_err_entry cq_err_entry_t;
typedef struct fi_context context_t;
typedef struct fi_msg_tagged msg_tagged_t;
typedef int (*event_callback_fn) (cq_tagged_entry_t * wc, MPIR_Request *);
typedef int (*req_fn) (MPIDI_VC_t *, MPIR_Request *, int *);

/* ******************************** */
/* Global Object for state tracking */
/* ******************************** */
typedef struct {
    char bound_addr[OFI_MAX_ADDR_LEN]; /* This ranks bound address    */
    size_t bound_addrlen;              /* length of the bound address */
    struct fid_fabric *fabric;         /* fabric object               */
    struct fid_domain *domain;         /* domain object               */
    struct fid_ep *endpoint;           /* endpoint object             */
    struct fid_cq *cq;                 /* completion queue            */
    struct fid_av *av;                 /* address vector              */
    struct fid_mr *mr;                 /* memory region               */
    size_t iov_limit;                  /* Max send iovec limit        */
    size_t max_buffered_send;          /* Buffered send threshold     */
    int rts_cts_in_flight;             /* Count of incompleted        */
                                       /*   RTS-CTS-DATA exchanges    */
    int api_set;                       /* Used OFI API for send       */
                                       /*   operations                */
    MPIR_Request *persistent_req;      /* Unexpected request queue    */
    MPIR_Request *conn_req;            /* Connection request          */
    MPIDI_PG_t *pg_p;                  /* MPI Process group           */
    MPIDI_VC_t *cm_vcs;                /* temporary VC's              */
} MPID_nem_ofi_global_t __attribute__ ((aligned (MPID_NEM_CACHE_LINE_LEN)));

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
    event_callback_fn event_callback;   /* Callback Event      */
    char  *pack_buffer;         /* MPI Pack Buffer             */
    size_t pack_buffer_size;    /* Pack buffer size            */
    size_t msg_bytes;           /* msg api bytes               */
    int    iov_count;           /* Number of iovecs            */
    void *real_hdr;             /* Extended header             */
    int match_state;            /* State of the match          */
    int req_started;            /* Request state               */
    MPIDI_VC_t *vc;             /* VC paired with this request */
    uint64_t tag;               /* 64 bit tag request          */
    struct iovec iov[3];        /* scatter gather list         */
    MPIR_Request *parent;       /* Parent request              */
} MPID_nem_ofi_req_t;
#define REQ_OFI(req) ((MPID_nem_ofi_req_t *)((req)->ch.netmod_area.padding))

/* ******************************** */
/* Logging and function macros      */
/* ******************************** */
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
#define OFI_COMPILE_TIME_ASSERT(expr_)                                  \
  do { switch(0) { case 0: case (expr_): default: break; } } while (0)

#define FI_RC(FUNC,STR)                                         \
  do                                                            \
    {                                                           \
      ssize_t _ret = FUNC;                                      \
      MPIR_ERR_##CHKANDJUMP4(_ret<0,                            \
                           mpi_errno,                           \
                           MPI_ERR_OTHER,                       \
                           "**ofi_"#STR,                        \
                           "**ofi_"#STR" %s %d %s %s",          \
                           __SHORT_FILE__,                      \
                           __LINE__,                            \
                           FCNAME,                              \
                           fi_strerror(-_ret));                 \
    } while (0)

#define FI_RC_RETRY(FUNC,STR)						\
	do {								\
		ssize_t _ret;                                           \
		do {							\
			_ret = FUNC;                                    \
			if(likely(_ret==0)) break;			\
			MPIR_ERR_##CHKANDJUMP4(_ret != -FI_EAGAIN,	\
					       mpi_errno,		\
					       MPI_ERR_OTHER,		\
					       "**ofi_"#STR,		\
					       "**ofi_"#STR" %s %d %s %s", \
					       __SHORT_FILE__,		\
					       __LINE__,		\
					       FCNAME,			\
					       fi_strerror(-_ret));	\
				mpi_errno = MPID_nem_ofi_poll(0);	\
				if(mpi_errno != MPI_SUCCESS)		\
					MPIR_ERR_POP(mpi_errno);	\
		} while (_ret == -FI_EAGAIN);				\
	} while (0)


#define PMI_RC(FUNC,STR)                                        \
  do                                                            \
    {                                                           \
      pmi_errno  = FUNC;                                        \
      MPIR_ERR_##CHKANDJUMP4(pmi_errno!=PMI_SUCCESS,            \
                           mpi_errno,                           \
                           MPI_ERR_OTHER,                       \
                           "**ofi_"#STR,                        \
                           "**ofi_"#STR" %s %d %s %s",          \
                           __SHORT_FILE__,                      \
                           __LINE__,                            \
                           FCNAME,                              \
                           #STR);                               \
    } while (0)

#define MPIDI_CH3I_NM_OFI_RC(FUNC)                          \
  do                                                        \
    {                                                       \
      mpi_errno  = FUNC;                                    \
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);               \
    } while (0);

#define VC_READY_CHECK(vc)                      \
({                                              \
  if (1 != VC_OFI(vc)->ready) {                 \
    MPIDI_CH3I_NM_OFI_RC(MPID_nem_ofi_vc_connect(vc));  \
  }                                             \
})

#define OFI_ADDR_INIT(src, vc, remote_proc) \
({                                          \
  if (MPI_ANY_SOURCE != src) {              \
    MPIR_Assert(vc != NULL);                \
    VC_READY_CHECK(vc);                     \
    remote_proc = VC_OFI(vc)->direct_addr;  \
  } else {                                  \
    MPIR_Assert(vc == NULL);                \
    remote_proc = FI_ADDR_UNSPEC;           \
  }                                         \
})


#define NO_PGID 0


#define PEEK_INIT      0
#define PEEK_FOUND     1
#define PEEK_NOT_FOUND 2

#define MEM_TAG_FORMAT (0xFFFF00000000LLU)
/* ******************************** */
/* Request manipulation inlines     */
/* ******************************** */
static inline void MPID_nem_ofi_init_req(MPIR_Request * req)
{
    memset(REQ_OFI(req), 0, sizeof(MPID_nem_ofi_req_t));
}

static inline int MPID_nem_ofi_create_req(MPIR_Request ** request, int refcnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_Assert(req);
    MPIR_Object_set_ref(req, refcnt);
    MPID_nem_ofi_init_req(req);
    *request = req;
    return mpi_errno;
}

static inline int MPID_nem_ofi_create_req_lw(MPIR_Request ** request, int refcnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;

    MPID_nem_ofi_create_req(&req, refcnt);

    /* resetting the kind and cc here should not add additional
     * instructions since a good compiler will discard the kind and cc
     * setting in the above request create anyway */
    req->kind = MPIR_REQUEST_KIND__SEND;
    MPIR_cc_set(&req->cc, 0); // request is already completed

    *request = req;
    return mpi_errno;
}

/* ************************************************************************** */
/* MPICH Comm Override and Netmod functions                                   */
/* ************************************************************************** */
#define DECLARE_TWO_API_SETS(_ret, _fc_name, ...) \
    _ret _fc_name(__VA_ARGS__);                   \
    _ret _fc_name##_2(__VA_ARGS__);

DECLARE_TWO_API_SETS(int, MPID_nem_ofi_recv_posted, struct MPIDI_VC *vc, struct MPIR_Request *req);

DECLARE_TWO_API_SETS(int, MPID_nem_ofi_send, struct MPIDI_VC *vc, const void *buf, MPI_Aint count,\
                     MPI_Datatype datatype, int dest, int tag, MPIR_Comm * comm,\
                     int context_offset, struct MPIR_Request **request);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_isend, struct MPIDI_VC *vc, const void *buf, MPI_Aint count,\
                     MPI_Datatype datatype, int dest, int tag, MPIR_Comm * comm,\
                     int context_offset, struct MPIR_Request **request);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_ssend, struct MPIDI_VC *vc, const void *buf, MPI_Aint count,\
                     MPI_Datatype datatype, int dest, int tag, MPIR_Comm * comm,
                     int context_offset, struct MPIR_Request **request);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_issend, struct MPIDI_VC *vc, const void *buf, MPI_Aint count,\
                     MPI_Datatype datatype, int dest, int tag, MPIR_Comm * comm,\
                     int context_offset, struct MPIR_Request **request);
int MPID_nem_ofi_cancel_send(struct MPIDI_VC *vc, struct MPIR_Request *sreq);
int MPID_nem_ofi_cancel_recv(struct MPIDI_VC *vc, struct MPIR_Request *rreq);

DECLARE_TWO_API_SETS(int, MPID_nem_ofi_iprobe, struct MPIDI_VC *vc, int source, int tag, MPIR_Comm * comm,
                     int context_offset, int *flag, MPI_Status * status);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_improbe,struct MPIDI_VC *vc, int source, int tag, MPIR_Comm * comm,
                     int context_offset, int *flag, MPIR_Request ** message,
                     MPI_Status * status);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_anysource_iprobe,int tag, MPIR_Comm * comm, int context_offset,
                     int *flag, MPI_Status * status);
DECLARE_TWO_API_SETS(int, MPID_nem_ofi_anysource_improbe,int tag, MPIR_Comm * comm, int context_offset,
                     int *flag, MPIR_Request ** message, MPI_Status * status);
DECLARE_TWO_API_SETS(void, MPID_nem_ofi_anysource_posted, MPIR_Request * rreq);

int MPID_nem_ofi_anysource_matched(MPIR_Request * rreq);
int MPID_nem_ofi_send_data(cq_tagged_entry_t * wc, MPIR_Request * sreq);
int MPID_nem_ofi_SendNoncontig(MPIDI_VC_t * vc, MPIR_Request * sreq,
                               void *hdr, intptr_t hdr_sz);
int MPID_nem_ofi_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, intptr_t hdr_sz,
                                 void *data, intptr_t data_sz, MPIR_Request ** sreq_ptr);
int MPID_nem_ofi_iSendContig(MPIDI_VC_t * vc, MPIR_Request * sreq, void *hdr,
                             intptr_t hdr_sz, void *data, intptr_t data_sz);

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
int MPID_nem_ofi_get_ordering(int *ordering);

extern MPID_nem_ofi_global_t gl_data;
extern MPIDI_Comm_ops_t _g_comm_ops;

#endif /* OFI_IMPL_H_INCLUDED */

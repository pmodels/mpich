/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */

#ifndef	LLC_MODULE_IMPL_H
#define LLC_MODULE_IMPL_H

#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif
#include "mpid_nem_impl.h"
#include "llc.h"

extern int MPID_nem_llc_my_llc_rank;

/* The vc provides a generic buffer in which network modules can store
 *    private fields This removes all dependencies from the VC struction
 *       on the network module, facilitating dynamic module loading. */

/*
 * GENERIC_Q_*:
 *   src/mpid/ch3/channels/nemesis/include/mpid_nem_generic_queue.h
 */
typedef GENERIC_Q_DECL(struct MPID_Request) rque_t;
/*
typedef GENERIC_Q_DECL(struct MPID_Request) MPIDI_nem_llc_request_queue_t;
 */

typedef struct {
    uint64_t remote_endpoint_addr;
    void *endpoint;
    rque_t send_queue;          /* MPID_Request Queue */
    unsigned int unsolicited_count;
} MPID_nem_llc_vc_area;

/* macro for llc private in VC */
#define VC_LLC(vc) ((MPID_nem_llc_vc_area *)(vc)->ch.netmod_area.padding)
#define VC_FIELD(vcp, field) (((MPID_nem_llc_vc_area *)(vc)->ch.netmod_area.padding)->field)

#define UNSOLICITED_NUM_INC(req) \
{ \
    MPID_Request *sreq = req; \
    MPIDI_VC_t *vc = sreq->ch.vc; \
    VC_FIELD(vc, unsolicited_count)++; \
}
#define UNSOLICITED_NUM_DEC(req) \
{ \
    MPID_Request *sreq = req; \
    MPIDI_VC_t *vc = sreq->ch.vc; \
    VC_FIELD(vc, unsolicited_count)--; \
}

typedef struct {
    void *cmds;
    void *pack_buf;             /* to pack non-contiguous data */

    void *rma_buf;
} MPID_nem_llc_req_area;

#define REQ_LLC(req) \
    ((MPID_nem_llc_req_area *)(&(req)->ch.netmod_area.padding))
#define REQ_FIELD(reqp, field) (((MPID_nem_llc_req_area *)((reqp)->ch.netmod_area.padding))->field)

struct llc_cmd_area {
    void *cbarg;
    uint32_t raddr;
};

/* functions */
int MPID_nem_llc_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_llc_finalize(void);
int MPID_nem_llc_poll(int in_blocking_progress);
int MPID_nem_llc_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_llc_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc);
int MPID_nem_llc_vc_init(MPIDI_VC_t * vc);
int MPID_nem_llc_vc_destroy(MPIDI_VC_t * vc);
int MPID_nem_llc_vc_terminate(MPIDI_VC_t * vc);

int MPID_nem_llc_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                  MPI_Status * status);
int MPID_nem_llc_anysource_improbe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                   MPID_Request ** message, MPI_Status * status);
int MPID_nem_llc_get_ordering(int *ordering);

int MPID_nem_llc_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                             void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_llc_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data,
                                 MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr);
int MPID_nem_llc_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                               MPIDI_msg_sz_t hdr_sz);

int MPIDI_nem_llc_Rqst_iov_update(MPID_Request * mreq, MPIDI_msg_sz_t consume);
int MPID_nem_llc_send_queued(MPIDI_VC_t * vc, rque_t * send_queue);

int MPID_nem_llc_isend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
                       int dest, int tag, MPID_Comm * comm, int context_offset,
                       struct MPID_Request **request);
int MPID_nem_llc_issend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
                        int dest, int tag, MPID_Comm * comm, int context_offset,
                        struct MPID_Request **request);
int MPID_nem_llc_recv_posted(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_llc_kvs_put_binary(int from, const char *postfix, const uint8_t * buf, int length);
int MPID_nem_llc_kvs_get_binary(int from, const char *postfix, char *buf, int length);
void MPID_nem_llc_anysource_posted(MPID_Request * req);
int MPID_nem_llc_anysource_matched(MPID_Request * req);
int MPID_nem_llc_probe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                       MPI_Status * status);
int MPID_nem_llc_iprobe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                        int *flag, MPI_Status * status);
int MPID_nem_llc_improbe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status);
int MPID_nem_llc_cancel_recv(struct MPIDI_VC *vc, struct MPID_Request *req);

/*
 * temporary llc api
 */
typedef void (*llc_send_f) (void *cba, uint64_t * reqid);
typedef void (*llc_recv_f)
 (void *cba, uint64_t addr, void *buf, size_t bsz);

extern ssize_t llc_writev(void *endpt, uint64_t raddr,
                          const struct iovec *iovs, int niov, void *cbarg, void **vpp_reqid);
extern int llc_bind(void **vpp_endpt, uint64_t raddr, void *cbarg);
extern int llc_unbind(void *endpt);

extern int llc_poll(int in_blocking_poll, llc_send_f sfnc, llc_recv_f rfnc);

extern int convert_rank_llc2mpi(MPID_Comm * comm, int llc_rank, int *mpi_rank);
typedef struct MPID_nem_llc_netmod_hdr {
    int initiator_pg_rank;
#ifndef	notdef_hsiz_hack
    int reserved_for_alignment;
#endif                          /* notdef_hsiz_hack */
} MPID_nem_llc_netmod_hdr_t;

#define MPID_nem_llc_segv printf("%d\n", *(int32_t*)0);

#endif /* LLC_MODULE_IMPL_H */

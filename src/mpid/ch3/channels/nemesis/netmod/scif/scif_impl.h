/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef SCIF_IMPL_H
#define SCIF_IMPL_H

#include "mpid_nem_impl.h"
#include <sys/types.h>
#include <scif.h>
#include <errno.h>
#include "scifrw.h"

typedef GENERIC_Q_DECL(struct MPID_Request) reqq_t;

typedef struct {
    int fd;
    struct scif_portID addr;
    MPIDI_VC_t *vc;
    shmchan_t csend, crecv;
} scifconn_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct {
    scifconn_t *sc;
    reqq_t send_queue;
    int terminate;
} MPID_nem_scif_vc_area;

/* macro for scif private in VC */
#define VC_SCIF(vc) ((MPID_nem_scif_vc_area *)vc->ch.netmod_area.padding)

typedef struct {
    uint64_t seqno;
} scifmsg_t;

#define RQ_SCIF(req) ((scifmsg_t *)(&(req)->ch.netmod_area.padding))

#define ASSIGN_SC_TO_VC(vc_scif_, sc_) do {      \
        (vc_scif_)->sc = (sc_);                  \
    } while (0)

/* functions */
int MPID_nem_scif_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_scif_finalize(void);

int MPID_nem_scif_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_scif_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc);
int MPID_nem_scif_vc_init(MPIDI_VC_t * vc);
int MPID_nem_scif_vc_destroy(MPIDI_VC_t * vc);
int MPID_nem_scif_vc_terminate(MPIDI_VC_t * vc);

int MPID_nem_scif_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq,
                              void *hdr, MPIDI_msg_sz_t hdr_sz,
                              void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_scif_iStartContigMsg(MPIDI_VC_t * vc, void *hdr,
                                  MPIDI_msg_sz_t hdr_sz, void *data,
                                  MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr);
int MPID_nem_scif_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq,
                                void *header, MPIDI_msg_sz_t hdr_sz);

int MPID_nem_scif_vc_terminated(MPIDI_VC_t * vc);
int MPID_nem_scif_connpoll(int in_blocking_poll);

int MPID_nem_scif_error_out_send_queue(struct MPIDI_VC *vc, int req_errno);
int MPID_nem_scif_send_queued(MPIDI_VC_t * vc, reqq_t * send_queue);

/* Keys for business cards */
#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY "description"
#define MPIDI_CH3I_NODE_KEY "node"

#define MPID_NEM_SCIF_RECV_MAX_PKT_LEN 1024

extern scifconn_t *MPID_nem_scif_conns;
extern int MPID_nem_scif_nranks;
extern char *MPID_nem_scif_recv_buf;
extern int MPID_nem_scif_myrank;

#endif /* SCIF_IMPL_H */

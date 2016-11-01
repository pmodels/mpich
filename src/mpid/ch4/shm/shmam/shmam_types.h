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

#ifndef SHM_SHMAM_TYPES_H_INCLUDED
#define SHM_SHMAM_TYPES_H_INCLUDED

#include "mpidu_shm.h"

enum {
    MPIDI_SHMAM_OK,
    MPIDI_SHMAM_NOK
};

#define MPIDI_SHM_DEFAULT_SHORT_SEND_SIZE  (64 * 1024)
#define MPIDI_SHM_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_SHM_BUF_POOL_SIZE            (1024)
#define MPIDI_SHM_BUF_POOL_NUM             (1024)
#define MPIDI_SHM_MAX_AM_HANDLERS          (MPIDI_CH4U_PKT_MAX)
#define MPIDI_SHM_MAX_AM_HANDLERS_TOTAL    (MPIDI_SHM_MAX_AM_HANDLERS)

#define MPIDI_SHM_AMREQUEST(req,field)      ((req)->dev.ch4.ch4u.shm_am.shmam.field)
#define MPIDI_SHM_AMREQUEST_HDR(req, field) ((req)->dev.ch4.ch4u.shm_am.shmam.req_hdr->field)
#define MPIDI_SHM_AMREQUEST_HDR_PTR(req)    ((req)->dev.ch4.ch4u.shm_am.shmam.req_hdr)
#define MPIDI_SHM_REQUEST(req, field)       ((req)->dev.ch4.shm.shmam.field)

typedef struct {
    MPIDI_NM_am_target_handler_fn am_handlers[MPIDI_SHM_MAX_AM_HANDLERS_TOTAL];
    MPIDI_NM_am_origin_handler_fn am_send_cmpl_handlers[MPIDI_SHM_MAX_AM_HANDLERS_TOTAL];
    MPIU_buf_pool_t *am_buf_pool;

    /* Postponed queue */

    MPIDI_CH4U_rreq_t *postponed_queue;

    /* Active recv requests array */

    MPIR_Request **active_rreq;

#ifdef SHM_AM_DEBUG
    FILE *logfile;
#endif                          /* SHM_AM_DEBUG */
} MPIDI_SHM_Shmam_global_t;

extern MPIDI_SHM_Shmam_global_t MPIDI_SHM_Shmam_global;

#endif /* SHM_SHMAM_TYPES_H_INCLUDED */

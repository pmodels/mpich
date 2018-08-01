/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef POSIX_TYPES_H_INCLUDED
#define POSIX_TYPES_H_INCLUDED

#include "mpidu_shm.h"

enum {
    MPIDI_POSIX_OK,
    MPIDI_POSIX_NOK
};

#define MPIDI_POSIX_DEFAULT_SHORT_SEND_SIZE  (64 * 1024)
#define MPIDI_POSIX_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_POSIX_BUF_POOL_SIZE            (1024)
#define MPIDI_POSIX_BUF_POOL_NUM             (1024)

#define MPIDI_POSIX_AMREQUEST_PTR(req)        (&((req)->dev.ch4.am.shm_am.posix))
#define MPIDI_POSIX_AMREQUEST(req,field)      ((req)->dev.ch4.am.shm_am.posix.field)
#define MPIDI_POSIX_AMREQUEST_HDR(req, field) ((req)->dev.ch4.am.shm_am.posix.req_hdr->field)
#define MPIDI_POSIX_AMREQUEST_HDR_PTR(req)    ((req)->dev.ch4.am.shm_am.posix.req_hdr)
#define MPIDI_POSIX_REQUEST(req, field)       ((req)->dev.ch4.shm.posix.field)

typedef struct {
    MPIU_buf_pool_t *am_buf_pool;

    /* Postponed queue */

    MPIDI_POSIX_am_request_t *postponed_queue;

    /* Active recv requests array */

    MPIR_Request **active_rreq;
} MPIDI_POSIX_global_t;

extern MPIDI_POSIX_global_t MPIDI_POSIX_global;
extern MPL_dbg_class MPIDI_CH4_SHM_POSIX_GENERAL;

#define POSIX_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_CH4_SHM_POSIX_GENERAL,VERBOSE,(MPL_DBG_FDEST, __VA_ARGS__))

#endif /* POSIX_TYPES_H_INCLUDED */

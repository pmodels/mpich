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
#ifndef STUBSHM_REQUEST_H_INCLUDED
#define STUBSHM_REQUEST_H_INCLUDED

#include "stubshm_impl.h"

static inline void MPIDI_STUBSHM_am_request_init(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_INIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_INIT);
}

static inline void MPIDI_STUBSHM_am_request_finalize(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_FINALIZE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_REQUEST_FINALIZE);
}

#endif /* STUBSHM_REQUEST_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_REQUEST_H_INCLUDED
#define CH4R_REQUEST_H_INCLUDED

#include "ch4_types.h"
#include "ch4r_buf.h"

static inline MPIR_Request *MPIDIG_request_create(MPIR_Request_kind_t kind, int ref_count)
{
    MPIR_Request *req;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REQUEST_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REQUEST_CREATE);

    req = MPIR_Request_create(kind);
    if (req == NULL)
        goto fn_fail;

    /* as long as ref_count is a constant, any compiler should be able
     * to unroll the below loop.  when threading is not enabled, the
     * compiler should be able to combine the below individual
     * increments to a single increment of "ref_count - 1".
     *
     * FIXME: when threading is enabled, the ref_count increase is an
     * atomic operation, so it might be more inefficient.  we should
     * use a new API to increase the ref_count value instead of the
     * for loop. */
    for (i = 0; i < ref_count - 1; i++)
        MPIR_Request_add_ref(req);

    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_BUF_POOL_SZ);
    MPIDIG_REQUEST(req, req) = (MPIDIG_req_ext_t *) MPIDIU_get_buf(MPIDI_global.buf_pool);
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REQUEST_CREATE);
    return req;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_request_init(MPIR_Request * req,
                                                           MPIR_Request_kind_t kind)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REQUEST_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REQUEST_INIT);

    MPIR_Assert(req != NULL);
    /* Increment the refcount by one to account for the MPIDIG layer */
    MPIR_Request_add_ref(req);

    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_BUF_POOL_SZ);
    MPIDIG_REQUEST(req, req) = (MPIDIG_req_ext_t *) MPIDIU_get_buf(MPIDI_global.buf_pool);
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REQUEST_INIT);
    return req;
}

#endif /* CH4R_REQUEST_H_INCLUDED */

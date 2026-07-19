/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MPID_UCC_COLLOPS_H_
#define _MPID_UCC_COLLOPS_H_

#include "mpid_ucc.h"

#define MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(_collop, _format, ...) \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COLLOP,     \
                             "posting ucc " #_collop " req | " _format, \
                             __VA_ARGS__);

#define MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(_collop)             \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COLLOP,     \
                             "trying to run ucc " #_collop);

#define MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(_collop)           \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COLLOP,     \
                             "ucc " #_collop " done successfully");

#define MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(_collop)               \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COLLOP,     \
                             "running fallback " #_collop);

#define MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(_collop)               \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COLLOP,     \
                             "ucc " #_collop " disabled");


#define MPIDI_COMMON_UCC_CHECK_ENABLED(_comm, _collop) do {             \
        if (!MPIDI_common_ucc_priv.ucc_enabled ||                       \
            !MPIDI_common_ucc_priv.ucc_initialized ||                   \
            !comm_ptr->ucc_priv.ucc_initialized) {                      \
            goto disabled;                                              \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_CALL_AND_CHECK(_call) do {                     \
        ucc_status_t __status = (_call);                                \
        if (UCC_OK != __status) {                                       \
            MPIDI_COMMON_UCC_WARNING("calling " #_call " failed: "      \
                                     "%s. Goto fallback.",              \
                                     ucc_status_string(__status));      \
            goto fallback;                                              \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_POST_AND_CHECK(_req) do {                      \
        ucc_status_t __status = ucc_collective_post(_req);              \
        if (UCC_OK != __status) {                                       \
            ucc_collective_finalize(_req);                              \
            MPIDI_COMMON_UCC_WARNING("ucc_collective_post() failed: "   \
                                     "%s. Goto fallback.",              \
                                     ucc_status_string(__status));      \
            goto fallback;                                              \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_REQ_INIT(_req, _coll_args, _comm) do {         \
        if (_req->mpir_req) {                                           \
            MPIR_Assert(0); /* not yet supported */                     \
            _coll_args.mask   |= UCC_COLL_ARGS_FIELD_CB;                \
            _coll_args.cb.cb   = NULL;                                  \
            _coll_args.cb.data = (void*)_req->mpir_req;                 \
        } else {                                                        \
            _coll_args.mask  |= UCC_COLL_ARGS_FIELD_FLAGS;              \
            _coll_args.flags |= UCC_COLL_ARGS_HINT_OPTIMIZE_LATENCY;    \
        }                                                               \
        MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_collective_init(&_coll_args,\
                                            &_req->ucc_req,             \
                                            _comm->ucc_priv.ucc_team)); \
        if (_req->mpir_req) {                                           \
            MPIR_Assert(0); /* not yet supported */                     \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_WAIT_AND_CHECK(_req) do {                      \
        ucc_status_t status;                                            \
        while ((status = ucc_collective_test(_req)) != UCC_OK) {        \
            if (status < 0) {                                           \
                MPIDI_COMMON_UCC_ERROR("ucc_collective_test failed:"    \
                    " %s", ucc_status_string(status));                  \
                MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_collective_finalize(\
                    _req));                                             \
                goto fallback;                                          \
            }                                                           \
            MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_context_progress(\
                                   MPIDI_common_ucc_priv.ucc_context)); \
            MPIDI_COMMON_UCC_PROGRESS_POKE_DEV();                       \
        }                                                               \
        MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_collective_finalize(_req)); \
    } while (0)

#define MPIDI_COMMON_UCC_RELEASE_REQUEST(_req) do {                     \
        MPL_free(_req.sbuf_free);                                       \
        MPL_free(_req.rbuf_free);                                       \
        MPL_free(_req.scounts_tmp);                                     \
        MPL_free(_req.sdispls_tmp);                                     \
        MPL_free(_req.rcounts_tmp);                                     \
        MPL_free(_req.rdispls_tmp);                                     \
    } while (0)

#define MPIDI_COMMON_UCC_WRAPPER_ENTER(_collop_name)                    \
    MPIDI_COMMON_UCC_CHECK_ENABLED(comm_ptr, _collop_name);             \
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(_collop_name);

#define MPIDI_COMMON_UCC_WRAPPER_EXECUTE(_collop_name, ...)             \
    MPIDI_COMMON_UCC_CALL_AND_CHECK(mpidi_ucc_ ## _collop_name ##       \
                                    _init(__VA_ARGS__));                \
    MPIDI_COMMON_UCC_POST_AND_CHECK(req.ucc_req);                       \
    MPIDI_COMMON_UCC_WAIT_AND_CHECK(req.ucc_req);

#define MPIDI_COMMON_UCC_WRAPPER_EXIT(_collop_name, ...)                \
    MPIDI_COMMON_UCC_RELEASE_REQUEST(req);                              \
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(_collop_name);         \
    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;                             \
  fallback:                                                             \
    MPIDI_COMMON_UCC_RELEASE_REQUEST(req);                              \
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(_collop_name);             \
    return MPIDI_COMMON_UCC_RETVAL_FALLBACK;                            \
  disabled:                                                             \
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(_collop_name);             \
    goto fallback;

#endif /*_MPID_UCC_COLLOPS_H_*/

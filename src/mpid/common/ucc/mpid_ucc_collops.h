/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef _MPID_UCC_COLLOPS_H_
#define _MPID_UCC_COLLOPS_H_

#include "mpid_ucc.h"

#define MPIDI_COMMON_UCC_VERBOSE_INPLACE_UNSUPPORTED(_collop)           \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI_INPLACE is not supported (" #_collop  \
                             ")");

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(_collop)             \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI datatype is not supported (" #_collop \
                             ")");

#define MPIDI_COMMON_UCC_VERBOSE_REDUCTION_OP_UNSUPPORTED(_collop)      \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI reduction operation is not supported" \
                             "(" #_collop ")");

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
            MPIDI_COMMON_UCC_WARNING("Calling " #_call " failed: "      \
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

#define MPIDI_COMMON_UCC_REQ_INIT(_coll_req, _req, _coll, _comm) do {   \
        if (_coll_req) {                                                \
            MPIR_Assert(0); /* not yet supported */                     \
            _coll.mask   |= UCC_COLL_ARGS_FIELD_CB;                     \
            _coll.cb.cb   = NULL;                                       \
            _coll.cb.data = (void*)_coll_req;                           \
        } else {                                                        \
            _coll.mask  |= UCC_COLL_ARGS_FIELD_FLAGS;                   \
            _coll.flags |= UCC_COLL_ARGS_HINT_OPTIMIZE_LATENCY;         \
        }                                                               \
        MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_collective_init(\
                              &_coll, _req, _comm->ucc_priv.ucc_team)); \
        if (_coll_req) {                                                \
            /* _coll_req->ucc_req = *(_req); TODO*/                     \
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
            MPID_Progress_test(NULL);                                   \
        }                                                               \
        MPIDI_COMMON_UCC_CALL_AND_CHECK(ucc_collective_finalize(_req)); \
    } while (0)

#endif /*_MPID_UCC_COLLOPS_H_*/

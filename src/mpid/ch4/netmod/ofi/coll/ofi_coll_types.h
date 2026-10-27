/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_COLL_TYPES_H_INCLUDED
#define OFI_COLL_TYPES_H_INCLUDED

#include "mpidimpl.h"

#include <rdma/fi_eq.h>

typedef enum {
    MPIDI_OFI_COLL_RETVAL_ERROR = -1,
    MPIDI_OFI_COLL_RETVAL_SUCCESS = 0,
    MPIDI_OFI_COLL_RETVAL_FALLBACK = 1,
} MPIDI_OFI_coll_retval_t;

typedef struct {
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_av *av;
    struct fid_ep *ep;
    struct fid_cq *cq;
    int verbosity_level;
} MPIDI_OFI_coll_config_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPL_atomic_int_t completed;
    int fi_error;
} MPIDI_OFI_coll_request_t;

typedef struct {
    MPL_atomic_int_t completed;
    int fi_error;
} MPIDI_OFI_coll_join_request_t;

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_coll_datatype_supported(enum fi_datatype datatype)
{
    switch (datatype) {
        case FI_INT8:
        case FI_UINT8:
        case FI_INT16:
        case FI_UINT16:
        case FI_INT32:
        case FI_UINT32:
        case FI_INT64:
        case FI_UINT64:
        case FI_FLOAT:
        case FI_DOUBLE:
            return 1;
        default:
            return 0;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_coll_op_supported(enum fi_op op)
{
    switch (op) {
        case FI_SUM:
        case FI_MAX:
        case FI_MIN:
        case FI_PROD:
        case FI_BAND:
        case FI_BOR:
        case FI_BXOR:
        case FI_LAND:
        case FI_LOR:
        case FI_LXOR:
            return 1;
        default:
            return 0;
    }
}

int MPIDI_OFI_coll_progress(int *made_progress);
int MPIDI_OFI_coll_cq_event(void *context);
int MPIDI_OFI_coll_cq_error(void *context, int fi_error);
int MPIDI_OFI_coll_comm_create_hook(MPIR_Comm * comm);
int MPIDI_OFI_coll_comm_destroy_hook(MPIR_Comm * comm);
MPIDI_OFI_coll_retval_t MPIDI_OFI_coll_bcast(void *buffer, MPI_Aint count,
                                             MPI_Datatype datatype, int root, MPIR_Comm * comm);
MPIDI_OFI_coll_retval_t MPIDI_OFI_coll_allreduce(const void *sendbuf, void *recvbuf,
                                                 MPI_Aint count, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm);

/* Use the offloaded collective if possible; otherwise fall through to the caller's fallback.
 * Expects mpi_errno and an fn_exit/fn_fail pair in the calling function. */
#define MPIDI_OFI_COLL_CHECK_AND_FALLBACK(_collop_fn, _fallback_stmt) \
    do {                                                                \
        MPIDI_OFI_coll_retval_t mpidi_ofi_coll_ret = (_collop_fn);    \
        if (mpidi_ofi_coll_ret == MPIDI_OFI_COLL_RETVAL_SUCCESS) {    \
            mpi_errno = MPI_SUCCESS;                                    \
            goto fn_exit;                                               \
        } else if (mpidi_ofi_coll_ret == MPIDI_OFI_COLL_RETVAL_ERROR) { \
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofi_collop_failed"); \
        }                                                               \
        _fallback_stmt;                                                 \
    } while (0)

typedef struct {
    MPIDI_OFI_coll_config_t config;
    int initialized;
    struct fid_eq *eq;
    struct fid_ep *eq_bound_ep;
} MPIDI_OFI_coll_global_t;

extern MPIDI_OFI_coll_global_t MPIDI_OFI_coll_global;

int MPIDI_OFI_coll_init(const MPIDI_OFI_coll_config_t * config);
int MPIDI_OFI_coll_pre_enable_bind(struct fid_ep *ep);
int MPIDI_OFI_coll_finalize(void);
int MPIDI_OFI_coll_query(MPIR_Comm * comm, enum fi_collective_op collective,
                         enum fi_datatype datatype, enum fi_op op);
#define MPIDI_OFI_COLL_EP (MPIDI_OFI_coll_global.config.ep)
int MPIDI_OFI_coll_wait(MPIDI_OFI_coll_request_t * request);

#endif /* OFI_COLL_TYPES_H_INCLUDED */

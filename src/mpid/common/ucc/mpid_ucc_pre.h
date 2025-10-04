/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef _MPID_UCC_PRE_H_
#define _MPID_UCC_PRE_H_

#include <ucc/api/ucc.h>

int MPIDI_common_ucc_enable(int verbose_level, const char *verbose_level_str, int debug_flag);
int MPIDI_common_ucc_progress(int *made_progress);
int MPIDI_common_ucc_comm_create_hook(MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_comm_destroy_hook(MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_barrier(MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_bcast(void *buf, MPI_Aint count, MPI_Datatype dtype, int root,
                           MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_gather(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                            MPI_Aint rcount, MPI_Datatype rdtype, int root, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_gatherv(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                             const MPI_Aint rcounts[], const MPI_Aint rdispls[],
                             MPI_Datatype rdtype, int root, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_scatter(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                             MPI_Aint rcount, MPI_Datatype rdtype, int root, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_scatterv(const void *sbuf, const MPI_Aint scounts[], const MPI_Aint rdispls[],
                              MPI_Datatype sdtype, void *rbuf, MPI_Aint rcount, MPI_Datatype rdtype,
                              int root, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_reduce(const void *sbuf, void *rbuf, MPI_Aint count, MPI_Datatype dtype,
                            MPI_Op op, int root, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_reduce_scatter(const void *sbuf, void *rbuf, const MPI_Aint rcounts[],
                                    MPI_Datatype dtype, MPI_Op op, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_reduce_scatter_block(const void *sbuf, void *rbuf, MPI_Aint rcount,
                                          MPI_Datatype dtype, MPI_Op op, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_allgather(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                               MPI_Aint rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_allgatherv(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                                const MPI_Aint rcounts[], const MPI_Aint rdispls[],
                                MPI_Datatype rdtype, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_allreduce(const void *sbuf, void *rbuf, MPI_Aint count, MPI_Datatype dtype,
                               MPI_Op op, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_alltoall(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype, void *rbuf,
                              MPI_Aint rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr);
int MPIDI_common_ucc_alltoallv(const void *sbuf, const MPI_Aint scounts[], const MPI_Aint sdispls[],
                               MPI_Datatype sdtype, void *rbuf, const MPI_Aint rcounts[],
                               const MPI_Aint rdispls[], MPI_Datatype rdtype, MPIR_Comm * comm_ptr);

#define MPIDI_DEV_COMM_DECL_UCC MPIDI_common_ucc_comm_t ucc_priv;

typedef struct {
    int ucc_enabled;            /* flag indicating whether UCC should be initialized for this communicator (for future use) */
    int ucc_initialized;        /* flag indicating whether UCC has been initialized successfully for this communicator */
    ucc_team_h ucc_team;        /* handle for the UCC team created for this communicator */
} MPIDI_common_ucc_comm_t;

typedef enum {
    MPIDI_COMMON_UCC_RETVAL_ERROR = -1,
    MPIDI_COMMON_UCC_RETVAL_SUCCESS = 0,
    MPIDI_COMMON_UCC_RETVAL_FALLBACK = 1,
} MPIDI_common_ucc_error_t;

/* Macro for ADI3 devices to encapsulate calls to the UCC wrappers for the
 * collective functions. Use is optional, but improves consistency due to
 * the differing return semantics of the UCC wrappers in comparison to the
 * usual error handling scheme for covering also the fallback case.
 * - Caller defines the labels `fn_exit`, `fn_fail`, and `int mpi_errno`.
 * - On success: `_success_stmt` is executed, then jump to `fn_exit`.
 * - On fatal error: `_error_stmt` is executed, error "ucc_collop_failed"
 *   is pushed, then jump to `fn_fail`.
 * - Fallback case: `_fallback_stmt` is executed and macro falls through
 *   for alternative handling of the collective.
 * - Otherwise: Same as fallback case, but w/o executing `_fallback_stmt`.
 * The statement parameters can be used for counting the different cases,
 * for example. If not needed, `(void)0` can be passed instead.
 */
#define MPIDI_COMMOM_UCC_CHECK_AND_FALLBACK(_collop_fn,                 \
                                            _success_stmt,              \
                                            _error_stmt,                \
                                            _fallback_stmt)             \
    do {                                                                \
        MPIDI_common_ucc_error_t mpidi_ucc_err = _collop_fn;            \
        switch (mpidi_ucc_err) {                                        \
        case MPIDI_COMMON_UCC_RETVAL_SUCCESS:                           \
            _success_stmt;                                              \
            mpi_errno = MPI_SUCCESS;                                    \
            goto fn_exit;                                               \
        case MPIDI_COMMON_UCC_RETVAL_ERROR:                             \
            _error_stmt;                                                \
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER,               \
                                "**ucc_collop_failed");                 \
        case MPIDI_COMMON_UCC_RETVAL_FALLBACK:                          \
            _fallback_stmt;                                             \
            break;                                                      \
        }                                                               \
    } while (0)

#endif /*_MPID_UCC_PRE_H_*/

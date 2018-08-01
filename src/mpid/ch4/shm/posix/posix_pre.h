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

#ifndef POSIX_PRE_H_INCLUDED
#define POSIX_PRE_H_INCLUDED

#include <mpi.h>

#define MPIDI_POSIX_MAX_AM_HDR_SIZE     (32)

#define MPIDI_POSIX_AM_HANDLER_ID_BITS  (8)
#define MPIDI_POSIX_AM_HDR_SZ_BITS      (8)
#define MPIDI_POSIX_AM_DATA_SZ_BITS     (48)

#define MPIDI_POSIX_AM_MSG_HEADER_SIZE  (sizeof(MPIDI_POSIX_am_header_t))
#define MPIDI_POSIX_MAX_IOV_NUM         (2)

typedef enum {
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_INITIALIZED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_REGISTERED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_FINALIZED
} MPIDI_POSIX_EAGER_recv_posted_hook_state_t;

struct MPIR_Request;
struct MPIR_Segment;

typedef struct {
    int dummy;
} MPIDI_POSIX_comm_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_dt_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_op_t;

typedef struct {
    MPIDI_POSIX_EAGER_recv_posted_hook_state_t eager_recv_posted_hook_state;
    int eager_recv_posted_hook_grank;
} MPIDI_POSIX_request_t;

typedef struct MPIDI_POSIX_am_header_t {
    uint64_t handler_id:MPIDI_POSIX_AM_HANDLER_ID_BITS;
    uint64_t am_hdr_sz:MPIDI_POSIX_AM_HDR_SZ_BITS;
    uint64_t data_sz:MPIDI_POSIX_AM_DATA_SZ_BITS;
#ifdef POSIX_AM_DEBUG
    int seq_num;
#endif                          /* POSIX_AM_DEBUG */
} MPIDI_POSIX_am_header_t;

typedef struct {
    void *pack_buffer;
    void *rreq_ptr;
    void *am_hdr;

    uint16_t am_hdr_sz;
    uint8_t pad[6];

    MPIDI_POSIX_am_header_t *msg_hdr;
    MPIDI_POSIX_am_header_t msg_hdr_buf;

    uint8_t am_hdr_buf[MPIDI_POSIX_MAX_AM_HDR_SIZE];

    int (*cmpl_handler_fn) (MPIR_Request * req);

    int handler_id;
    uint32_t dst_grank;

    struct iovec *iov_ptr;
    struct iovec iov[MPIDI_POSIX_MAX_IOV_NUM];
    size_t iov_num;
    size_t iov_num_total;

    int is_contig;

    size_t in_total_data_sz;

} MPIDI_POSIX_am_request_header_t;

struct MPIR_Segment;
typedef struct MPIDI_POSIX_am_request_t {
    int dest;
    int rank;
    int tag;
    int context_id;
    char *user_buf;
    size_t data_sz;
    int type;
    int user_count;
    MPI_Datatype datatype;
    struct MPIR_Segment *segment_ptr;
    size_t segment_first;
    size_t segment_size;

    MPIDI_POSIX_am_request_header_t *req_hdr;
#ifdef POSIX_AM_REQUEST_INLINE
    MPIDI_POSIX_am_request_header_t req_hdr_buffer;
#endif                          /* POSIX_AM_REQUEST_INLINE */

    /* Structure used with POSIX postponed_queue */
    uint64_t request;           /* Store address of MPIR_Request* sreq */
    struct MPIDI_POSIX_am_request_t *prev, *next;
} MPIDI_POSIX_am_request_t;

#define MPIDI_POSIX_EAGER_RECV_INITIALIZE_HOOK(request)\
do { \
    (request)->dev.ch4.shm.posix.eager_recv_posted_hook_state = MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_INITIALIZED; \
} while (0)

#define MPIDI_POSIX_EAGER_RECV_POSTED_HOOK(request,rank,communicator)\
do { \
    if ((request) && ((request)->dev.ch4.shm.posix.eager_recv_posted_hook_state == MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_INITIALIZED)) { \
        int grank_ = ((rank) >= 0) ? MPIDI_CH4U_rank_to_lpid((rank), (communicator)) : (rank); \
        (request)->dev.ch4.shm.posix.eager_recv_posted_hook_state = MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_REGISTERED; \
        (request)->dev.ch4.shm.posix.eager_recv_posted_hook_grank = grank_; \
        MPIDI_POSIX_eager_recv_posted_hook(grank_); \
    } \
} while (0)

#define MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(request)\
do { \
    if ((request)->dev.ch4.shm.posix.eager_recv_posted_hook_state == MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_REGISTERED) { \
        MPIDI_POSIX_eager_recv_completed_hook((request)->dev.ch4.shm.posix.eager_recv_posted_hook_grank); \
    } \
    (request)->dev.ch4.shm.posix.eager_recv_posted_hook_state = MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_FINALIZED; \
} while (0)

typedef struct {
    MPL_proc_mutex_t *shm_mutex_ptr;    /* interprocess mutex for shm atomic RMA */
    MPL_shm_hnd_t shm_mutex_segment_handle;
} MPIDI_POSIX_win_t;

/*
 * Wrapper routines of process mutex for shared memory RMA.
 * Called by both POSIX RMA and fallback AM handlers through CS hooks.
 */
#define MPIDI_POSIX_RMA_MUTEX_INIT(mutex_ptr) do {                                  \
    int pt_err = MPL_PROC_MUTEX_SUCCESS;                                            \
    MPL_proc_mutex_create(mutex_ptr, &pt_err);                                      \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_PROC_MUTEX_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_create");            \
} while (0);

#define MPIDI_POSIX_RMA_MUTEX_DESTROY(mutex_ptr)  do {                              \
    int pt_err = MPL_PROC_MUTEX_SUCCESS;                                            \
    MPL_proc_mutex_destroy(mutex_ptr, &pt_err);                                     \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_PROC_MUTEX_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_destroy");           \
} while (0);

#define MPIDI_POSIX_RMA_MUTEX_LOCK(mutex_ptr) do {                                  \
    int pt_err = MPL_PROC_MUTEX_SUCCESS;                                            \
    MPL_proc_mutex_lock(mutex_ptr, &pt_err);                                        \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_PROC_MUTEX_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_lock");              \
} while (0)

#define MPIDI_POSIX_RMA_MUTEX_UNLOCK(mutex_ptr) do {                                \
        int pt_err = MPL_PROC_MUTEX_SUCCESS;                                        \
        MPL_proc_mutex_unlock(mutex_ptr, &pt_err);                                  \
        MPIR_ERR_CHKANDJUMP1(pt_err != MPL_PROC_MUTEX_SUCCESS, mpi_errno,           \
                             MPI_ERR_OTHER, "**windows_mutex",                      \
                             "**windows_mutex %s", "MPL_proc_mutex_unlock");        \
} while (0)

#include "posix_coll_params.h"
#include "posix_coll_containers.h"
#endif /* POSIX_PRE_H_INCLUDED */

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PRE_H_INCLUDED
#define POSIX_PRE_H_INCLUDED

#include <mpi.h>
#include "release_gather_types.h"

#define MPIDI_POSIX_MAX_AM_HDR_SIZE     800     /* constrained by MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE */
#define MPIDI_POSIX_AM_MSG_HEADER_SIZE  (sizeof(MPIDI_POSIX_am_header_t))
#define MPIDI_POSIX_MAX_IOV_NUM         (3)     /* am_hdr, [padding], payload */

typedef enum {
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_INITIALIZED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_REGISTERED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_FINALIZED
} MPIDI_POSIX_EAGER_recv_posted_hook_state_t;

typedef enum {
    MPIDI_POSIX_AM_TYPE__HDR,
    MPIDI_POSIX_AM_TYPE__SHORT,
    MPIDI_POSIX_AM_TYPE__PIPELINE
} MPIDI_POSIX_am_type_t;

struct MPIR_Request;

typedef struct {
    void *csel_root;
} MPIDI_POSIX_Global_t;

extern char MPIDI_POSIX_coll_generic_json[];

/* These structs are populated with dummy variables because empty structs are not supported in all
 * compilers: https://stackoverflow.com/a/755339/491687 */
typedef struct {
    void *csel_comm;
    MPIDI_POSIX_release_gather_comm_t release_gather, nb_release_gather;
    int nb_bcast_seq_no;        /* Seq number of the release-gather based nonblocking bcast call */
    int nb_reduce_seq_no;       /* Seq number of the release-gather based nonblocking reduce call */
} MPIDI_POSIX_comm_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_dt_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_op_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_request_t;

typedef struct MPIDI_POSIX_am_header {
    int8_t am_type;
    int8_t handler_id;
    int16_t am_hdr_sz;
    int32_t unused;
} MPIDI_POSIX_am_header_t;

typedef struct MPIDI_POSIX_am_request_header {
    void *pack_buffer;
    void *rreq_ptr;
    void *am_hdr;

    int8_t src_vsi;
    int8_t dst_vsi;
    uint16_t am_hdr_sz;
    uint8_t pad[4];

    MPIDI_POSIX_am_header_t *msg_hdr;
    MPIDI_POSIX_am_header_t msg_hdr_buf;

    uint8_t am_hdr_buf[MPIDI_POSIX_MAX_AM_HDR_SIZE];

    int dst_grank;

    size_t in_total_data_sz;

    /* For postponed operation */
    MPI_Datatype datatype;
    const void *buf;
    MPI_Aint count;

    /* Structure used with POSIX postponed_queue */
    MPIR_Request *request;      /* Store address of MPIR_Request* sreq */
    struct MPIDI_POSIX_am_request_header *prev, *next;
} MPIDI_POSIX_am_request_header_t;

typedef struct MPIDI_POSIX_am_request {
    int dest;
    int rank;
    int tag;
    int context_id;
    char *user_buf;
    size_t data_sz;
    int type;
    int user_count;
    MPI_Datatype datatype;
    size_t segment_first;
    size_t segment_size;

    int eager_recv_posted_hook_grank;

    MPIDI_POSIX_am_request_header_t *req_hdr;

#ifdef POSIX_AM_REQUEST_INLINE
    MPIDI_POSIX_am_request_header_t req_hdr_buffer;
#endif                          /* POSIX_AM_REQUEST_INLINE */
} MPIDI_POSIX_am_request_t;

#define MPIDI_POSIX_EAGER_RECV_INITIALIZE_HOOK(request)\
do { \
} while (0)

#define MPIDI_POSIX_EAGER_RECV_POSTED_HOOK(request,rank,communicator)\
do { \
    int grank_ = ((rank) >= 0) ? MPIDIU_rank_to_lpid((rank), (communicator)) : (rank); \
    (request)->dev.ch4.am.shm_am.posix.eager_recv_posted_hook_grank = grank_; \
    MPIDI_POSIX_eager_recv_posted_hook(grank_); \
} while (0)

#define MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(request)\
do { \
    MPIDI_POSIX_eager_recv_completed_hook((request)->dev.ch4.am.shm_am.posix.eager_recv_posted_hook_grank); \
} while (0)

typedef struct MPIDI_POSIX_rma_req {
    MPIR_Typerep_req typerep_req;
    struct MPIDI_POSIX_rma_req *next;
} MPIDI_POSIX_rma_req_t;

typedef struct {
    MPL_proc_mutex_t *shm_mutex_ptr;    /* interprocess mutex for shm atomic RMA */

    /* Linked list to keep track of outstanding RMA issued via shm.
     * Host-only copy is always blocking, thus this list should contain only
     * GPU-involved operations. */
    MPIDI_POSIX_rma_req_t *outstanding_reqs_head;
    MPIDI_POSIX_rma_req_t *outstanding_reqs_tail;
} MPIDI_POSIX_win_t;

/*
 * Wrapper routines of process mutex for shared memory RMA.
 * Called by both POSIX RMA and fallback AM handlers through CS hooks.
 */
#define MPIDI_POSIX_RMA_MUTEX_INIT(mutex_ptr) do {                                  \
    int pt_err = MPL_SUCCESS;                                            \
    MPL_proc_mutex_create(mutex_ptr, &pt_err);                                      \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_create");            \
} while (0);

#define MPIDI_POSIX_RMA_MUTEX_DESTROY(mutex_ptr)  do {                              \
    int pt_err = MPL_SUCCESS;                                            \
    MPL_proc_mutex_destroy(mutex_ptr, &pt_err);                                     \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_destroy");           \
} while (0);

#define MPIDI_POSIX_RMA_MUTEX_LOCK(mutex_ptr) do {                                  \
    int pt_err = MPL_SUCCESS;                                            \
    MPL_proc_mutex_lock(mutex_ptr, &pt_err);                                        \
    MPIR_ERR_CHKANDJUMP1(pt_err != MPL_SUCCESS, mpi_errno,               \
                         MPI_ERR_OTHER, "**windows_mutex",                          \
                         "**windows_mutex %s", "MPL_proc_mutex_lock");              \
} while (0)

#define MPIDI_POSIX_RMA_MUTEX_UNLOCK(mutex_ptr) do {                                \
        int pt_err = MPL_SUCCESS;                                        \
        MPL_proc_mutex_unlock(mutex_ptr, &pt_err);                                  \
        MPIR_ERR_CHKANDJUMP1(pt_err != MPL_SUCCESS, mpi_errno,           \
                             MPI_ERR_OTHER, "**windows_mutex",                      \
                             "**windows_mutex %s", "MPL_proc_mutex_unlock");        \
} while (0)

#endif /* POSIX_PRE_H_INCLUDED */

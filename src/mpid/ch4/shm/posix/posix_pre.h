/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PRE_H_INCLUDED
#define POSIX_PRE_H_INCLUDED

#include <mpi.h>
#include "release_gather_types.h"

#define MPIDI_POSIX_MAX_AM_HDR_SIZE     ((1 << MPIDI_POSIX_AM_HDR_SZ_BITS) - 1)

#define MPIDI_POSIX_AM_KIND_BITS  (1)   /* 0 or 1 */
#define MPIDI_POSIX_AM_HANDLER_ID_BITS  (7)     /* up to 64 */
#define MPIDI_POSIX_AM_HDR_SZ_BITS      (8)
#define MPIDI_POSIX_AM_DATA_SZ_BITS     (48)

#define MPIDI_POSIX_AM_MSG_HEADER_SIZE  (sizeof(MPIDI_POSIX_am_header_t))
#define MPIDI_POSIX_MAX_IOV_NUM         (3)     /* am_hdr, [padding], payload */

typedef enum {
    MPIDI_POSIX_AM_HDR_SHM = 0, /* SHM internal AM header */
    MPIDI_POSIX_AM_HDR_CH4 = 1  /* CH4 level AM header */
} MPIDI_POSIX_am_header_kind_t;

typedef enum {
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_INITIALIZED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_REGISTERED,
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK_STATE_FINALIZED
} MPIDI_POSIX_EAGER_recv_posted_hook_state_t;

struct MPIR_Request;

typedef struct {
    void *csel_root;
} MPIDI_POSIX_Global_t;

extern char MPIDI_POSIX_coll_generic_json[];

/* These structs are populated with dummy variables because empty structs are not supported in all
 * compilers: https://stackoverflow.com/a/755339/491687 */
typedef struct {
    MPIDI_POSIX_release_gather_comm_t release_gather;
    void *csel_comm;
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
    MPIDI_POSIX_am_header_kind_t kind:MPIDI_POSIX_AM_KIND_BITS;
    uint32_t handler_id:MPIDI_POSIX_AM_HANDLER_ID_BITS;
    uint64_t am_hdr_sz:MPIDI_POSIX_AM_HDR_SZ_BITS;
    uint64_t data_sz:MPIDI_POSIX_AM_DATA_SZ_BITS;
} MPIDI_POSIX_am_header_t;

typedef struct MPIDI_POSIX_am_request_header {
    void *pack_buffer;
    void *rreq_ptr;
    void *am_hdr;

    uint16_t am_hdr_sz;
    uint8_t pad[6];

    MPIDI_POSIX_am_header_t *msg_hdr;
    MPIDI_POSIX_am_header_t msg_hdr_buf;

    uint8_t am_hdr_buf[MPIDI_POSIX_MAX_AM_HDR_SIZE];

    int handler_id;
    int dst_grank;

    struct iovec *iov_ptr;
    struct iovec iov[MPIDI_POSIX_MAX_IOV_NUM];
    size_t iov_num;
    size_t iov_num_total;

    int is_contig;

    size_t in_total_data_sz;

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

typedef struct {
    MPL_proc_mutex_t *shm_mutex_ptr;    /* interprocess mutex for shm atomic RMA */
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

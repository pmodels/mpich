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

#ifndef SHM_SHMAM_PRE_H_INCLUDED
#define SHM_SHMAM_PRE_H_INCLUDED

#include <mpi.h>

#define MPIDI_SHM_MAX_AM_HDR_SIZE     (32)

#define MPIDI_SHM_AM_HANDLER_ID_BITS  (8)
#define MPIDI_SHM_AM_HDR_SZ_BITS      (8)
#define MPIDI_SHM_AM_DATA_SZ_BITS     (48)

#define MPIDI_SHM_AM_MSG_HEADER_SIZE  (sizeof(MPIDI_SHM_am_header_t))
#define MPIDI_SHM_MAX_IOV_NUM         (8)

struct MPIR_Request;

typedef struct {
    int dummy;
} MPIDI_SHMAM_comm_t;

typedef struct {
    int dummy;
} MPIDI_SHMAM_dt_t;

typedef struct {
    int dummy;
} MPIDI_SHMAM_op_t;

typedef struct {
    int dummy;
} MPIDI_SHMAM_request_t;

typedef struct MPIDI_SHM_am_header_t {
    uint64_t handler_id:MPIDI_SHM_AM_HANDLER_ID_BITS;
    uint64_t am_hdr_sz:MPIDI_SHM_AM_HDR_SZ_BITS;
    uint64_t data_sz:MPIDI_SHM_AM_DATA_SZ_BITS;
#ifdef SHM_AM_DEBUG
    int seq_num;
#endif                          /* SHM_AM_DEBUG */
} MPIDI_SHM_am_header_t;

typedef struct {
    void *pack_buffer;
    void *rreq_ptr;
    void *am_hdr;

    uint16_t am_hdr_sz;
    uint8_t pad[6];

    MPIDI_SHM_am_header_t *msg_hdr;
    MPIDI_SHM_am_header_t msg_hdr_buf;

    uint8_t am_hdr_buf[MPIDI_SHM_MAX_AM_HDR_SIZE];

    int (*cmpl_handler_fn) (MPIR_Request * req);

    int handler_id;
    uint32_t dst_grank;

    struct iovec *iov_ptr;
    struct iovec iov[MPIDI_SHM_MAX_IOV_NUM];
    size_t iov_num;

    int is_contig;

    size_t in_total_data_sz;

} MPIDI_SHM_am_request_header_t;

struct MPIDU_Segment;

typedef struct {
    struct MPIR_Request *next;
    struct MPIR_Request *pending;
    int dest;
    int rank;
    int tag;
    int context_id;
    char *user_buf;
    size_t data_sz;
    int type;
    int user_count;
    MPI_Datatype datatype;
    struct MPIDU_Segment *segment_ptr;
    size_t segment_first;
    size_t segment_size;

    MPIDI_SHM_am_request_header_t *req_hdr;
#ifdef SHM_SHMAM_AM_REQUEST_INLINE
    MPIDI_SHM_am_request_header_t req_hdr_buffer;
#endif                          /* SHM_SHMAM_AM_REQUEST_INLINE */
} MPIDI_SHMAM_am_request_t;

#endif /* SHM_SHMAM_PRE_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_LMT_RNDV_PRE_H_INCLUDED
#define POSIX_LMT_RNDV_PRE_H_INCLUDED

#include "mpiimpl.h"

/* This is a somewhat arbitray limit, but it appears that in mpl_shm_mmap.c, this will never be
 * longer than 30 characters so we'll use 48 just to be safe. */
#define MPIDI_MAX_HANDLE_LEN 48

/* Struct to track data needed for the RTS message in the RNDV protocol. */
typedef struct {
    uint64_t sreq_ptr;          /* A "serialized" version of the pointer to the send request. Needed
                                 * so the receiver can help the sender match the request when the
                                 * CTS message comes back. */
    uint64_t data_sz;           /* Data size in bytes */

    MPI_Aint rank;              /* The rank of the sending process. */
    int tag;                    /* The tag used when sending the message. */
    MPIR_Context_id_t context_id;       /* The context id used to send the message (includes the offset) */
    int error_bits;             /* The error bits used during collective communication. */
    int handler_id;             /* The handler type of the message. Used to tell the sender when an
                                 * MPI_SSEND is done. */
} MPIDI_SHM_lmt_rndv_long_msg_t;

typedef struct {
    uint64_t sreq_ptr;          /* A "serialized" version of the pointer to the send request. Needed
                                 * so the receiver can help the sender match the request when the
                                 * CTS message comes back. */
    uint64_t rreq_ptr;          /* A "serialized" version of the receive request. */
    int handler_id;             /* The handler type of the message. Used to tell the sender when an
                                 * MPI_SSEND is done. */

    int copy_buf_serialized_handle_len; /* The length of the serialized handle. */
    char copy_buf_serialized_handle[MPIDI_MAX_HANDLE_LEN];      /* Serialized handle for the shared
                                                                 * memory region */
} MPIDI_SHM_lmt_rndv_long_ack_t;

#define MPIDI_SHM_LMT_CTRL_MESSAGE_TYPES \
    MPIDI_SHM_lmt_rndv_long_msg_t lmt_rndv_long_msg; \
    MPIDI_SHM_lmt_rndv_long_ack_t lmt_rndv_long_ack;

/* Goes in the MPIDIG_req_t struct to describe LMT-related  requests. */
#define MPIDI_SHM_LMT_AM_DECL size_t lmt_data_sz;      /* The size of the data in the message */ \
                              int lmt_buf_num;         /* The number of the buffer being used */ \
                                                       /* at the moment. */                      \
                              intptr_t lmt_msg_offset; /* The offset of the current point */     \
                                                       /* that's been transfered into the */     \
                                                       /* original message buffer. */            \
                              intptr_t lmt_leftover;   /* The amount of data "left over" from the */
                                                       /* previous memory segment that needs to */
                                                       /* be transfered before the next one. */



#endif /* POSIX_LMT_RNDV_PRE_H_INCLUDED */

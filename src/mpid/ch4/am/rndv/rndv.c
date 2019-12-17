/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "rndv.h"

typedef struct am_rndv_rts_hdr {
    int src_rank;
    int tag;
    size_t data_sz;
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
    MPIR_Context_id_t context_id;
    bool is_local;
} am_rndv_rts_hdr_t;

typedef struct am_rndv_cts_hdr {
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
} am_rndv_cts_hdr;

typedef struct am_rndv_msg_hdr {
    MPIR_Request *rreq_ptr;
} am_rndv_msg_hdr_t;

typedef struct am_rndv_hdr {

    union {
        am_rndv_rts_hdr_t rts_hdr;
        am_rndv_cts_hdr_t cts_hdr;
        am_rndv_msg_hdr_t msg_hdr;
    };

} am_rndv_hdr_t;

static int am_rndv_rts_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload);
static int am_rndv_cts_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload);
static int am_rndv_send_src_cb(MPIR_Request * sreq);
static int am_rndv_send_dst_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload);

static int rts_hnd;
static int cts_hnd;
static int src_hnd;
static int dst_hnd;

/* MPIDI_am_rndv_init - register callback functions */
int MPIDI_am_rndv_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

/* am_rndv_rts_cb - ready to send (MPIDI_AM_RNDV_RTS) message handling
 *
 * The handler function performs the following steps:
 *
 * 1. Inspect header
 *   - get transports supported by sender (pt2pt, pt2pt-am, one-sided)
 *   - get src buffer (and remote key)
 *   - [...]
 * 2. Prepare hdr for clear to send (CTS) message
 *   - add handler id (...)
 *   - add selected transport
 *   - add dst buffer (and remote key)
 *   - [...]
 * 3. Depending on selected transport
 *   - for pt2pt/-am post recv operation
 *   - for one-sided get (contig), do get
 *   - for one-sided put (contig), do nothing
 * 4. Send clear to send (CTS) message
 *   - for pt2pt/-am, this tells sender to send data
 *   - for one-sided put (contig), tell sender do put
 *   - for one-sided get (contig), tell sender src buffer can be freed as
 *     get has already happened in Step 3
 */
static int am_rndv_rts_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

/* am_rndv_cts_cb - clear to send (MPIDI_AM_RNDV_CTS) message handling
 *
 * This handler function performs the following steps:
 *
 * 1. Inspect the header
 *   - get transport selected by receiver (pt2pt, pt2pt-am, one-sided)
 *   - get dst buffer (and remote key)
 * 2. Prepare send message
 *   - for pt2pt/-am post send operation (do packing if needed)
 *   - for one-sided put (contig), do put
 *   - for one-sided get (contig), call sender callback to cleanup buffer as get
 *     has already completed at this point
 */
static int am_rndv_cts_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

/* am_rndv_send_src_cb - send message (MPIDI_AM_SEND_MSG) handling in sender
 *
 * This handler function performs the following steps:
 *
 * 1. Clean up resources
 *   - request handler
 *   - packing buffer
 *   - [...]
 */
static int am_rndv_send_src_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

/* am_rndv_send_dst_cb - send message (MPIDI_AM_SEND_MSG) handling in receiver
 *
 * This handler function performs the following steps:
 *
 * 1. Receive message
 * 2. Inspect header
 *   - do unpacking if needed
 * 2. Clean up resources
 *   - request handle
 *   - tmp buffer
 *   - [...]
 */
static int am_rndv_send_dst_cb(am_rndv_hdr_t * header, size_t payload_sz, void *payload)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

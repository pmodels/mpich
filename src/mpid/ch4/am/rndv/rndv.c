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

    /* register callback functions */
    rts_hnd = MPIDI_am_register_handler(&am_rndv_rts_cb);
    cts_hnd = MPIDI_am_register_handler(&am_rndv_cts_cb);
    dst_hnd = MPIDI_am_register_handler(&am_rndv_send_dst_cb);
    src_hnd = MPIDI_am_register_completion(&am_rndv_send_src_cb);

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
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm = NULL;
    am_rndv_cts_hdr_t cts_hdr;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIR_Request *anysource_partner = NULL;
#endif

    am_rndv_rts_hdr_t *rts_hdr = (am_rndv_rts_hdr_t *) header;
    root_comm = MPIDIG_context_id_to_comm(rts_hdr->context_id);

  root_comm_retry:
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
#ifdef MPIDI_CH4_DIRECT_NETMOD
        rreq = MPIDIG_dequeue_posted(rts_hdr->src, rts_hdr->tag, rts_hdr->context_id,
                                     &MPIDIG_COMM(root_comm, posted_list));
#else /* MPIDI_CH4_DIRECT_NETMOD */
        int continue_matching = 1;
        while (continue_matching) {
            rreq =
                MPIDIG_dequeue_posted(rts_hdr->src_rank, rts_hdr->tag, rts_hdr->context_id,
                                      &MPIDIG_COMM(root_comm, posted_list));

            if (rreq && MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq)) {
                anysource_partner = MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq);

                mpi_errno = MPIDI_anysource_matched(anysource_partner,
                                                    MPIDI_REQUEST(rreq, is_local) ?
                                                    MPIDI_SHM : MPIDI_NETMOD, &continue_matching);

                MPIR_ERR_CHECK(mpi_errno);

                MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
                MPIDI_REQUEST_ANYSOURCE_PARTNER(anysource_partner) = NULL;
            }

            break;
        }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        MPIDIG_REQUEST(rreq, buffer) = NULL;
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = rts_hdr->data_sz;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = rts_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = rts_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = rts_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = rts_hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_REQUEST(rreq, is_local) = rts_hdr->is_local;
#endif
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIR_Comm *root_comm_again;
            /* This branch means that last time we checked, there was no communicator
             * associated with the arriving message.
             * In a multi-threaded environment, it is possible that the communicator
             * has been created since we checked root_comm last time.
             * If that is the case, the new message must be put into a queue in
             * the new communicator. Otherwise that message will be lost forever.
             * Here that strategy is to query root_comm again, and if found,
             * simply re-execute the per-communicator enqueue logic above. */
            root_comm_again = MPIDIG_context_id_to_comm(rts_hdr->context_id);
            if (unlikely(root_comm_again != NULL)) {
                MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
                MPL_free(MPIDIG_REQUEST(rreq, buffer));
                MPIR_Request_free(rreq);
                MPID_Request_complete(rreq);
                rreq = NULL;
                root_comm = root_comm_again;
                goto root_comm_retry;
            }
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    } else {
        /* Matching receive was posted, tell the netmod */
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = rts_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = rts_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = rts_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = rts_hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;
        /* Mark `match_req` as NULL so that we know nothing else to complete when
         * `unexp_req` finally completes. (See MPIDI_recv_target_cmpl_cb) */
        MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;

        /* prepare clear to send header */
        cts_hdr.sreq_ptr = rts_hdr->sreq_ptr;
        cts_hdr.rreq_ptr = rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_REQUEST(rreq, is_local)) {
            mpi_errno =
                MPIDI_SHM_am_send_header(rts_hdr->src_rank, cts_hnd, 0, sizeof(am_rndv_cts_hdr_t),
                                         &cts_hdr, rreq);
        } else
#endif
        {
            mpi_errno =
                MPIDI_NM_am_send_header(rts_hdr->src_rank, cts_hnd, 0, sizeof(am_rndv_cts_hdr_t),
                                        &cts_hdr, rreq);
        }

        MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (unlikely(anysource_partner)) {
            anysource_partner->status = rreq->status;
        }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
    int is_contig;
    int is_local;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr = NULL;
    am_rndv_msg_hdr_t msg_hdr;

    am_rndv_cts_hdr_t *cts_hdr = (am_rndv_cts_hdr_t *) header;
    MPIR_Request *sreq = (MPIR_Request *) cts_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    /* prepare message header */
    msg_hdr.rreq_ptr = cts_hdr->rreq_ptr;

    is_local = MPIDI_REQUEST(sreq, is_local);
    MPIDI_Datatype_get_info(MPIDIG_REQUEST(sreq, req->rndv_req).count,
                            MPIDIG_REQUEST(sreq, req->rndv_req).datatype,
                            is_contig, data_sz, dt_ptr, dt_true_lb);

    if (is_contig) {
        if (is_local) {
            mpi_errno =
                MPIDI_SHM_am_send_contig(MPIDIG_REQUEST(sreq, rank), dst_hnd, src_hnd,
                                         sizeof(am_rndv_msg_hdr_t), &msg_hdr, data_sz,
                                         MPIDIG_REQUEST(sreq, req->rndv_req).src_buf, sreq);
        } else {
            mpi_errno =
                MPIDI_NM_am_send_contig(MPIDIG_REQUEST(sreq, rank), dst_hnd, src_hnd,
                                        sizeof(am_rndv_msg_hdr_t), &msg_hdr, data_sz,
                                        MPIDIG_REQUEST(sreq, req->rndv_req).src_buf, sreq);
        }
    } else {
        if (is_local) {
            mpi_errno =
                MPIDI_SHM_am_send_noncontig(MPIDIG_REQUEST(sreq, rank),
                                            MPIDIG_REQUEST(sreq, req->rndv_req).src_buf,
                                            MPIDIG_REQUEST(sreq, req->rndv_req).count,
                                            MPIDIG_REQUEST(sreq, req->rndv_req).datatype, dst_hnd,
                                            src_hnd, sizeof(am_rndv_msg_hdr_t), &msg_hdr, sreq);
        } else {
            mpi_errno = MPIDI_NM_am_send_noncontig(MPIDIG_REQUEST(sreq, rank),
                                                   MPIDIG_REQUEST(sreq, req->rndv_req).src_buf,
                                                   MPIDIG_REQUEST(sreq, req->rndv_req).count,
                                                   MPIDIG_REQUEST(sreq, req->rndv_req).datatype,
                                                   dst_hnd, src_hnd, sizeof(am_rndv_msg_hdr_t),
                                                   &msg_hdr, sreq);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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

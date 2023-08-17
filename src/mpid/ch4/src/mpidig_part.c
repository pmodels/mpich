/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_part_utils.h"

static int part_req_create(void *buf, int partitions, MPI_Aint count,
                           MPI_Datatype datatype, int rank, int tag,
                           MPIR_Comm * comm, MPIR_Request_kind_t kind, MPIR_Request ** req_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = NULL;

    MPIR_FUNC_ENTER;

    /* Set refcnt=1 for user-defined partitioned pattern; decrease at request_free. */
    MPIDI_CH4_REQUEST_CREATE(req, kind, 0, 1);
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIR_Comm_add_ref(comm);
    req->comm = comm;
    MPIR_Comm_save_inactive_request(comm, req);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    MPIDI_PART_REQUEST(req, buffer) = buf;
    MPIDI_PART_REQUEST(req, count) = count;
    MPIDI_PART_REQUEST(req, datatype) = datatype;
    if (kind == MPIR_REQUEST_KIND__PART_SEND) {
        MPIDI_PART_REQUEST(req, u.send.dest) = rank;
    } else {
        MPIDI_PART_REQUEST(req, u.recv.source) = rank;
        MPIDI_PART_REQUEST(req, u.recv.tag) = tag;
        MPIDI_PART_REQUEST(req, u.recv.context_id) = comm->context_id;
    }

    req->u.part.partitions = partitions;
    MPIR_Part_request_inactivate(req);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, rank);
    MPIDI_REQUEST(req, is_local) = MPIDI_av_is_local(av);
#endif

    /* Inactive partitioned comm request can be freed by request_free.
     * Completion cntr increases when request becomes active at start. */
    MPIR_cc_set(req->cc_ptr, 0);

    *req_ptr = req;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void part_req_am_init(MPIR_Request * part_req)
{
    MPIDIG_PART_REQUEST(part_req, peer_req_ptr) = NULL;
    if (part_req->kind == MPIR_REQUEST_KIND__PART_SEND) {
        /* partitions + 1: once all partitions are ready and CTS is received, data can be issued */
        MPIR_cc_set(&MPIDIG_PART_REQUEST(part_req, u.send).ready_cntr,
                    part_req->u.part.partitions + 1);
    }
}

void MPIDIG_precv_matched(MPIR_Request * part_req)
{
    MPI_Aint sdata_size = MPIDIG_PART_REQUEST(part_req, u.recv).sdata_size;

    /* Set status for partitioned req */
    MPIR_STATUS_SET_COUNT(part_req->status, sdata_size);
    part_req->status.MPI_SOURCE = MPIDI_PART_REQUEST(part_req, u.recv.source);
    part_req->status.MPI_TAG = MPIDI_PART_REQUEST(part_req, u.recv.tag);
    part_req->status.MPI_ERROR = MPI_SUCCESS;

    /* Additional check for partitioned pt2pt: require identical buffer size */
    if (part_req->status.MPI_ERROR == MPI_SUCCESS) {
        MPI_Aint rdata_size;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(part_req, datatype), rdata_size);
        rdata_size *= MPIDI_PART_REQUEST(part_req, count) * part_req->u.part.partitions;
        if (sdata_size != rdata_size) {
            part_req->status.MPI_ERROR =
                MPIR_Err_create_code(part_req->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __FUNCTION__,
                                     __LINE__, MPI_ERR_OTHER, "**ch4|partmismatchsize",
                                     "**ch4|partmismatchsize %d %d",
                                     (int) rdata_size, (int) sdata_size);
        }
    }
}

int MPIDIG_mpi_psend_init(const void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int dest, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Create and initialize device-layer partitioned request */
    mpi_errno = part_req_create((void *) buf, partitions, count, datatype, dest, tag, comm,
                                MPIR_REQUEST_KIND__PART_SEND, request);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize am components for send */
    part_req_am_init(*request);

    /* Initiate handshake with receiver for message matching */
    MPIDIG_part_send_init_msg_t am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id;
    am_hdr.sreq_ptr = *request;

    MPI_Aint dtype_size = 0;
    MPIR_Datatype_get_size_macro(datatype, dtype_size);
    am_hdr.data_sz = dtype_size * count * partitions;   /* count is per partition */

    CH4_CALL(am_send_hdr(dest, comm, MPIDIG_PART_SEND_INIT, &am_hdr, sizeof(am_hdr), 0, 0),
             MPIDI_REQUEST(*request, is_local), mpi_errno);

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int source, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Create and initialize device-layer partitioned request */
    mpi_errno = part_req_create(buf, partitions, count, datatype, source, tag, comm,
                                MPIR_REQUEST_KIND__PART_RECV, request);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize am components for receive */
    part_req_am_init(*request);

    /* Try matching a request or post a new one */
    MPIR_Request *unexp_req = NULL;
    unexp_req =
        MPIDIG_rreq_dequeue(source, tag, comm->context_id, &MPIDI_global.part_unexp_list,
                            MPIDIG_PART);
    if (unexp_req) {
        /* Copy sender info from unexp_req to local part_rreq */
        MPIDIG_PART_REQUEST(*request, u.recv).sdata_size =
            MPIDIG_PART_REQUEST(unexp_req, u.recv).sdata_size;
        MPIDIG_PART_REQUEST(*request, peer_req_ptr) = MPIDIG_PART_REQUEST(unexp_req, peer_req_ptr);
        MPIDI_CH4_REQUEST_FREE(unexp_req);

        MPIDIG_precv_matched(*request);
    } else {
        /* add a reference for handshake */
        MPIR_Request_add_ref(*request);

        MPIDIG_enqueue_request(*request, &MPIDI_global.part_posted_list, MPIDIG_PART);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

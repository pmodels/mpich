/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

static int part_req_create(void *buf, int partitions, MPI_Aint count,
                           MPI_Datatype datatype, int rank, int tag,
                           MPIR_Comm * comm, MPIR_Request_kind_t kind, MPIR_Request ** req_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;

    /* Set refcnt=2. 1 for user reference, 1 for handshake protocol */
    MPIDI_CH4_REQUEST_CREATE(req, kind, 0, 2);
    MPIR_ERR_CHKANDSTMT(req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIR_Comm_add_ref(comm);
    req->comm = comm;
    req->u.part.partitions = partitions;

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_PART_REQUEST(req, datatype) = datatype;
    MPIDI_PART_REQUEST(req, count) = count;
    if (kind == MPIR_REQUEST_KIND__PART_SEND) {
        MPIDI_PART_REQUEST(req, u.send.dest) = rank;
    } else {
        MPIDI_PART_REQUEST(req, u.recv.source) = rank;
        MPIDI_PART_REQUEST(req, u.recv.tag) = tag;
        MPIDI_PART_REQUEST(req, u.recv.context_id) = comm->context_id;
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(req, is_local) = 0;
#endif

    MPIDI_UCX_PART_REQ(req).peer_req = MPI_REQUEST_NULL;
    MPIDI_UCX_PART_REQ(req).ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    int is_contig;
    MPI_Aint true_lb;
    MPIDI_Datatype_check_contig_lb(datatype, is_contig, true_lb);
    if (!is_contig) {
        MPIR_Datatype *dt_ptr;

        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        MPIR_Assert(dt_ptr != NULL);
        MPIDI_UCX_PART_REQ(req).ucp_dt = dt_ptr->dev.netmod.ucx.ucp_datatype;
        MPIDI_PART_REQUEST(req, buffer) = buf;
    } else {
        MPI_Aint dt_size;

        MPIR_Datatype_get_size_macro(datatype, dt_size);
        MPIDI_UCX_PART_REQ(req).ucp_dt = ucp_dt_make_contig(dt_size);
        MPIDI_PART_REQUEST(req, buffer) = (char *) buf + true_lb;
    }

    /* Inactive partitioned comm request can be freed by request_free.
     * Completion cntr increases when request becomes active at start. */
    MPIR_cc_set(req->cc_ptr, 0);
    MPIR_Part_request_inactivate(req);

    *req_ptr = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_psend_init(void *buf, int partitions, MPI_Aint count,
                             MPI_Datatype datatype, int dest, int tag,
                             MPIR_Comm * comm, MPIR_Info * info,
                             MPIDI_av_entry_t * av, MPIR_Request ** request)
{
#ifdef MPIDI_ENABLE_AM_ONLY
    return MPIDIG_mpi_psend_init(buf, partitions, count, datatype, dest, tag, comm, info, request);
#else
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        part_req_create(buf, partitions, count, datatype, dest, tag, comm,
                        MPIR_REQUEST_KIND__PART_SEND, request);
    MPIR_ERR_CHECK(mpi_errno);

    /* partitions + 1: once all partitions are ready and CTS is received, data can be issued */
    MPIR_cc_set(&MPIDI_UCX_PART_REQ(*request).ready_cntr, partitions + 1);

    mpi_errno = MPIDI_UCX_part_send_init_hdr(*request, tag);

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}

int MPIDI_UCX_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                             MPI_Datatype datatype, int source, int tag,
                             MPIR_Comm * comm, MPIR_Info * info,
                             MPIDI_av_entry_t * av, MPIR_Request ** request)
{
#ifdef MPIDI_ENABLE_AM_ONLY
    return MPIDIG_mpi_precv_init(buf, partitions, count, datatype, source, tag, comm, info,
                                 request);
#else
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = part_req_create(buf, partitions, count, datatype, source, tag, comm,
                                MPIR_REQUEST_KIND__PART_RECV, request);
    MPIR_ERR_CHECK(mpi_errno);

    /* Try matching a request or post a new one */
    MPIR_Request *unexp_req =
        MPIDIG_rreq_dequeue(source, tag, comm->context_id, &MPIDI_global.part_unexp_list,
                            MPIDIG_PART);
    if (unexp_req) {
        MPIDI_UCX_PART_REQ(*request).peer_req = MPIDI_UCX_PART_REQ(unexp_req).peer_req;
        MPIDI_UCX_PART_REQ(*request).data_sz = MPIDI_UCX_PART_REQ(unexp_req).data_sz;
        MPIDI_CH4_REQUEST_FREE(unexp_req);

        MPIDI_UCX_precv_matched(*request);
    } else {
        MPIDIG_enqueue_request(*request, &MPIDI_global.part_posted_list, MPIDIG_PART);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}

void MPIDI_UCX_precv_matched(MPIR_Request * request)
{
    MPI_Aint sdata_size = MPIDI_UCX_PART_REQ(request).data_sz;

    /* Set status for partitioned req */
    MPIR_STATUS_SET_COUNT(request->status, sdata_size);
    request->status.MPI_SOURCE = MPIDI_PART_REQUEST(request, u.recv.source);
    request->status.MPI_TAG = MPIDI_PART_REQUEST(request, u.recv.tag);
    request->status.MPI_ERROR = MPI_SUCCESS;

    /* Additional check for partitioned pt2pt: require identical buffer size */
    if (request->status.MPI_ERROR == MPI_SUCCESS) {
        MPI_Aint rdata_size;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(request, datatype), rdata_size);
        rdata_size *= MPIDI_PART_REQUEST(request, count) * request->u.part.partitions;
        if (sdata_size != rdata_size) {
            request->status.MPI_ERROR =
                MPIR_Err_create_code(request->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __FUNCTION__,
                                     __LINE__, MPI_ERR_OTHER, "**ch4|partmismatchsize",
                                     "**ch4|partmismatchsize %d %d",
                                     (int) rdata_size, (int) sdata_size);
        }
    }

    MPIDI_UCX_part_recv_init_hdr(request);
    /* release handshake reference */
    MPIR_Request_free_unsafe(request);
    return;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* Common internal routine for send_init family */
static int psend_init(MPIDI_ptype ptype,
                      const void *buf,
                      MPI_Aint count,
                      MPI_Datatype datatype,
                      int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vci = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    MPIDI_CH4_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__PREQUEST_SEND, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;
    MPIR_Comm_save_inactive_request(comm, sreq);

    MPIDI_PREQUEST(sreq, p_type) = ptype;
    MPIDI_PREQUEST(sreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(sreq, count) = count;
    MPIDI_PREQUEST(sreq, datatype) = datatype;
    MPIDI_PREQUEST(sreq, rank) = rank;
    MPIDI_PREQUEST(sreq, tag) = tag;
    MPIDI_PREQUEST(sreq, context_offset) = context_offset;
    sreq->u.persist.real_request = NULL;
    MPIR_cc_set(sreq->cc_ptr, 0);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Send_init(const void *buf,
                   MPI_Aint count,
                   MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Ssend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_PT2PT_ATTR_SET_SYNCFLAG(attr);
    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Bsend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = psend_init(MPIDI_PTYPE_BSEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Rsend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* TODO: Currently we don't distinguish SEND and RSEND */
    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

int MPID_Recv_init(void *buf,
                   MPI_Aint count,
                   MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vci = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    MPIDI_CH4_REQUEST_CREATE(rreq, MPIR_REQUEST_KIND__PREQUEST_RECV, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *request = rreq;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIR_Comm_save_inactive_request(comm, rreq);

    MPIDI_PREQUEST(rreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(rreq, count) = count;
    MPIDI_PREQUEST(rreq, datatype) = datatype;
    MPIDI_PREQUEST(rreq, rank) = rank;
    MPIDI_PREQUEST(rreq, tag) = tag;
    MPIDI_PREQUEST(rreq, context_offset) = context_offset;
    rreq->u.persist.real_request = NULL;
    MPIR_cc_set(rreq->cc_ptr, 0);

    MPIDI_PREQUEST(rreq, p_type) = MPIDI_PTYPE_RECV;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPID_Bcast_init(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                    int root, MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Bcast_init_impl(buffer, count, datatype, root, comm_ptr, info_ptr, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Allreduce_init(const void *sendbuf, void *recvbuf, MPI_Aint count,
                        MPI_Datatype datatype, MPI_Op op,
                        MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allreduce_init_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, info_ptr,
                                         request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Reduce_init(const void *sendbuf, void *recvbuf, MPI_Aint count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_init_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      info_ptr, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Alltoall_init(const void *sendbuf, MPI_Aint sendcount,
                       MPI_Datatype sendtype, void *recvbuf,
                       MPI_Aint recvcount, MPI_Datatype recvtype,
                       MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoall_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, info_ptr, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Alltoallv_init(const void *sendbuf, const MPI_Aint sendcounts[],
                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                        void *recvbuf, const MPI_Aint recvcounts[],
                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                        MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoallv_init_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                         recvcounts, rdispls, recvtype, comm_ptr, info_ptr,
                                         request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Alltoallw_init(const void *sendbuf, const MPI_Aint sendcounts[],
                        const MPI_Aint sdispls[],
                        const MPI_Datatype sendtypes[],
                        void *recvbuf, const MPI_Aint recvcounts[],
                        const MPI_Aint rdispls[],
                        const MPI_Datatype recvtypes[],
                        MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Alltoallw_init_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                         recvcounts, rdispls, recvtypes, comm_ptr, info_ptr,
                                         request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Allgather_init(const void *sendbuf, MPI_Aint sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        MPI_Aint recvcount, MPI_Datatype recvtype,
                        MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allgather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, info_ptr, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Allgatherv_init(const void *sendbuf, MPI_Aint sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         const MPI_Aint * recvcounts,
                         const MPI_Aint * displs, MPI_Datatype recvtype,
                         MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Allgatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, comm_ptr, info_ptr, request);

    return mpi_errno;
}

int MPID_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf,
                                   MPI_Aint recvcount,
                                   MPI_Datatype datatype, MPI_Op op,
                                   MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_scatter_block_init_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                                    info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Reduce_scatter_init(const void *sendbuf, void *recvbuf,
                             const MPI_Aint recvcounts[],
                             MPI_Datatype datatype, MPI_Op op,
                             MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Reduce_scatter_init_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                              info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Scan_init(const void *sendbuf, void *recvbuf, MPI_Aint count,
                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                   MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scan_init_impl(sendbuf, recvbuf, count, datatype, op, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Gather_init(const void *sendbuf, MPI_Aint sendcount,
                     MPI_Datatype sendtype, void *recvbuf,
                     MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                     MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Gather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Gatherv_init(const void *sendbuf, MPI_Aint sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
                      MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                      MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Gatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                       recvtype, root, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Scatter_init(const void *sendbuf, MPI_Aint sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                      MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scatter_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       root, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Scatterv_init(const void *sendbuf, const MPI_Aint sendcounts[],
                       const MPI_Aint displs[], MPI_Datatype sendtype,
                       void *recvbuf, MPI_Aint recvcount,
                       MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                       MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Scatterv_init_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Barrier_init(MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Barrier_init_impl(comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Exscan_init(const void *sendbuf, void *recvbuf, MPI_Aint count,
                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                     MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Exscan_init_impl(sendbuf, recvbuf, count, datatype, op, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Neighbor_allgather_init(const void *sendbuf, MPI_Aint sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_allgather_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Neighbor_allgatherv_init(const void *sendbuf, MPI_Aint sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  const MPI_Aint recvcounts[],
                                  const MPI_Aint displs[],
                                  MPI_Datatype recvtype, MPIR_Comm * comm,
                                  MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_allgatherv_init_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcounts, displs, recvtype, comm, info,
                                                   request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Neighbor_alltoall_init(const void *sendbuf, MPI_Aint sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                MPI_Aint recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoall_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, comm, info, request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Neighbor_alltoallv_init(const void *sendbuf,
                                 const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[],
                                 MPI_Datatype sendtype, void *recvbuf,
                                 const MPI_Aint recvcounts[],
                                 const MPI_Aint rdispls[],
                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                 MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoallv_init_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                  recvcounts, rdispls, recvtype, comm, info,
                                                  request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Neighbor_alltoallw_init(const void *sendbuf,
                                 const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[],
                                 const MPI_Datatype sendtypes[],
                                 void *recvbuf,
                                 const MPI_Aint recvcounts[],
                                 const MPI_Aint rdispls[],
                                 const MPI_Datatype recvtypes[],
                                 MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Neighbor_alltoallw_init_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                  recvcounts, rdispls, recvtypes, comm, info,
                                                  request);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

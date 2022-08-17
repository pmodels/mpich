/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a recursive exchange based allgatherv */
int MPIR_TSP_Iallgatherv_sched_intra_ring(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                          MPI_Datatype recvtype, MPIR_Comm * comm,
                                          MPIR_TSP_sched_t sched)
{
    size_t extent;
    MPI_Aint lb, true_extent;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, src, dst;
    int nranks, is_inplace, rank;
    int send_rank, recv_rank;
    void *data_buf, *buf1, *buf2, *sbuf, *rbuf;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    /* find out the buffer which has the send data and point data_buf to it */
    if (is_inplace) {
        sendcount = recvcounts[rank];
        sendtype = recvtype;
        data_buf = (char *) recvbuf;
    } else
        data_buf = (char *) sendbuf;

    MPIR_Datatype_get_extent_macro(recvtype, extent);
    MPIR_Type_get_true_extent_impl(recvtype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPI_Aint max_count;
    max_count = recvcounts[0];
    for (i = 1; i < nranks; i++) {
        if (recvcounts[i] > max_count)
            max_count = recvcounts[i];
    }

    /* allocate space for temporary buffers to accommodate the largest recvcount */
    buf1 = MPIR_TSP_sched_malloc(max_count * extent, sched);
    buf2 = MPIR_TSP_sched_malloc(max_count * extent, sched);

    /* Phase 1: copy data to buf1 from sendbuf or recvbuf(in case of inplace) */
    int dtcopy_id[3];
    if (is_inplace) {
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) data_buf + displs[rank] * extent, sendcount, sendtype,
                                     buf1, recvcounts[rank], recvtype, sched, 0, NULL,
                                     &dtcopy_id[0]);
    } else {
        /* copy your data into your recvbuf from your sendbuf */
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype,
                                             (char *) recvbuf + displs[rank] * extent,
                                             recvcounts[rank], recvtype, sched, 0, NULL, &vtx_id);
        /* copy data from sendbuf to tmp_sendbuf to send the data */
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype, buf1, recvcounts[rank], recvtype,
                                     sched, 0, NULL, &dtcopy_id[0]);
    }

    MPIR_ERR_CHECK(mpi_errno);

    src = (nranks + rank - 1) % nranks;
    dst = (rank + 1) % nranks;

    sbuf = buf1;
    rbuf = buf2;

    int send_id[3];
    int recv_id[3] = { 0 };     /* warning fix: icc: maybe used before set */
    for (i = 0; i < nranks - 1; i++) {
        recv_rank = (rank - i - 1 + nranks) % nranks;   /* Rank whose data you're receiving */
        send_rank = (rank - i + nranks) % nranks;       /* Rank whose data you're sending */

        /* New tag for each send-recv pair. */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        int nvtcs, vtcs[3];
        if (i == 0) {
            nvtcs = 1;
            vtcs[0] = dtcopy_id[0];

            mpi_errno =
                MPIR_TSP_sched_isend(sbuf, recvcounts[send_rank], recvtype, dst, tag, comm, sched,
                                     nvtcs, vtcs, &send_id[i % 3]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            nvtcs = 0;
        } else {
            nvtcs = 2;
            vtcs[0] = recv_id[(i - 1) % 3];
            vtcs[1] = send_id[(i - 1) % 3];

            mpi_errno =
                MPIR_TSP_sched_isend(sbuf, recvcounts[send_rank], recvtype, dst, tag, comm, sched,
                                     nvtcs, vtcs, &send_id[i % 3]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            if (i == 1) {
                nvtcs = 2;
                vtcs[0] = send_id[0];
                vtcs[1] = recv_id[0];
            } else {
                nvtcs = 3;
                vtcs[0] = send_id[(i - 1) % 3];
                vtcs[1] = dtcopy_id[(i - 2) % 3];
                vtcs[2] = recv_id[(i - 1) % 3];
            }
        }

        mpi_errno =
            MPIR_TSP_sched_irecv(rbuf, recvcounts[recv_rank], recvtype, src, tag, comm, sched,
                                 nvtcs, vtcs, &recv_id[i % 3]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        /* Copy to correct position in recvbuf */
        mpi_errno =
            MPIR_TSP_sched_localcopy(rbuf, recvcounts[recv_rank], recvtype,
                                     (char *) recvbuf + displs[recv_rank] * extent,
                                     recvcounts[recv_rank], recvtype, sched, 1, &recv_id[i % 3],
                                     &dtcopy_id[i % 3]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        data_buf = sbuf;
        sbuf = rbuf;
        rbuf = data_buf;

    }

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: MPI_Alltoallv
 *
 * Since each process sends/receives different amounts of data to
 * every other process, we don't know the total message size for all
 * processes without additional communication. Therefore we simply use
 * the "middle of the road" isend/irecv algorithm that works
 * reasonably well in all cases.
 *
 * We post all irecvs and isends and then do a waitall. We scatter the
 * order of sources and destinations among the processes, so that all
 * processes don't try to send/recv to/from the same process at the
 * same time.
 *
 * *** Modification: We post only a small number of isends and irecvs
 * at a time and wait on them as suggested by Tony Ladd. ***
*/
int MPIR_Alltoallv_intra_scattered(const void *sendbuf, const MPI_Aint * sendcounts,
                                   const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                   const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t errflag)
{
    int comm_size, i;
    MPI_Aint send_extent, recv_extent;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status *starray;
    MPIR_Request **reqarray;
    int dst, rank, req_cnt;
    int ii, ss, bblock;

    MPIR_CHKLMEM_DECL(2);

    MPIR_THREADCOMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* Get extent of recv type, but send type is only valid if (sendbuf!=MPI_IN_PLACE) */
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);       /* call the pairwise_sendrecv_algo */
#endif

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    MPIR_Datatype_get_extent_macro(sendtype, send_extent);

    MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, 2 * bblock * sizeof(MPI_Status), mpi_errno,
                        "starray", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, 2 * bblock * sizeof(MPIR_Request *), mpi_errno,
                        "reqarray", MPL_MEM_BUFFER);

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < comm_size; ii += bblock) {
        req_cnt = 0;
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;

        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            if (recvcounts[dst]) {
                MPI_Aint type_size;
                MPIR_Datatype_get_size_macro(recvtype, type_size);
                if (type_size) {
                    mpi_errno = MPIC_Irecv((char *) recvbuf + rdispls[dst] * recv_extent,
                                           recvcounts[dst], recvtype, dst,
                                           MPIR_ALLTOALLV_TAG, comm_ptr, &reqarray[req_cnt]);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                    req_cnt++;
                }
            }
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            if (sendcounts[dst]) {
                MPI_Aint type_size;
                MPIR_Datatype_get_size_macro(sendtype, type_size);
                if (type_size) {
                    mpi_errno = MPIC_Isend((char *) sendbuf + sdispls[dst] * send_extent,
                                           sendcounts[dst], sendtype, dst,
                                           MPIR_ALLTOALLV_TAG, comm_ptr,
                                           &reqarray[req_cnt], errflag);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                    req_cnt++;
                }
            }
        }

        mpi_errno = MPIC_Waitall(req_cnt, reqarray, starray);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < req_cnt; i++) {
                if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = starray[i].MPI_ERROR;
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}

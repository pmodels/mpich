/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_intra_scattered(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                   const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag)
{
    int comm_size, i;
    MPI_Aint send_extent, recv_extent;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status *starray;
    MPIR_Request **reqarray;
    int dst, rank, req_cnt;
    int ii, ss, bblock;
    int type_size;

    MPIR_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

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
                MPIR_Datatype_get_size_macro(recvtype, type_size);
                if (type_size) {
                    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                     rdispls[dst] * recv_extent);
                    mpi_errno = MPIC_Irecv((char *) recvbuf + rdispls[dst] * recv_extent,
                                           recvcounts[dst], recvtype, dst,
                                           MPIR_ALLTOALLV_TAG, comm_ptr, &reqarray[req_cnt]);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                    req_cnt++;
                }
            }
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            if (sendcounts[dst]) {
                MPIR_Datatype_get_size_macro(sendtype, type_size);
                if (type_size) {
                    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                     sdispls[dst] * send_extent);
                    mpi_errno = MPIC_Isend((char *) sendbuf + sdispls[dst] * send_extent,
                                           sendcounts[dst], sendtype, dst,
                                           MPIR_ALLTOALLV_TAG, comm_ptr,
                                           &reqarray[req_cnt], errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                    req_cnt++;
                }
            }
        }

        mpi_errno = MPIC_Waitall(req_cnt, reqarray, starray, errflag);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIR_ERR_POP(mpi_errno);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < req_cnt; i++) {
                if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = starray[i].MPI_ERROR;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

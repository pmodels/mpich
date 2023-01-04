/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Blocked Alltoallw
 *
 * Since each process sends/receives different amounts of data to every other
 * process, we don't know the total message size for all processes without
 * additional communication. Therefore we simply use the "middle of the road"
 * isend/irecv algorithm that works reasonably well in all cases.
 *
 * We post all irecvs and isends and then do a waitall. We scatter the order of
 * sources and destinations among the processes, so that all processes don't
 * try to send/recv to/from the same process at the same time.
 *
 * *** Modification: We post only a small number of isends and irecvs at a time
 * and wait on them as suggested by Tony Ladd. ***
 */
int MPIR_Alltoallw_intra_scattered(const void *sendbuf, const MPI_Aint sendcounts[],
                                   const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                   void *recvbuf, const MPI_Aint recvcounts[],
                                   const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int comm_size, i;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status *starray;
    MPIR_Request **reqarray;
    int dst, rank;
    int outstanding_requests;
    int ii, ss, bblock;
    MPI_Aint type_size;
    MPIR_CHKLMEM_DECL(2);

    MPIR_THREADCOMM_RANK_SIZE(comm_ptr, rank, comm_size);

#ifdef HAVE_ERROR_CHECKING
    /* When MPI_IN_PLACE, we use pair-wise sendrecv_replace in order to conserve memory usage,
     * which is keeping with the spirit of the MPI-2.2 Standard.  But
     * because of this approach all processes must agree on the global
     * schedule of sendrecv_replace operations to avoid deadlock.
     */
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, 2 * bblock * sizeof(MPI_Status), mpi_errno,
                        "starray", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, 2 * bblock * sizeof(MPIR_Request *), mpi_errno,
                        "reqarray", MPL_MEM_BUFFER);

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < comm_size; ii += bblock) {
        outstanding_requests = 0;
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;

        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            if (recvcounts[dst]) {
                MPIR_Datatype_get_size_macro(recvtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIC_Irecv((char *) recvbuf + rdispls[dst],
                                           recvcounts[dst], recvtypes[dst], dst,
                                           MPIR_ALLTOALLW_TAG, comm_ptr,
                                           &reqarray[outstanding_requests]);
                    MPIR_ERR_CHECK(mpi_errno);

                    outstanding_requests++;
                }
            }
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            if (sendcounts[dst]) {
                MPIR_Datatype_get_size_macro(sendtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIC_Isend((char *) sendbuf + sdispls[dst],
                                           sendcounts[dst], sendtypes[dst], dst,
                                           MPIR_ALLTOALLW_TAG, comm_ptr,
                                           &reqarray[outstanding_requests], errflag);
                    MPIR_ERR_CHECK(mpi_errno);

                    outstanding_requests++;
                }
            }
        }

        mpi_errno = MPIC_Waitall(outstanding_requests, reqarray, starray);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < outstanding_requests; i++) {
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

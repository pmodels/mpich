/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Scatter
 *
 * It posts all irecvs and isends and then does a waitall. We scatter the order
 * of sources and destinations among the processes, so that all processes don't
 * try to send/recv to/from the same process at the same time.
 *
 * *** Modification: We post only a small number of isends and irecvs at a time
 * and wait on them as suggested by Tony Ladd. See comments below about
 * an additional modification that we may want to consider ***
 *
 *  FIXME: This converts the Alltoall to a set of blocking phases.  Two
 *  alternatives should be considered:
 *
 * 1) the choice of communication pattern could try to avoid contending routes
 * in each phase
 *
 * 2) rather than wait for all communication to finish (waitall), we could
 * maintain constant queue size by using waitsome and posting new isend/irecv
 * as others complete.  This avoids synchronization delays at the end of each
 * block (when there are only a few isend/irecvs left).
 */

int MPIR_Alltoall_intra_scattered(const void *sendbuf,
                                  MPI_Aint sendcount,
                                  MPI_Datatype sendtype,
                                  void *recvbuf,
                                  MPI_Aint recvcount,
                                  MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag)
{
    int comm_size, i;
    MPI_Aint sendtype_extent, recvtype_extent;
    int mpi_errno = MPI_SUCCESS, dst, rank;
    MPIR_Request **reqarray;
    MPI_Status *starray;
    MPIR_CHKLMEM_DECL(6);

    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    int ii, ss, bblock;

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, 2 * bblock * sizeof(MPIR_Request *), mpi_errno,
                        "reqarray", MPL_MEM_BUFFER);

    MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, 2 * bblock * sizeof(MPI_Status), mpi_errno,
                        "starray", MPL_MEM_BUFFER);

    for (ii = 0; ii < comm_size; ii += bblock) {
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;
        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            mpi_errno = MPIC_Irecv((char *) recvbuf +
                                   dst * recvcount * recvtype_extent,
                                   recvcount, recvtype, dst,
                                   MPIR_ALLTOALL_TAG, comm_ptr, coll_group, &reqarray[i]);
            MPIR_ERR_CHECK(mpi_errno);
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            mpi_errno = MPIC_Isend((char *) sendbuf +
                                   dst * sendcount * sendtype_extent,
                                   sendcount, sendtype, dst,
                                   MPIR_ALLTOALL_TAG, comm_ptr, coll_group, &reqarray[i + ss],
                                   errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* ... then wait for them to finish: */
        mpi_errno = MPIC_Waitall(2 * ss, reqarray, starray);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

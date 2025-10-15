/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALLV_PAIRWISE_NEW
      category    : COLLECTIVE
      alt-env     : MPIR_CVAR_ALLTOALL_PAIRWISE_NEW, MPIR_CVAR_ALLTOALLW_PAIRWISE_NEW
      type        : boolean
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If on, use a new algorithm that orders bit-wise pairs on all processes.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Algorithm: Pairwise sendrecv
 *
 * Restrictions: For MPI_IN_PLACE
 *
 * We use pair-wise sendrecv_replace in order to conserve memory usage,
 * which is keeping with the spirit of the MPI-2.2 Standard.  But
 * because of this approach all processes must agree on the global
 * schedule of sendrecv_replace operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of
 * time and there will be multiple repeated malloc/free's rather than
 * maintaining a single buffer across the whole loop.  Something like
 * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */

#define SET_BUF_COUNT_TYPE(i) \
    do { \
        if (is_v) { \
            buf = (char *) recvbuf + rdispls[i] * recv_extent; \
            count = recvcounts[i]; \
            datatype = recvtypes[0]; \
        } else if (is_w) { \
            buf = (char *) recvbuf + rdispls[i]; \
            count = recvcounts[i]; \
            datatype = recvtypes[i]; \
        } else { \
            buf = (char *) recvbuf + i * recvcounts[0] * recv_extent; \
            count = recvcounts[0]; \
            datatype = recvtypes[0]; \
        } \
    } while (0)

static int MPII_alltoall_pairwise_sendrecv_replace(void *recvbuf, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * rdispls,
                                                   const MPI_Datatype * recvtypes,
                                                   MPIR_Comm * comm_ptr, int coll_attr,
                                                   bool is_v, bool is_w)
{
    int mpi_errno = MPI_SUCCESS;

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    MPI_Aint recv_extent = 0;
    if (!is_w) {
        MPIR_Datatype_get_extent_macro(recvtypes[0], recv_extent);
    }

    /* We use pair-wise sendrecv_replace in order to conserve memory usage,
     * which is keeping with the spirit of the MPI-2.2 Standard.  But
     * because of this approach all processes must agree on the global
     * schedule of sendrecv_replace operations to avoid deadlock.
     *
     * Note that this is not an especially efficient algorithm in terms of
     * time and there will be multiple repeated malloc/free's rather than
     * maintaining a single buffer across the whole loop.  Something like
     * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */

    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    if (MPIR_CVAR_ALLTOALLV_PAIRWISE_NEW) {
        /* Ordering the pairs by flipping bits */
        int max_mask = 1;
        while (max_mask < comm_size) {
            max_mask *= 2;
        }
        for (int mask = 0; mask < max_mask; mask++) {
            int r = comm_ptr->rank ^ mask;
            if (r >= comm_size) {
                continue;
            }
            SET_BUF_COUNT_TYPE(r);
            mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                              r, MPIR_ALLTOALLV_TAG, r, MPIR_ALLTOALLV_TAG,
                                              comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else if (comm_ptr->intranode_table) {
        /* -- 1st exchange within intranode -- */
        for (int i = 0; i < comm_size; ++i) {
            if (comm_ptr->intranode_table[i] == -1 || rank == i)
                continue;
            SET_BUF_COUNT_TYPE(i);
            mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                              i, MPIR_ALLTOALLV_TAG, i, MPIR_ALLTOALLV_TAG,
                                              comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        /* -- 2nd exchange over internode -- */
        for (int i = 0; i < comm_size; ++i) {
            if (comm_ptr->intranode_table[i] > -1)
                continue;
            SET_BUF_COUNT_TYPE(i);
            mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                              i, MPIR_ALLTOALLV_TAG, i, MPIR_ALLTOALLV_TAG,
                                              comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        for (int i = 0; i < comm_size; ++i) {
            if (rank == i)
                continue;
            SET_BUF_COUNT_TYPE(i);
            mpi_errno = MPIC_Sendrecv_replace(buf, count, datatype,
                                              i, MPIR_ALLTOALLV_TAG, i, MPIR_ALLTOALLV_TAG,
                                              comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
    return MPII_alltoall_pairwise_sendrecv_replace(recvbuf, &recvcount, NULL, &recvtype,
                                                   comm_ptr, coll_attr, false, false);
}

int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const MPI_Aint * sendcounts,
                                                   const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                                   void *recvbuf, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
    return MPII_alltoall_pairwise_sendrecv_replace(recvbuf, recvcounts, rdispls, &recvtype,
                                                   comm_ptr, coll_attr, true /* is_v */ , false);
}

int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const MPI_Aint sendcounts[],
                                                   const MPI_Aint sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const MPI_Aint recvcounts[],
                                                   const MPI_Aint rdispls[],
                                                   const MPI_Datatype recvtypes[],
                                                   MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
    return MPII_alltoall_pairwise_sendrecv_replace(recvbuf, recvcounts, rdispls, recvtypes,
                                                   comm_ptr, coll_attr, false, true /* is_w */);
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

/* special case when count == 0 */
static int iallreduce_recexch_step1_zero(MPIR_Comm * comm, int tag,
                                         MPII_Recexchalgo_t *recexch, MPIR_TSP_sched_t sched);

/* MPIR_TSP_Iallreduce_sched_intra_recexch_step1 - non participating ranks
   send their data to participating ranks for allreduce in step2.
*/
int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm,
                                                  int tag,
                                                  MPI_Aint dt_size, int dtcopy_id,
                                                  MPII_Recexchalgo_t *recexch,
                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (count == 0) {
        mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step1_zero(comm, tag, recexch, sched);
        goto fn_exit;
    }

    if (recexch->step1_sendto >= 0) {
        /* non-participating rank sends the data to a participating rank */
        const void *buf_to_send = (sendbuf == MPI_IN_PLACE) ? (const void *) recvbuf : sendbuf;
        int dummy_id;
        mpi_errno = MPIR_TSP_sched_isend(buf_to_send, count, datatype,
                                         recexch->step1_sendto, tag, comm,
                                         sched, 0, NULL, &dummy_id);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* participating rank */
        int reduce_id = -1;
        int recv_id = -1;
        for (int i = 0; i < recexch->step1_nrecvs; i++) {
            int vtcs[2];

            int nbr = recexch->step1_recvfrom[i];
            void *nbr_buf;

            if (recexch->per_nbr_buffer) {
                /* concurrent irecv */
                nbr_buf = recexch->nbr_bufs[i];
                vtcs[0] = -1;
            } else {
                /* serial irecv */
                nbr_buf = recexch->nbr_bufs[0];
                vtcs[0] = reduce_id;
            }

            mpi_errno = MPIR_TSP_sched_irecv(nbr_buf, count, datatype,
                                             nbr, tag, comm, sched, 1, vtcs, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);

            if (i == 0 || (recexch->is_commutative && recexch->per_nbr_buffer)) {
                /* concurrent reduce_local */
                vtcs[0] = recv_id;
                vtcs[1] = dtcopy_id;
            } else {
                /* serial reduce_local */
                vtcs[0] = recv_id;
                vtcs[1] = reduce_id;
            }

            mpi_errno = MPIR_TSP_sched_reduce_local(nbr_buf, recvbuf, count,
                                                    datatype, op, sched, 2, vtcs,
                                                    &reduce_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_TSP_Iallreduce_sched_intra_recexch_step3 - reverse of step1. Participating ranks
   send final data to non-participating ranks.
*/
int MPIR_TSP_Iallreduce_sched_intra_recexch_step3(void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Comm * comm, int tag,
                                                  MPII_Recexchalgo_t *recexch,
                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int dummy_id;
    if (recexch->step1_sendto >= 0) {
        mpi_errno = MPIR_TSP_sched_irecv(recvbuf, count, datatype, recexch->step1_sendto,
                                         tag, comm, sched, 0, NULL, &dummy_id);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* wait for all previous operations */
        mpi_errno = MPIR_TSP_sched_fence(sched);
        MPIR_ERR_CHECK(mpi_errno);
        for (int i = 0; i < recexch->step1_nrecvs; i++) {
            mpi_errno = MPIR_TSP_sched_isend(recvbuf, count, datatype,
                                             recexch->step1_recvfrom[i], tag, comm, sched,
                                             0, NULL, &dummy_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:  
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* special case when count == 0 */

int MPIR_TSP_Iallreduce_sched_intra_recexch_step1_zero(MPIR_Comm * comm, int tag,
                                                       MPII_Recexchalgo_t *recexch,
                                                       MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int dummy_id;
    if (recexch->step1_sendto >= 0) {
        /* non-participating rank sends the data to a participating rank */
        mpi_errno = MPIR_TSP_sched_isend(NULL, 0, MPI_DATATYPE_NULL,
                                         recexch->step1_sendto, tag, comm,
                                         sched, 0, NULL, &dummy_id);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* participating rank */
        for (int i = 0; i < recexch->step1_nrecvs; i++) {
            mpi_errno = MPIR_TSP_sched_irecv(NULL, 0, MPI_DATATYPE_NULL,
                                             recexch->step1_recvfrom[i],
                                             tag, comm, sched, 0, NULL, &dummy_id);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:  
    goto fn_fail;
}

int MPIR_TSP_Iallreduce_sched_intra_recexch_step3_zero(MPIR_Comm * comm, int tag,
                                                       MPII_Recexchalgo_t *recexch,
                                                       MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int dummy_id;
    if (recexch->step1_sendto >= 0) {
        mpi_errno = MPIR_TSP_sched_irecv(NULL, 0, MPI_DATATYPE_NULL, recexch->step1_sendto,
                                         tag, comm, sched, 0, NULL, &dummy_id);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* wait for all previous operations */
        mpi_errno = MPIR_TSP_sched_fence(sched);
        MPIR_ERR_CHECK(mpi_errno);
        for (int i = 0; i < recexch->step1_nrecvs; i++) {
            mpi_errno = MPIR_TSP_sched_isend(NULL, 0, MPI_DATATYPE_NULL,
                                             recexch->step1_recvfrom[i], tag, comm, sched,
                                             0, NULL, &dummy_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:  
    goto fn_fail;
}


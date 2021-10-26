/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

/* MPIR_TSP_Iallreduce_sched_intra_recexch_step1 - non participating ranks
   send their data to participating ranks for allreduce in step2.
*/
int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm,
                                                  int tag,
                                                  MPI_Aint dt_size, int recvbuf_ready,
                                                  MPII_Recexchalgo_t * recexch,
                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* FIXME: replace the trivial cases with MPIR_Assert */
    if (count == 0) {
        mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step1_zero(comm, tag, recexch, sched);
        goto fn_exit;
    }

    if (recexch->k == 1) {
        /* this is the case when nprocs is 1 */
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
        /* require MPIR_TSP_Iallreduce_recexch_alloc_nbr_bufs */
        MPIR_Assert(recexch->nbr_bufs);

        /* participating rank */
        int nbr_buf_ready = -1;
        for (int i = 0; i < recexch->step1_nrecvs; i++) {
            int nbr = recexch->step1_recvfrom[i];
            void *nbr_buf = MPIR_TSP_Iallreduce_recexch_get_nbr_buf(recexch, 0, i);

            /* -- irecv -- */
            int recv_id;
            mpi_errno = MPIR_TSP_sched_irecv(nbr_buf, count, datatype,
                                             nbr, tag, comm, sched, 1, &nbr_buf_ready, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);

            /* -- reduce -- */
            int vtcs[2];
            vtcs[0] = recv_id;
            vtcs[1] = recvbuf_ready;

            int reduce_id;
            mpi_errno = MPIR_TSP_sched_reduce_local(nbr_buf, recvbuf, count,
                                                    datatype, op, sched, 2, vtcs, &reduce_id);
            MPIR_ERR_CHECK(mpi_errno);

            if (!recexch->per_nbr_buffer) {
                nbr_buf_ready = reduce_id;
            }
            if (!recexch->is_commutative) {
                recvbuf_ready = reduce_id;
            }
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
                                                  MPII_Recexchalgo_t * recexch,
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
                                                       MPII_Recexchalgo_t * recexch,
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
    goto fn_exit;
}

int MPIR_TSP_Iallreduce_sched_intra_recexch_step3_zero(MPIR_Comm * comm, int tag,
                                                       MPII_Recexchalgo_t * recexch,
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
    goto fn_exit;
}

int MPIR_TSP_Iallreduce_recexch_alloc_nbr_bufs(MPI_Aint dt_size,
                                               MPII_Recexchalgo_t * recexch, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    void **bufs = NULL;

    /* FIXME: replace the trivial cases with MPIR_Assert */
    if (recexch->k == 1) {
        /* this is the case when number of procs is 1 */
        goto fn_exit;
    }

    int n;
    if (recexch->per_nbr_buffer) {
        if (recexch->step2_nphases > 0) {
            n = recexch->step2_nphases * (recexch->k - 1);
        } else {
            n = recexch->k - 1;
        }
    } else {
        /* single buffer */
        n = 1;
    }

    if (recexch->step1_sendto == -1) {
        bufs = (void **) MPL_malloc(n * sizeof(void *), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!bufs, mpi_errno, MPI_ERR_OTHER, "**nomem");

        if (!recexch->per_nbr_buffer) {
            /* single buffer */
            bufs[0] = MPIR_TSP_sched_malloc(dt_size, sched);
            MPIR_ERR_CHKANDJUMP(!bufs[0], mpi_errno, MPI_ERR_OTHER, "**nomem");
            for (int i = 1; i < n; i++) {
                bufs[i] = (char *) bufs[0];
            }
        } else {
            /* multiple buffer */
            bufs[0] = MPIR_TSP_sched_malloc(dt_size * n, sched);
            MPIR_ERR_CHKANDJUMP(!bufs[0], mpi_errno, MPI_ERR_OTHER, "**nomem");
            for (int i = 1; i < n; i++) {
                bufs[i] = (char *) bufs[0] + dt_size * i;
            }
        }
    }

  fn_exit:
    recexch->nbr_bufs = bufs;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (bufs) {
        MPL_free(bufs);
        bufs = NULL;
    }
    goto fn_exit;
}

void *MPIR_TSP_Iallreduce_recexch_get_nbr_buf(MPII_Recexchalgo_t * recexch, int phase, int i)
{
    if (recexch->per_nbr_buffer) {
        int k = recexch->k;
        return recexch->nbr_bufs[phase * (k - 1) + i];
    } else {
        return recexch->nbr_bufs[0];
    }
}

/* For non-commutative op, re-order the rank list and fill myidxes. The algorithms following
 * the reduction order will ensure correctness */
int MPIR_TSP_Iallreduce_recexch_check_commutative(MPII_Recexchalgo_t * recexch, int myrank)
{
    int mpi_errno = MPI_SUCCESS;
    int k = recexch->k;
    int nphases = recexch->step2_nphases;
    int *myidxes = NULL;

    if (recexch->is_commutative) {
        goto fn_exit;
    }

    if (recexch->step1_recvfrom) {
        /* assuming step1_recvfrom is ordered. Or we may add a MPL_sort_int_array */
        MPL_reverse_int_array(recexch->step1_recvfrom, recexch->step1_nrecvs);
    }

    if (recexch->in_step2 && nphases > 0) {
        myidxes = (int *) MPL_malloc(nphases * sizeof(int), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!myidxes, mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (int phase = 0; phase < nphases; phase++) {
            int *nbrs = recexch->step2_nbrs[phase];
            MPL_sort_int_array(nbrs, k - 1);

            /* myidx is the index in the neighbors list such that
             * all neighbors before myidx have ranks less than my rank
             * and neighbors at and after myidx have rank greater than me.
             * This is useful only in the non-commutative case. */
            int myidx = 0;
            for (int i = 0; i < k - 1; i++) {
                if (myrank > nbrs[i]) {
                    myidx = i + 1;
                }
            }
            myidxes[phase] = myidx;

            /* NOTE: since we are accumulating into recvbuf, in order to
             * preserve operation order, we need follow the reduction sequence:
             *     myidx-1, myidx-2, ..., 0, myidx, myidx+1, ..., k-1
             * Rather than use spearate for-loops, rearrange the list here so
             * a single for i=0:k-1 loop will work.
             */
            MPL_reverse_int_array(nbrs, myidx);
        }
        recexch->myidxes = myidxes;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

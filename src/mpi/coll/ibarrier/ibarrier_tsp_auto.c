/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ibarrier_sched_intra_tsp_auto(MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBARRIER,
        .comm_ptr = comm,
    };
    MPII_Csel_container_s *cnt;
    void *recvbuf = NULL;

    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    switch (MPIR_CVAR_IBARRIER_INTRA_ALGORITHM) {
        case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_tsp_recexch:
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE, MPI_SUM,
                                                        comm,
                                                        MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                        MPIR_CVAR_IBARRIER_RECEXCH_KVAL, sched);
            break;

        case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_tsp_k_dissemination:
            mpi_errno =
                MPIR_TSP_Ibarrier_sched_intra_k_dissemination(comm, MPIR_CVAR_IBARRIER_DISSEM_KVAL,
                                                              sched);
            break;

        default:
            cnt = MPIR_Csel_search(comm->csel_comm, coll_sig);
            MPIR_Assert(cnt);

            switch (cnt->id) {
                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_recexch:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE,
                                                                MPI_SUM, comm,
                                                                MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                                cnt->u.ibarrier.intra_tsp_recexch.k,
                                                                sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_k_dissemination:
                    mpi_errno =
                        MPIR_TSP_Ibarrier_sched_intra_k_dissemination(comm,
                                                                      cnt->u.
                                                                      ibarrier.intra_tsp_k_dissemination.
                                                                      k, sched);
                    break;

                default:
                    /* Replace this call with MPIR_Assert(0) when json files have gentran algos */
                    goto fallback;
                    break;
            }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, NULL, 0,
                                                        MPI_BYTE, MPI_SUM, comm, 0, 2, sched);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

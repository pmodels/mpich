/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../igather/igather_tsp_tree_algos_prototypes.h"

/*
*/

int MPIR_Igather_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr, bool is_persistent,
                                    void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IGATHER,
        .comm_ptr = comm_ptr,

        .u.igather.sendbuf = sendbuf,
        .u.igather.sendcount = sendcount,
        .u.igather.sendtype = sendtype,
        .u.igather.recvcount = recvcount,
        .u.igather.recvbuf = recvbuf,
        .u.igather.recvtype = recvtype,
        .u.igather.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_gentran_tree:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Igather_sched_intra_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, root, comm_ptr,
                                                  cnt->u.igather.intra_gentran_tree.k, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_binomial:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Igather_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, root, comm_ptr,
                                                          *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_long:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Igather_inter_sched_long(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm_ptr,
                                                      *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_short:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Igather_inter_sched_short(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, root, comm_ptr,
                                                       *sched_p);
            break;

        default:
            MPIR_Assert(0);
        /* *INDENT-ON* */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                            enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IGATHER_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_gentran_tree:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Igather_sched_intra_tree(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm_ptr,
                                                      MPIR_CVAR_IGATHER_TREE_KVAL, *sched_p);
                break;

            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Igather_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcount, recvtype, root, comm_ptr,
                                                              *sched_p);
                break;

            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Igather_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, root, comm_ptr,
                                                    is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IGATHER_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_sched_long:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Igather_inter_sched_long(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, root, comm_ptr,
                                                          *sched_p);
                break;

            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_sched_short:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Igather_inter_sched_short(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, root, comm_ptr,
                                                           *sched_p);
                break;

            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Igather_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, root, comm_ptr,
                                                    is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather_impl(const void *sendbuf, MPI_Aint sendcount,
                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                      MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Igather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        root, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                 MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr,
                         request);
    } else {
        mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm_ptr, request);
    }

    return mpi_errno;
}

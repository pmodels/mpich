/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iscatterv/iscatterv_tsp_linear_algos_prototypes.h"

/*
*/

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Iscatterv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                      const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                      MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                      enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ISCATTERV,
        .comm_ptr = comm_ptr,

        .u.iscatterv.sendbuf = sendbuf,
        .u.iscatterv.sendcounts = sendcounts,
        .u.iscatterv.displs = displs,
        .u.iscatterv.sendtype = sendtype,
        .u.iscatterv.recvcount = recvcount,
        .u.iscatterv.recvbuf = recvbuf,
        .u.iscatterv.recvtype = recvtype,
        .u.iscatterv.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_gentran_linear:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iscatterv_sched_allcomm_linear(sendbuf, sendcounts, displs, sendtype,
                                                        recvbuf, recvcount, recvtype, root,
                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatterv_intra_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                        recvbuf, recvcount, recvtype, root,
                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatterv_inter_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                        recvbuf, recvcount, recvtype, root,
                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_sched_linear:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs, sendtype,
                                                            recvbuf, recvcount, recvtype, root,
                                                            comm_ptr, *sched_p);
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

int MPIR_Iscatterv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                    const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                    const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                              const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno =
            MPIR_Iscatterv_intra_sched_auto(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Iscatterv_inter_sched_auto(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                              const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype, int root,
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
        switch (MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM_gentran_linear:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iscatterv_sched_allcomm_linear(sendbuf, sendcounts, displs, sendtype,
                                                            recvbuf, recvcount, recvtype, root,
                                                            comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM_sched_linear:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs,
                                                                sendtype, recvbuf, recvcount,
                                                                recvtype, root, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatterv_intra_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                            recvbuf, recvcount, recvtype, root,
                                                            comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatterv_allcomm_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                      recvbuf, recvcount, recvtype, root, comm_ptr,
                                                      is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_ISCATTERV_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_ISCATTERV_INTER_ALGORITHM_sched_linear:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs,
                                                                sendtype, recvbuf, recvcount,
                                                                recvtype, root, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTERV_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatterv_inter_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                            recvbuf, recvcount, recvtype, root,
                                                            comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTERV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatterv_allcomm_sched_auto(sendbuf, sendcounts, displs, sendtype,
                                                      recvbuf, recvcount, recvtype, root, comm_ptr,
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

int MPIR_Iscatterv_impl(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint displs[],
                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                        MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iscatterv_sched_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint displs[],
                   MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                   MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype,
                           root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm_ptr, request);
    }

    return mpi_errno;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iscan/iscan_tsp_recursive_doubling_algos_prototypes.h"

/*
*/

int MPIR_Iscan_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  bool is_persistent, void **sched_p,
                                  enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ISCAN,
        .comm_ptr = comm_ptr,

        .u.iscan.sendbuf = sendbuf,
        .u.iscan.recvbuf = recvbuf,
        .u.iscan.count = count,
        .u.iscan.datatype = datatype,
        .u.iscan.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                                  op, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_smp:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscan_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_gentran_recursive_doubling:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iscan_sched_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
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

int MPIR_Iscan_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                          enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    switch (MPIR_CVAR_ISCAN_INTRA_ALGORITHM) {
        /* *INDENT-OFF* */
        case MPIR_CVAR_ISCAN_INTRA_ALGORITHM_gentran_recursive_doubling:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iscan_sched_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                              comm_ptr, *sched_p);
            break;

        case MPIR_CVAR_ISCAN_INTRA_ALGORITHM_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                                  op, comm_ptr, *sched_p);
            break;

        case MPIR_CVAR_ISCAN_INTRA_ALGORITHM_sched_smp:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscan_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   *sched_p);
            break;

        case MPIR_CVAR_ISCAN_INTRA_ALGORITHM_auto:
            mpi_errno =
                MPIR_Iscan_allcomm_sched_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                              is_persistent, sched_p, sched_type_p);
            break;

        default:
            MPIR_Assert(0);
        /* *INDENT-ON* */
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscan_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, false,
                                      &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ISCAN_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Iscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    }

    MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, count, datatype,
                                    *request);

    return mpi_errno;
}

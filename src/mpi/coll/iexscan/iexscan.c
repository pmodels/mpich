/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iexscan algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_recursive_doubling - Force recursive doubling algorithm

    - name        : MPIR_CVAR_IEXSCAN_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Iexscan will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


int MPIR_Iexscan_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IEXSCAN,
        .comm_ptr = comm_ptr,

        .u.iexscan.sendbuf = sendbuf,
        .u.iexscan.recvbuf = recvbuf,
        .u.iexscan.count = count,
        .u.iexscan.datatype = datatype,
        .u.iexscan.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iexscan_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iexscan_intra_sched_auto(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iexscan_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iexscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count,
                                                                    datatype, op, comm_ptr,
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

int MPIR_Iexscan_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iexscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                    s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iexscan_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                            bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    switch (MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM) {
        /* *INDENT-OFF* */
        case MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iexscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count,
                                                                    datatype, op, comm_ptr,
                                                                    *sched_p);
            break;

        case MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iexscan_intra_sched_auto(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, *sched_p);
            break;

        case MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM_auto:
            mpi_errno = MPIR_Iexscan_allcomm_sched_auto(sendbuf, recvbuf, count, datatype, op,
                                                        comm_ptr, is_persistent, sched_p,
                                                        sched_type_p);
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

int MPIR_Iexscan_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iexscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, false,
                                        &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iexscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
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
         MPIR_CVAR_IEXSCAN_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Iexscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    }

    MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, count, datatype,
                                    *request);

    return mpi_errno;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GATHERV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select gatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        linear - Force linear algorithm
        nb     - Force nonblocking algorithm

    - name        : MPIR_CVAR_GATHERV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select gatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        linear - Force linear algorithm
        nb     - Force nonblocking algorithm

    - name        : MPIR_CVAR_GATHERV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Gatherv will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Gatherv_allcomm_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const int *recvcounts, const int *displs,
                              MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHERV,
        .comm_ptr = comm_ptr,

        .u.gatherv.sendbuf = sendbuf,
        .u.gatherv.sendcount = sendcount,
        .u.gatherv.sendtype = sendtype,
        .u.gatherv.recvbuf = recvbuf,
        .u.gatherv.recvcounts = recvcounts,
        .u.gatherv.displs = displs,
        .u.gatherv.recvtype = recvtype,
        .u.gatherv.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_linear:
            mpi_errno =
                MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_nb:
            mpi_errno =
                MPIR_Gatherv_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                        recvtype, root, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Gatherv_impl(const void *sendbuf, int sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      const int *recvcounts, const int *displs,
                      MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_GATHERV_INTRA_ALGORITHM) {
            case MPIR_CVAR_GATHERV_INTRA_ALGORITHM_linear:
                mpi_errno =
                    MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHERV_INTRA_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Gatherv_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHERV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Gatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_GATHERV_INTER_ALGORITHM) {
            case MPIR_CVAR_GATHERV_INTER_ALGORITHM_linear:
                mpi_errno =
                    MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHERV_INTER_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Gatherv_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHERV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Gatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root,
                         comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                      recvtype, root, comm_ptr, errflag);
    }

    return mpi_errno;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb   - Force nonblocking algorithm

    - name        : MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb   - Force nonblocking algorithm

    - name        : MPIR_CVAR_NEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Neighbor_allgather will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* any non-MPI functions go here, especially non-static ones */


int MPIR_Neighbor_allgather_allcomm_auto(const void *sendbuf, MPI_Aint sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHER,
        .comm_ptr = comm_ptr,

        .u.neighbor_allgather.sendbuf = sendbuf,
        .u.neighbor_allgather.sendcount = sendcount,
        .u.neighbor_allgather.sendtype = sendtype,
        .u.neighbor_allgather.recvbuf = recvbuf,
        .u.neighbor_allgather.recvcount = recvcount,
        .u.neighbor_allgather.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgather_allcomm_nb:
            mpi_errno =
                MPIR_Neighbor_allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm_ptr);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Neighbor_allgather_impl(const void *sendbuf, MPI_Aint sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 MPI_Aint recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Neighbor_allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Neighbor_allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Neighbor_allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Neighbor_allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Neighbor_allgather(const void *sendbuf, MPI_Aint sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            MPI_Aint recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                    comm_ptr);
    } else {
        mpi_errno = MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype, comm_ptr);
    }

    return mpi_errno;
}

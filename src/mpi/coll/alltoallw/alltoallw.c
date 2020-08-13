/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                        - Force nonblocking algorithm
        pairwise_sendrecv_replace - Force pairwise sendrecv replace algorithm
        scattered                 - Force scattered algorithm

    - name        : MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                - Force nonblocking algorithm
        pairwise_exchange - Force pairwise exchange algorithm

    - name        : MPIR_CVAR_ALLTOALLW_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Alltoallw will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Alltoallw_allcomm_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                void *recvbuf, const MPI_Aint recvcounts[],
                                const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLW,
        .comm_ptr = comm_ptr,

        .u.alltoallw.sendbuf = sendbuf,
        .u.alltoallw.sendcounts = sendcounts,
        .u.alltoallw.sdispls = sdispls,
        .u.alltoallw.sendtypes = sendtypes,
        .u.alltoallw.recvbuf = recvbuf,
        .u.alltoallw.recvcounts = recvcounts,
        .u.alltoallw.rdispls = rdispls,
        .u.alltoallw.recvtypes = recvtypes,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_pairwise_sendrecv_replace:
            mpi_errno =
                MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtypes, recvbuf, recvcounts,
                                                               rdispls, recvtypes, comm_ptr,
                                                               errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_scattered:
            mpi_errno =
                MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                               recvcounts, rdispls, recvtypes, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_inter_pairwise_exchange:
            mpi_errno =
                MPIR_Alltoallw_inter_pairwise_exchange(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_allcomm_nb:
            mpi_errno =
                MPIR_Alltoallw_allcomm_nb(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                          recvcounts, rdispls, recvtypes, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Alltoallw_impl(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                        const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Aint recvcounts[],
                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM_pairwise_sendrecv_replace:
                mpi_errno = MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts,
                                                                           sdispls, sendtypes,
                                                                           recvbuf, recvcounts,
                                                                           rdispls, recvtypes,
                                                                           comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM_scattered:
                mpi_errno = MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts,
                                                           sdispls, sendtypes, recvbuf, recvcounts,
                                                           rdispls, recvtypes, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoallw_allcomm_nb(sendbuf, sendcounts,
                                                      sdispls, sendtypes, recvbuf, recvcounts,
                                                      rdispls, recvtypes, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoallw_allcomm_auto(sendbuf, sendcounts,
                                                        sdispls, sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM_pairwise_exchange:
                mpi_errno = MPIR_Alltoallw_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                   sendtypes, recvbuf, recvcounts,
                                                                   rdispls, recvtypes, comm_ptr,
                                                                   errflag);
                break;
            case MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoallw_allcomm_nb(sendbuf, sendcounts,
                                                      sdispls, sendtypes, recvbuf, recvcounts,
                                                      rdispls, recvtypes, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoallw_allcomm_auto(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, errflag);
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

int MPIR_Alltoallw(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Aint recvcounts[],
                   const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLTOALLW_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                           recvtypes, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                        recvcounts, rdispls, recvtypes, comm_ptr, errflag);
    }

    return mpi_errno;
}

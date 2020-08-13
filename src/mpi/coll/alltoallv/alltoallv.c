/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                        - Force nonblocking algorithm
        pairwise_sendrecv_replace - Force pairwise_sendrecv_replace algorithm
        scattered                 - Force scattered algorithm

    - name        : MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        pairwise_exchange - Force pairwise exchange algorithm
        nb                - Force nonblocking algorithm

    - name        : MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Alltoallv will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Alltoallv_allcomm_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLV,
        .comm_ptr = comm_ptr,

        .u.alltoallv.sendbuf = sendbuf,
        .u.alltoallv.sendcounts = sendcounts,
        .u.alltoallv.sdispls = sdispls,
        .u.alltoallv.sendtype = sendtype,
        .u.alltoallv.recvbuf = recvbuf,
        .u.alltoallv.recvcounts = recvcounts,
        .u.alltoallv.rdispls = rdispls,
        .u.alltoallv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_pairwise_sendrecv_replace:
            mpi_errno =
                MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtype, recvbuf, recvcounts,
                                                               rdispls, recvtype, comm_ptr,
                                                               errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_scattered:
            mpi_errno =
                MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                               recvcounts, rdispls, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_inter_pairwise_exchange:
            mpi_errno =
                MPIR_Alltoallv_inter_pairwise_exchange(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_allcomm_nb:
            mpi_errno =
                MPIR_Alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                          recvcounts, rdispls, recvtype, comm_ptr, errflag);
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

int MPIR_Alltoallv_impl(const void *sendbuf, const MPI_Aint * sendcounts, const MPI_Aint * sdispls,
                        MPI_Datatype sendtype, void *recvbuf, const MPI_Aint * recvcounts,
                        const MPI_Aint * rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM_pairwise_sendrecv_replace:
                mpi_errno = MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts,
                                                                           sdispls, sendtype,
                                                                           recvbuf, recvcounts,
                                                                           rdispls, recvtype,
                                                                           comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM_scattered:
                mpi_errno = MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls,
                                                           sendtype, recvbuf, recvcounts,
                                                           rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoallv_allcomm_auto(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM) {
                /* intercommunicator */
            case MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM_pairwise_exchange:
                mpi_errno = MPIR_Alltoallv_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                   sendtype, recvbuf, recvcounts,
                                                                   rdispls, recvtype, comm_ptr,
                                                                   errflag);
                break;
            case MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoallv_allcomm_auto(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, errflag);
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

int MPIR_Alltoallv(const void *sendbuf, const MPI_Aint * sendcounts, const MPI_Aint * sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const MPI_Aint * recvcounts,
                   const MPI_Aint * rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                           recvtype, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                        rdispls, recvtype, comm_ptr, errflag);
    }

    return mpi_errno;
}

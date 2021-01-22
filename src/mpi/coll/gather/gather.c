/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        use the short message algorithm for intercommunicator MPI_Gather if the
        send buffer size is < this value (in bytes)
        (See also: MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)

    - name        : MPIR_CVAR_GATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select gather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        binomial - Force binomial algorithm
        nb       - Force nonblocking algorithm

    - name        : MPIR_CVAR_GATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select gather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        linear                   - Force linear algorithm
        local_gather_remote_send - Force local-gather-remote-send algorithm
        nb                       - Force nonblocking algorithm

    - name        : MPIR_CVAR_GATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Gather will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Gather_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHER,
        .comm_ptr = comm_ptr,

        .u.gather.sendbuf = sendbuf,
        .u.gather.sendcount = sendcount,
        .u.gather.sendtype = sendtype,
        .u.gather.recvcount = recvcount,
        .u.gather.recvbuf = recvbuf,
        .u.gather.recvtype = recvtype,
        .u.gather.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_intra_binomial:
            mpi_errno =
                MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_linear:
            mpi_errno =
                MPIR_Gather_inter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_local_gather_remote_send:
            mpi_errno =
                MPIR_Gather_inter_local_gather_remote_send(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, root, comm_ptr,
                                                           errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_allcomm_nb:
            mpi_errno =
                MPIR_Gather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       root, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Gather_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                     int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_GATHER_INTRA_ALGORITHM) {
            case MPIR_CVAR_GATHER_INTRA_ALGORITHM_binomial:
                mpi_errno = MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, root,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHER_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Gather_allcomm_nb(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr,
                                                   errflag);
                break;
            case MPIR_CVAR_GATHER_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Gather_allcomm_auto(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root,
                                                     comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_GATHER_INTER_ALGORITHM) {
            case MPIR_CVAR_GATHER_INTER_ALGORITHM_linear:
                mpi_errno = MPIR_Gather_inter_linear(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root,
                                                     comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHER_INTER_ALGORITHM_local_gather_remote_send:
                mpi_errno = MPIR_Gather_inter_local_gather_remote_send(sendbuf, sendcount, sendtype,
                                                                       recvbuf, recvcount, recvtype,
                                                                       root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_GATHER_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Gather_allcomm_nb(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr,
                                                   errflag);
                break;
            case MPIR_CVAR_GATHER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Gather_allcomm_auto(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root,
                                                     comm_ptr, errflag);
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

int MPIR_Gather(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_GATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr,
                        errflag);
    } else {
        mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     root, comm_ptr, errflag);
    }

    return mpi_errno;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 81920
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the short message algorithm will
        be used if the send buffer size is < this value (in bytes).
        (See also: MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the long message algorithm will be
        used if the send buffer size is >= this value (in bytes)
        (See also: MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        brucks             - Force brucks algorithm
        nb                 - Force nonblocking algorithm
        recursive_doubling - Force recursive doubling algorithm
        ring               - Force ring algorithm

    - name        : MPIR_CVAR_ALLGATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        local_gather_remote_bcast - Force local-gather-remote-bcast algorithm
        nb                        - Force nonblocking algorithm

    - name        : MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Allgather will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Allgather_allcomm_auto(const void *sendbuf,
                                int sendcount,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                int recvcount,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHER,
        .comm_ptr = comm_ptr,

        .u.allgather.sendbuf = sendbuf,
        .u.allgather.sendcount = sendcount,
        .u.allgather.sendtype = sendtype,
        .u.allgather.recvbuf = recvbuf,
        .u.allgather.recvcount = recvcount,
        .u.allgather.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_brucks:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recursive_doubling:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_ring:
            mpi_errno =
                MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_inter_local_gather_remote_bcast:
            mpi_errno =
                MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype,
                                                               comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_allcomm_nb:
            mpi_errno =
                MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_brucks:
                mpi_errno =
                    MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno =
                    MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_ring:
                mpi_errno =
                    MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLGATHER_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_local_gather_remote_bcast:
                mpi_errno =
                    MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
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

int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                           errflag);
    } else {
        mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, errflag);
    }

    return mpi_errno;
}

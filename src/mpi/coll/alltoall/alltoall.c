/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
   - name      : MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 256
     class     : none
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the short message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value
       (See also: MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 32768
     class     : none
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the medium message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value and
       larger than MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
       (See also: MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_THROTTLE
     category  : COLLECTIVE
     type      : int
     default   : 32
     class     : none
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       max no. of irecvs/isends posted at a time in some alltoall
       algorithms. Setting it to 0 causes all irecvs/isends to be
       posted at once

   - name        : MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM
     category    : COLLECTIVE
     type        : enum
     default     : auto
     class       : none
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : |-
       Variable to select alltoall algorithm
       auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
       brucks                    - Force brucks algorithm
       nb                        - Force nonblocking algorithm
       pairwise                  - Force pairwise algorithm
       pairwise_sendrecv_replace - Force pairwise sendrecv replace algorithm
       scattered                 - Force scattered algorithm

   - name        : MPIR_CVAR_ALLTOALL_INTER_ALGORITHM
     category    : COLLECTIVE
     type        : enum
     default     : auto
     class       : none
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : |-
       Variable to select alltoall algorithm
       auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
       nb                - Force nonblocking algorithm
       pairwise_exchange - Force pairwise exchange algorithm

   - name        : MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE
     category    : COLLECTIVE
     type        : boolean
     default     : true
     class       : none
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Alltoall will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Alltoall_allcomm_auto(const void *sendbuf,
                               int sendcount,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               int recvcount,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALL,
        .comm_ptr = comm_ptr,

        .u.alltoall.sendbuf = sendbuf,
        .u.alltoall.sendcount = sendcount,
        .u.alltoall.sendtype = sendtype,
        .u.alltoall.recvcount = recvcount,
        .u.alltoall.recvbuf = recvbuf,
        .u.alltoall.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_brucks:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise_sendrecv_replace:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcount, recvtype, comm_ptr,
                                                              errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_scattered:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_inter_pairwise_exchange:
            mpi_errno =
                MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_allcomm_nb:
            mpi_errno =
                MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, errflag);
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

int MPIR_Alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_brucks:
                mpi_errno = MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_pairwise:
                mpi_errno = MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcount, recvtype,
                                                         comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_pairwise_sendrecv_replace:
                mpi_errno = MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount,
                                                                          sendtype, recvbuf,
                                                                          recvcount, recvtype,
                                                                          comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_scattered:
                mpi_errno = MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm_ptr,
                                                       errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLTOALL_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_pairwise_exchange:
                mpi_errno = MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype,
                                                                  recvbuf, recvcount, recvtype,
                                                                  comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm_ptr,
                                                       errflag);
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

int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                          errflag);
    } else {
        mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       comm_ptr, errflag);
    }

    return mpi_errno;
}

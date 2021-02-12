/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_SCAN_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                 - Force nonblocking algorithm
        smp                - Force smp algorithm
        recursive_doubling - Force recursive doubling algorithm

    - name        : MPIR_CVAR_SCAN_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Scan will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


int MPIR_Scan_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCAN,
        .comm_ptr = comm_ptr,

        .u.scan.sendbuf = sendbuf,
        .u.scan.recvbuf = recvbuf,
        .u.scan.count = count,
        .u.scan.datatype = datatype,
        .u.scan.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_recursive_doubling:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_smp:
            mpi_errno =
                MPIR_Scan_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_allcomm_nb:
            mpi_errno =
                MPIR_Scan_allcomm_nb(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Scan_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    switch (MPIR_CVAR_SCAN_INTRA_ALGORITHM) {
        case MPIR_CVAR_SCAN_INTRA_ALGORITHM_recursive_doubling:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   errflag);
            break;
        case MPIR_CVAR_SCAN_INTRA_ALGORITHM_nb:
            mpi_errno =
                MPIR_Scan_allcomm_nb(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
        case MPIR_CVAR_SCAN_INTRA_ALGORITHM_smp:
            mpi_errno =
                MPIR_Scan_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
        case MPIR_CVAR_SCAN_INTRA_ALGORITHM_auto:
            mpi_errno =
                MPIR_Scan_allcomm_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
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

int MPIR_Scan(const void *sendbuf, void *recvbuf, MPI_Aint count,
              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
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
         MPIR_CVAR_SCAN_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Scan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }

    /* Copy out data from host recv buffer to GPU buffer */
    if (host_recvbuf) {
        recvbuf = in_recvbuf;
        MPIR_Localcopy(host_recvbuf, count, datatype, recvbuf, count, datatype);
    }

    MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);

    return mpi_errno;
}

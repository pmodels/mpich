/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ialltoallw/ialltoallw_tsp_blocked_algos_prototypes.h"
#include "../ialltoallw/ialltoallw_tsp_inplace_algos_prototypes.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_blocked           - Force blocked algorithm
        sched_inplace           - Force inplace algorithm
        gentran_blocked   - Force generic transport based blocked algorithm
        gentran_inplace   - Force generic transport based inplace algorithm

    - name        : MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_pairwise_exchange - Force pairwise exchange algorithm

    - name        : MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ialltoallw will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Ialltoallw_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                       const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                       void *recvbuf, const MPI_Aint * recvcounts,
                                       const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                       MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLTOALLW,
        .comm_ptr = comm_ptr,

        .u.ialltoallw.sendbuf = sendbuf,
        .u.ialltoallw.sendcounts = sendcounts,
        .u.ialltoallw.sdispls = sdispls,
        .u.ialltoallw.sendtypes = sendtypes,
        .u.ialltoallw.recvbuf = recvbuf,
        .u.ialltoallw.recvcounts = recvcounts,
        .u.ialltoallw.rdispls = rdispls,
        .u.ialltoallw.recvtypes = recvtypes,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_gentran_blocked:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoallw_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm_ptr,
                                                        cnt->u.ialltoallw.
                                                        intra_gentran_blocked.bblock, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_gentran_inplace:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallw_intra_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                         recvbuf, recvcounts, rdispls, recvtypes,
                                                         comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_blocked:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallw_intra_sched_blocked(sendbuf, sendcounts, sdispls, sendtypes,
                                                            recvbuf, recvcounts, rdispls, recvtypes,
                                                            comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_inplace:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallw_intra_sched_inplace(sendbuf, sendcounts, sdispls, sendtypes,
                                                            recvbuf, recvcounts, rdispls, recvtypes,
                                                            comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallw_inter_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                         recvbuf, recvcounts, rdispls, recvtypes,
                                                         comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_inter_sched_pairwise_exchange:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallw_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                      sendtypes, recvbuf,
                                                                      recvcounts, rdispls,
                                                                      recvtypes, comm_ptr,
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

int MPIR_Ialltoallw_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const MPI_Aint recvcounts[],
                                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Ialltoallw_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallw_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallw_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const MPI_Aint recvcounts[],
                                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallw_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                              sendtypes, recvbuf, recvcounts,
                                                              rdispls, recvtypes, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoallw_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                               const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                               void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                               const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ialltoallw_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                     sendtypes, recvbuf, recvcounts,
                                                     rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallw_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                     sendtypes, recvbuf, recvcounts,
                                                     rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallw_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                               const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                               void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                               const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                               bool is_persistent, void **sched_p,
                               enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_gentran_blocked:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallw gentran_blocked cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoallw_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtypes,
                                                            recvbuf, recvcounts, rdispls, recvtypes,
                                                            comm_ptr, MPIR_CVAR_ALLTOALL_THROTTLE,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_gentran_inplace:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf == MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallw gentran_inplace cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtypes,
                                                            recvbuf, recvcounts, rdispls, recvtypes,
                                                            comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_sched_blocked:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallw_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                                sendtypes, recvbuf, recvcounts,
                                                                rdispls, recvtypes, comm_ptr,
                                                                *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_sched_inplace:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallw_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                                sendtypes, recvbuf, recvcounts,
                                                                rdispls, recvtypes, comm_ptr,
                                                                *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallw_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                             sendtypes, recvbuf, recvcounts,
                                                             rdispls, recvtypes, comm_ptr,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallw_allcomm_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm_ptr, is_persistent, sched_p,
                                                       sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM_sched_pairwise_exchange:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallw_inter_sched_pairwise_exchange(sendbuf, sendcounts,
                                                                          sdispls, sendtypes,
                                                                          recvbuf, recvcounts,
                                                                          rdispls, recvtypes,
                                                                          comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallw_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                             sendtypes, recvbuf, recvcounts,
                                                             rdispls, recvtypes, comm_ptr,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallw_allcomm_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm_ptr, is_persistent, sched_p,
                                                       sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_CREATE_SCHED_P();
        mpi_errno = MPIR_Ialltoallw_intra_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                     recvbuf, recvcounts, rdispls, recvtypes,
                                                     comm_ptr, *sched_p);
    } else {
        MPII_SCHED_CREATE_SCHED_P();
        mpi_errno = MPIR_Ialltoallw_inter_sched_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                     recvbuf, recvcounts, rdispls, recvtypes,
                                                     comm_ptr, *sched_p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallw_impl(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                         const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Aint recvcounts[],
                         const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                         MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ialltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                           recvcounts, rdispls, recvtypes, comm_ptr, false, &sched,
                                           &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallw(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                    const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Aint recvcounts[],
                    const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                    MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                            recvtypes, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                         recvcounts, rdispls, recvtypes, comm_ptr, request);
    }

    return mpi_errno;
}

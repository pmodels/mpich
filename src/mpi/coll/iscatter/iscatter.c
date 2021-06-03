/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iscatter/iscatter_tsp_tree_algos_prototypes.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ISCATTER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iscatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_binomial     - Force binomial algorithm
        gentran_tree       - Force genetric transport based tree algorithm

    - name        : MPIR_CVAR_ISCATTER_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based iscatter

    - name        : MPIR_CVAR_ISCATTER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iscatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_linear                    - Force linear algorithm
        sched_remote_send_local_scatter - Force remote-send-local-scatter algorithm

    - name        : MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Iscatter will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* helper callbacks and associated state structures */
struct shared_state {
    int sendcount;
    int curr_count;
    MPI_Aint send_subtree_count;
    int nbytes;
    MPI_Status status;
};

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Iscatter_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm_ptr, bool is_persistent,
                                     void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ISCATTER,
        .comm_ptr = comm_ptr,

        .u.iscatter.sendbuf = sendbuf,
        .u.iscatter.sendcount = sendcount,
        .u.iscatter.sendtype = sendtype,
        .u.iscatter.recvcount = recvcount,
        .u.iscatter.recvbuf = recvbuf,
        .u.iscatter.recvtype = recvtype,
        .u.iscatter.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_gentran_tree:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iscatter_sched_intra_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, root, comm_ptr,
                                                   cnt->u.iscatter.intra_gentran_tree.k, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatter_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, root, comm_ptr,
                                                       *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_binomial:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatter_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, root, comm_ptr,
                                                           *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatter_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, root, comm_ptr,
                                                       *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_linear:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iscatter_inter_sched_linear(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, root, comm_ptr,
                                                         *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_remote_send_local_scatter:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Iscatter_inter_sched_remote_send_local_scatter(sendbuf, sendcount, sendtype,
                                                                    recvbuf, recvcount, recvtype,
                                                                    root, comm_ptr, *sched_p);
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

int MPIR_Iscatter_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatter_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int local_size, remote_size, sendtype_size, recvtype_size, nbytes;
    int mpi_errno = MPI_SUCCESS;

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Iscatter_inter_sched_remote_send_local_scatter(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype, root,
                                                                comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iscatter_inter_sched_linear(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root, comm_ptr,
                                                     s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Iscatter_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iscatter_inter_sched_auto(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Iscatter_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                             enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_ISCATTER_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_gentran_tree:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iscatter_sched_intra_tree(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, root, comm_ptr,
                                                       MPIR_CVAR_ISCATTER_TREE_KVAL, *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatter_intra_sched_binomial(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype, root,
                                                               comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatter_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, root, comm_ptr,
                                                           *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatter_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm_ptr,
                                                     is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_ISCATTER_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_linear:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatter_inter_sched_linear(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, root, comm_ptr,
                                                             *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_remote_send_local_scatter:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Iscatter_inter_sched_remote_send_local_scatter(sendbuf, sendcount,
                                                                        sendtype, recvbuf,
                                                                        recvcount, recvtype, root,
                                                                        comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iscatter_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, root, comm_ptr,
                                                           *sched_p);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatter_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm_ptr,
                                                     is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_impl(const void *sendbuf, MPI_Aint sendcount,
                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iscatter_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                  int root, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                          comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                       recvtype, root, comm_ptr, request);
    }

    return mpi_errno;
}

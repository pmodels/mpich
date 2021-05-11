/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ibcast/ibcast_tsp_tree_algos_prototypes.h"
#include "../ibcast/ibcast_tsp_scatterv_allgatherv_algos_prototypes.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IBCAST_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree (kary, knomial, etc.) based ibcast

    - name        : MPIR_CVAR_IBCAST_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for tree based ibcast
        kary      - kary tree type
        knomial_1 - knomial_1 tree type
        knomial_2 - knomial_2 tree type

    - name        : MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in tree based
        ibcast. Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IBCAST_RING_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in ibcast ring algorithm.
        Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IBCAST_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibcast algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_binomial                             - Force Binomial algorithm
        sched_smp                                  - Force smp algorithm
        sched_scatter_recursive_doubling_allgather - Force Scatter Recursive Doubling Allgather algorithm
        sched_scatter_ring_allgather               - Force Scatter Ring Allgather algorithm
        gentran_tree                               - Force Generic Transport Tree algorithm
        gentran_scatterv_recexch_allgatherv        - Force Generic Transport Scatterv followed by Recursive Exchange Allgatherv algorithm
        gentran_ring                               - Force Generic Transport Ring algorithm

    - name        : MPIR_CVAR_IBCAST_SCATTERV_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based scatter in scatter_recexch_allgather algorithm

    - name        : MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based allgather in scatter_recexch_allgather algorithm

    - name        : MPIR_CVAR_IBCAST_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibcast algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_flat - Force flat algorithm

    - name        : MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ibcast will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Provides a "flat" broadcast that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */

int MPIR_Ibcast_allcomm_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                   int root, MPIR_Comm * comm_ptr,
                                   bool is_persistent, void **sched_p,
                                   enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBCAST,
        .comm_ptr = comm_ptr,

        .u.ibcast.buffer = buffer,
        .u.ibcast.count = count,
        .u.ibcast.datatype = datatype,
        .u.ibcast.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_tree:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                 cnt->u.ibcast.intra_gentran_tree.tree_type,
                                 cnt->u.ibcast.intra_gentran_tree.k,
                                 cnt->u.ibcast.intra_gentran_tree.chunk_size,
                                 *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_scatterv_recexch_allgatherv:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                root, comm_ptr,
                                cnt->u.ibcast.intra_gentran_scatterv_recexch_allgatherv.scatterv_k,
                                cnt->u.ibcast.intra_gentran_scatterv_recexch_allgatherv.allgatherv_k,
                                *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                root, comm_ptr,
                                MPIR_TREE_TYPE_KARY, 1,
                                cnt->u.ibcast.intra_gentran_ring.chunk_size,
                                *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_auto(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_binomial:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_ring_allgather:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_smp:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_inter_sched_auto(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_flat:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
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

int MPIR_Ibcast_intra_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint type_size, nbytes;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT) {
        mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPL_is_pof2(comm_size, NULL))) {
            mpi_errno =
                MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count,
                                                                             datatype, root,
                                                                             comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno =
                MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root,
                                                               comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Provides a "flat" broadcast for intercommunicators that doesn't
 * know anything about hierarchy.  It will choose between several
 * different algorithms based on the given parameters. */
int MPIR_Ibcast_inter_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ibcast_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ibcast_intra_sched_auto(buffer, count, datatype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibcast_inter_sched_auto(buffer, count, datatype, root, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ibcast_sched_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, bool is_persistent,
                           void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    /* *INDENT-OFF* */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_tree:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_Ibcast_tree_type,
                                                    MPIR_CVAR_IBCAST_TREE_KVAL,
                                                    MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_scatterv_recexch_allgatherv:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_CVAR_IBCAST_SCATTERV_KVAL,
                                                    MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_TREE_TYPE_KARY, 1,
                                                    MPIR_CVAR_IBCAST_RING_CHUNK_SIZE,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_smp:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_scatter_recursive_doubling_allgather:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_scatter_ring_allgather:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_auto(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Ibcast_allcomm_sched_auto(buffer, count, datatype, root, comm_ptr,
                                                           is_persistent, sched_p, sched_type_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IBCAST_INTER_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_sched_flat:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_inter_sched_auto(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Ibcast_allcomm_sched_auto(buffer, count, datatype, root, comm_ptr,
                                                           is_persistent, sched_p, sched_type_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            default:
                MPIR_Assert(0);
        }
    }
    /* *INDENT-ON* */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibcast_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                     MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ibcast_sched_impl(buffer, count, datatype, root, comm_ptr,
                                       false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibcast(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Ibcast(buffer, count, datatype, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, request);
    }

    return mpi_errno;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4_proc.h"
#include "ch4_coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BARRIER_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Barrier
        0 Auto selection
        1 NM + SHM
        2 NM only

    - name        : MPIR_CVAR_BCAST_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Bcast
        0 Auto selection
        1 NM + SHM with explicit send-recv between rank 0 and root
        2 NM + SHM without the explicit send-recv
        3 NM only

    - name        : MPIR_CVAR_ALLREDUCE_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Allreduce
        0 Auto selection
        1 NM + SHM with reduce + bcast
        2 NM only composition
        3 SHM only composition
        4 Multi leaders based inter node + intra node composition

    - name        : MPIR_CVAR_ALLGATHER_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Allgather
        0 Auto selection
        1 Multi leaders based inter node + intra node composition
        2 NM only composition

    - name        : MPIR_CVAR_ALLTOALL_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Alltoall
        0 Auto selection
        1 Multi leaders based inter node + intra node composition
        2 NM only composition

    - name        : MPIR_CVAR_REDUCE_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Reduce
        0 Auto selection
        1 NM + SHM with explicit send-recv between rank 0 and root
        2 NM + SHM without the explicit send-recv
        3 NM only

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_allcomm_composition_json(MPIR_Comm * comm,
                                                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BARRIER,
        .comm_ptr = comm,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Barrier_impl(comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_alpha:
            mpi_errno = MPIDI_Barrier_intra_composition_alpha(comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_beta:
            mpi_errno = MPIDI_Barrier_intra_composition_beta(comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;


    switch (MPIR_CVAR_BARRIER_COMPOSITION) {
        case 1:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) &&
                                           (comm->hierarchy_kind ==
                                            MPIR_COMM_HIERARCHY_KIND__PARENT), mpi_errno,
                                           "Barrier composition alpha cannot be applied.\n");
            mpi_errno = MPIDI_Barrier_intra_composition_alpha(comm, errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Barrier composition beta cannot be applied.\n");
            mpi_errno = MPIDI_Barrier_intra_composition_beta(comm, errflag);
            break;
        default:
            mpi_errno = MPIDI_Barrier_allcomm_composition_json(comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno = MPIR_Barrier_impl(comm, errflag);
    else
        mpi_errno = MPIDI_Barrier_intra_composition_beta(comm, errflag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_allcomm_composition_json(void *buffer, MPI_Aint count,
                                                                  MPI_Datatype datatype, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BCAST,
        .comm_ptr = comm,
        .u.bcast.buffer = buffer,
        .u.bcast.count = count,
        .u.bcast.datatype = datatype,
        .u.bcast.root = root,
    };

    const MPIDI_Csel_container_s *cnt = NULL;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_alpha:
            mpi_errno =
                MPIDI_Bcast_intra_composition_alpha(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_beta:
            mpi_errno =
                MPIDI_Bcast_intra_composition_beta(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_gamma:
            mpi_errno =
                MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (MPIR_CVAR_BCAST_COMPOSITION) {
        case 1:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) &&
                                           (comm->hierarchy_kind ==
                                            MPIR_COMM_HIERARCHY_KIND__PARENT), mpi_errno,
                                           "Bcast composition alpha cannot be applied.\n");
            mpi_errno =
                MPIDI_Bcast_intra_composition_alpha(buffer, count, datatype, root, comm, errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) &&
                                           (comm->hierarchy_kind ==
                                            MPIR_COMM_HIERARCHY_KIND__PARENT), mpi_errno,
                                           "Bcast composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Bcast_intra_composition_beta(buffer, count, datatype, root, comm, errflag);
            break;
        case 3:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Bcast composition gamma cannot be applied.\n");
            mpi_errno =
                MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm, errflag);
            break;
        default:
            mpi_errno =
                MPIDI_Bcast_allcomm_composition_json(buffer, count, datatype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
    else
        mpi_errno =
            MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm, errflag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Allreduce_fill_multi_leads_info(MPIR_Comm * comm)
{
    int node_comm_size = 0, num_nodes;
    bool node_balanced = false;

    if (MPIDI_COMM(comm, allreduce_comp_info) == NULL) {
        /* If multi-leads allreduce is enabled and the info object is not yet set */
        MPIDI_COMM(comm, allreduce_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, allreduce_comp_info));
        MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLREDUCE(comm, shm_addr) = NULL;
        MPIDI_COMM(comm, shm_size_per_lead) = 1;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) == -1) {
        if (comm->node_comm)
            node_comm_size = MPIR_Comm_size(comm->node_comm);
        /* Get number of nodes it spans over and if it has equal number of ranks per node */
        MPII_Comm_is_node_balanced(comm, &num_nodes, &node_balanced);
        MPIDI_COMM(comm, spanned_num_nodes) = num_nodes;

        if (num_nodes > 1 && node_comm_size > 1 && node_balanced) {
            MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = 1;
        } else {
            MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = 0;
        }
    }

}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_allcomm_composition_json(const void *sendbuf,
                                                                      void *recvbuf, MPI_Aint count,
                                                                      MPI_Datatype datatype,
                                                                      MPI_Op op, MPIR_Comm * comm,
                                                                      MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;
    int num_leads = 0, node_comm_size = 0;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLREDUCE,
        .comm_ptr = comm,

        .u.allreduce.sendbuf = sendbuf,
        .u.allreduce.recvbuf = recvbuf,
        .u.allreduce.count = count,
        .u.allreduce.datatype = datatype,
        .u.allreduce.op = op,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                       comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_delta:
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIDI_Allreduce_fill_multi_leads_info(comm);
                if (comm->node_comm)
                    node_comm_size = MPIR_Comm_size(comm->node_comm);
                /* Reset number of leaders, so that (node_comm_size % num_leads) is zero. The new number of
                 * leaders must lie within a range +/- <num> from the leaders specified, or every rank is made
                 * as a leader. Currently we use range <num> as 15. */
                num_leads =
                    MPL_round_closest_multiple(node_comm_size, MPIR_CVAR_NUM_MULTI_LEADS, 15);
            }
            /* make sure that the algo can be run */
            if (MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) == 1 &&
                comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                count >= num_leads && MPIR_Op_is_commutative(op)) {
                mpi_errno =
                    MPIDI_Allreduce_intra_composition_delta(sendbuf, recvbuf, count, datatype, op,
                                                            num_leads, comm, errflag);
            } else
                mpi_errno =
                    MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = -1;
    int num_leads = 0, node_comm_size = 0;

    MPIR_FUNC_ENTER;

    is_commutative = MPIR_Op_is_commutative(op);

    switch (MPIR_CVAR_ALLREDUCE_COMPOSITION) {
        case 1:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) &&
                                           (comm->hierarchy_kind ==
                                            MPIR_COMM_HIERARCHY_KIND__PARENT) &&
                                           is_commutative, mpi_errno,
                                           "Allreduce composition alpha cannot be applied.\n");
            mpi_errno =
                MPIDI_Allreduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op, comm,
                                                        errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Allreduce composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op, comm,
                                                       errflag);
            break;
        case 3:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) &&
                                           (comm->node_comm != NULL) &&
                                           (MPIR_Comm_size(comm) ==
                                            MPIR_Comm_size(comm->node_comm)), mpi_errno,
                                           "Allreduce composition gamma cannot be applied.\n");
            mpi_errno =
                MPIDI_Allreduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op, comm,
                                                        errflag);
            break;
        case 4:
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIDI_Allreduce_fill_multi_leads_info(comm);
                if (comm->node_comm)
                    node_comm_size = MPIR_Comm_size(comm->node_comm);
                /* Reset number of leaders, so that (node_comm_size % num_leads) is zero. The new number of
                 * leaders must lie within a range +/- <num> from the leaders specified, or every rank is made
                 * as a leader. Currently we use range <num> as 15. */
                num_leads =
                    MPL_round_closest_multiple(node_comm_size, MPIR_CVAR_NUM_MULTI_LEADS, 15);
            }

            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, comm->comm_kind == MPIR_COMM_KIND__INTRACOMM
                                           && MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) == 1 &&
                                           count >= num_leads && is_commutative, mpi_errno,
                                           "Allreduce composition delta cannot be applied.\n");
            /* Multi-leaders based composition can only be used if the comm is spanned over more than
             * 1 node, has equal number of ranks on each node, count is more than number of leaders and
             * the operation is commutative. This composition is beneficial for large messages.
             */

            mpi_errno =
                MPIDI_Allreduce_intra_composition_delta(sendbuf, recvbuf, count, datatype, op,
                                                        num_leads, comm, errflag);
            break;

        default:
            mpi_errno =
                MPIDI_Allreduce_allcomm_composition_json(sendbuf, recvbuf, count, datatype, op,
                                                         comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    else
        mpi_errno =
            MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op, comm,
                                                   errflag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Allgather_fill_multi_leads_info(MPIR_Comm * comm)
{
    int node_comm_size = 0, num_nodes;
    bool node_balanced = false;

    if (MPIDI_COMM(comm, allgather_comp_info) == NULL) {
        MPIDI_COMM(comm, allgather_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, allgather_comp_info));
        MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLGATHER(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == -1) {
        if (comm->node_comm)
            node_comm_size = MPIR_Comm_size(comm->node_comm);
        /* Get number of nodes it spans over and if it has equal number of ranks per node */
        MPII_Comm_is_node_balanced(comm, &num_nodes, &node_balanced);
        MPIDI_COMM(comm, spanned_num_nodes) = num_nodes;

        if (num_nodes > 1 && node_comm_size > 1 && node_balanced &&
            MPII_Comm_is_node_consecutive(comm))
            MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = 1;
        else
            MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = 0;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_allcomm_composition_json(const void *sendbuf,
                                                                      MPI_Aint sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      MPI_Aint recvcount,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm,
                                                                      MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHER,
        .comm_ptr = comm,

        .u.allgather.sendbuf = sendbuf,
        .u.allgather.sendcount = sendcount,
        .u.allgather.sendtype = sendtype,
        .u.allgather.recvbuf = recvbuf,
        .u.allgather.recvcount = recvcount,
        .u.allgather.recvtype = recvtype,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                                errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha:
            MPIDI_Allgather_fill_multi_leads_info(comm);
            /* make sure that the algo can be run */
            if (MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == 1) {
                mpi_errno =
                    MPIDI_Allgather_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                            recvbuf, recvcount, recvtype,
                                                            comm, errflag);
            } else
                mpi_errno =
                    MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_beta:
            mpi_errno =
                MPIDI_Allgather_intra_composition_beta(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, MPI_Aint sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            MPI_Aint recvcount, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;

    MPIR_FUNC_ENTER;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
    }

    switch (MPIR_CVAR_ALLGATHER_COMPOSITION) {
        case 1:
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                MPIDI_Allgather_fill_multi_leads_info(comm);

            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == 1 &&
                                           (MPL_MAX(sendcount, recvcount) * type_size <=
                                            MPIR_CVAR_ALLGATHER_SHM_PER_RANK), mpi_errno,
                                           "Allgather composition alpha cannot be applied.\n");
            /* Multi-leaders based composition can only be used if the comm is spanned over more than
             * 1 node, has equal number of ranks on each node, ranks on a node are consecutive and
             * the combined msg from all the ranks on a node fits the allocated shared memory buffer
             */

            mpi_errno =
                MPIDI_Allgather_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Allgather composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Allgather_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIDI_Allgather_allcomm_composition_json(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype, comm,
                                                                 errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno =
            MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                                errflag);
    else
        mpi_errno =
            MPIDI_Allgather_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm, errflag);

  fn_exit:
    MPIR_FUNC_ENTER;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHERV,
        .comm_ptr = comm,

        .u.allgatherv.sendbuf = sendbuf,
        .u.allgatherv.sendcount = sendcount,
        .u.allgatherv.sendtype = sendtype,
        .u.allgatherv.recvbuf = recvbuf,
        .u.allgatherv.recvcounts = recvcounts,
        .u.allgatherv.displs = displs,
        .u.allgatherv.recvtype = recvtype,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgatherv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allgatherv_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCATTER,
        .comm_ptr = comm,

        .u.scatter.sendbuf = sendbuf,
        .u.scatter.sendcount = sendcount,
        .u.scatter.sendtype = sendtype,
        .u.scatter.recvcount = recvcount,
        .u.scatter.recvbuf = recvbuf,
        .u.scatter.recvtype = recvtype,
        .u.scatter.root = root,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm,
                          errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatter_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scatter_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const MPI_Aint * sendcounts,
                                           const MPI_Aint * displs, MPI_Datatype sendtype,
                                           void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCATTERV,
        .comm_ptr = comm,

        .u.scatterv.sendbuf = sendbuf,
        .u.scatterv.sendcounts = sendcounts,
        .u.scatterv.displs = displs,
        .u.scatterv.sendtype = sendtype,
        .u.scatterv.recvcount = recvcount,
        .u.scatterv.recvbuf = recvbuf,
        .u.scatterv.recvtype = recvtype,
        .u.scatterv.root = root,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype,
                           root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatterv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scatterv_intra_composition_alpha(sendbuf, sendcounts, displs, sendtype,
                                                       recvbuf, recvcount, recvtype, root,
                                                       comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, MPI_Aint sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                         MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                         MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHER,
        .comm_ptr = comm,

        .u.gather.sendbuf = sendbuf,
        .u.gather.sendcount = sendcount,
        .u.gather.sendtype = sendtype,
        .u.gather.recvcount = recvcount,
        .u.gather.recvbuf = recvbuf,
        .u.gather.recvtype = recvtype,
        .u.gather.root = root,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm,
                             errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gather_intra_composition_alpha:
            mpi_errno =
                MPIDI_Gather_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHERV,
        .comm_ptr = comm,

        .u.gatherv.sendbuf = sendbuf,
        .u.gatherv.sendcount = sendcount,
        .u.gatherv.sendtype = sendtype,
        .u.gatherv.recvbuf = recvbuf,
        .u.gatherv.recvcounts = recvcounts,
        .u.gatherv.displs = displs,
        .u.gatherv.recvtype = recvtype,
        .u.gatherv.root = root,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                      displs, recvtype, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gatherv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Gatherv_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, root,
                                                      comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Alltoall_fill_multi_leads_info(MPIR_Comm * comm)
{
    int node_comm_size = 0, num_nodes;
    bool node_balanced = false;

    if (MPIDI_COMM(comm, alltoall_comp_info) == NULL) {
        MPIDI_COMM(comm, alltoall_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, alltoall_comp_info));
        MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLTOALL(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == -1) {
        if (comm->node_comm)
            node_comm_size = MPIR_Comm_size(comm->node_comm);
        /* Get number of nodes it spans over and if it has equal number of ranks per node */
        MPII_Comm_is_node_balanced(comm, &num_nodes, &node_balanced);
        MPIDI_COMM(comm, spanned_num_nodes) = num_nodes;

        if (num_nodes > 1 && node_comm_size > 1 && node_balanced &&
            MPII_Comm_is_node_consecutive(comm))
            MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = 1;
        else
            MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = 0;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_allcomm_composition_json(const void *sendbuf,
                                                                     MPI_Aint sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     MPI_Aint recvcount,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALL,
        .comm_ptr = comm,

        .u.alltoall.sendbuf = sendbuf,
        .u.alltoall.sendcount = sendcount,
        .u.alltoall.sendtype = sendtype,
        .u.alltoall.recvcount = recvcount,
        .u.alltoall.recvbuf = recvbuf,
        .u.alltoall.recvtype = recvtype,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_alpha:
            MPIDI_Alltoall_fill_multi_leads_info(comm);
            /* make sure that the algo can be run */
            if (MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == 1) {
                mpi_errno =
                    MPIDI_Alltoall_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                           recvbuf, recvcount, recvtype,
                                                           comm, errflag);
            } else
                mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_beta:
            mpi_errno =
                MPIDI_Alltoall_intra_composition_beta(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, MPI_Aint sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;

    MPIR_FUNC_ENTER;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
    }

    switch (MPIR_CVAR_ALLTOALL_COMPOSITION) {
        case 1:
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                MPIDI_Alltoall_fill_multi_leads_info(comm);

            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, comm->comm_kind == MPIR_COMM_KIND__INTRACOMM
                                           && MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == 1 &&
                                           (MPL_MAX(sendcount, recvcount) * type_size <=
                                            MPIR_CVAR_ALLTOALL_SHM_PER_RANK), mpi_errno,
                                           "Alltoall composition alpha cannot be applied.\n");
            /* Multi-leaders based composition can only be used if the comm is spanned over more than
             * 1 node, has equal number of ranks on each node, ranks on a node are consecutive and
             * the combined msg from all the ranks on a node fits the allocated shared memory buffer
             */

            mpi_errno =
                MPIDI_Alltoall_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Alltoall composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Alltoall_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIDI_Alltoall_allcomm_composition_json(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype, comm,
                                                                errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno =
            MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                               errflag);
    else
        mpi_errno = MPIDI_Alltoall_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, comm, errflag);

  fn_exit:
    MPIR_FUNC_ENTER;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const MPI_Aint * sendcounts,
                                            const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const MPI_Aint * recvcounts,
                                            const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLV,
        .comm_ptr = comm,

        .u.alltoallv.sendbuf = sendbuf,
        .u.alltoallv.sendcounts = sendcounts,
        .u.alltoallv.sdispls = sdispls,
        .u.alltoallv.sendtype = sendtype,
        .u.alltoallv.recvbuf = recvbuf,
        .u.alltoallv.recvcounts = recvcounts,
        .u.alltoallv.rdispls = rdispls,
        .u.alltoallv.recvtype = recvtype,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                        sendtype, recvbuf, recvcounts,
                                        rdispls, recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Alltoallv_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[],
                                            const MPI_Datatype sendtypes[], void *recvbuf,
                                            const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                            const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                            MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLW,
        .comm_ptr = comm,

        .u.alltoallw.sendbuf = sendbuf,
        .u.alltoallw.sendcounts = sendcounts,
        .u.alltoallw.sdispls = sdispls,
        .u.alltoallw.sendtypes = sendtypes,
        .u.alltoallw.recvbuf = recvbuf,
        .u.alltoallw.recvcounts = recvcounts,
        .u.alltoallw.rdispls = rdispls,
        .u.alltoallw.recvtypes = recvtypes,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts,
                                        rdispls, recvtypes, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallw_intra_composition_alpha:
            mpi_errno =
                MPIDI_Alltoallw_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_allcomm_composition_json(const void *sendbuf,
                                                                   void *recvbuf, MPI_Aint count,
                                                                   MPI_Datatype datatype, MPI_Op op,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE,
        .comm_ptr = comm,

        .u.reduce.sendbuf = sendbuf,
        .u.reduce.recvbuf = recvbuf,
        .u.reduce.count = count,
        .u.reduce.datatype = datatype,
        .u.reduce.op = op,
        .u.reduce.root = root,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Reduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                    root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag);
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

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                         MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (MPIR_CVAR_REDUCE_COMPOSITION) {
        case 1:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, comm->comm_kind == MPIR_COMM_KIND__INTRACOMM
                                           && comm->hierarchy_kind ==
                                           MPIR_COMM_HIERARCHY_KIND__PARENT &&
                                           MPIR_Op_is_commutative(op), mpi_errno,
                                           "Reduce composition alpha cannot be applied.\n");
            mpi_errno =
                MPIDI_Reduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op, root,
                                                     comm, errflag);
            break;
        case 2:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, comm->comm_kind == MPIR_COMM_KIND__INTRACOMM
                                           && comm->hierarchy_kind ==
                                           MPIR_COMM_HIERARCHY_KIND__PARENT &&
                                           MPIR_Op_is_commutative(op), mpi_errno,
                                           "Reduce composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Reduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm, errflag);
            break;
        case 3:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Reduce composition gamma cannot be applied.\n");
            mpi_errno =
                MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op, root,
                                                     comm, errflag);
            break;
        default:
            mpi_errno =
                MPIDI_Reduce_allcomm_composition_json(sendbuf, recvbuf, count, datatype, op,
                                                      root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
    else
        mpi_errno =
            MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op, root, comm,
                                                 errflag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                 const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER,
        .comm_ptr = comm,

        .u.reduce_scatter.sendbuf = sendbuf,
        .u.reduce_scatter.recvbuf = recvbuf,
        .u.reduce_scatter.recvcounts = recvcounts,
        .u.reduce_scatter.datatype = datatype,
        .u.reduce_scatter.op = op,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_scatter_intra_composition_alpha(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                       MPI_Aint recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
        .comm_ptr = comm,

        .u.reduce_scatter_block.sendbuf = sendbuf,
        .u.reduce_scatter_block.recvbuf = recvbuf,
        .u.reduce_scatter_block.recvcount = recvcount,
        .u.reduce_scatter_block.datatype = datatype,
        .u.reduce_scatter_block.op = op,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                           errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_block_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_scatter_block_intra_composition_alpha(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCAN,
        .comm_ptr = comm,

        .u.scan.sendbuf = sendbuf,
        .u.scan.recvbuf = recvbuf,
        .u.scan.count = count,
        .u.scan.datatype = datatype,
        .u.scan.op = op,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_beta:
            mpi_errno =
                MPIDI_Scan_intra_composition_beta(sendbuf, recvbuf, count,
                                                  datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__EXSCAN,
        .comm_ptr = comm,

        .u.exscan.sendbuf = sendbuf,
        .u.exscan.recvbuf = recvbuf,
        .u.exscan.count = count,
        .u.exscan.datatype = datatype,
        .u.exscan.op = op,
    };

    MPIR_FUNC_ENTER;

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);;
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Exscan_intra_composition_alpha:
            mpi_errno =
                MPIDI_Exscan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                     datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     const MPI_Datatype * sendtypes, void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     const MPI_Datatype * recvtypes,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *sendbuf, MPI_Aint sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      MPI_Aint recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       const MPI_Aint * displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *sendbuf,
                                                      const MPI_Aint * sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *sendbuf,
                                                      const MPI_Aint * sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             MPI_Aint recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *sendbuf, MPI_Aint sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *sendbuf, MPI_Aint sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            MPI_Aint recvcount, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *sendbuf, const MPI_Aint * sendcounts,
                                             const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const MPI_Aint * recvcounts,
                                             const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *sendbuf, const MPI_Aint * sendcounts,
                                             const MPI_Aint * sdispls,
                                             const MPI_Datatype * sendtypes, void *recvbuf,
                                             const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                             const MPI_Datatype * recvtypes, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *sendbuf, MPI_Aint sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        MPI_Aint recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm,
                                                        MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const MPI_Aint * recvcounts,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *sendbuf, MPI_Aint sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *sendbuf, const MPI_Aint * sendcounts,
                                            const MPI_Aint * displs, MPI_Datatype sendtype,
                                            void *recvbuf, MPI_Aint recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */

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

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_allcomm_composition_json(MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BARRIER,
        .comm_ptr = comm,
    };

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Barrier_impl(comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_alpha:
            mpi_errno = MPIDI_Barrier_intra_composition_alpha(comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_beta:
            mpi_errno = MPIDI_Barrier_intra_composition_beta(comm, coll_attr);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_allcomm_composition_json(void *buffer, MPI_Aint count,
                                                                  MPI_Datatype datatype, int root,
                                                                  MPIR_Comm * comm, int coll_attr)
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

    if (MPIR_CVAR_COLL_HYBRID_MEMORY) {
        cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);
    } else {
        /* In no hybird case, local memory type can be used to select algorithm */
        MPL_pointer_attr_t pointer_attr;
        MPIR_GPU_query_pointer_attr(buffer, &pointer_attr);
        if (MPL_gpu_attr_is_strict_dev(&pointer_attr)) {
            cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm_gpu), coll_sig);
        } else {
            cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);
        }
    }
    if (cnt == NULL) {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_alpha:
            mpi_errno =
                MPIDI_Bcast_intra_composition_alpha(buffer, count, datatype, root, comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_beta:
            mpi_errno =
                MPIDI_Bcast_intra_composition_beta(buffer, count, datatype, root, comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_gamma:
            mpi_errno =
                MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_delta:
            mpi_errno =
                MPIDI_Bcast_intra_composition_delta(buffer, count, datatype, root, comm, coll_attr);
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

MPL_STATIC_INLINE_PREFIX void MPIDI_Allreduce_fill_multi_leads_info(MPIR_Comm * comm)
{
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
        if (MPII_Comm_is_node_balanced(comm)) {
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
                                                                      int coll_attr)
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
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                        comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                       comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                        comm, coll_attr);
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
                                                            num_leads, comm, coll_attr);
            } else
                mpi_errno =
                    MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, coll_attr);
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

MPL_STATIC_INLINE_PREFIX void MPIDI_Allgather_fill_multi_leads_info(MPIR_Comm * comm)
{
    if (MPIDI_COMM(comm, allgather_comp_info) == NULL) {
        MPIDI_COMM(comm, allgather_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, allgather_comp_info));
        MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLGATHER(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == -1) {
        if (MPII_Comm_is_node_canonical(comm))
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
                                                                      int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size, data_size;
    const MPIDI_Csel_container_s *cnt = NULL;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
        data_size = sendcount * type_size;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
        data_size = recvcount * type_size;
    }

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
                                coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha:
            /* make sure that the algo can be run */
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                MPIDI_Allgather_fill_multi_leads_info(comm);
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == 1 &&
                                           data_size <= MPIR_CVAR_ALLGATHER_SHM_PER_RANK, mpi_errno,
                                           "Allgather composition alpha cannot be applied.\n");
            mpi_errno =
                MPIDI_Allgather_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_beta:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Allgather composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Allgather_intra_composition_beta(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm,
                                                       coll_attr);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno =
            MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                                coll_attr);
    else
        mpi_errno =
            MPIDI_Allgather_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm, coll_attr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Alltoall_fill_multi_leads_info(MPIR_Comm * comm)
{
    if (MPIDI_COMM(comm, alltoall_comp_info) == NULL) {
        MPIDI_COMM(comm, alltoall_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, alltoall_comp_info));
        MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLTOALL(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == -1) {
        if (MPII_Comm_is_node_canonical(comm))
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
                                                                     int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size, data_size;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
        data_size = sendcount * type_size;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
        data_size = recvcount * type_size;
    }

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
                                       recvbuf, recvcount, recvtype, comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_alpha:
            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                MPIDI_Alltoall_fill_multi_leads_info(comm);
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, comm->comm_kind == MPIR_COMM_KIND__INTRACOMM
                                           && MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == 1 &&
                                           data_size <= MPIR_CVAR_ALLTOALL_SHM_PER_RANK, mpi_errno,
                                           "Alltoall composition alpha cannot be applied.\n");
            mpi_errno =
                MPIDI_Alltoall_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm,
                                                       coll_attr);
            break;

        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_beta:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank,
                                           comm->comm_kind == MPIR_COMM_KIND__INTRACOMM, mpi_errno,
                                           "Alltoall composition beta cannot be applied.\n");
            mpi_errno =
                MPIDI_Alltoall_intra_composition_beta(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, comm,
                                                      coll_attr);
            break;

        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        mpi_errno =
            MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                               coll_attr);
    else
        mpi_errno = MPIDI_Alltoall_intra_composition_beta(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, comm, coll_attr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_allcomm_composition_json(const void *sendbuf,
                                                                   void *recvbuf, MPI_Aint count,
                                                                   MPI_Datatype datatype, MPI_Op op,
                                                                   int root, MPIR_Comm * comm,
                                                                   int coll_attr)
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
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Reduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                    root, comm, coll_attr);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, coll_attr);
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

#endif /* CH4_COLL_H_INCLUDED */

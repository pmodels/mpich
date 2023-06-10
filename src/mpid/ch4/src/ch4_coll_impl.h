/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALL_SHM_PER_RANK
      category    : COLLECTIVE
      type        : int
      default     : 4096
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Shared memory region per rank for multi-leaders based composition for MPI_Alltoall (in bytes)

    - name        : MPIR_CVAR_ALLGATHER_SHM_PER_RANK
      category    : COLLECTIVE
      type        : int
      default     : 4096
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Shared memory region per rank for multi-leaders based composition for MPI_Allgather (in bytes)

    - name        : MPIR_CVAR_NUM_MULTI_LEADS
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of leader ranks per node to be used for multi-leaders based collective algorithms

    - name        : MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Shared memory region per node-leader for multi-leaders based composition for MPI_Allreduce (in bytes)
        If it is undefined by the user, it is set to the message size of the first call to the algorithm.
        Max shared memory size is limited to 4MB.

    - name        : MPIR_CVAR_ALLREDUCE_CACHE_PER_LEADER
      category    : COLLECTIVE
      type        : int
      default     : 512
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Amount of data reduced in allreduce delta composition's reduce local step (in bytes). Smaller msg size
        per leader avoids cache misses and improves performance. Experiments indicate 512 to be the best value.

    - name        : MPIR_CVAR_ALLREDUCE_LOCAL_COPY_OFFSETS
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        number of offsets in the allreduce delta composition's local copy
        The value of 2 performed the best in our 2 NIC test cases.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef CH4_COLL_IMPL_H_INCLUDED
#define CH4_COLL_IMPL_H_INCLUDED

#include "ch4_csel_container.h"
#include "ch4_comm.h"
#include "algo_common.h"

#define MPIR_ALLREDUCE_SHM_PER_LEADER_MAX 4194304

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_alpha(MPIR_Comm * comm,
                                                                   MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_barrier(comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_barrier(comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        coll_ret = MPIDI_NM_mpi_barrier(comm->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (comm->node_comm != NULL) {
        int i = 0;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_beta(MPIR_Comm * comm,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_barrier(comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_alpha(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    MPI_Aint nbytes, type_size, recvd_size;
#endif

    if (comm->node_roots_comm == NULL && comm->rank == root) {
        coll_ret = MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    if (comm->node_roots_comm != NULL && comm->rank != root &&
        MPIR_Get_intranode_rank(comm, root) != -1) {
#ifndef HAVE_ERROR_CHECKING
        coll_ret =
            MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                      comm->node_comm, MPI_STATUS_IGNORE);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret =
            MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                      comm->node_comm, &status);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size * count;
        /* check that we received as much as we expected */
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        if (recvd_size != nbytes) {
            MPIR_ERR_SET2(coll_ret, MPI_ERR_OTHER,
                          "**collective_size_mismatch",
                          "**collective_size_mismatch %d %d", recvd_size, nbytes);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }
#endif
    }

    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_beta(void *buffer, MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                int root, MPIR_Comm * comm,
                                                                MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) > 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                               comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }
    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) <= 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_gamma(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_alpha(const void *sendbuf,
                                                                     void *recvbuf, MPI_Aint count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    if (comm->node_comm != NULL) {
        if ((sendbuf == MPI_IN_PLACE) && (comm->node_comm->rank != 0)) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm,
                                     errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
            coll_ret =
                MPIDI_NM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm,
                                    errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        } else {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                     errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
            coll_ret =
                MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                    errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }
    } else {
        if (sendbuf != MPI_IN_PLACE) {
            coll_ret = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }
    }

    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op,
                                   comm->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_beta(const void *sendbuf,
                                                                    void *recvbuf, MPI_Aint count,
                                                                    MPI_Datatype datatype,
                                                                    MPI_Op op,
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_gamma(const void *sendbuf,
                                                                     void *recvbuf, MPI_Aint count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
#else
    mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Multi-leaders based composition. Have num_leaders per node, which reduce the data within
 * sub-node_comm. It is followed by intra_node reduce and inter_node allreduce on the piece of data
 * the leader is responsible for. A shared memory buffer is allocated per leader. If size of
 * message exceeds this shm buffer, the message is chunked.
 * Constraints: For a comm, all nodes should have same number of ranks per node, op should be
 * commutative.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_delta(const void *sendbuf,
                                                                     void *recvbuf, int count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     int num_leads,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, coll_ret = MPI_SUCCESS;
    char *shm_addr;
    int my_leader_rank = -1, iter;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0, i;
    MPI_Aint lb, true_extent, extent;
    int num_offsets = MPIR_CVAR_ALLREDUCE_LOCAL_COPY_OFFSETS;
    int local_copy_rank = MPIR_Comm_rank(comm_ptr->node_comm);
    int local_copy_offset = 0;
    int local_copy_group = 0;
    int shm_size_per_lead = MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER;

    MPIR_Type_get_extent_impl(datatype, &lb, &extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    if (sendbuf == MPI_IN_PLACE)
        sendbuf = recvbuf;

    if (MPIDI_COMM(comm_ptr, sub_node_comm) == NULL) {
        /* Create multi-leaders comm in a lazily */
        coll_ret = MPIDI_Comm_create_multi_leader_subcomms(comm_ptr, num_leads);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Allocate the shared memory buffer per node, if it is not already done */
    if (MPIDI_COMM(comm_ptr, allreduce_comp_info->shm_addr) == NULL) {
        /* Determine the shm_size_per_lead */
        /* If user didn't set anything, set shm_size_per_lead according to the first call */
        /* since CVAR is set only once this check should only happen once during the course of
         * execution. */
        if (shm_size_per_lead == -1) {
            MPI_Aint packsize;
            MPIR_Pack_size(count, datatype, &packsize);
            /* TODO: should we set a minimum size, and potentially a sane maximum? */
            MPIR_Assert(packsize <= INT_MAX);
            shm_size_per_lead = (int) packsize;
        }
        /* Do not create shm_size_per_lead buffers greater than 4MB. */
        if (shm_size_per_lead > MPIR_ALLREDUCE_SHM_PER_LEADER_MAX) {
            shm_size_per_lead = MPIR_ALLREDUCE_SHM_PER_LEADER_MAX;
        }
        MPIDI_COMM(comm_ptr, shm_size_per_lead) = shm_size_per_lead;

        coll_ret = MPIDU_shm_alloc(comm_ptr->node_comm, num_leads * shm_size_per_lead,
                                   (void **) &MPIDI_COMM_ALLREDUCE(comm_ptr, shm_addr));
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Store the address of shared buffer into a local variable */
    shm_addr = MPIDI_COMM_ALLREDUCE(comm_ptr, shm_addr);
    /* Retrieve the shm_size_per_lead for subsequent calls */
    shm_size_per_lead = MPIDI_COMM(comm_ptr, shm_size_per_lead);

    if (MPIDI_COMM(comm_ptr, intra_node_leads_comm) != NULL) {
        my_leader_rank = MPIR_Comm_rank(MPIDI_COMM(comm_ptr, intra_node_leads_comm));
    }

    /* Calculate chunking information. Extent handles contiguous and non-contiguous datatypes both */
    MPIR_Algo_calculate_pipeline_chunk_info(shm_size_per_lead,
                                            extent, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    for (iter = 0; iter < num_chunks; iter++) {
        int chunk_count = (iter == 0) ? chunk_size_floor : chunk_size_ceil;
        int per_leader_count = chunk_count / num_leads;
        if (my_leader_rank == (num_leads - 1)) {
            /* If chunk_count is not perfectly divisible by num_leaders. The last leader gets the
             * leftover count as well */
            per_leader_count = ((chunk_count / num_leads) + (chunk_count % num_leads));
        }

        /* Step 0: Barrier to make sure the shm_buffer can be reused after the previous call */
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
        coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

        /* Step 1: Leaders perform reduce on is intra_node_sub_communicator. Reduced data is
         * available in the leader's shared buffer */
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_reduce((char *) sendbuf + offset * extent,
                                 (char *) shm_addr + my_leader_rank * shm_size_per_lead,
                                 chunk_count, datatype, op, 0, MPIDI_COMM(comm_ptr, sub_node_comm),
                                 errflag);
#else
        coll_ret =
            MPIDI_NM_mpi_reduce((char *) sendbuf + offset * extent,
                                (char *) shm_addr + my_leader_rank * shm_size_per_lead, chunk_count,
                                datatype, op, 0, MPIDI_COMM(comm_ptr, sub_node_comm), errflag);
#endif
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

        /* Step 2: Barrier to make sure all the leaders have data reduced into is respective shm
         * buffers. */
        if (MPIDI_COMM(comm_ptr, intra_node_leads_comm) != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret = MPIDI_SHM_mpi_barrier(MPIDI_COMM(comm_ptr, intra_node_leads_comm), errflag);
#else
            coll_ret = MPIDI_NM_mpi_barrier(MPIDI_COMM(comm_ptr, intra_node_leads_comm), errflag);
#endif
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }

        /* Step 3: Each leader is responsible to reduce a portion of the data (chunk_count/num_leads),
         * from shm_buffer of every leader into shm_buffer of leader 0 */
        if (MPIDI_COMM(comm_ptr, intra_node_leads_comm) != NULL) {

            int j;
            MPI_Aint cache_tile_size, cache_chunk_count;
            int leader_offset = my_leader_rank * (chunk_count / num_leads) * extent;
            cache_tile_size = MPIR_CVAR_ALLREDUCE_CACHE_PER_LEADER;
            MPI_Aint cache_chunk_size_floor = 0, cache_chunk_size_ceil = 0;

            /* The reduce local is executed for cache_tile_size sized chunks by all the leaders. So
             * each leader finishes reduce on one cache_tile_size sized chunk and moves to the next
             * chunk till it reduces a total of per_leader_count bytes data. */
            MPIR_Algo_calculate_pipeline_chunk_info(cache_tile_size,
                                                    extent, per_leader_count, &cache_chunk_count,
                                                    &cache_chunk_size_floor,
                                                    &cache_chunk_size_ceil);
            for (j = 0; j < cache_chunk_count; j++) {
                for (i = 1; i < num_leads; i++) {
                    coll_ret =
                        MPIR_Reduce_local((char *) shm_addr +
                                          (i * shm_size_per_lead + leader_offset +
                                           j * cache_tile_size),
                                          (char *) shm_addr + leader_offset +
                                          (j * cache_tile_size),
                                          (j ==
                                           (cache_chunk_count -
                                            1)) ? cache_chunk_size_floor : cache_chunk_size_ceil,
                                          datatype, op);
                    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
                }
            }
        }

        /* Step 4: Inter-node Allreduce on all the inter_node_multi_leader_comm. Each leader is
         * responsible for (chunk_count/num_leads) data */
        if (MPIDI_COMM(comm_ptr, inter_node_leads_comm != NULL)) {
            coll_ret = MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, (char *) shm_addr +
                                              my_leader_rank * ((chunk_count / num_leads) * extent),
                                              per_leader_count, datatype, op, MPIDI_COMM(comm_ptr,
                                                                                         inter_node_leads_comm),
                                              errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }

        /* Step 5: Barrier to make sure non-leaders wait for leaders to finish reducing the data
         * from other nodes */
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
        coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

        /* Step 6: Copy data from shm buffer into the recvbuf buffer */
        /* TODO: Do not use offsets for single NIC runs, it shows a slowdown of 0.95x with 2 offsets.
         * Implementing the discrimation in number of offsets depending on number of NICs will be
         * possible when MPICH has a function to tell how many NICs we are using.
         * For now we are using 2 offsets with single NIC too since we do not see impact on overall
         * performance of the composition. This only works when chunk count perfectly divides by
         * number of offsets. */

        if ((chunk_count % num_offsets) != 0)
            num_offsets = 1;

        local_copy_offset = chunk_count * extent / num_offsets;
        local_copy_group = (local_copy_rank / num_offsets);
        for (i = 0; i < num_offsets; i++) {
            coll_ret =
                MPIR_Localcopy(shm_addr +
                               ((local_copy_group + i) % num_offsets) * local_copy_offset,
                               chunk_count / num_offsets, datatype,
                               (char *) recvbuf + offset * extent +
                               ((local_copy_group + i) % num_offsets) * local_copy_offset,
                               chunk_count / num_offsets, datatype);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }
        offset += chunk_count;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf, MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    void *ori_recvbuf = recvbuf;

    MPIR_CHKLMEM_DECL(1);
    /* TODO: we can safe the last send/recv if --
     *     1. if node_comm is the same size as original comm, fallback
     *     2. if root is in node_roots_comm but not rank 0, reduce to root rather than 0
     */

    /* Create a temporary buffer on local roots of all nodes,
     * except for root if it is also a local root */
    if (comm->node_roots_comm != NULL && comm->rank != root) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(recvbuf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    /* intranode reduce on all nodes */
    if (comm->node_comm != NULL) {
        const void *intra_sendbuf;
        /* non-zero root needs to send from recvbuf if using MPI_IN_PLACE  */
        intra_sendbuf = (sendbuf == MPI_IN_PLACE && comm->node_comm->rank != 0) ? recvbuf : sendbuf;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_reduce(intra_sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                 errflag);
#else
        coll_ret =
            MPIDI_NM_mpi_reduce(intra_sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */

        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* internode reduce with rank 0 in node_roots_comm as the root */
    if (comm->node_roots_comm != NULL) {
        const void *inter_sendbuf;
        if (!comm->node_comm && sendbuf != MPI_IN_PLACE) {
            /* individual rank that has skipped node_comm stage, data is still in sendbuf */
            inter_sendbuf = sendbuf;
        } else if (comm->node_roots_comm->rank == 0) {
            /* root use MPI_IN_PLACE, data in recvbuf */
            inter_sendbuf = MPI_IN_PLACE;
        } else {
            inter_sendbuf = recvbuf;
        }

        coll_ret = MPIDI_NM_mpi_reduce(inter_sendbuf, recvbuf, count, datatype, op, 0,
                                       comm->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Send data to root via point-to-point message if root is not rank 0 in comm */
    if (root != 0) {
        if (comm->rank == 0) {
            MPIC_Send(recvbuf, count, datatype, root, MPIR_REDUCE_TAG, comm, errflag);
        } else if (comm->rank == root) {
            MPIC_Recv(ori_recvbuf, count, datatype, 0, MPIR_REDUCE_TAG, comm, MPI_STATUS_IGNORE);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_beta(const void *sendbuf,
                                                                 void *recvbuf, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 MPI_Op op, int root,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;

    MPIR_CHKLMEM_DECL(1);

    void *tmp_buf = NULL;

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) == -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm,
                                 errflag);
#else
        coll_ret =
            MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* do the internode reduce to the root's node */
    if (comm->node_roots_comm != NULL) {
        if (comm->node_roots_comm->rank != MPIR_Get_internode_rank(comm, root)) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm->node_comm == NULL ? sendbuf : tmp_buf);
            coll_ret =
                MPIDI_NM_mpi_reduce(buf, NULL, count, datatype,
                                    op, MPIR_Get_internode_rank(comm, root),
                                    comm->node_roots_comm, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */
                coll_ret =
                    MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);

                MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */
                coll_ret =
                    MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) != -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                 op, MPIR_Get_intranode_rank(comm, root), comm->node_comm, errflag);
#else
        coll_ret =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm, root), comm->node_comm, errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}


MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_gamma(const void *sendbuf,
                                                                  void *recvbuf, MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Node-aware multi-leaders based inter-node and intra-node composition. Each rank on a node places
 * the data for ranks sitting on other nodes into a shared memory buffer. Next each rank participates
 * as a leader in inter-node Alltoall */
MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_intra_composition_alpha(const void *sendbuf,
                                                                    int sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, coll_ret = MPI_SUCCESS;
    int num_nodes;
    int num_ranks = MPIR_Comm_size(comm_ptr);
    int node_comm_size = MPIR_Comm_size(comm_ptr->node_comm);
    int my_node_comm_rank = MPIR_Comm_rank(comm_ptr->node_comm);
    int i, j, p = 0;
    MPI_Aint type_size;

    if (sendcount == 0)
        goto fn_exit;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
    }

    num_nodes = MPIDI_COMM(comm_ptr, spanned_num_nodes);
    if (sendbuf == MPI_IN_PLACE) {
        sendbuf = recvbuf;
        sendcount = recvcount;
        sendtype = recvtype;
    }

    if (MPIDI_COMM(comm_ptr, multi_leads_comm) == NULL) {
        /* Create multi-leaders comm in a lazy manner */
        coll_ret = MPIDI_Comm_create_multi_leaders(comm_ptr);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Allocate the shared memory buffer per node, if it is not already done */
    if (MPIDI_COMM(comm_ptr, alltoall_comp_info->shm_addr) == NULL) {
        coll_ret =
            MPIDU_shm_alloc(comm_ptr->node_comm,
                            node_comm_size * num_ranks * MPIR_CVAR_ALLTOALL_SHM_PER_RANK,
                            (void **) &MPIDI_COMM_ALLTOALL(comm_ptr, shm_addr));
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Barrier to make sure that the shm buffer can be reused after the previous call to Alltoall */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
    coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif
    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

    /* Each rank on a node copy its data into shm buffer */
    /* Example - 2 ranks per node on 2 nodes. R0 and R1 on node 0, R2 and R3 on node 1.
     * R0 buf is (0, 4, 8, 12). R1 buf is (1, 5, 9, 13). R2 buf is (2, 6, 10, 14) and R3 buf is
     * (3, 7, 11, 15). In shm_buf of node 0, place data from (R0, R1) for R0, R2, R1, and R3. In
     * shm_buf of node 1, place data from (R2, R3) for R0, R2, R1, R3. The node 0 shm_buf becomes
     * (0, 1, 8, 9, 4, 5, 12, 13). The node 1 shm_buf becomes (2, 3, 10, 11, 6, 7, 14, 15). */
    for (i = 0; i < node_comm_size; i++) {
        for (j = 0; j < num_nodes; j++) {
            coll_ret = MPIR_Localcopy((void *) ((char *) sendbuf +
                                                (i + j * node_comm_size) * type_size * sendcount),
                                      sendcount, sendtype, (void *) ((char *)
                                                                     MPIDI_COMM_ALLTOALL(comm_ptr,
                                                                                         shm_addr)
                                                                     + (p * node_comm_size +
                                                                        my_node_comm_rank) *
                                                                     type_size * sendcount),
                                      sendcount, sendtype);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
            p++;
        }
    }

    /* Barrier to make sure each rank has copied the data to the shm buf */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
    coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif
    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

    /* Call internode alltoall on the shm_bufs and multi-leaders communicator */
    /* In the above example, first half on shm_bufs are used by the first multi-leader comm of R0
     * and R2 for Alltoall. Second half is used by R1 and R3, which is in the second multi-leader
     * comm. That is, for the alltoall, R1's buf is (4, 5, 12, 13) and R3's buf is (6, 7, 14, 15).
     * After Alltoall R1's buf is (4, 5, 6, 7) and R3's buf is (12, 13, 14, 15), which is the
     * expected result */
    coll_ret = MPIDI_NM_mpi_alltoall((void *) ((char *)
                                               MPIDI_COMM_ALLTOALL(comm_ptr,
                                                                   shm_addr) +
                                               my_node_comm_rank * num_nodes * node_comm_size *
                                               type_size * sendcount), node_comm_size * sendcount,
                                     sendtype, recvbuf, sendcount * node_comm_size, sendtype,
                                     MPIDI_COMM(comm_ptr, multi_leads_comm), errflag);
    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

  fn_exit:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_intra_composition_beta(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv_intra_composition_alpha(const void *sendbuf,
                                                                     const MPI_Aint * sendcounts,
                                                                     const MPI_Aint * sdispls,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const MPI_Aint * recvcounts,
                                                                     const MPI_Aint * rdispls,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls,
                               sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw_intra_composition_alpha(const void *sendbuf,
                                                                     const MPI_Aint sendcounts[],
                                                                     const MPI_Aint sdispls[],
                                                                     const MPI_Datatype
                                                                     sendtypes[],
                                                                     void *recvbuf,
                                                                     const MPI_Aint recvcounts[],
                                                                     const MPI_Aint rdispls[],
                                                                     const MPI_Datatype
                                                                     recvtypes[],
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls,
                               sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_intra_composition_alpha(const void *sendbuf,
                                                                     int sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     int recvcount,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, coll_ret = MPI_SUCCESS;
    int node_comm_size = MPIR_Comm_size(comm_ptr->node_comm);
    int my_node_comm_rank = MPIR_Comm_rank(comm_ptr->node_comm);
    MPI_Aint type_size, extent, true_extent, lb;
    int is_contig, offset;

    if (sendcount < 1 && sendbuf != MPI_IN_PLACE)
        goto fn_exit;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
    }

    if (sendbuf == MPI_IN_PLACE) {
        sendcount = recvcount;
        sendtype = recvtype;
    }

    MPIR_Type_get_extent_impl(sendtype, &lb, &extent);
    MPIR_Type_get_true_extent_impl(sendtype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_Datatype_is_contig(sendtype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Pack_size_impl(1, sendtype, comm_ptr, &type_size);
    }

    /* Using MPL_MAX handles non-contiguous datatype as well */
    offset = MPL_MAX(type_size, extent) * sendcount;

    /* When using MPI_IN_PLACE, the "senddata" from each rank is at its receive index */
    if (sendbuf == MPI_IN_PLACE)
        sendbuf = (char *) recvbuf + MPIR_Comm_rank(comm_ptr) * offset;

    if (MPIDI_COMM(comm_ptr, multi_leads_comm) == NULL) {
        /* Create multi-leaders comm in a lazy manner */
        coll_ret = MPIDI_Comm_create_multi_leaders(comm_ptr);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Allocate the shared memory buffer per node, if it is not already done */
    if (MPIDI_COMM(comm_ptr, allgather_comp_info->shm_addr) == NULL) {
        coll_ret =
            MPIDU_shm_alloc(comm_ptr->node_comm, node_comm_size * MPIR_CVAR_ALLGATHER_SHM_PER_RANK,
                            (void **) &MPIDI_COMM_ALLGATHER(comm_ptr, shm_addr));
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

    /* Barrier to make sure that the shm buffer can be reused after the previous call to Allgather */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
    coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif
    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

    /* Copy data to shm buffers */
    coll_ret = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                              (char *) MPIDI_COMM_ALLGATHER(comm_ptr,
                                                            shm_addr) + my_node_comm_rank * offset,
                              recvcount, recvtype);

    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

    /* Barrier to make sure all the ranks in a node_comm copied data to shm buffer */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    coll_ret = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag);
#else
    coll_ret = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag);
#endif

    /* Perform inter-node allgather on the multi leader comms */
    coll_ret =
        MPIDI_NM_mpi_allgather((char *) MPIDI_COMM_ALLGATHER(comm_ptr, shm_addr),
                               sendcount * node_comm_size, sendtype,
                               recvbuf, recvcount * node_comm_size, recvtype,
                               MPIDI_COMM(comm_ptr, multi_leads_comm), errflag);
    MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

  fn_exit:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_intra_composition_beta(const void *sendbuf,
                                                                    MPI_Aint sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv_intra_composition_alpha(const void *sendbuf,
                                                                      MPI_Aint sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      const MPI_Aint * recvcounts,
                                                                      const MPI_Aint * displs,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype,
                                recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather_intra_composition_alpha(const void *sendbuf,
                                                                  MPI_Aint sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, MPI_Aint recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  int root, MPIR_Comm * comm,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv_intra_composition_alpha(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const MPI_Aint * recvcounts,
                                                                   const MPI_Aint * displs,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter_intra_composition_alpha(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv_intra_composition_alpha(const void *sendbuf,
                                                                    const MPI_Aint * sendcounts,
                                                                    const MPI_Aint * displs,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    int root, MPIR_Comm * comm,
                                                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_intra_composition_alpha(const void *sendbuf,
                                                                          void *recvbuf,
                                                                          const MPI_Aint
                                                                          recvcounts[],
                                                                          MPI_Datatype
                                                                          datatype, MPI_Op op,
                                                                          MPIR_Comm * comm_ptr,
                                                                          MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block_intra_composition_alpha(const void
                                                                                *sendbuf,
                                                                                void *recvbuf,
                                                                                MPI_Aint recvcount,
                                                                                MPI_Datatype
                                                                                datatype,
                                                                                MPI_Op op,
                                                                                MPIR_Comm *
                                                                                comm_ptr,
                                                                                MPIR_Errflag_t
                                                                                errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                          op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_alpha(const void *sendbuf,
                                                                void *recvbuf,
                                                                MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL;
    void *localfulldata = NULL;
    void *prefulldata = NULL;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    int noneed = 1;             /* noneed=1 means no need to bcast tempbuf and
                                 * reduce tempbuf & recvbuf */
    MPIR_CHKLMEM_DECL(3);


    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_CHKLMEM_MALLOC(tempbuf, void *, count * (MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_CHKLMEM_MALLOC(prefulldata, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan", MPL_MEM_BUFFER);
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (comm_ptr->node_comm != NULL) {
            MPIR_CHKLMEM_MALLOC(localfulldata, void *, count * (MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan", MPL_MEM_BUFFER);
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret =
            MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    } else if (sendbuf != MPI_IN_PLACE) {
        coll_ret = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }
    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL) {
        coll_ret = MPIC_Recv(localfulldata, count, datatype,
                             comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG,
                             comm_ptr->node_comm, &status);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    } else if (comm_ptr->node_roots_comm == NULL &&
               comm_ptr->node_comm != NULL &&
               MPIR_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1) {
        coll_ret = MPIC_Send(recvbuf, count, datatype,
                             0, MPIR_SCAN_TAG, comm_ptr->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    } else if (comm_ptr->node_roots_comm != NULL) {
        localfulldata = recvbuf;
    }
    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the main
     * process of node 3. */
    if (comm_ptr->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_scan(localfulldata, prefulldata, count, datatype,
                              op, comm_ptr->node_roots_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);

        if (MPIR_Get_internode_rank(comm_ptr, rank) != comm_ptr->node_roots_comm->local_size - 1) {
            coll_ret = MPIC_Send(prefulldata, count, datatype,
                                 MPIR_Get_internode_rank(comm_ptr, rank) + 1,
                                 MPIR_SCAN_TAG, comm_ptr->node_roots_comm, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }
        if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
            coll_ret = MPIC_Recv(tempbuf, count, datatype,
                                 MPIR_Get_internode_rank(comm_ptr, rank) - 1,
                                 MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status);
            noneed = 0;
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
        coll_ret = MPIDI_NM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#else
            coll_ret =
                MPIDI_NM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }

        coll_ret = MPIR_Reduce_local(tempbuf, recvbuf, count, datatype, op);
        MPIR_ERR_COLL_CHECKANDCONT(coll_ret, errflag, mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_beta(const void *sendbuf,
                                                               void *recvbuf,
                                                               MPI_Aint count,
                                                               MPI_Datatype datatype,
                                                               MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Exscan_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_COLL_IMPL_H_INCLUDED */

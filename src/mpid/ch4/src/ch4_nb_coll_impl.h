/* -*- Mode: C ; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_NB_COLL_IMPL_H_INCLUDED
#define CH4_NB_COLL_IMPL_H_INCLUDED

#include "ch4_comm.h"
#include "algo_common.h"
#include "gentran_utils.h"
#include "../../../mpi/coll/ibcast/ibcast.h"

#define MPIDI_IBCAST_FUNC_DECL(NAME)                                                                     \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast_sched_intra_composition_ ## NAME(void *buffer, MPI_Aint count,      \
                                                                           MPI_Datatype datatype,        \
                                                                           int root, MPIR_Comm * comm,   \
                                                                           MPIR_TSP_sched_t sched,     \
                                                                           bool is_persist);             \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast_intra_composition_ ## NAME(void *buffer, MPI_Aint count,            \
                                                                     MPI_Datatype datatype,              \
                                                                     int root, MPIR_Comm * comm,         \
                                                                     MPIR_Request ** req)                \
{                                                                                                        \
    int mpi_errno = MPI_SUCCESS;                                                                         \
    MPIR_TSP_sched_t sched;                                                                             \
                                                                                                         \
    *req = NULL;                                                                                         \
                                                                                                         \
    /* generate the schedule */                                                                          \
    mpi_errno = MPIR_TSP_sched_create(&sched, false);                                                     \
    MPIR_ERR_CHECK(mpi_errno);                                                                           \
                                                                                                         \
    mpi_errno =                                                                                          \
        MPIDI_Ibcast_sched_intra_composition_ ## NAME(buffer, count, datatype, root, comm, sched, false);\
    MPIR_ERR_CHECK(mpi_errno);                                                                           \
                                                                                                         \
    /* start and register the schedule */                                                                \
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);                                                  \
    MPIR_ERR_CHECK(mpi_errno);                                                                           \
                                                                                                         \
  fn_exit:                                                                                               \
    return mpi_errno;                                                                                    \
  fn_fail:                                                                                               \
    goto fn_exit;                                                                                        \
}

#define MPIDI_IBARRIER_FUNC_DECL(NAME)                                                                       \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier_sched_intra_composition_ ## NAME(MPIR_Comm * comm,               \
                                                                             MPIR_TSP_sched_t sched,       \
                                                                             bool is_persist);               \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier_intra_composition_ ## NAME(MPIR_Comm * comm,                     \
                                                                       MPIR_Request ** req)                  \
{                                                                                                            \
    int mpi_errno = MPI_SUCCESS;                                                                             \
    MPIR_TSP_sched_t sched;                                                                                 \
                                                                                                             \
    *req = NULL;                                                                                             \
                                                                                                             \
    /* generate the schedule */                                                                              \
    mpi_errno = MPIR_TSP_sched_create(&sched, false);                                                         \
    MPIR_ERR_CHECK(mpi_errno);                                                                               \
                                                                                                             \
    mpi_errno =                                                                                              \
        MPIDI_Ibarrier_sched_intra_composition_ ## NAME(comm, sched, false);  \
    MPIR_ERR_CHECK(mpi_errno);                                                                               \
                                                                                                             \
    /* start and register the schedule */                                                                    \
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);                                                      \
    MPIR_ERR_CHECK(mpi_errno);                                                                               \
                                                                                                             \
  fn_exit:                                                                                                   \
    return mpi_errno;                                                                                        \
  fn_fail:                                                                                                   \
    goto fn_exit;                                                                                            \
}

#define MPIDI_IREDUCE_FUNC_DECL(NAME)                                                                       \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_sched_intra_composition_ ## NAME(const void *sendbuf,            \
                                                                            void *recvbuf, MPI_Aint count,       \
                                                                            MPI_Datatype datatype,          \
                                                                            MPI_Op op, int root,            \
                                                                            MPIR_Comm * comm,               \
                                                                            MPIR_TSP_sched_t sched,       \
                                                                            bool is_persist);               \
MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_intra_composition_ ## NAME(const void *sendbuf,                  \
                                                                      void *recvbuf, MPI_Aint count,             \
                                                                      MPI_Datatype datatype,                \
                                                                      MPI_Op op, int root,                  \
                                                                      MPIR_Comm * comm,                     \
                                                                      MPIR_Request ** req)                  \
{                                                                                                           \
    int mpi_errno = MPI_SUCCESS;                                                                            \
    MPIR_TSP_sched_t sched;                                                                                \
                                                                                                            \
    *req = NULL;                                                                                            \
                                                                                                            \
    /* generate the schedule */                                                                             \
    mpi_errno = MPIR_TSP_sched_create(&sched, false);                                                        \
    MPIR_ERR_CHECK(mpi_errno);                                                                              \
                                                                                                            \
    mpi_errno =                                                                                             \
        MPIDI_Ireduce_sched_intra_composition_ ## NAME(sendbuf, recvbuf, count, datatype, op, root, comm,   \
                                                       sched, false);                                       \
    MPIR_ERR_CHECK(mpi_errno);                                                                              \
                                                                                                            \
    /* start and register the schedule */                                                                   \
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);                                                     \
    MPIR_ERR_CHECK(mpi_errno);                                                                              \
                                                                                                            \
  fn_exit:                                                                                                  \
    return mpi_errno;                                                                                       \
  fn_fail:                                                                                                  \
    goto fn_exit;                                                                                           \
}

/* *INDENT-OFF* */
MPIDI_IBCAST_FUNC_DECL(alpha)
MPIDI_IBCAST_FUNC_DECL(beta)
MPIDI_IBCAST_FUNC_DECL(gamma)

MPIDI_IBARRIER_FUNC_DECL(alpha)
MPIDI_IBARRIER_FUNC_DECL(beta)

MPIDI_IREDUCE_FUNC_DECL(alpha)
MPIDI_IREDUCE_FUNC_DECL(beta)
MPIDI_IREDUCE_FUNC_DECL(gamma)
/* *INDENT-ON* */

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast_sched_intra_composition_alpha(void *buffer,
                                                                        MPI_Aint count,
                                                                        MPI_Datatype datatype,
                                                                        int root, MPIR_Comm * comm,
                                                                        MPIR_TSP_sched_t sched,
                                                                        bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes;
    int tag, i;
    MPI_Aint num_chunks, chunk_count_floor, chunk_count_ceil;
    int offset = 0, is_contig, ori_count = count;
    MPI_Aint actual_packed_unpacked_bytes;
    void *ori_buffer = buffer;
    MPI_Datatype ori_datatype = datatype;
    int *nm_vtx_id, *shm_vtx_id, vtx_id;
    MPIR_TSP_sched_t nm_sub_sched, shm_sub_sched;
    int prev_vtx_id = -1;
    int dependence_arr[2] = { 0, 0 };
    int window_val = MPIR_CVAR_IBCAST_COMPOSITION_WINDOW_SIZE;

    MPIR_CHKLMEM_DECL(2);
    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, comm, &type_size);
    }

    nbytes = type_size * count;

    if (MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE < 1) {
        /* Disable pipelining */
        num_chunks = 1;
        chunk_count_floor = nbytes;
        chunk_count_ceil = nbytes;
    } else {
        if (is_contig) {
            MPI_Aint true_lb = 0, true_extent = 0;
            /* contiguous. no need to pack. */
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            buffer = (char *) ori_buffer + true_lb;
        } else {
            buffer = MPIR_TSP_sched_malloc(nbytes, sched);
            if (comm->rank == root) {
                /* Root packs the data before sending, for non contiguous datatypes */
                mpi_errno_ret =
                    MPIR_Typerep_pack(ori_buffer, ori_count, ori_datatype, 0, buffer, nbytes,
                                      &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
                if (mpi_errno_ret)
                    MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
        }

        /* Calculate chunking information for pipelining */
        /* Chunking information is calculated in terms of bytes (not type_size), so we may copy only
         * half a datatype in one chunk, but that is fine */
        MPIR_Algo_calculate_pipeline_chunk_info(MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE, 1,
                                                nbytes, &num_chunks, &chunk_count_floor,
                                                &chunk_count_ceil);
    }

    MPIR_CHKLMEM_MALLOC(nm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "nm_vtx_ids",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(shm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "shm_vtx_ids",
                        MPL_MEM_COLL);

    /* Split a large message into multiple chunks. Create a tree consisting of vertices coming the
     * send, recv, NM_Ibcast and SHM_Ibcast for each chunk. These are vertical dependencies. For
     * horizontal dependencies, there is sliding window among NM vertices of different chunks. SHM
     * vertices of different chunks have linear dependencies (window_size = 1) */
    for (i = 0; i < num_chunks; i++) {
#ifdef HAVE_ERROR_CHECKING
        struct MPII_Ibcast_state *ibcast_state = NULL;
#endif
        MPI_Aint chunk_count = (i == 0) ? chunk_count_floor : chunk_count_ceil;
#ifdef HAVE_ERROR_CHECKING
        ibcast_state = MPIR_TSP_sched_malloc(sizeof(struct MPII_Ibcast_state), sched);
        if (ibcast_state == NULL)
            MPIR_ERR_POP(mpi_errno);
        ibcast_state->n_bytes = chunk_count;
#endif
        int n_incoming = 0;
        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        if (comm->node_roots_comm == NULL && comm->rank == root && comm->node_comm != NULL) {
            /* Root sends the data to its node leader */
            mpi_errno_ret =
                MPIR_TSP_sched_isend((char *) buffer + offset, chunk_count, MPI_BYTE, 0, tag,
                                     comm->node_comm, sched, 0, NULL, &prev_vtx_id);
            /* Update dependence array and number of incoming vertices to be used in the next vertex */
            dependence_arr[0] = prev_vtx_id;
            n_incoming = 1;
        }

        if (comm->node_roots_comm != NULL && comm->node_comm != NULL && comm->rank != root &&
            MPIR_Get_intranode_rank(comm, root) != -1) {
            /* Node leader receive data from the root */
#ifdef HAVE_ERROR_CHECKING
            mpi_errno_ret =
                MPIR_TSP_sched_irecv_status((char *) buffer + offset, chunk_count, MPI_BYTE,
                                            MPIR_Get_intranode_rank(comm, root), tag,
                                            comm->node_comm, &ibcast_state->status, sched, 0, NULL,
                                            &prev_vtx_id);
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }

            mpi_errno_ret =
                MPIR_TSP_sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, sched, 1,
                                  &prev_vtx_id, &vtx_id);
#else
            mpi_errno_ret = MPIR_TSP_sched_irecv((char *) buffer + offset, chunk_count, MPI_BYTE,
                                                 MPIR_Get_intranode_rank(comm, root), tag,
                                                 comm->node_comm, sched, 0, NULL, &prev_vtx_id);
#endif
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }

            /* Update dependence array and number of incoming vertices to be used in the next vertex */
            dependence_arr[0] = prev_vtx_id;
            n_incoming = 1;
        }

        if (comm->node_roots_comm != NULL) {
            /*nm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!nm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&nm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            mpi_errno_ret =
                MPIDI_NM_mpi_ibcast_sched((char *) buffer + offset, chunk_count, MPI_BYTE,
                                          MPIR_Get_internode_rank(comm, root),
                                          comm->node_roots_comm, nm_sub_sched);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (i > window_val - 1) {
                /* Create dependence from NM sub-schedule vertex of chunk i to NM sub-schedule vertex
                 * of chunk (i+window_size) */
                dependence_arr[n_incoming] = nm_vtx_id[i - window_val];
                n_incoming++;
            }
            /* Create the NM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, nm_sub_sched, n_incoming, dependence_arr,
                                         &nm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = nm_vtx_id[i];
        }

        if (comm->node_comm != NULL) {
            /*shm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!shm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ibcast_sched((char *) buffer + offset, chunk_count,
                                           MPI_BYTE, 0, comm->node_comm, shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ibcast_sched((char *) buffer + offset, chunk_count, MPI_BYTE,
                                          0, comm->node_comm, shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (i > 0) {
                /* Create dependence from SHM sub-schedule of previous chunk to SHM sub-schedule of
                 * current chunk */
                dependence_arr[n_incoming] = shm_vtx_id[i - 1];
                n_incoming++;
            }
            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &shm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
        }

        offset += chunk_count;
    }

    if (MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE >= 1) {
        /* Pipelining was enabled */
        if (!is_contig) {
            if (comm->rank != root) {
                /* Non-root unpack the data if expecting non-contiguous datatypes */
                /* Dependency is created from the last vertex of each chunk to the unpack vertex */
                mpi_errno_ret =
                    MPIR_TSP_sched_localcopy(buffer, nbytes, MPI_BYTE, ori_buffer, ori_count,
                                             ori_datatype, sched, num_chunks, shm_vtx_id, &vtx_id);

                if (mpi_errno_ret)
                    MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
        }
    }

    /* optimize scheduler to prevent posting too many irecv */
    if (MPIR_CVAR_BCAST_PERSISTENT_OPT)
        MPIR_TSP_sched_optimize(sched);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast_sched_intra_composition_beta(void *buffer, MPI_Aint count,
                                                                       MPI_Datatype datatype,
                                                                       int root, MPIR_Comm * comm,
                                                                       MPIR_TSP_sched_t sched,
                                                                       bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes;
    int i;
    MPI_Aint num_chunks, chunk_count_floor, chunk_count_ceil;
    int offset = 0, is_contig, ori_count = count;
    MPI_Aint actual_packed_unpacked_bytes;
    void *ori_buffer = buffer;
    MPI_Datatype ori_datatype = datatype;
    int *nm_vtx_id, *shm_vtx_id, barrier_vtx_id, vtx_id;
    MPIR_TSP_sched_t nm_sub_sched, shm_sub_sched, barrier_sub_sched;
    int dependence_arr[2] = { 0, 0 };
    int window_val = MPIR_CVAR_IBCAST_COMPOSITION_WINDOW_SIZE;

    MPIR_CHKLMEM_DECL(2);

    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, comm, &type_size);
    }

    nbytes = type_size * count;

    if (MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE < 1) {
        /* Disable pipelining */
        num_chunks = 1;
        chunk_count_floor = nbytes;
        chunk_count_ceil = nbytes;
    } else {
        if (is_contig) {
            MPI_Aint true_lb = 0, true_extent = 0;
            /* contiguous. no need to pack. */
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            buffer = (char *) ori_buffer + true_lb;
        } else {
            buffer = MPIR_TSP_sched_malloc(nbytes, sched);
            if (comm->rank == root) {
                /* Root packs the data before sending, for non contiguous datatypes */
                mpi_errno_ret =
                    MPIR_Typerep_pack(ori_buffer, ori_count, ori_datatype, 0, buffer, nbytes,
                                      &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
                if (mpi_errno_ret)
                    MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
        }

        /* Calculate chunking information for pipelining */
        /* Chunking information is calculated in terms of bytes (not type_size), so we may copy only
         * half a datatype in one chunk, but that is fine */
        MPIR_Algo_calculate_pipeline_chunk_info(MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE, 1,
                                                nbytes, &num_chunks, &chunk_count_floor,
                                                &chunk_count_ceil);
    }

    MPIR_CHKLMEM_MALLOC(nm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "nm_vtx_ids",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(shm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "shm_vtx_ids",
                        MPL_MEM_COLL);

    /* Split a large message into multiple chunks. Create a tree consisting of vertices coming the
     * send, recv, NM_Ibcast and SHM_Ibcast for each chunk. These are vertical dependencies. For
     * horizontal dependencies, there is sliding window among NM vertices of different chunks. SHM
     * vertices of different chunks have linear dependencies (window_size = 1) */
    for (i = 0; i < num_chunks; i++) {
        MPI_Aint chunk_count = (i == 0) ? chunk_count_floor : chunk_count_ceil;
        int n_incoming = 0;

        if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) > 0) {
            /* Intra-node bcast on the node containing root of the collective, if root is not one
             * of the node leaders */
            /*shm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!shm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ibcast_sched((char *) buffer + offset, chunk_count,
                                           MPI_BYTE, MPIR_Get_intranode_rank(comm, root),
                                           comm->node_comm, shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ibcast_sched((char *) buffer + offset, chunk_count, MPI_BYTE,
                                          MPIR_Get_intranode_rank(comm, root), comm->node_comm,
                                          shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (i > 0) {
                /* Create dependence from SHM sub-schedule of previous chunk to SHM sub-schedule of
                 * current chunk */
                dependence_arr[n_incoming] = shm_vtx_id[i - 1];
                n_incoming++;
            }
            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &shm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = shm_vtx_id[i];
        }

        /*barrier_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
         * MPIR_ERR_CHKANDJUMP(!barrier_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
        mpi_errno_ret = MPIR_TSP_sched_create(&barrier_sub_sched, is_persist);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret = MPIDI_NM_mpi_ibarrier_sched(comm, barrier_sub_sched);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        /* Create the SHM sub-schedule vertex */
        mpi_errno_ret =
            MPIR_TSP_sched_sub_sched(sched, barrier_sub_sched, n_incoming, dependence_arr,
                                     &barrier_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        n_incoming = 1;
        dependence_arr[0] = barrier_vtx_id;

        /* Ideally this barrier is not needed, but somehow incorrect results are produced without it.
         * Guess is ranks of node_roots_comm start doing the inter-node and intra-node bcast in
         * incorrect order even though the fence is put */

        if (comm->node_roots_comm != NULL) {
            /* Inter-node bcast on all the node leaders */
            /*nm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!nm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&nm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            mpi_errno_ret =
                MPIDI_NM_mpi_ibcast_sched((char *) buffer + offset, chunk_count, MPI_BYTE,
                                          MPIR_Get_internode_rank(comm, root),
                                          comm->node_roots_comm, nm_sub_sched);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (i > window_val - 1) {
                /* Create dependence from NM sub-schedule vertex of chunk i to NM sub-schedule vertex
                 * of chunk (i+window_size) */
                dependence_arr[n_incoming] = nm_vtx_id[i - window_val];
                n_incoming++;
            }
            /* Create the NM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, nm_sub_sched, n_incoming, dependence_arr,
                                         &nm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = nm_vtx_id[i];
        }

        if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) <= 0) {
            /* Intra-node Bcast on the remaining nodes */

            mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ibcast_sched((char *) buffer + offset, chunk_count,
                                           MPI_BYTE, 0, comm->node_comm, shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ibcast_sched((char *) buffer + offset, chunk_count, MPI_BYTE,
                                          0, comm->node_comm, shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (i > 0) {
                /* Create dependence from SHM sub-schedule of previous chunk to SHM sub-schedule of
                 * current chunk */
                dependence_arr[n_incoming] = shm_vtx_id[i - 1];
                n_incoming++;
            }
            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &shm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        }

        offset += chunk_count;
    }

    if (MPIR_CVAR_IBCAST_COMPOSITION_PIPELINE_CHUNK_SIZE >= 1) {
        /* Pipelining was enabled */
        if (!is_contig) {
            if (comm->rank != root) {
                /* Non-root unpack the data if expecting non-contiguous datatypes */
                /* Dependency is created on the sink vertices to make sure unpack is done only after
                 * all the vertices in each iteration are executed */
                mpi_errno_ret =
                    MPIR_TSP_sched_localcopy(buffer, nbytes, MPI_BYTE, ori_buffer, ori_count,
                                             ori_datatype, sched, num_chunks, shm_vtx_id, &vtx_id);

                if (mpi_errno_ret)
                    MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
        }
    }

    /* optimize scheduler to prevent posting too many irecv */
    if (MPIR_CVAR_BCAST_PERSISTENT_OPT)
        MPIR_TSP_sched_optimize(sched);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast_sched_intra_composition_gamma(void *buffer,
                                                                        MPI_Aint count,
                                                                        MPI_Datatype datatype,
                                                                        int root, MPIR_Comm * comm,
                                                                        MPIR_TSP_sched_t sched,
                                                                        bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_ibcast_sched(buffer, count, datatype, root, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier_sched_intra_composition_alpha(MPIR_Comm * comm,
                                                                          MPIR_TSP_sched_t sched,
                                                                          bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int curr_vtx_id = -1, prev_vtx_id = -1;
    MPIR_TSP_sched_t nm_sub_sched, shm_sub_sched_0, shm_sub_sched_1;
    int n_incoming = 0;

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL) {
        mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched_0, is_persist);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno_ret = MPIDI_SHM_mpi_ibarrier_sched(comm->node_comm, shm_sub_sched_0);
#else
        mpi_errno_ret = MPIDI_NM_mpi_ibarrier_sched(comm->node_comm, shm_sub_sched_0);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        /* Create the SHM sub-schedule vertex */
        mpi_errno_ret =
            MPIR_TSP_sched_sub_sched(sched, shm_sub_sched_0, n_incoming, &prev_vtx_id,
                                     &curr_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        n_incoming = 1;
        prev_vtx_id = curr_vtx_id;
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        mpi_errno_ret = MPIR_TSP_sched_create(&nm_sub_sched, is_persist);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret = MPIDI_NM_mpi_ibarrier_sched(comm->node_roots_comm, nm_sub_sched);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        /* Create the SHM sub-schedule vertex */
        mpi_errno_ret =
            MPIR_TSP_sched_sub_sched(sched, nm_sub_sched, n_incoming, &prev_vtx_id, &curr_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        n_incoming = 1;
        prev_vtx_id = curr_vtx_id;
    }

    /* release the local processes on each node by doing a barrier */
    if (comm->node_comm != NULL) {
        mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched_1, is_persist);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno_ret = MPIDI_SHM_mpi_ibarrier_sched(comm->node_comm, shm_sub_sched_1);
#else
        mpi_errno_ret = MPIDI_NM_mpi_ibarrier_sched(comm->node_comm, shm_sub_sched_1);
#endif /* MPIDI_CH4_DIRECT_NETMOD */

        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        /* Create the SHM sub-schedule vertex */
        mpi_errno_ret =
            MPIR_TSP_sched_sub_sched(sched, shm_sub_sched_1, n_incoming, &prev_vtx_id,
                                     &curr_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier_sched_intra_composition_beta(MPIR_Comm * comm,
                                                                         MPIR_TSP_sched_t sched,
                                                                         bool is_persist)
{
    int mpi_errno_ret = MPI_SUCCESS;

    mpi_errno_ret = MPIDI_NM_mpi_ibarrier_sched(comm, &sched);

    return mpi_errno_ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_sched_intra_composition_alpha(const void *sendbuf,
                                                                         void *recvbuf,
                                                                         MPI_Aint count,
                                                                         MPI_Datatype datatype,
                                                                         MPI_Op op, int root,
                                                                         MPIR_Comm * comm,
                                                                         MPIR_TSP_sched_t sched,
                                                                         bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent;
    const void *inter_sendbuf;
    void *ori_recvbuf = recvbuf;
    int tag, i, vtx_id;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil, offset = 0;
    int *nm_vtx_id, prev_vtx_id = -1;
    int dependence_arr[2] = { 0, 0 };
    int window_val = MPIR_CVAR_IREDUCE_COMPOSITION_WINDOW_SIZE;
    MPIR_TSP_sched_t nm_sub_sched, shm_sub_sched;

    if (count == 0)
        return mpi_errno;

    MPIR_CHKLMEM_DECL(1);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    if (MPIR_CVAR_IREDUCE_COMPOSITION_PIPELINE_CHUNK_SIZE < 1) {
        /* Disable pipelining */
        num_chunks = 1;
        chunk_size_floor = count;
        chunk_size_ceil = count;
    } else {
        MPIR_Algo_calculate_pipeline_chunk_info(MPIR_CVAR_IREDUCE_COMPOSITION_PIPELINE_CHUNK_SIZE,
                                                extent, count, &num_chunks, &chunk_size_floor,
                                                &chunk_size_ceil);
    }

    MPIR_CHKLMEM_MALLOC(nm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "nm_vtx_ids",
                        MPL_MEM_COLL);

    /* Create a temporary buffer on local roots of all nodes,
     * except for root if it is also a local root */
    if (comm->node_roots_comm != NULL && comm->rank != root) {
        recvbuf = MPIR_TSP_sched_malloc(count * extent, sched);
        /* adjust for potential negative lower bound in datatype */
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }
    /* Split a large message into multiple chunks. Create a tree consisting of vertices coming from
     * NM_Ireduce, SHM_Ireduce, send, and recv for each chunk. These are vertical dependencies. For
     * horizontal dependencies, there is sliding window among NM vertices of different chunks. SHM
     * vertices of different chunks have linear dependencies (window_size = 1) */
    for (i = 0; i < num_chunks; ++i) {
        MPI_Aint chunk_count = (i == 0) ? chunk_size_floor : chunk_size_ceil;
        int n_incoming = 0;

        /* intranode reduce on all nodes */
        if (comm->node_comm != NULL) {
            //shm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
            //MPIR_ERR_CHKANDJUMP(!shm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
            mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                            (char *) recvbuf + offset * extent,
                                            chunk_count, datatype, op, 0, comm->node_comm,
                                            shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                           (char *) recvbuf + offset * extent,
                                           chunk_count, datatype, op, 0, comm->node_comm,
                                           shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
            if (i > 0) {
                /* Create dependence from SHM sub-schedule vertex of prev chunk to SHM sub-schedule
                 * vertex of current chunk */
                dependence_arr[0] = prev_vtx_id;
                n_incoming = 1;
            }

            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &prev_vtx_id);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = prev_vtx_id;

            /* recvbuf becomes the sendbuf for internode reduce */
            inter_sendbuf = recvbuf;

        } else {
            inter_sendbuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
        }
        /* internode reduce with rank 0 in node_roots_comm as the root */
        if (comm->node_roots_comm != NULL) {
            /*nm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!nm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&nm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            mpi_errno_ret =
                MPIDI_NM_mpi_ireduce_sched(comm->node_roots_comm->rank ==
                                           0 ? MPI_IN_PLACE : (char *) inter_sendbuf +
                                           offset * extent, (char *) recvbuf + offset * extent,
                                           chunk_count, datatype, op, 0, comm->node_roots_comm,
                                           nm_sub_sched);

            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
            if (i > window_val - 1) {
                /* Create dependence from NM sub-schedule vertex of chunk i to NM sub-schedule vertex
                 * of chunk (i+window_size) */
                dependence_arr[n_incoming] = nm_vtx_id[i - window_val];
                n_incoming++;
            }

            /* Create the NM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, nm_sub_sched, n_incoming, dependence_arr,
                                         &nm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = nm_vtx_id[i];
        }

        /* Send data to root via point-to-point message if root is not rank 0 in comm */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        if (root != 0) {
            if (comm->rank == 0) {
                mpi_errno_ret =
                    MPIR_TSP_sched_isend((char *) recvbuf + offset * extent, chunk_count, datatype,
                                         root, tag, comm, sched, n_incoming, dependence_arr,
                                         &vtx_id);
            } else if (comm->rank == root) {
                mpi_errno_ret =
                    MPIR_TSP_sched_irecv((char *) ori_recvbuf + offset * extent, chunk_count,
                                         datatype, 0, tag, comm, sched, n_incoming, dependence_arr,
                                         &vtx_id);
            }
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }

        }

        offset += chunk_count;
    }
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_sched_intra_composition_beta(const void *sendbuf,
                                                                        void *recvbuf,
                                                                        MPI_Aint count,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op, int root,
                                                                        MPIR_Comm * comm,
                                                                        MPIR_TSP_sched_t sched,
                                                                        bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil, offset = 0;
    void *tmp_buf = NULL;
    int *nm_vtx_id, prev_vtx_id = -1, i;
    int dependence_arr[2] = { 0, 0 };
    int window_val = MPIR_CVAR_IREDUCE_COMPOSITION_WINDOW_SIZE;
    MPIR_TSP_sched_t nm_sub_sched, shm_sub_sched;

    MPIR_CHKLMEM_DECL(1);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    if (MPIR_CVAR_IREDUCE_COMPOSITION_PIPELINE_CHUNK_SIZE < 1) {
        /* Disable pipelining */
        num_chunks = 1;
        chunk_size_floor = count;
        chunk_size_ceil = count;
    } else {
        MPIR_Algo_calculate_pipeline_chunk_info(MPIR_CVAR_IREDUCE_COMPOSITION_PIPELINE_CHUNK_SIZE,
                                                extent, count, &num_chunks, &chunk_size_floor,
                                                &chunk_size_ceil);
    }

    MPIR_CHKLMEM_MALLOC(nm_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "nm_vtx_ids",
                        MPL_MEM_COLL);

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        tmp_buf = MPIR_TSP_sched_malloc(count * extent, sched);

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }
    /* Split a large message into multiple chunks. Create a tree consisting of vertices coming the
     * NM_Ireduce and SHM_Ireduce for each chunk. These are vertical dependencies. For
     * horizontal dependencies, there is sliding window among NM vertices of different chunks. SHM
     * vertices of different chunks have linear dependencies (window_size = 1) */
    for (i = 0; i < num_chunks; ++i) {
        MPI_Aint chunk_count = (i == 0) ? chunk_size_floor : chunk_size_ceil;
        int n_incoming = 0;

        /* do the intranode reduce on all nodes other than the root's node */
        if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) == -1) {
            /*shm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!shm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                            (char *) tmp_buf + offset * extent, chunk_count,
                                            datatype, op, 0, comm->node_comm, shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                           (char *) tmp_buf + offset * extent, chunk_count,
                                           datatype, op, 0, comm->node_comm, shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
            if (i > 0) {
                /* Create dependence from SHM sub-schedule vertex of prev chunk to SHM sub-schedule
                 * vertex of current chunk */
                dependence_arr[0] = prev_vtx_id;
                n_incoming = 1;
            }

            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &prev_vtx_id);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = prev_vtx_id;
        }
        /* do the internode reduce to the root's node */
        if (comm->node_roots_comm != NULL) {
            /*nm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!nm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&nm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (comm->node_roots_comm->rank != MPIR_Get_internode_rank(comm, root)) {
                /* I am not on root's node.  Use tmp_buf if we
                 * participated in the first reduce, otherwise use sendbuf */
                const void *buf = (comm->node_comm == NULL ? sendbuf : tmp_buf);
                mpi_errno_ret =
                    MPIDI_NM_mpi_ireduce_sched((char *) buf + offset * extent, NULL, chunk_count,
                                               datatype, op, MPIR_Get_internode_rank(comm, root),
                                               comm->node_roots_comm, nm_sub_sched);
                if (mpi_errno_ret) {
                    /* for communication errors, just record the error but continue */
                    MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
                }
            } else {    /* I am on root's node. I have not participated in the earlier reduce. */
                if (comm->rank != root) {
                    /* I am not the root though. I don't have a valid recvbuf.
                     * Use tmp_buf as recvbuf. */
                    mpi_errno_ret =
                        MPIDI_NM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                                   (char *) tmp_buf + offset * extent, chunk_count,
                                                   datatype, op, MPIR_Get_internode_rank(comm,
                                                                                         root),
                                                   comm->node_roots_comm, nm_sub_sched);

                    if (mpi_errno_ret) {
                        /* for communication errors, just record the error but continue */
                        MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
                    }

                    /* point sendbuf at tmp_buf to make final intranode reduce easy */
                    sendbuf = tmp_buf;
                } else {
                    /* I am the root. in_place is automatically handled. */
                    mpi_errno_ret =
                        MPIDI_NM_mpi_ireduce_sched((char *) sendbuf + offset * extent,
                                                   (char *) recvbuf + offset * extent, chunk_count,
                                                   datatype, op, MPIR_Get_internode_rank(comm,
                                                                                         root),
                                                   comm->node_roots_comm, nm_sub_sched);
                    if (mpi_errno_ret) {
                        /* for communication errors, just record the error but continue */
                        MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
                    }
                }
            }
            if (i > window_val - 1) {
                /* Create dependence from NM sub-schedule vertex of chunk i to NM sub-schedule vertex
                 * of chunk (i+window_size) */
                dependence_arr[n_incoming] = nm_vtx_id[i - window_val];
                n_incoming++;
            }
            /* Create the NM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, nm_sub_sched, n_incoming, dependence_arr,
                                         &nm_vtx_id[i]);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            n_incoming = 1;
            dependence_arr[0] = nm_vtx_id[i];
        }

        /* do the intranode reduce on the root's node */
        if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) != -1) {
            /*shm_sub_sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
             * MPIR_ERR_CHKANDJUMP(!shm_sub_sched, mpi_errno, MPI_ERR_OTHER, "**nomem"); */
            mpi_errno_ret = MPIR_TSP_sched_create(&shm_sub_sched, is_persist);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

            if (comm->node_roots_comm != NULL && comm->rank == root) {
                /* set sendbuf to MPI_IM_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            } else {
                sendbuf = (char *) sendbuf + offset * extent;
            }
#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno_ret =
                MPIDI_SHM_mpi_ireduce_sched(sendbuf, (char *) recvbuf + offset * extent,
                                            chunk_count, datatype, op, MPIR_Get_intranode_rank(comm,
                                                                                               root),
                                            comm->node_comm, shm_sub_sched);
#else
            mpi_errno_ret =
                MPIDI_NM_mpi_ireduce_sched(sendbuf, (char *) recvbuf + offset * extent, chunk_count,
                                           datatype, op, MPIR_Get_intranode_rank(comm, root),
                                           comm->node_comm, shm_sub_sched);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            if (mpi_errno_ret) {
                /* for communication errors, just record the error but continue */
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
            /* Create the SHM sub-schedule vertex */
            mpi_errno_ret =
                MPIR_TSP_sched_sub_sched(sched, shm_sub_sched, n_incoming, dependence_arr,
                                         &prev_vtx_id);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        }

        offset += chunk_count;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_sched_intra_composition_gamma(const void *sendbuf,
                                                                         void *recvbuf,
                                                                         MPI_Aint count,
                                                                         MPI_Datatype datatype,
                                                                         MPI_Op op, int root,
                                                                         MPIR_Comm * comm,
                                                                         MPIR_TSP_sched_t sched,
                                                                         bool is_persist)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    mpi_errno_ret =
        MPIDI_NM_mpi_ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm, sched);

    if (mpi_errno_ret)
        MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

    return mpi_errno;
}

#endif /* CH4_NB_COLL_IMPL_H_INCLUDED */

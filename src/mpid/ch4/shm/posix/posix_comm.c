/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_noinline.h"
#include "release_gather.h"
#include "nb_bcast_release_gather.h"
#include "nb_reduce_release_gather.h"

int MPIDI_POSIX_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_init_null(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDI_POSIX_COMM(comm, nb_bcast_seq_no) = 0;
    MPIDI_POSIX_COMM(comm, nb_reduce_seq_no) = 0;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL();

    if (comm == MPIR_Process.comm_world) {
        /* gather topo info from local procs and calculate distance */
        if (MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE && MPIR_Process.local_size > 1) {
            int topo_info_size = sizeof(MPIDI_POSIX_topo_info_t);
            MPIDI_POSIX_topo_info_t *local_rank_topo;
            MPIR_CHKLMEM_MALLOC(local_rank_topo, MPIR_Process.local_size * topo_info_size);

            MPIR_Comm *node_comm = MPIR_Comm_get_node_comm(comm);
            mpi_errno = MPIR_Allgather_fallback(&MPIDI_POSIX_global.topo, topo_info_size,
                                                MPIR_BYTE_INTERNAL,
                                                local_rank_topo, topo_info_size, MPIR_BYTE_INTERNAL,
                                                node_comm, MPIR_COLL_ATTR_SYNC);
            MPIR_ERR_CHECK(mpi_errno);

            for (int i = 0; i < MPIR_Process.local_size; i++) {
                if (local_rank_topo[i].l3_cache_id == -1 || local_rank_topo[i].numa_id == -1) {
                    /* if topo info is incomplete, treat the node as local as fallback */
                    MPIDI_POSIX_global.local_rank_dist[i] = MPIDI_POSIX_DIST__LOCAL;
                    continue;
                }
                if (local_rank_topo[i].l3_cache_id != MPIDI_POSIX_global.topo.l3_cache_id) {
                    MPIDI_POSIX_global.local_rank_dist[i] = MPIDI_POSIX_DIST__NO_SHARED_CACHE;
                    continue;
                }
                if (local_rank_topo[i].numa_id != MPIDI_POSIX_global.topo.numa_id) {
                    MPIDI_POSIX_global.local_rank_dist[i] = MPIDI_POSIX_DIST__INTER_NUMA;
                    continue;
                }
            }

            if (MPIR_CVAR_DEBUG_SUMMARY >= 2) {
                if (MPIR_Process.rank == 0) {
                    fprintf(stdout, "====== POSIX Topo Dist ======\n");
                }
                fprintf(stdout, "Rank: %d, Local_rank: %d [ %d", MPIR_Process.rank,
                        MPIR_Process.local_rank, MPIDI_POSIX_global.local_rank_dist[0]);
                for (int i = 1; i < MPIR_Process.local_size; i++) {
                    fprintf(stdout, ", %d", MPIDI_POSIX_global.local_rank_dist[i]);
                }
                fprintf(stdout, " ]\n");
                if (MPIR_Process.rank == 0) {
                    fprintf(stdout, "=============================\n");
                }
            }
        }
    }

    /* prune selection tree */
    if (MPIDI_global.shm.posix.csel_root) {
        mpi_errno = MPIR_Csel_prune(MPIDI_global.shm.posix.csel_root, comm,
                                    &MPIDI_POSIX_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_POSIX_COMM(comm, csel_comm) = NULL;
    }

    /* prune selection tree for gpu */
    if (MPIDI_global.shm.posix.csel_root_gpu) {
        mpi_errno = MPIR_Csel_prune(MPIDI_global.shm.posix.csel_root_gpu, comm,
                                    &MPIDI_POSIX_COMM(comm, csel_comm_gpu));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_POSIX_COMM(comm, csel_comm_gpu) = NULL;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_POSIX_COMM(comm, csel_comm)) {
        mpi_errno = MPIR_Csel_free(MPIDI_POSIX_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIDI_POSIX_COMM(comm, csel_comm_gpu)) {
        mpi_errno = MPIR_Csel_free(MPIDI_POSIX_COMM(comm, csel_comm_gpu));
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_free(comm);
        mpi_errno = MPIDI_POSIX_nb_release_gather_comm_free(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

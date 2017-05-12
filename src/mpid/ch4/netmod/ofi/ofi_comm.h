/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_COMM_H_INCLUDED
#define OFI_COMM_H_INCLUDED

#include "ofi_impl.h"
#include "mpl_utlist.h"
#include "mpidu_shm.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);

    MPIDI_OFI_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDI_OFI_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters);
    MPIDI_OFI_index_allocator_create(&MPIDI_OFI_COMM(comm).win_id_allocator, 0);
    MPIDI_OFI_index_allocator_create(&MPIDI_OFI_COMM(comm).rma_id_allocator, 1);

    mpi_errno = MPIDI_CH4U_init_comm(comm);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* Do not handle intercomms */
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        goto fn_exit;

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    if (comm == MPIR_Process.comm_world) {
        MPIDU_shm_seg_t memory;
        MPIDU_shm_barrier_t *barrier;
        char *table, *local_table;;
        int i, local_rank, local_rank_0 = -1, num_local = 0;
        fi_addr_t *mapped_table;
        int rank = MPIR_Comm_rank(comm);
        int size = MPIR_Comm_size(comm);

        /* locality information for shm segment */
        for (i = 0; i < size; i++) {
            if (MPIDI_CH4_rank_is_local(i, comm)) {
                if (i == rank)
                    local_rank = num_local;

                if (local_rank_0 == -1)
                    local_rank_0 = i;

                num_local++;
            }
        }

        MPIDU_shm_seg_alloc(size * MPIDI_Global.addrnamelen, (void **)&table);
        MPIDU_shm_seg_commit(&memory, &barrier, num_local, local_rank, local_rank_0, rank);
        /* FIXME: assumes all nodes have same number of ranks */
        local_table = &table[MPIDI_CH4_Global.node_map[0][rank] * num_local * MPIDI_Global.addrnamelen];
        memcpy(&local_table[local_rank * MPIDI_Global.addrnamelen], MPIDI_Global.addrname, MPIDI_Global.addrnamelen);
        MPIDU_shm_barrier(barrier, num_local);
        if (rank == local_rank_0) {
            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            MPIR_Comm *allgather_comm = comm->node_roots_comm ? comm->node_roots_comm : comm;
            MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, table, num_local * MPIDI_Global.addrnamelen,
                                MPI_BYTE, allgather_comm, &errflag);
        }
        MPIDU_shm_barrier(barrier, num_local);

        mapped_table = MPL_malloc(size * sizeof(fi_addr_t));
        for (i = 0; i < size; i++) {
            /* only insert new addresses */
            if (MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest == FI_ADDR_UNSPEC) {
                fi_av_insert(MPIDI_Global.av, &table[i * MPIDI_Global.addrnamelen], 1, &mapped_table[i], 0ULL, NULL);
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[i];
            }
        }

        MPIDU_shm_barrier(barrier, num_local);
        MPIDU_shm_seg_destroy(&memory, num_local);
        MPL_free(mapped_table);
        PMI_Barrier();
        /* now everyone can communicate */
    }

    MPIR_Assert(comm->coll_fns != NULL);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);

    mpi_errno = MPIDI_CH4U_destroy_comm(comm);
    MPIDI_OFI_map_destroy(MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDI_OFI_map_destroy(MPIDI_OFI_COMM(comm).huge_recv_counters);
    MPIDI_OFI_index_allocator_destroy(MPIDI_OFI_COMM(comm).win_id_allocator);
    MPIDI_OFI_index_allocator_destroy(MPIDI_OFI_COMM(comm).rma_id_allocator);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}


#endif /* OFI_COMM_H_INCLUDED */

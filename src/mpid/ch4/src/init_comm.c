/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* During init, in particular, when using MPIR_CVAR_CH4_ROOTS_ONLY_PMI=1,
 * we need create a temporary node_roots only communicator, called init_comm.
 * They are directly called by netmod init hooks as needed.
 */

int MPIDI_create_init_comm(MPIR_Comm ** comm)
{
    int i, mpi_errno = MPI_SUCCESS;
    int world_rank = MPIR_Process.rank;
    int node_root_rank = MPIR_Process.node_root_map[MPIR_Process.node_map[world_rank]];

    /* if the process is not a node root, exit */
    if (node_root_rank == world_rank) {
        int node_roots_comm_size = MPIR_Process.num_nodes;
        int node_roots_comm_rank = MPIR_Process.node_map[world_rank];
        MPIR_Comm *init_comm = NULL;
        MPIDI_rank_map_lut_t *lut = NULL;

        mpi_errno = MPIR_Comm_create(&init_comm);
        MPIR_ERR_CHECK(mpi_errno);

        init_comm->context_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
        init_comm->recvcontext_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
        init_comm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
        init_comm->rank = node_roots_comm_rank;
        init_comm->remote_size = node_roots_comm_size;
        init_comm->local_size = node_roots_comm_size;
        init_comm->coll.pof2 = MPL_pof2(node_roots_comm_size);
        MPIDI_COMM(init_comm, map).mode = MPIDI_RANK_MAP_LUT_INTRA;
        mpi_errno = MPIDIU_alloc_lut(&lut, node_roots_comm_size);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_COMM(init_comm, map).size = node_roots_comm_size;
        MPIDI_COMM(init_comm, map).avtid = 0;
        MPIDI_COMM(init_comm, map).irreg.lut.t = lut;
        MPIDI_COMM(init_comm, map).irreg.lut.lpid = lut->lpid;
        MPIDI_COMM(init_comm, local_map).mode = MPIDI_RANK_MAP_NONE;
        for (i = 0; i < node_roots_comm_size; ++i) {
            lut->lpid[i] = MPIR_Process.node_root_map[i];
        }
        mpi_errno = MPIDIG_init_comm(init_comm);
        MPIR_ERR_CHECK(mpi_errno);

        *comm = init_comm;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDI_destroy_init_comm(MPIR_Comm ** comm_ptr)
{
    int in_use;
    MPIR_Comm *comm = NULL;
    if (*comm_ptr != NULL) {
        comm = *comm_ptr;
        MPIDIU_release_lut(MPIDI_COMM(comm, map).irreg.lut.t);
        MPIDIG_destroy_comm(comm);
        MPIR_Object_release_ref(comm, &in_use);
        MPIR_Assertp(in_use == 0);
        MPII_COMML_FORGET(comm);
        MPIR_Handle_obj_free(&MPIR_Comm_mem, comm);
        *comm_ptr = NULL;
    }
}

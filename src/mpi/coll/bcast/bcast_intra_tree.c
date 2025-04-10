/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* Algorithm: Tree-based bcast
 * For short messages, we use a kary/knomial tree-based algorithm.
 * Cost = lgp.alpha + n.lgp.beta
 */

int MPIR_Bcast_intra_tree(void *buffer,
                          MPI_Aint count,
                          MPI_Datatype datatype,
                          int root, MPIR_Comm * comm_ptr, int tree_type,
                          int branching_factor, int is_nb, MPIR_Errflag_t errflag)
{
    int rank, comm_size, src, dst, *p, j, k, lrank = -1, is_contig;
    int parent = -1, num_children = 0, num_req = 0, is_root = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint nbytes = 0, type_size, actual_packed_unpacked_bytes, recvd_size;
    MPI_Aint saved_count = count;
    MPI_Status status;
    void *send_buf = NULL;
    MPIR_Request **reqs = NULL;
    MPI_Status *statuses = NULL;
    MPI_Datatype dtype;

    MPIR_Treealgo_tree_t my_tree;
    MPIR_CHKLMEM_DECL();

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1)
        goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    send_buf = buffer;
    dtype = datatype;

    if (!is_contig) {
        MPIR_CHKLMEM_MALLOC(send_buf, nbytes);

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Typerep_pack(buffer, count, datatype, 0, send_buf, nbytes,
                                          &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
        count = count * type_size;
        dtype = MPIR_BYTE_INTERNAL;
    }

    if (tree_type == MPIR_TREE_TYPE_KARY) {
        if (rank == root)
            is_root = 1;
        lrank = (rank + (comm_size - root)) % comm_size;
        parent = (lrank == 0) ? -1 : (((lrank - 1) / branching_factor) + root) % comm_size;
        num_children = branching_factor;
    } else {
        /*construct the knomial tree */
        my_tree.children = NULL;
        if (tree_type == MPIR_TREE_TYPE_TOPOLOGY_AWARE ||
            tree_type == MPIR_TREE_TYPE_TOPOLOGY_AWARE_K) {
            mpi_errno =
                MPIR_Treealgo_tree_create_topo_aware(comm_ptr, tree_type, branching_factor, root,
                                                     MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE, &my_tree);
        } else if (tree_type == MPIR_TREE_TYPE_TOPOLOGY_WAVE) {
            int overhead = MPIR_CVAR_BCAST_TOPO_OVERHEAD;
            int lat_diff_groups = MPIR_CVAR_BCAST_TOPO_DIFF_GROUPS;
            int lat_diff_switches = MPIR_CVAR_BCAST_TOPO_DIFF_SWITCHES;
            int lat_same_switches = MPIR_CVAR_BCAST_TOPO_SAME_SWITCHES;

            if (comm_ptr->csel_comm) {
                MPIR_Csel_coll_sig_s coll_sig = {
                    .coll_type = MPIR_CSEL_COLL_TYPE__BCAST,
                    .comm_ptr = comm_ptr,
                    .u.bcast.buffer = buffer,
                    .u.bcast.count = count,
                    .u.bcast.datatype = datatype,
                    .u.bcast.root = root,
                };

                MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
                MPIR_Assert(cnt);

                if (cnt->id == MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree) {
                    overhead = cnt->u.bcast.intra_tree.topo_overhead;
                    lat_diff_groups = cnt->u.bcast.intra_tree.topo_diff_groups;
                    lat_diff_switches = cnt->u.bcast.intra_tree.topo_diff_switches;
                    lat_same_switches = cnt->u.bcast.intra_tree.topo_same_switches;
                }
            }

            mpi_errno =
                MPIR_Treealgo_tree_create_topo_wave(comm_ptr, branching_factor, root,
                                                    MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE,
                                                    overhead, lat_diff_groups, lat_diff_switches,
                                                    lat_same_switches, &my_tree);

        } else {
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, comm_size, tree_type, branching_factor, root,
                                          &my_tree);
        }
        MPIR_ERR_CHECK(mpi_errno);
        num_children = my_tree.num_children;
        parent = my_tree.parent;
    }

    if (is_nb) {
        MPIR_CHKLMEM_MALLOC(reqs, sizeof(MPIR_Request *) * num_children);
        MPIR_CHKLMEM_MALLOC(statuses, sizeof(MPI_Status) * num_children);
    }

    if ((parent != -1 && tree_type != MPIR_TREE_TYPE_KARY)
        || (!is_root && tree_type == MPIR_TREE_TYPE_KARY)) {
        src = parent;
        mpi_errno = MPIC_Recv(send_buf, count, dtype, src, MPIR_BCAST_TAG, comm_ptr, &status);
        MPIR_ERR_CHECK(mpi_errno);
        /* check that we received as much as we expected */
        MPIR_Get_count_impl(&status, MPIR_BYTE_INTERNAL, &recvd_size);
        MPIR_ERR_CHKANDJUMP2(recvd_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                             "**collective_size_mismatch",
                             "**collective_size_mismatch %d %d", (int) recvd_size, (int) nbytes);
    }
    if (tree_type == MPIR_TREE_TYPE_KARY) {
        for (k = 1; k <= branching_factor; k++) {       /* Send to children */
            dst = lrank * branching_factor + k;
            if (dst >= comm_size)
                break;

            dst = (dst + root) % comm_size;

            if (!is_nb) {
                mpi_errno =
                    MPIC_Send(send_buf, count, dtype, dst, MPIR_BCAST_TAG, comm_ptr, errflag);
            } else {
                mpi_errno = MPIC_Isend(send_buf, count, dtype, dst,
                                       MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++], errflag);
            }
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        for (j = 0; j < num_children; j++) {
            p = (int *) utarray_eltptr(my_tree.children, j);
            dst = *p;

            if (!is_nb) {
                mpi_errno =
                    MPIC_Send(send_buf, count, dtype, dst, MPIR_BCAST_TAG, comm_ptr, errflag);
            } else {
                mpi_errno = MPIC_Isend(send_buf, count, dtype, dst,
                                       MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++], errflag);
            }
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    if (is_nb) {
        mpi_errno = MPIC_Waitall(num_req, reqs, statuses);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (tree_type != MPIR_TREE_TYPE_KARY)
        MPIR_Treealgo_tree_free(&my_tree);

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Typerep_unpack(send_buf, nbytes, buffer, saved_count, datatype, 0,
                                            &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

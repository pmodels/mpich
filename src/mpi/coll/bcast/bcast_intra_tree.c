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
                          int branching_factor, int is_nb, MPIR_Errflag_t * errflag)
{
    int rank, comm_size, src, dst, *p, j, k, lrank, is_contig;
    int parent = -1, num_children = 0, num_req = 0, is_root = 0;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint nbytes = 0, type_size, actual_packed_unpacked_bytes, recvd_size;
    MPI_Aint saved_count = count;
    MPI_Status status;
    void *send_buf = NULL;
    MPIR_Request **reqs = NULL;
    MPI_Status *statuses = NULL;
    MPI_Datatype dtype;

    MPIR_Treealgo_tree_t my_tree;
    MPIR_CHKLMEM_DECL(3);

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
        MPIR_CHKLMEM_MALLOC(send_buf, void *, nbytes, mpi_errno, "send_buf", MPL_MEM_BUFFER);

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Typerep_pack(buffer, count, datatype, 0, send_buf, nbytes,
                                          &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
        count = count * type_size;
        dtype = MPI_BYTE;
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
        mpi_errno =
            MPIR_Treealgo_tree_create(rank, comm_size, tree_type, branching_factor, root, &my_tree);
        MPIR_ERR_CHECK(mpi_errno);
        num_children = my_tree.num_children;
        parent = my_tree.parent;
    }

    if (is_nb) {
        MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, sizeof(MPIR_Request *) * num_children,
                            mpi_errno, "request array", MPL_MEM_COLL);
        MPIR_CHKLMEM_MALLOC(statuses, MPI_Status *, sizeof(MPI_Status) * num_children,
                            mpi_errno, "status array", MPL_MEM_COLL);
    }

    if ((parent != -1 && tree_type != MPIR_TREE_TYPE_KARY)
        || (!is_root && tree_type == MPIR_TREE_TYPE_KARY)) {
        src = parent;
        mpi_errno = MPIC_Recv(send_buf, count, dtype, src,
                              MPIR_BCAST_TAG, comm_ptr, &status, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        /* check that we received as much as we expected */
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        if (recvd_size != nbytes) {
            if (*errflag == MPIR_ERR_NONE)
                *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
                          "**collective_size_mismatch",
                          "**collective_size_mismatch %d %d", recvd_size, nbytes);
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

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
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
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
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        }
    }
    if (is_nb) {
        mpi_errno = MPIC_Waitall(num_req, reqs, statuses, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
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
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

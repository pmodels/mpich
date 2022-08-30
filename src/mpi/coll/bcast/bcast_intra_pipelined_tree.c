/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Algorithm: Pipelined bcast
 * For large messages, we use the tree-based pipelined algorithm.
 * Time = Time to send the first chunk to the rightmost child + (num_chunks - 1) * software overhead to inject a chunk
 */
int MPIR_Bcast_intra_pipelined_tree(void *buffer,
                                    MPI_Aint count,
                                    MPI_Datatype datatype,
                                    int root, MPIR_Comm * comm_ptr, int tree_type,
                                    int branching_factor, int is_nb, int chunk_size,
                                    int recv_pre_posted, MPIR_Errflag_t * errflag)
{
    int rank, comm_size, i, j, k, *p, src = -1, dst, offset = 0;
    int is_contig;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint type_size, num_chunks, chunk_size_floor, chunk_size_ceil;
    MPI_Aint true_lb, true_extent, recvd_size, actual_packed_unpacked_bytes, nbytes = 0;
    void *sendbuf = NULL;
    int parent = -1, num_children = 0, lrank = 0, num_req = 0;
    MPIR_Request **reqs = NULL;
    MPI_Status *statuses = NULL;
    MPIR_Treealgo_tree_t my_tree;
    MPIR_CHKLMEM_DECL(3);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1)
        goto fn_exit;

    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size(1, datatype, &type_size);
    }

    nbytes = type_size * count;

    if (nbytes == 0)
        goto fn_exit;

    if (is_contig) {    /* no need to pack */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        sendbuf = (char *) buffer + true_lb;
    } else {
        MPIR_CHKLMEM_MALLOC(sendbuf, void *, nbytes, mpi_errno, "sendbuf", MPL_MEM_BUFFER);
        if (rank == root) {
            mpi_errno = MPIR_Typerep_pack(buffer, count, datatype, 0, sendbuf, nbytes,
                                          &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* treat all cases as MPI_BYTE */
    MPIR_Algo_calculate_pipeline_chunk_info(chunk_size, 1, nbytes, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    if (tree_type == MPIR_TREE_TYPE_KARY) {
        lrank = (rank + (comm_size - root)) % comm_size;
        parent = (lrank == 0) ? -1 : (((lrank - 1) / branching_factor) + root) % comm_size;
        num_children = branching_factor;
    } else {
        mpi_errno =
            MPIR_Treealgo_tree_create(rank, comm_size, tree_type, branching_factor, root, &my_tree);
        MPIR_ERR_CHECK(mpi_errno);
        num_children = my_tree.num_children;
    }

    if (is_nb) {
        MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **,
                            sizeof(MPIR_Request *) * (num_children * num_chunks + num_chunks),
                            mpi_errno, "request array", MPL_MEM_COLL);
        MPIR_CHKLMEM_MALLOC(statuses, MPI_Status *,
                            sizeof(MPI_Status) * (num_children * num_chunks + num_chunks),
                            mpi_errno, "status array", MPL_MEM_COLL);
    }

    if (tree_type != MPIR_TREE_TYPE_KARY && my_tree.parent != -1)
        src = my_tree.parent;
    else if (tree_type == MPIR_TREE_TYPE_KARY && parent != -1)
        src = parent;

    if (is_nb) {
        if (num_chunks > 3 && !recv_pre_posted) {
            /* For large number of chunks, pre-posting all the receives can add overhead
             * so posting three IRecvs to keep the pipeline going*/
            for (i = 0; i < 3; i++) {
                MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;

                if (src != -1) {        /* post receive from parent */
                    mpi_errno =
                        MPIC_Irecv((char *) sendbuf + offset, msgsize, MPI_BYTE,
                                   src, MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++]);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                }
                offset += msgsize;
            }

        } else {
            /* For small number of chunks, all the receives can be pre-posted */
            for (i = 0; i < num_chunks; i++) {
                MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;
                if (src != -1) {
                    mpi_errno =
                        MPIC_Irecv((char *) sendbuf + offset, msgsize, MPI_BYTE,
                                   src, MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++]);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                }
                offset += msgsize;
            }
        }
    }
    offset = 0;

    for (i = 0; i < num_chunks; i++) {
        MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;

        if ((num_chunks <= 3 && is_nb) || (recv_pre_posted && is_nb)) {
            /* Wait to receive the chunk before it can be sent to the children */
            if (src != -1) {
                mpi_errno = MPIC_Wait(reqs[i], errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                MPIR_Get_count_impl(&reqs[i]->status, MPI_BYTE, &recvd_size);
                if (recvd_size != msgsize) {
                    if (*errflag == MPIR_ERR_NONE)
                        *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
                                  "**collective_size_mismatch",
                                  "**collective_size_mismatch %d %d", recvd_size, msgsize);
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else if (num_chunks > 3 && is_nb && i < 3 && !recv_pre_posted) {
            /* Wait to receive the chunk before it can be sent to the children */
            if (src != -1) {
                mpi_errno = MPIC_Wait(reqs[i], errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                MPIR_Get_count_impl(&reqs[i]->status, MPI_BYTE, &recvd_size);
                if (recvd_size != msgsize) {
                    if (*errflag == MPIR_ERR_NONE)
                        *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
                                  "**collective_size_mismatch",
                                  "**collective_size_mismatch %d %d", recvd_size, msgsize);
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else {
            /* Receive message from parent */
            if (src != -1) {
                mpi_errno =
                    MPIC_Recv((char *) sendbuf + offset, msgsize, MPI_BYTE,
                              src, MPIR_BCAST_TAG, comm_ptr, &status, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
                if (recvd_size != msgsize) {
                    if (*errflag == MPIR_ERR_NONE)
                        *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
                                  "**collective_size_mismatch",
                                  "**collective_size_mismatch %d %d", recvd_size, msgsize);
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

            }
        }
        if (tree_type == MPIR_TREE_TYPE_KARY) {
            /* Send data to the children */
            for (k = 1; k <= branching_factor; k++) {
                dst = lrank * branching_factor + k;
                if (dst >= comm_size)
                    break;

                dst = (dst + root) % comm_size;

                if (!is_nb) {
                    mpi_errno =
                        MPIC_Send((char *) sendbuf + offset, msgsize, MPI_BYTE, dst,
                                  MPIR_BCAST_TAG, comm_ptr, errflag);
                } else {
                    mpi_errno =
                        MPIC_Isend((char *) sendbuf + offset, msgsize, MPI_BYTE, dst,
                                   MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++], errflag);
                }
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

            }
        } else if (num_children) {
            /* Send data to the children */
            for (j = 0; j < num_children; j++) {
                p = (int *) utarray_eltptr(my_tree.children, j);
                dst = *p;
                if (!is_nb) {
                    mpi_errno = MPIC_Send((char *) sendbuf + offset, msgsize, MPI_BYTE, dst,
                                          MPIR_BCAST_TAG, comm_ptr, errflag);
                } else {
                    mpi_errno =
                        MPIC_Isend((char *) sendbuf + offset, msgsize, MPI_BYTE, dst,
                                   MPIR_BCAST_TAG, comm_ptr, &reqs[num_req++], errflag);
                }
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            }
        }
        offset += msgsize;
    }

    if (is_nb) {
        mpi_errno = MPIC_Waitall(num_req, reqs, statuses, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Typerep_unpack(sendbuf, nbytes, buffer, count, datatype, 0,
                                            &actual_packed_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    if (tree_type != MPIR_TREE_TYPE_KARY)
        MPIR_Treealgo_tree_free(&my_tree);
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

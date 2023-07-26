/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "bcast.h"

/*
 * Adaptive Bcast creates a priority queue of ranks (0th index in the queue is most important rank to send to and (comm_size - 1)th 
 * index is the least important) and sends using the binomial tree algorithm with sends reversed. 
 * If optimized properly, this should perform the same if not better than the SMP algorithm. */

int MPIR_Bcast_intra_adaptive(void* buffer, MPI_Aint count, MPI_Datatype datatype, int root, 
                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag) {
    
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint nbytes = 0;
    MPI_Status *status_p;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif
    int is_contig;
    MPI_Aint type_size;
    void *tmp_buf = NULL;
    MPIR_CHKLMEM_DECL(3);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;


    int break_point;
    struct Rank_Info * ranks = NULL;
    int* priority_queue = NULL;

    MPIR_CHKLMEM_MALLOC(ranks, struct Rank_Info * , sizeof(struct Rank_Info) * comm_size, mpi_errno, "ranks", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC(priority_queue, int*, sizeof(int) * comm_size, mpi_errno, "priority_queue", MPL_MEM_BUFFER);
    retrieve_weights(comm_ptr, ranks);
    build_queue(comm_ptr, ranks, priority_queue);

    
    if (!rank) {
        for (int i = 0; i < comm_size; i++) {
            printf("%d\n", priority_queue[i]);
        }
    }
    

    if (HANDLE_IS_BUILTIN(datatype))
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if (!is_contig) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;
            if (!is_contig)
                mpi_errno = MPIC_Recv(tmp_buf, nbytes, MPI_BYTE, src,
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            else
                mpi_errno = MPIC_Recv(buffer, count, datatype, src,
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
#ifdef HAVE_ERROR_CHECKING
            /* check that we received as much as we expected */
            MPIR_Get_count_impl(status_p, MPI_BYTE, &recvd_size);
            MPIR_ERR_COLL_CHECK_SIZE(recvd_size, nbytes, errflag, mpi_errno_ret);
#endif
            break;
        }
        mask <<= 1;
    }

    mask >>= 1;

    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size)
                dst -= comm_size;
            if (!is_contig)
                mpi_errno = MPIC_Send(tmp_buf, nbytes, MPI_BYTE, dst,
                                      MPIR_BCAST_TAG, comm_ptr, errflag);
            else
                mpi_errno = MPIC_Send(buffer, count, datatype, dst,
                                      MPIR_BCAST_TAG, comm_ptr, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
        }
        mask >>= 1;
    }

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);

        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;

}


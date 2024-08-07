/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "bcast.h"

int MPIR_Bcast_intra_binomial_group(void *buffer,
                              MPI_Aint count,
                              MPI_Datatype datatype,
                              int root, MPIR_Comm * comm_ptr, int* group, int group_size, MPIR_Errflag_t errflag)
{
    int rank, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint nbytes = 0;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status *status_p;
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif
    int is_contig;
    MPI_Aint type_size;
    void *tmp_buf = NULL; 
    MPIR_CHKLMEM_DECL(1);

    rank = comm_ptr->rank;
    int group_rank; // local ranking of the process within the group
    int group_root; // the root of the group
    
    bool found_rank_in_group = find_local_rank_linear(group, group_size, rank, root, &group_rank, &group_root);
    if (!found_rank_in_group) goto fn_exit;
    
    /* Uncomment the below code snippet for isolated testing */

    // if (!found_rank_in_group) {
    //     return mpi_errno_ret;
    // }

    MPIR_Assert(found_rank_in_group);

    if (HANDLE_IS_BUILTIN(datatype))
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size); 

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;

    if (!is_contig) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE); 
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    relative_rank = (group_rank >= group_root) ? group_rank - group_root : group_rank - group_root + group_size;

    mask = 0x1;
    while (mask < group_size) {
        if (relative_rank & mask) {
            src = group_rank - mask;
            
            if (src < 0)
                src += group_size;
            if (!is_contig)
                mpi_errno = MPIC_Recv(tmp_buf, nbytes, MPI_BYTE, group[src],
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            else
                mpi_errno = MPIC_Recv(buffer, count, datatype, group[src],
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
#ifdef HAVE_ERROR_CHECKING            
            MPIR_Get_count_impl(status_p, MPI_BYTE, &recvd_size);
            MPIR_ERR_COLL_CHECK_SIZE(recvd_size, nbytes, errflag, mpi_errno_ret);
#endif

            break;
        }
        mask <<= 1;
    }

    mask >>= 1;
    while (mask > 0) {
    
        if (relative_rank + mask < group_size) {
            dst = group_rank + mask;
            if (dst >= group_size)
                dst -= group_size;
            if (!is_contig)
                mpi_errno = MPIC_Send(tmp_buf, nbytes, MPI_BYTE, group[dst],
                                      MPIR_BCAST_TAG, comm_ptr, errflag);
            else
                mpi_errno = MPIC_Send(buffer, count, datatype, group[dst],
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

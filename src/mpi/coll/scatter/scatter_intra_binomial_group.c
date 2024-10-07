/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "scatter.h"

int MPIR_Scatter_intra_binomial_group(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                MPIR_Comm * comm_ptr, int* group, int group_size, MPIR_Errflag_t errflag)
{
    MPI_Status status;
    MPI_Aint extent = 0;
    int rank;
    int relative_rank;
    MPI_Aint curr_cnt, send_subtree_cnt;
    int mask, src, dst;
    MPI_Aint tmp_buf_size = 0;
    void *tmp_buf = NULL;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(4);

    rank = comm_ptr->rank;
    int group_rank, group_root;
    bool found_rank_in_group;

    found_rank_in_group = find_local_rank_linear(group, group_size, rank, &group_rank, &group_root);
    
    MPIR_Assert(found_rank_in_group);

    if (rank == root)
        MPIR_Datatype_get_extent_macro(sendtype, extent);

    relative_rank = (group_rank >= group_root) ? group_rank - group_root : group_rank - group_root + group_size;

    MPI_Aint nbytes;
    if (rank == root) {
        
        MPI_Aint sendtype_size;
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount;
    } else {
        MPI_Aint recvtype_size;
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount;
    }

    curr_cnt = 0;

    
    if (relative_rank && !(relative_rank % 2)) {
        tmp_buf_size = (nbytes * comm_size) / 2;
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
    }

    if (rank == root) {
        if (group_root != 0) {
            tmp_buf_size = nbytes * group_size;
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf",
                                MPL_MEM_BUFFER);

            if (recvbuf != MPI_IN_PLACE)
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent * sendcount * group_rank),
                                           sendcount * (group_size - group_rank), sendtype, tmp_buf,
                                           nbytes * (group_size - group_rank), MPI_BYTE);
            else
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent * sendcount * (group_rank + 1)),
                                           sendcount * (group_size - group_rank - 1),
                                           sendtype, (char *) tmp_buf + nbytes,
                                           nbytes * (group_size - group_rank - 1), MPI_BYTE);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Localcopy(sendbuf, sendcount * group_rank, sendtype,
                                       ((char *) tmp_buf + nbytes * (group_size - group_rank)),
                                       nbytes * group_rank, MPI_BYTE);
            MPIR_ERR_CHECK(mpi_errno);

            curr_cnt = nbytes * group_size;
        } else
            curr_cnt = sendcount * group_size;
    }

    mask = 0x1;
    while (mask < group_size) {
        if (relative_rank & mask) {
            src = group_rank - mask;
            if (src < 0)
                src += group_size;

            if (relative_rank % 2) {
                mpi_errno = MPIC_Recv(recvbuf, recvcount, recvtype,
                                      group[src], MPIR_SCATTER_TAG, comm_ptr, &status);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            } else {
                mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPI_BYTE, group[src],
                                      MPIR_SCATTER_TAG, comm_ptr, &status);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                if (mpi_errno) {
                    curr_cnt = 0;
                } else
                    MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
            }
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

            if ((rank == root) && (group_root == 0)) {
                send_subtree_cnt = curr_cnt - sendcount * mask;
                
                mpi_errno = MPIC_Send(((char *) sendbuf +
                                       extent * sendcount * mask),
                                      send_subtree_cnt,
                                      sendtype, group[dst], MPIR_SCATTER_TAG, comm_ptr, errflag);
            } else {
                
                send_subtree_cnt = curr_cnt - nbytes * mask;
              
                mpi_errno = MPIC_Send(((char *) tmp_buf + nbytes * mask),
                                      send_subtree_cnt,
                                      MPI_BYTE, group[dst], MPIR_SCATTER_TAG, comm_ptr, errflag);
            }
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            curr_cnt -= send_subtree_cnt;
        }
        mask >>= 1;
    }

    if ((rank == root) && (group_root == 0) && (recvbuf != MPI_IN_PLACE)) {

        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
        
    } else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
        
        mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPI_BYTE, recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}

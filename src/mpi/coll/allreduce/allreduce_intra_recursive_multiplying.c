/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <math.h>

#include "mpiimpl.h"

/*
 * Algorithm: Recursive Multiplying
 *
 * This algorithm is a generalization of the recursive doubling algorithm.
 * Setting k according to the network hierarchy (e.g., the number of NICs
 * in a node) can improve performance.
 */


int MPIR_Allreduce_intra_recursive_multiplying(const void *sendbuf,
                                            void *recvbuf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            MPI_Op op,
                                            MPIR_Comm * comm_ptr, 
                                            const int k,
                                            MPIR_Errflag_t errflag)
{
    /* If the op is non-commutative, fall back to recursive doubling */
    if(!MPIR_Op_is_commutative(op)) {
      return MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                op, comm_ptr, errflag);
    }

    /*
     * If the comm size is not a multiple of k, fall back to recursive
     * doubling. Recursive multiplying performs best when k is a multiple
     * of comm size, so this case should not be used anyways.
     */
    int comm_size, rank, virt_rank;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    if(comm_size % k) {
      return MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                op, comm_ptr, errflag);
    }

    MPIR_CHKLMEM_DECL(2);
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int dst, pofk;
    int comm_size_div_pofk, comm_size_mod_pofk, group, group_size, group_leader;
    int radix_size, next_radix_size, next_radix_root, radix_loc;
    int dst_root, dst_root_send, dst_group_size, real_dst;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf, *red_buf;
    MPIR_Request **reqs;
    int i, p;
    int num_reqs = 0, parity = 0;

    /* need to allocate temporary buffer to store incoming data */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, (k - 1) * count * extent, mpi_errno,
                        "temporary buffer", MPL_MEM_BUFFER); // cut to half
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, (2 * (k - 1) * sizeof(MPIR_Request *)), mpi_errno,
                        "reqs", MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* get nearest power-of-two less than or equal to comm_size */
    p = (int) (log(comm_size) / log(k));
    pofk = (int) pow(k, p);

    /* Compute group-specific information */
    comm_size_div_pofk = comm_size / pofk;
    comm_size_mod_pofk = comm_size % pofk;

    if (rank > (comm_size_div_pofk + 1) * (comm_size_mod_pofk)) {
        group = (comm_size_mod_pofk) + (rank - (comm_size_div_pofk + 1) * comm_size_mod_pofk) / comm_size_div_pofk;
    } else {
        group = rank / (comm_size_div_pofk + 1);
    }

    group_size = (comm_size_div_pofk) + (group < (comm_size_mod_pofk) ? 1 : 0);
    group_leader = group * (comm_size_div_pofk) + MPL_MIN(group, comm_size_mod_pofk) + group_size - 1;

    /* PRE-STAGE: Followers should send data to leaders and leaders 
     * should receive data from followers */
    if (rank == group_leader) {
        /* This is leader so receive data */
        for (i = 0; i < group_size - 1; i++) {
            dst = rank - group_size + i + 1;
            mpi_errno = MPIC_Irecv(tmp_buf + i * count * extent, count,
                                  datatype, dst,
                                  MPIR_ALLREDUCE_TAG, comm_ptr, &reqs[num_reqs++]);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        
        MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);

        for (i = group_size - 2; i >= 0; i--) {
            /* Perform local reduction */
            mpi_errno = MPIR_Reduce_local(tmp_buf + i * count * extent, recvbuf, count, datatype, op);
            MPIR_ERR_CHECK(mpi_errno);  
        }

        /* Virtual rank should be used for leader for recursive operations */
        virt_rank = group;
    } else {
        /* This is follower so send data */
        mpi_errno = MPIC_Send(recvbuf, count,
                              datatype, group_leader, MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        /* Virtual rank should not be used */
        virt_rank = -1;
    }

    /*MAIN-STAGE*/
    if (virt_rank != -1) {
        radix_size = 1;
        next_radix_size = k;

        while (radix_size < pofk) {
            next_radix_root = (virt_rank / next_radix_size) * next_radix_size;
            radix_loc = (int) virt_rank % radix_size;
            dst_root = next_radix_root + next_radix_size - radix_size;

            /* Asynchronous sends */
            num_reqs = 0;
            i = 0;
            dst_root_send = dst_root;
            while (dst_root_send >= next_radix_root) {
                dst = dst_root_send + radix_loc;
                
                if(dst != virt_rank) {
                    dst_group_size = (comm_size / pofk) + (dst < (comm_size % pofk) ? 1 : 0);
                    real_dst = dst * (comm_size / pofk) + MPL_MIN(dst, comm_size % pofk) + dst_group_size - 1;

                    mpi_errno = MPIC_Isend(recvbuf, count, datatype, real_dst, MPIR_ALLREDUCE_TAG, comm_ptr,
                                        &reqs[num_reqs++], errflag);
                    if (mpi_errno) {
                        MPIR_ERR_POP(mpi_errno);
                    }

                    mpi_errno = MPIC_Irecv(tmp_buf + i * count * extent, count, datatype, real_dst, MPIR_ALLREDUCE_TAG, comm_ptr,
                                        &reqs[num_reqs++]);
                    if (mpi_errno) {
                        MPIR_ERR_POP(mpi_errno);
                    }   

                    i++;
                }
                dst_root_send -= radix_size;
            }

            /* Wait for Asynchronous operations to complete */
            MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);

            /* Perform reduction on the received values */
            i = 0;
            while (dst_root >= next_radix_root) {
                dst = dst_root + radix_loc;

                if (dst_root == (next_radix_root + next_radix_size - radix_size)) {
                    if (dst == virt_rank) {
                        red_buf = recvbuf;
                        parity = 0;
                    } else {
                        red_buf = tmp_buf;
                        parity = 1;
                        i++;
                    }
                } else {
                    if (dst == virt_rank) {
                        mpi_errno = MPIR_Reduce_local(recvbuf, red_buf, count, datatype, op);
                        MPIR_ERR_CHECK(mpi_errno);
                    } else {
                        mpi_errno = MPIR_Reduce_local(tmp_buf + i * count * extent, red_buf, count, datatype, op);
                        MPIR_ERR_CHECK(mpi_errno);
                        i++;
                    }
                }

                dst_root -= radix_size;
            }

            if (parity) {
                mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype, recvbuf, count, datatype);
                MPIR_ERR_CHECK(mpi_errno);
            }

            radix_size = next_radix_size;
            next_radix_size *= k;
        }
    }

    /* POST-STAGE: Leaders should send reduced data to all its followers */
    if (rank == group_leader) {
        /* This is leader so send data */
        num_reqs = 0;
        for (i = (rank - 1); i > rank - group_size; i--) {
            mpi_errno = MPIC_Isend(recvbuf, count, datatype, i, MPIR_ALLREDUCE_TAG, comm_ptr,
                                    &reqs[num_reqs++], errflag);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }

        MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);
    } else {
        /* This is follower so receive data */
        mpi_errno = MPIC_Recv(recvbuf, count,
                              datatype, group_leader,
                              MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);
    }
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

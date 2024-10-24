/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include <math.h>

/*
 * Algorithm: Recursive Multiplying
 *
 * This algorithm is a generalization of the recursive doubling algorithm,
 * and it has three stages. In the first stage, ranks above the nearest
 * power of k less than or equal to comm_size collapse their data to the 
 * lower ranks. The main stage proceeds with power-of-k ranks. In the main
 * stage, ranks exchange data within groups of size k in rounds with 
 * increasing distance (k, k^2, ...). Lastly, those in the main stage 
 * disperse the result back to the excluded ranks. Setting k according 
 * to the network hierarchy (e.g., the number of NICs in a node) can 
 * improve performance.
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
    int mpi_errno = MPI_SUCCESS;
    /* Ensure the op is commutative */
    MPIR_Assert(MPIR_Op_is_commutative(op));

    int comm_size, rank, virt_rank;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    virt_rank = rank;
    
    /* get nearest power-of-two less than or equal to comm_size */
    int power = (int) (log(comm_size) / log(k));
    int pofk = (int) lround(pow(k, power));

    MPIR_CHKLMEM_DECL(2);
    void *tmp_buf;

    /* need to allocate temporary buffer to store incoming data */
    MPI_Aint true_extent, true_lb, extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPI_Aint single_node_data_size = extent * count - (extent - true_extent);

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, (k - 1) * count * single_node_data_size, mpi_errno,
                        "temporary buffer", MPL_MEM_BUFFER); // cut to half
    
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* PRE-STAGE: ranks over pofk collapse their data into the others
     * using reverse round-robin */
    if (pofk < comm_size) {
        if (rank >= pofk) {
            /* This process is outside the core algorithm so send data */
            int pre_dst = pofk - (rank % pofk) - 1;
            /* This is follower so send data */
            mpi_errno = MPIC_Send(recvbuf, count, datatype,
                    pre_dst, MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
            MPIR_ERR_CHECK(mpi_errno);
            /* Set virtual rank so this rank is not used in main stage */
            virt_rank = -1;
        } else {
            /* Receive data from all those greater than pofk */
            for (int pre_src = 2 * pofk - (rank % pofk) - 1; pre_src < comm_size; pre_src += pofk) {
                mpi_errno = MPIC_Recv(tmp_buf, count, datatype, pre_src,
                        MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    /*MAIN-STAGE: Ranks exchange data with groups size k over increasing
     * distances */
    if (virt_rank != -1) {
        /*Allocate for nb requests*/
        MPIR_Request **reqs;
        int num_reqs = 0;
        MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, (2 * (k - 1) * sizeof(MPIR_Request *)), mpi_errno,
                "reqs", MPL_MEM_BUFFER);

        /*Do exchanges*/
        int exchanges = 0;
        int distance = 1;
        int next_distance = k;
        while (distance < pofk) {
            /* Asynchronous sends */
            
            int starting_rank = rank/next_distance * next_distance;
            for(int dst = starting_rank; dst < starting_rank + next_distance; dst++) {
                int dst_distance = (dst > rank) ? (dst - rank) : (rank - dst);
                if(dst != rank && !(dst_distance % distance)) {
                    mpi_errno = MPIC_Isend(recvbuf, count, datatype, dst, MPIR_ALLREDUCE_TAG, comm_ptr,
                                        &reqs[num_reqs++], errflag);
                    MPIR_ERR_CHECK(mpi_errno);
                    mpi_errno = MPIC_Irecv(((char *)tmp_buf) + exchanges * count * extent, count, datatype, dst, MPIR_ALLREDUCE_TAG, comm_ptr,
                                        &reqs[num_reqs++]);
                    MPIR_ERR_CHECK(mpi_errno);
                    exchanges++;
                }
            }

            /* Wait for asynchronous operations to complete */
            MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);
            num_reqs = 0;
            exchanges = 0;

            /* Perform reduction on the received values */
            for(int i = 0; i < k - 1; i++) {
                mpi_errno = MPIR_Reduce_local(((char *)tmp_buf) + i * count * extent, recvbuf, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
            }

            distance = next_distance;
            next_distance *= k;
        }
    }

    /* POST-STAGE: Send result to ranks outside main algorithm */
    if(pofk < comm_size) {
        if(rank >= pofk) {
            int post_src = pofk - (rank % pofk) - 1;
            /* This process is outside the core algorithm, so receive data */
            mpi_errno = MPIC_Recv(recvbuf, count,
                                  datatype, post_src,
                                  MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* This is process is in the algorithm, so send data */
            for (int post_dst = 2 * pofk - (rank % pofk) - 1; post_dst < comm_size; post_dst += pofk) {
                mpi_errno = MPIC_Send(recvbuf, count, datatype, post_dst, MPIR_ALLREDUCE_TAG, comm_ptr,
                                        errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

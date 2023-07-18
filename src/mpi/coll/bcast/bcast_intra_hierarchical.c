/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"



int MPIR_Bcast_intra_hierarchical(void* buffer, MPI_Aint count, MPI_Datatype datatype, int root, 
                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag) 
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;
    MPI_Status *status_p;

#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif

// It is probabaly necessary to have the parent communicator here

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
#endif
    
    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */
    
    int* intranode_table;
    int* internode_table; 
    int* node_group;
    int* local_group;
    int node_size, local_size;
    int rank, node_rank, local_rank;

    double external_process_weight = 1.0;
    double internal_process_weight = 1.0;

    /* TODO: calculate weights */

    rank = comm_ptr->rank;

    /*
    
    Possible addition to the hierarchical algorithm. It seems as though for really small messages sizes, the flat binomial bcast outperforms the hierarchical
    Bcast.
    
    if (nbytes < MPIR_CVAR_BCAST_SMP_MSG_SIZE_MIN) {
        MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
        goto fn_exit;
    }

    */

    /* Retrieves the intranode group */
    MPIR_Find_local(comm_ptr, &local_size, &local_rank, &local_group, &intranode_table);

    /* Retrieves the internode group */
    MPIR_Find_external(comm_ptr, &node_size, &node_rank, &node_group, &internode_table);


    /* If the root process is not an external process, we need to perform a send to the external process on the root's node */
    if (intranode_table[root] > 0) {
        if (rank == root) {
            mpi_errno = MPIC_Send(buffer, count, datatype, local_group[0],
                                    MPIR_BCAST_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

        /* If we are on an external process and on the same node as root */
        } else if (node_rank != -1) {
            mpi_errno =
                MPIC_Recv(buffer, count, datatype, root,
                            MPIR_BCAST_TAG, comm_ptr, status_p);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
#ifdef HAVE_ERROR_CHECKING
            /* check that we received as much as we expected */
            MPIR_Get_count_impl(status_p, MPI_BYTE, &recvd_size);
            MPIR_ERR_COLL_CHECK_SIZE(recvd_size, nbytes, errflag, mpi_errno_ret);
#endif
        }
    }


    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) 
        || (local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        
        /* Perform the internode broadcast */
        if (node_rank != -1) { // If node_rank is not -1 then this process must be an external process
            mpi_errno = MPIR_Bcast_intra_binomial_group(buffer, count, datatype, node_group[MPIR_Get_internode_rank(comm_ptr, root)], comm_ptr, node_group, node_size, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
        }

        mpi_errno = MPIR_Bcast_intra_binomial_group(buffer, count, datatype, local_group[0], comm_ptr, local_group, local_size, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

    } else {

        /* TODO: Need to add support for medium sized msgs and pof2 number of processes*/
        if (node_rank != -1) {
            mpi_errno = MPIR_Bcast_intra_scatter_ring_allgather_group(buffer, count, datatype, node_group[MPIR_Get_internode_rank(comm_ptr, root)], comm_ptr, node_group, node_size, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
        }

        mpi_errno = MPIR_Bcast_intra_scatter_ring_allgather_group(buffer, count, datatype, local_group[0], comm_ptr, local_group, local_size, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

    }

    fn_exit:
        return mpi_errno_ret;

}








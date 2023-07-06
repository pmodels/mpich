/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"



int MPIR_Bcast_intra_hierarchical(void* buffer, MPI_Aint count, MPI_Datatype datatype, int root, 
                                  MPIR_Comm * comm_ptr, int** hierarchy, int* group_sizes, int hierarchy_size, MPIR_Errflag_t errflag) 
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;
    MPI_Status *status_p;

    int rank, group_idx, group_root, group_size;
    int* group;
    bool is is_group_root;

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

    rank = comm_ptr->rank;

    /* TODO: find_group_idx needs to be implemented */

    group_idx = find_group_idx(hierarchy, hierarchy_size, group_sizes, rank);

    MPIR_Assert(group_idx > 0);

    group = hierarchy[group_idx];
    group_root = group[0];
    group_size = group_sizes[group_idx];
    is_group_root = group[0] == rank;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
    (group_sizes[group_idx] < MPIR_CVAR_BCAST_MIN_PROCS)) {
        // small msgs or small num procs
        MPIR_Bcast_intra_binomial_group(buffer,
                              count,
                              datatype,
                              group_root, comm_ptr, group, group_size, errflag);
        
    } else {
        // large and medium sized msgs
        MPIR_Bcast_intra_scatter_ring_allgather_group(buffer, 
                                            count, 
                                            datatype, 
                                            group_root, comm_ptr, group, group_size, errflag);
    }

    fn_exit:
        return mpi_errno_ret;

}








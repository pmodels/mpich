/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*



*/

int MPIR_Bcast_intra_adaptive(void* buffer, MPI_Aint count, MPI_Datatype datatype, int root, 
                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag) {
    
    int rank, comm_size, src, dst;
    int relative_rank, mask;
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

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
#endif
    
    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    rank = comm_ptr->rank;
    comm_size = comm_ptr->local_size;




    fn_exit:
        MPIR_CHKLMEM_FREEALL();
        return mpi_errno_ret;
    fn_fail:
        mpi_errno_ret = mpi_errno;
        goto fn_exit;

}


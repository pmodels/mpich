/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_ring ( 
    const void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcount,
    MPI_Datatype recvtype,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint   recvtype_extent;
    int j, i;
    int type_size, left, right, jnext;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro( recvtype, recvtype_extent );
    MPIR_Datatype_get_size_macro( recvtype, type_size );

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
				     (comm_size * recvcount * recvtype_extent));

    /* First, load the "local" version in the recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, 
                                   ((char *)recvbuf +
                                    rank*recvcount*recvtype_extent),  
                                   recvcount, recvtype);
        if (mpi_errno) { 
            MPIR_ERR_POP(mpi_errno);
        }
    }
    
    /* 
       Now, send left to right.  This fills in the receive area in 
       reverse order.
    */
    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;
        
    j     = rank;
    jnext = left;
    for (i=1; i<comm_size; i++) {
        mpi_errno = MPIC_Sendrecv(((char *)recvbuf +
                                      j*recvcount*recvtype_extent), 
                                     recvcount, recvtype, right,
                                     MPIR_ALLGATHER_TAG, 
                                     ((char *)recvbuf +
                                      jnext*recvcount*recvtype_extent), 
                                     recvcount, recvtype, left, 
                                     MPIR_ALLGATHER_TAG, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        j	    = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

 fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

 fn_fail:
    goto fn_exit;
}

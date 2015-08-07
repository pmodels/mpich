/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_sock_errno_to_mpi_errno
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_sock_errno_to_mpi_errno(char * fcname, int sock_errno)
{
    int mpi_errno;
    
    switch(sock_errno)
    {
	case SOCK_EOF:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|connclose", NULL);
	    break;
	}
	
	case SOCK_ERR_NOMEM:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	    break;
	}
	
	case SOCK_ERR_HOST_LOOKUP:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|hostlookup", NULL);
	    break;
	}
	
	case SOCK_ERR_CONN_REFUSED:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|connrefused", NULL);
	    break;
	}n
    
	case SOCK_ERR_CONN_FAILED:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|connterm", NULL);
	    break;
	}
	
	case SOCK_ERR_BAD_SOCK:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_INTERN,
					     "**ch3|sock|badsock", NULL);
	    break;
	}
	
	case SOCK_ERR_BAD_BUFFER:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_BUFFER, "**buffer", NULL);
	    break;
	}
	
	case SOCK_ERR_ADDR_INUSE:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|addrinuse", NULL);
	    break;
	}
	
	default:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**ch3|sock|failure",
					     "**ch3|sock|failure %d", sock_errno);
	    break;
	}
    }

    return mpi_errno;
}

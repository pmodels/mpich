/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
 
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_spawn_multiple(int count, char **commands, 
                                  char ***argvs, int *maxprocs, 
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}

/* This function initializes the table of routines used for 
   implementing the MPI Port-related functions.  This channel does not
   support those operations, so all functions are set to NULL */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns )
{
    portFns->OpenPort    = 0;
    portFns->ClosePort   = 0;
    portFns->CommAccept  = 0;
    portFns->CommConnect = 0;
    
    return MPI_SUCCESS;
}

int MPIDI_Comm_spawn_multiple(int count, char **commands, 
			      char ***argvs, int *maxprocs, 
			      MPID_Info **info_ptrs, int root,
			      MPID_Comm *comm_ptr, MPID_Comm
			      **intercomm, int *errcodes)
{
    return MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, "MPIDI_Comm_spawn_multiple", __LINE__, MPI_ERR_OTHER,"**notimpl", 0);
}

int MPIDI_CH3_GetParentPort(char ** parent_port)
{
    return MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, "MPIDI_CH3_GetParentPort", __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
}


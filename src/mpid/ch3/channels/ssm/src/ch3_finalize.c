/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPIDI_CH3I_Progress_finalize();

    /* Free memory allocated in ch3_progress */
    mpi_errno = MPIDI_CH3U_Finalize_ssm_memory();

    /* Free other (common) shared-memory */
    mpi_errno = MPIDI_CH3U_Finalize_sshm();

    return mpi_errno;
}

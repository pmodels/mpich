/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPI_SUCCESS;

    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**finalize_progress_finalize");
    }

    /* FIXME: This should be used in abort as well */
    /* Free resources allocated in CH3_Init() */
    if (MPIDI_PG_Get_size(MPIDI_Process.my_pg) > 1)
	mpi_errno = MPIDI_CH3I_SHM_Release_mem(MPIDI_Process.my_pg, TRUE);
    else
	mpi_errno = MPIDI_CH3I_SHM_Release_mem(MPIDI_Process.my_pg, FALSE);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**finalize_release_mem");
    }

    return mpi_errno;
}

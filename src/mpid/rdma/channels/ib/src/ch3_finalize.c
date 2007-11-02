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

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**finalize_progress", 0);
    }

    /* Free resources allocated in CH3_Init() */
    MPID_VCRT_Release(MPIR_Process.comm_self->vcrt);
    MPID_VCRT_Release(MPIR_Process.comm_world->vcrt);
    MPIU_Free(MPIDI_CH3I_Process.pg->vc_table);
    MPIU_Free(MPIDI_CH3I_Process.pg->kvs_name);
    MPIU_Free(MPIDI_CH3I_Process.pg);
    
    /* Let PMI know the process is about to exit */
    mpi_errno = PMI_Finalize();
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
	    MPI_ERR_OTHER, "**pmi_finalize", "**pmi_finalize %d", mpi_errno );
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*
 * MPID_Get_universe_size - Get the universe size from the process manager
 *
 * Notes: The ch3 device requires that the PMI routines are used to 
 * communicate with the process manager.  If a channel wishes to 
 * bypass the standard PMI implementations, it is the responsibility of the
 * channel to provide an implementation of the PMI routines.
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_universe_size
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;

    pmi_errno = PMI_Get_universe_size(universe_size);
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, 
			     "**pmi_get_universe_size",
			     "**pmi_get_universe_size %d", pmi_errno);
    }
    if (*universe_size < 0)
    {
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    }
    
fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

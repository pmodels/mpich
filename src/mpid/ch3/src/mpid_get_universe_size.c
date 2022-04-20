/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/*
 * MPID_Get_universe_size - Get the universe size from the process manager
 *
 * Notes: The ch3 device requires that the PMI routines are used to 
 * communicate with the process manager.  If a channel wishes to 
 * bypass the standard PMI implementations, it is the responsibility of the
 * channel to provide an implementation of the PMI routines.
 */
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_pmi_get_universe_size(universe_size);
    MPIR_ERR_CHECK(mpi_errno);
    
fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

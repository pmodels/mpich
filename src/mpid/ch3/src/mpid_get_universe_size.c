/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

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
#ifdef USE_PMI2_API
    char val[PMI2_MAX_VALLEN];
    int found = 0;
    char *endptr;
    
    mpi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (!found)
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    else {
        *universe_size = strtol(val, &endptr, 0);
        MPIU_ERR_CHKINTERNAL(endptr - val != strlen(val), mpi_errno, "can't parse universe size");
    }

#else
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
#endif
    
fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

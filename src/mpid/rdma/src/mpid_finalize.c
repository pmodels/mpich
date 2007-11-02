/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Finalize()
{
    int mpi_errno;
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    mpi_errno = MPIDI_CH3_Finalize();
    
    MPIU_Free(MPIDI_Process.processor_name);
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}

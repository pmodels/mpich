/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize( void )
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;
}

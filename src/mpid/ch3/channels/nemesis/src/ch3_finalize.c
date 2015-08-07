/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_FINALIZE);

    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    mpi_errno = MPID_nem_finalize();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_FINALIZE);
    return mpi_errno;
}

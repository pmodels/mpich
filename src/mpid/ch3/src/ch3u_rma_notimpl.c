/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

/* FUNCNAME macros are included for the MPIDI_FUNC_NOTIMPL macro to keep the
 * state checker from emitting warnings */
#undef FUNCNAME
#define FUNCNAME MPIDI_FUNC_NOTIMPL
#define MPIDI_FUNC_NOTIMPL(state_name)                          \
    int mpi_errno = MPI_SUCCESS;                                \
                                                                \
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_##state_name);            \
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_##state_name);        \
                                                                \
    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl"); \
                                                                \
 fn_exit:                                                       \
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_##state_name);         \
    return mpi_errno;                                           \
    /* --BEGIN ERROR HANDLING-- */                              \
 fn_fail:                                                       \
    goto fn_exit;                                               \
    /* --END ERROR HANDLING-- */


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_shared_query
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_shared_query(MPID_Win *win_ptr, int target_rank, MPI_Aint *size, int *disp_unit, void *baseptr)
{
    MPIDI_FUNC_NOTIMPL(WIN_FLUSH_LOCAL_ALL)
}

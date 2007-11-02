/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_ops *curr_ptr, *prev_ptr, *new_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);

#ifdef MPICH_SINGLE_THREADED
    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
#endif

    curr_ptr = MPIDI_RMA_ops_list;
    prev_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    new_ptr = (MPIDI_RMA_ops *) MPIU_Malloc(sizeof(MPIDI_RMA_ops));
    if (!new_ptr) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
        return mpi_errno;
    }
    if (prev_ptr != NULL)
        prev_ptr->next = new_ptr;
    else 
        MPIDI_RMA_ops_list = new_ptr;

    new_ptr->next = NULL;  
    new_ptr->type = MPIDI_RMA_LOCK;
    new_ptr->target_rank = dest;
    new_ptr->lock_type = lock_type;

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);

    return mpi_errno;
}

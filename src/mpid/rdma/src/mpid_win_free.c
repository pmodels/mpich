/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
#ifndef MPICH_SINGLE_THREADED
    int err;
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FREE);

    MPIR_Nest_incr();

    NMPI_Barrier((*win_ptr)->comm);

    MPIDI_Passive_target_thread_exit_flag = 1;

#ifdef FOOOOOOOOOOO

#ifndef MPICH_SINGLE_THREADED
#ifdef HAVE_PTHREAD_H
    pthread_join((*win_ptr)->passive_target_thread_id, (void **) &err);
#elif defined(HAVE_WINTHREADS)
    if (WaitForSingleObject((*win_ptr)->passive_target_thread_id, INFINITE) == WAIT_OBJECT_0)
	err = GetExitCodeThread((*win_ptr)->passive_target_thread_id, &err);
    else
	err = GetLastError();
#else
#error Error: No thread package specified.
#endif
    mpi_errno = err;
#endif

#endif

    NMPI_Comm_free(&((*win_ptr)->comm));

    MPIR_Nest_decr();

    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_counters);
 
    /* check whether refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );
 
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FREE);

    return mpi_errno;
}

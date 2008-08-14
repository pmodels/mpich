/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);

    /* The default for this function is MPIDI_Win_create.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Win_create.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    /* We pass the RMAFns function table to this function because a channel may 
       want to reset it to the default if it finds that it cannot
       optimize for this   
       set of windows. The sshm channel did this if windows are not
       allocated in shared memory. */
       
    mpi_errno = MPIDI_Win_create(base, size, disp_unit, info, comm_ptr, 
				 win_ptr );
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* Set the defaults */
#ifdef USE_CHANNEL_RMA_TABLE
    mpi_errno = MPIDI_CH3_RMAWinFnsInit( *win_ptr );
#else    
    {
	MPIRI_RMAFns *ftable = &(*win_ptr)->RMAFns;
	ftable->Win_free   = MPIDI_Win_free;
	ftable->Put        = MPIDI_Put;
	ftable->Get        = MPIDI_Get;
	ftable->Accumulate = MPIDI_Accumulate;
	ftable->Win_fence  = MPIDI_Win_fence;
	ftable->Win_post   = MPIDI_Win_post;
	ftable->Win_start  = MPIDI_Win_start;
	ftable->Win_complete = MPIDI_Win_complete;
	ftable->Win_wait   = MPIDI_Win_wait;
	ftable->Win_test   = MPIDI_Win_test;
	ftable->Win_lock   = MPIDI_Win_lock;
	ftable->Win_unlock = MPIDI_Win_unlock;
    }
#endif

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
}


/* The memory allocation functions */
#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPID_Alloc_mem( size_t size, MPID_Info *info_ptr )
{
    void *ap=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ALLOC_MEM);

    /* The default for this function is MPIDI_Alloc_mem.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Alloc_mem.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    ap = MPIDI_Alloc_mem(size, info_ptr);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return ap;
}


#undef FUNCNAME
#define FUNCNAME MPID_Free_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Free_mem( void *ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_FREE_MEM);

    /* The default for this function is MPIDI_Free_mem.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Free_mem.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    mpi_errno = MPIDI_Free_mem(ptr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
        
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
}


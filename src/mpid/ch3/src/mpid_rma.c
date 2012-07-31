/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#define MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr)                   \
    do {                                                        \
        /* Get ptr to RMAFns, which is embedded in MPID_Win */  \
        MPIRI_RMAFns *ftable        = &(*(win_ptr))->RMAFns;    \
        ftable->Win_free            = MPIDI_Win_free;           \
        ftable->Win_attach          = MPIDI_Win_attach;         \
        ftable->Win_detach          = MPIDI_Win_detach;         \
        ftable->Win_shared_query    = MPIDI_Win_shared_query;   \
                                                                \
        ftable->Put                 = MPIDI_Put;                \
        ftable->Get                 = MPIDI_Get;                \
        ftable->Accumulate          = MPIDI_Accumulate;         \
        ftable->Get_accumulate      = MPIDI_Get_accumulate;     \
        ftable->Fetch_and_op        = MPIDI_Fetch_and_op;       \
        ftable->Compare_and_swap    = MPIDI_Compare_and_swap;   \
                                                                \
        ftable->Rput                = MPIDI_Rput;               \
        ftable->Rget                = MPIDI_Rget;               \
        ftable->Raccumulate         = MPIDI_Raccumulate;        \
        ftable->Rget_accumulate     = MPIDI_Rget_accumulate;    \
                                                                \
        ftable->Win_fence           = MPIDI_Win_fence;          \
        ftable->Win_post            = MPIDI_Win_post;           \
        ftable->Win_start           = MPIDI_Win_start;          \
        ftable->Win_complete        = MPIDI_Win_complete;       \
        ftable->Win_wait            = MPIDI_Win_wait;           \
        ftable->Win_test            = MPIDI_Win_test;           \
                                                                \
        ftable->Win_lock            = MPIDI_Win_lock;           \
        ftable->Win_unlock          = MPIDI_Win_unlock;         \
        ftable->Win_lock_all        = MPIDI_Win_lock_all;       \
        ftable->Win_unlock_all      = MPIDI_Win_unlock_all;     \
                                                                \
        ftable->Win_flush           = MPIDI_Win_flush;          \
        ftable->Win_flush_all       = MPIDI_Win_flush_all;      \
        ftable->Win_flush_local     = MPIDI_Win_flush_local;    \
        ftable->Win_flush_local_all = MPIDI_Win_flush_local_all;\
        ftable->Win_sync            = MPIDI_Win_sync;           \
    } while (0)


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
    MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr);
#endif

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_allocate(MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, void *baseptr, MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_ALLOCATE);

    /* The default for this function is MPIDI_Win_allocate.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Win_allocate.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    /* We pass the RMAFns function table to this function because a channel may 
       want to reset it to the default if it finds that it cannot
       optimize for this   
       set of windows. The sshm channel did this if windows are not
       allocated in shared memory. */
       
    mpi_errno = MPIDI_Win_allocate(size, disp_unit, info, comm_ptr, 
                                   baseptr, win_ptr );
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Set the defaults */
#ifdef USE_CHANNEL_RMA_TABLE
    mpi_errno = MPIDI_CH3_RMAWinFnsInit( *win_ptr );
#else    
    MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr);
#endif

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_ALLOCATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_create_dynamic
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_create_dynamic(MPID_Info *info, MPID_Comm *comm_ptr,
                            MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);

    /* The default for this function is MPIDI_Win_allocate.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Win_allocate.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    /* We pass the RMAFns function table to this function because a channel may 
       want to reset it to the default if it finds that it cannot
       optimize for this   
       set of windows. The sshm channel did this if windows are not
       allocated in shared memory. */
       
    mpi_errno = MPIDI_Win_create_dynamic(info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Set the defaults */
#ifdef USE_CHANNEL_RMA_TABLE
    mpi_errno = MPIDI_CH3_RMAWinFnsInit( *win_ptr );
#else    
    MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr);
#endif

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
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

#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate_shared
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_allocate_shared(MPI_Aint size, MPID_Info *info_ptr, MPID_Comm *comm_ptr,
                             void **base_ptr, MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);

    /* The default for this function is MPIDI_Win_allocate_shared.
       A channel may define its own function and set it in the
       init check above; such a function may be named MPIDI_CH3_Win_allocate_shared.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    /* We pass the RMAFns function table to this function because a channel may
       want to reset it to the default if it finds that it cannot
       optimize for this
       set of windows. The sshm channel did this if windows are not
       allocated in shared memory. */
       
    mpi_errno = MPIDI_Win_allocate_shared(size, info_ptr, comm_ptr, base_ptr, win_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Set the defaults */
#ifdef USE_CHANNEL_RMA_TABLE
    mpi_errno = MPIDI_CH3_RMAWinFnsInit( *win_ptr );
#else
    MPID_WIN_FTABLE_SET_DEFAULTS(win_ptr);
#endif

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    return mpi_errno;
}

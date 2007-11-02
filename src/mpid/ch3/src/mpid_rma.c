/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int setupRMAFunctions = 1;
/* Define the functions that are used to implement the RMA operations */
static MPIDI_RMAFns RMAFns = { MPIDI_Win_create, MPIDI_Win_free,
			       MPIDI_Put, MPIDI_Get, MPIDI_Accumulate,
			       MPIDI_Win_fence, MPIDI_Win_post, 
			       MPIDI_Win_start, MPIDI_Win_complete,
			       MPIDI_Win_wait, MPIDI_Win_lock,
			       MPIDI_Win_unlock, MPIDI_Alloc_mem, MPIDI_Free_mem };

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

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_create.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Win_create.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    /* We pass the RMAFns function table to this function because a channel may 
       want to reset it to the default if it finds that it cannot optimize for this 
       set of windows. The sshm channel does this if windows are not allocated in 
       shared memory. */
       
    if (RMAFns.Win_create) {
	mpi_errno = RMAFns.Win_create(base, size, disp_unit, info, comm_ptr, 
				      win_ptr, &RMAFns);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_free(MPID_Win **win_ptr)
{
   int mpi_errno=MPI_SUCCESS;
   MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FREE);
        
   MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FREE);
        
    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_free.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Win_free.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_free) {
	mpi_errno = RMAFns.Win_free(win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_FREE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_PUT);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_PUT);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Put.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Put.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Put) {
	mpi_errno = RMAFns.Put(origin_addr, origin_count, origin_datatype, 
			       target_rank, target_disp, target_count, target_datatype,
			       win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_PUT);    
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPID_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_GET);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_GET);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Get.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Get.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Get) {
	mpi_errno = RMAFns.Get(origin_addr, origin_count, origin_datatype, 
			       target_rank, target_disp, target_count, target_datatype,
			       win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_GET);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPID_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ACCUMULATE);
        
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_ACCUMULATE);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Accumulate.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Accumulate.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Accumulate) {
	mpi_errno = RMAFns.Accumulate(origin_addr, origin_count, origin_datatype, 
			       target_rank, target_disp, target_count, target_datatype,
			       op, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_ACCUMULATE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_fence.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_fence.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_fence) {
	mpi_errno = RMAFns.Win_fence(assert, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_post.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_post.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_post) {
	mpi_errno = RMAFns.Win_post(group_ptr, assert, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_START);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_start.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_start.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_start) {
	mpi_errno = RMAFns.Win_start(group_ptr, assert, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_complete.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_complete.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_complete) {
	mpi_errno = RMAFns.Win_complete(win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_wait.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_wait.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_wait) {
	mpi_errno = RMAFns.Win_wait(win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_lock.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_lock.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_lock) {
	mpi_errno = RMAFns.Win_lock(lock_type, dest, assert, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Win_unlock.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Win_unlock.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Win_unlock) {
	mpi_errno = RMAFns.Win_unlock(dest, win_ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPID_Alloc_mem( size_t size, MPID_Info *info_ptr )
{
    void *ap=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ALLOC_MEM);

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Alloc_mem.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Alloc_mem.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Alloc_mem) {
	ap = RMAFns.Alloc_mem(size, info_ptr);
    }
    
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

    /* Check to see if we need to setup channel-specific functions
       for handling the RMA operations */
    if (setupRMAFunctions) {
	MPIU_CALL(MPIDI_CH3,RMAFnsInit( &RMAFns ));
	setupRMAFunctions = 0;
    }

    /* The default for this function is MPIDI_Free_mem.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_Free_mem.
       If a channel does not implement this operation, it will set
       the function pointer to NULL */

    if (RMAFns.Free_mem) {
	mpi_errno = RMAFns.Free_mem(ptr);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }
        
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_test(MPID_Win *win_ptr, int *flag)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_TEST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_TEST);

    mpi_errno = MPID_Progress_test();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    *flag = (win_ptr->my_counter) ? 0 : 1;

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;
}



/* This was probably added by Brad. Leaving it here for now. Needs to be fixed 
   according to the FIXME below. */
/* FIXME: It would be better to install this as an abort and a finalize 
   handler, ranter than expecting all routines to call this.  It
   can be installed on first use of alloc_mem */
#undef FUNCNAME
#define FUNCNAME MPID_Cleanup_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Cleanup_mem()
{
#if defined(MPIDI_CH3_IMPLEMENTS_CLEANUP_MEM)
    MPIDI_CH3_Cleanup_mem();
#endif
}

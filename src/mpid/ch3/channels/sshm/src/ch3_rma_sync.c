/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FENCE);

    mpi_errno = NMPI_Barrier(win_ptr->comm);

    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**fail");
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FENCE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Group win_grp, grp;
    int i, grp_size, *ranks_in_grp, *ranks_in_win, dst, rank;
    volatile char *pscw_sync_addr;
    MPIU_CHKLMEM_DECL(1);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_POST);

    MPIR_Nest_incr();

    /* First translate the ranks of the processes in
       group_ptr to ranks in win_ptr->comm. Save the translated ranks
       and group_ptr in the win object because they will be needed in 
       end_epoch. */
        
    grp_size = group_ptr->size;
    
    MPIU_CHKLMEM_MALLOC(ranks_in_grp, int *, grp_size * sizeof(int),
			mpi_errno, "ranks_in_grp");
    
    MPIU_CHKPMEM_MALLOC(ranks_in_win, int *, grp_size * sizeof(int),
			mpi_errno, "ranks_in_win");
    
    for (i=0; i<grp_size; i++)
    {
	ranks_in_grp[i] = i;
    }
    
    grp = group_ptr->handle;
    
    mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = NMPI_Group_translate_ranks(grp, grp_size, ranks_in_grp, win_grp, ranks_in_win);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    if ((assert & MPI_MODE_NOCHECK) == 0) {
	/* MPI_MODE_NOCHECK not specified. We need to notify the 
	   source processes that Post has been called. */
	NMPI_Comm_rank(win_ptr->comm, &rank);
	for (i=0; i<grp_size; i++)
	{
	    dst = ranks_in_win[i];
	    if (dst != rank) {
		/* Write a '1' in the sync array of the origin process at the location
		   indexed by the rank of this process */
		pscw_sync_addr = win_ptr->pscw_shm_structs[dst].addr;
		pscw_sync_addr[rank] = '1';
	    }
	}
    }    

    /* save the ranks_in_win and group_ptr in win object */
    win_ptr->exposure_epoch_grp_ranks_in_win = ranks_in_win;
    win_ptr->exposure_epoch_grp_ptr = group_ptr;
    
    MPIR_Group_add_ref( group_ptr );
    
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_POST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPI_Group win_grp, grp;
    int i, grp_size, *ranks_in_grp, *ranks_in_win, src, rank;
    volatile char *pscw_sync_addr;
    MPIU_CHKLMEM_DECL(1);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_START);

    MPIR_Nest_incr();

    /* First translate the ranks of the processes in
       group_ptr to ranks in win_ptr->comm. Save the translated ranks
       and group_ptr in the win object because they will be needed in 
       end_epoch. */
        
    grp_size = group_ptr->size;
    
    MPIU_CHKLMEM_MALLOC(ranks_in_grp, int *, grp_size * sizeof(int),
			mpi_errno, "ranks_in_grp");
    
    MPIU_CHKPMEM_MALLOC(ranks_in_win, int *, grp_size * sizeof(int),
			mpi_errno, "ranks_in_win");
    
    for (i=0; i<grp_size; i++)
    {
	ranks_in_grp[i] = i;
    }
    
    grp = group_ptr->handle;
    
    mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = NMPI_Group_translate_ranks(grp, grp_size, ranks_in_grp, win_grp, ranks_in_win);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    if ((assert & MPI_MODE_NOCHECK) == 0) {
	/* MPI_MODE_NOCHECK not specified. Synchronization is necessary. */
	NMPI_Comm_rank(win_ptr->comm, &rank);
	for (i=0; i<grp_size; i++)
	{
	    src = ranks_in_win[i];
	    if (src != rank) {
		/* Wait until a '1' is written in the sync array by the target 
		   at the location indexed by the rank of the target */
		pscw_sync_addr = win_ptr->pscw_shm_structs[rank].addr;
		while (pscw_sync_addr[src] == '0');

		/* reset it back to 0 */
		pscw_sync_addr[src] = '0';
	    }
	}
    }    

    /* save the ranks_in_win and group_ptr in win object */
    win_ptr->access_epoch_grp_ranks_in_win = ranks_in_win;
    win_ptr->access_epoch_grp_ptr = group_ptr;

    MPIR_Group_add_ref( group_ptr );

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_START);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_complete(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, dst, *ranks_in_win, grp_size, rank, comm_size;
    volatile char *pscw_sync_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_COMPLETE);

    MPIR_Nest_incr();

    NMPI_Comm_rank(win_ptr->comm, &rank);
    NMPI_Comm_size(win_ptr->comm, &comm_size);

    /* Send a sync message to each target process */
    grp_size = win_ptr->access_epoch_grp_ptr->size;
    ranks_in_win = win_ptr->access_epoch_grp_ranks_in_win;
    for (i=0; i<grp_size; i++)
    {
	dst = ranks_in_win[i];
	if (dst != rank) {
	    /* Write a '1' in the sync array of the target process at the location
	       indexed by the rank of this process */
	    pscw_sync_addr = (char *) win_ptr->pscw_shm_structs[dst].addr + comm_size;
	    pscw_sync_addr[rank] = '1';
	}
    }

    MPIU_Free(win_ptr->access_epoch_grp_ranks_in_win);
    MPIR_Group_release(win_ptr->access_epoch_grp_ptr);

    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_COMPLETE);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, *ranks_in_win, grp_size, rank, comm_size;
    volatile char *pscw_sync_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_WAIT);

    MPIR_Nest_incr();

    NMPI_Comm_rank(win_ptr->comm, &rank);
    NMPI_Comm_size(win_ptr->comm, &comm_size);

    grp_size = win_ptr->exposure_epoch_grp_ptr->size;
    ranks_in_win = win_ptr->exposure_epoch_grp_ranks_in_win;

    for (i=0; i<grp_size; i++)
    {
	src = ranks_in_win[i];
	if (src != rank) {
	    /* Wait until a '1' is written in the sync array by the origin
	       at the location indexed by the rank of the origin */
	    pscw_sync_addr = (char *) win_ptr->pscw_shm_structs[rank].addr + comm_size;
	    while (pscw_sync_addr[src] == '0');

	    /* reset it to 0 */
	    pscw_sync_addr[src] = '0';
	}
    }

    MPIU_Free(win_ptr->exposure_epoch_grp_ranks_in_win);
    MPIR_Group_release(win_ptr->exposure_epoch_grp_ptr);

    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_WAIT);
    return mpi_errno;
}



/*
 * MPIDI_CH3_Win_lock()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size;
    volatile int *shared_lock_state_baseaddr;
    MPIDU_Process_lock_t *locks_base_addr;
    MPID_Comm *comm_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_LOCK);

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    locks_base_addr = win_ptr->locks->addr;
    shared_lock_state_baseaddr = (volatile int *) ((char *) win_ptr->locks->addr + 
        comm_size * sizeof(MPIDU_Process_lock_t));

    if (lock_type == MPI_LOCK_SHARED) {

        /* acquire lock. increment shared lock state. release lock */

        MPIDU_Process_lock( &locks_base_addr[dest] );

        shared_lock_state_baseaddr[dest]++;

        MPIDU_Process_unlock( &locks_base_addr[dest] );

    }
    else {
        /* exclusive lock. acquire lock. check shared lock state. if non-zero 
           (shared lock exists), unlock and try again. else retain lock. */

        while (1) {
            MPIDU_Process_lock( &locks_base_addr[dest] );

            if (shared_lock_state_baseaddr[dest]) {
                MPIDU_Process_unlock( &locks_base_addr[dest] );
            }
            else {
                break;
            }
        }

        /* mark this as a passive target excl lock epoch so that a following accumulate 
           does not need to acquire a lock */
        win_ptr->pt_rma_excl_lock = 1;
    }


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_LOCK);
    return mpi_errno;
}


/*
 * MPIDI_CH3_Win_unlock()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size;
    volatile int *shared_lock_state_baseaddr;
    MPIDU_Process_lock_t *locks_base_addr;
    MPID_Comm *comm_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_UNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_UNLOCK);

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    locks_base_addr = win_ptr->locks->addr;
    shared_lock_state_baseaddr = (volatile int *) ((char *) win_ptr->locks->addr + 
        comm_size * sizeof(MPIDU_Process_lock_t));

    if (win_ptr->pt_rma_excl_lock == 0) {

        /* it's a shared lock. acquire lock. decrement shared lock state. release lock */

        MPIDU_Process_lock( &locks_base_addr[dest] );

        shared_lock_state_baseaddr[dest]--;

        MPIDU_Process_unlock( &locks_base_addr[dest] );

    }
    else {
        /* exclusive lock. just release it */

        MPIDU_Process_unlock( &locks_base_addr[dest] );

        win_ptr->pt_rma_excl_lock = 0;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_UNLOCK);
    return mpi_errno;
}

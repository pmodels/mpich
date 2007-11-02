/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Win_create()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr, MPIDI_RMAFns *RMAFns)
{
    int mpi_errno=MPI_SUCCESS, i, comm_size, rank, found, result;
    void *offset=0;
    MPIDI_CH3I_Alloc_mem_list_t *curr_ptr;
    MPIDU_Process_lock_t *locks_base_addr;
    int *shared_lock_state_baseaddr;
    volatile char *pscw_sync_addr;
    MPIU_CHKPMEM_DECL(4);
        
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_CREATE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_CREATE);

    /* We first call the generic MPIDI_Win_create */
    mpi_errno = MPIDI_Win_create(base, size, disp_unit, info, comm_ptr, win_ptr, RMAFns);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* Now we do the channel-specific stuff */

    /* All processes first check whether their base address refers to an address in 
       shared memory. If everyone's address is in shared memory, we set 
       MPIDI_Use_optimized_rma=1 to indicate that shared memory optimizations are possible.
       If even one process's win_base is not in shared memory, we revert to the generic 
       implementation of RMA in CH3 by setting MPIDI_Use_optimized_rma=0. */

    /* For the special case where win_base is NULL, we treat it as if
     * it is found in shared memory, so as not to disable
       optimizations. For example, where the window is allocated in
       shared memory on one process and others call win_create with NULL. */

    MPIR_Nest_incr();
        
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    curr_ptr = MPIDI_CH3I_Get_mem_list_head();
    if (base == NULL) {
        found = 1;
    }
    else {
        found = 0;
        while (curr_ptr != NULL) {
            if ((curr_ptr->shm_struct->addr <= base) && 
                (base < (void *) ((char *) curr_ptr->shm_struct->addr + curr_ptr->shm_struct->size))) {
                found = 1;
                offset = (void *) ((char *) curr_ptr->shm_struct->addr - (char *) base);
                break;
            }
            curr_ptr = curr_ptr->next;
        }
    }

    mpi_errno = NMPI_Allreduce(&found, &result, 1, MPI_INT, MPI_BAND, comm_ptr->handle);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    if (result == 0) { 
	/* not all windows are in shared memory. can't be optimized. Reset functions pointers for
	   the RMA functions to the default. */
        (*win_ptr)->shm_structs = NULL;
        (*win_ptr)->locks = NULL;

	RMAFns->Win_free = MPIDI_Win_free;
	RMAFns->Put = MPIDI_Put;
	RMAFns->Get = MPIDI_Get;
	RMAFns->Accumulate = MPIDI_Accumulate;
	RMAFns->Win_fence = MPIDI_Win_fence;
	RMAFns->Win_post = MPIDI_Win_post ;
	RMAFns->Win_start = MPIDI_Win_start;
	RMAFns->Win_complete = MPIDI_Win_complete ;
	RMAFns->Win_wait = MPIDI_Win_wait;
	RMAFns->Win_lock = MPIDI_Win_lock;
	RMAFns->Win_unlock = MPIDI_Win_unlock;
	/* leave win_create, alloc_mem, and free_mem as they are 
	   (set to the channel-specific version)  */
    }
    else {   /* all windows in shared memory. can be optimized */
	
	/* again set the channel-specific version of the RMA functions because they may 
	   have been reset to the default in an earlier call to win_create */
	MPIDI_CH3_RMAFnsInit( RMAFns );

        /* allocate memory for the shm_structs */
	MPIU_CHKPMEM_MALLOC((*win_ptr)->shm_structs, MPIDI_CH3I_Shmem_block_request_result *, 
			    comm_size * sizeof(MPIDI_CH3I_Shmem_block_request_result), 
			    mpi_errno, "(*win_ptr)->shm_structs");

        /* allocate memory for the offsets from base of shared memory */
	MPIU_CHKPMEM_MALLOC((*win_ptr)->offsets, void **, comm_size * sizeof(void *), 
			    mpi_errno, "(*win_ptr)->offsets");

        if (base != NULL) {
            /* copy this process's shmem struct into right location in array of shmem structs */
            MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
            memcpy(&((*win_ptr)->shm_structs[rank]), curr_ptr->shm_struct, 
                   sizeof(MPIDI_CH3I_Shmem_block_request_result));
            MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
            
            /* copy this process's offset into right location in array of offsets */
            (*win_ptr)->offsets[rank] = offset;
        }
        else {
            (*win_ptr)->shm_structs[rank].addr = NULL;
            (*win_ptr)->shm_structs[rank].size = 0;

            (*win_ptr)->offsets[rank] = 0;
        }

        /* collect everyone's shm_structs and offsets */
        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   (*win_ptr)->shm_structs, 
                                   sizeof(MPIDI_CH3I_Shmem_block_request_result), 
                                   MPI_BYTE, comm_ptr->handle);   
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        
        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   (*win_ptr)->offsets, sizeof(void *), 
                                   MPI_BYTE, comm_ptr->handle);   
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }


        /* each process now attaches to the shared memory segments
           (windows) of other processes (if they are non-zero), so that direct RMA is
           possible. The newly mapped addresses are stored in the addr
           field in the shmem struct. */ 

        for (i=0; i<comm_size; i++) {
            if ((i != rank) && ((*win_ptr)->shm_structs[i].size != 0)) {
                mpi_errno = MPIDI_CH3I_SHM_Attach_notunlink_mem( &((*win_ptr)->shm_structs[i]) );
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            }
        }

        /* allocate memory for the locks and the shared lock
         * state. locks are needed for accumulate operations and for
         *  passive target RMA. */
	MPIU_CHKPMEM_MALLOC((*win_ptr)->locks, MPIDI_CH3I_Shmem_block_request_result *, 
			    sizeof(MPIDI_CH3I_Shmem_block_request_result), 
			    mpi_errno, "(*win_ptr)->locks");


        /* Rank 0 allocates shared memory for an array of locks, one for each process, and 
           for an array of shared lock states and initializes the
           locks and the shared lock state. */  
        if (rank == 0) {
            mpi_errno = MPIDI_CH3I_SHM_Get_mem(comm_size * sizeof(MPIDU_Process_lock_t)
                                               + comm_size * sizeof(int), 
                                               (*win_ptr)->locks);
	    MPIU_ERR_CHKANDJUMP(mpi_errno,mpi_errno,MPI_ERR_OTHER,"**nomem");

            locks_base_addr = (*win_ptr)->locks->addr;
            for (i=0; i<comm_size; i++)
                MPIDU_Process_lock_init(&locks_base_addr[i]);

            shared_lock_state_baseaddr = (int *) ((char *) (*win_ptr)->locks->addr + 
                                         comm_size * sizeof(MPIDU_Process_lock_t));
            /* initialize shared lock state of all processes to 0 */
            for (i=0; i<comm_size; i++)
                shared_lock_state_baseaddr[i] = 0;
        }

	/* rank 0 broadcasts the locks struct to others so that they can attach to the shared 
           memory containing the locks */
        mpi_errno = NMPI_Bcast((*win_ptr)->locks, 
                               sizeof(MPIDI_CH3I_Shmem_block_request_result), MPI_BYTE,
                               0, comm_ptr->handle);   
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* Processes other than rank 0 attach to the shared memory containing the lock
           structure. */
        if (rank != 0) {
            mpi_errno = MPIDI_CH3I_SHM_Attach_notunlink_mem( (*win_ptr)->locks );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }

        /* allocate memory for the shm struct needed to allocate shared memory
           for synchronizing post-start-complete-wait. */
	MPIU_CHKPMEM_MALLOC((*win_ptr)->pscw_shm_structs, MPIDI_CH3I_Shmem_block_request_result *, 
			    comm_size * sizeof(MPIDI_CH3I_Shmem_block_request_result), 
			    mpi_errno, "(*win_ptr)->pscw_shm_structs");

        
        mpi_errno = MPIDI_CH3I_SHM_Get_mem(2 * comm_size, 
                                           &(*win_ptr)->pscw_shm_structs[rank]);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* initialize it */
        pscw_sync_addr = (*win_ptr)->pscw_shm_structs[rank].addr;
        for (i=0; i<2*comm_size; i++)
            pscw_sync_addr[i] = '0';

        /* collect everyone's pscw_shm_structs*/
        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   (*win_ptr)->pscw_shm_structs, 
                                   sizeof(MPIDI_CH3I_Shmem_block_request_result), 
                                   MPI_BYTE, comm_ptr->handle);   
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        

        /* each process now attaches to the shared memory for pscw sync
           of other processes. */ 
        for (i=0; i<comm_size; i++) {
            if (i != rank) {
                mpi_errno = MPIDI_CH3I_SHM_Attach_notunlink_mem( &((*win_ptr)->pscw_shm_structs[i]) );
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            }
        }
    }

    (*win_ptr)->access_epoch_grp_ptr = NULL;
    (*win_ptr)->access_epoch_grp_ranks_in_win = NULL;
    (*win_ptr)->exposure_epoch_grp_ptr = NULL;
    (*win_ptr)->exposure_epoch_grp_ranks_in_win = NULL;
    (*win_ptr)->pt_rma_excl_lock = 0;
        
 fn_exit:
    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_CREATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



/*
 * MPIDI_CH3_Win_free()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, rank, i;
    MPID_Comm *comm_ptr;
    MPIDU_Process_lock_t *locks_base_addr;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FREE);

    MPID_Comm_get_ptr( (*win_ptr)->comm, comm_ptr );
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Nest_incr();

    /* barrier needed so that all passive target rmas directed toward this process are over */
    mpi_errno = NMPI_Barrier((*win_ptr)->comm);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = NMPI_Comm_free(&((*win_ptr)->comm));
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    MPIR_Nest_decr();
    
    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->pt_rma_puts_accs);
    
    for (i=0; i<comm_size; i++) {
        if ((i != rank) && ((*win_ptr)->shm_structs[i].size != 0)) {
            mpi_errno = MPIDI_CH3I_SHM_Release_mem( &((*win_ptr)->shm_structs[i]) );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }
    }

    if (rank != 0) {
        mpi_errno = MPIDI_CH3I_SHM_Release_mem( (*win_ptr)->locks );
    }
    else {
        locks_base_addr = (*win_ptr)->locks->addr;
        for (i=0; i<comm_size; i++) 
            MPIDU_Process_lock_free(&locks_base_addr[i]);
        
        mpi_errno = MPIDI_CH3I_SHM_Unlink_and_detach_mem( (*win_ptr)->locks );
    }
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    for (i=0; i<comm_size; i++) {
        if (i != rank) {
            mpi_errno = MPIDI_CH3I_SHM_Release_mem( &((*win_ptr)->pscw_shm_structs[i]) );
        }
        else {
            mpi_errno = 
                MPIDI_CH3I_SHM_Unlink_and_detach_mem( &((*win_ptr)->pscw_shm_structs[i]) );
        }
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    MPIU_Free((*win_ptr)->locks);
    MPIU_Free((*win_ptr)->shm_structs);
    MPIU_Free((*win_ptr)->offsets);
    MPIU_Free((*win_ptr)->pscw_shm_structs);

    /* check whether refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FREE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



/*
 * MPIDI_CH3_Put()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *target_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PUT);

    target_addr = (char *) win_ptr->shm_structs[target_rank].addr + 
                  (long) win_ptr->offsets[target_rank] +
                  win_ptr->disp_units[target_rank] * target_disp;

    mpi_errno = MPIR_Localcopy (origin_addr, origin_count, origin_datatype, 
                                target_addr, target_count, target_datatype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    /* --END ERROR HANDLING-- */

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PUT);
    return mpi_errno;
}




/*
 * MPIDI_CH3_Get()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *target_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_GET);

    target_addr = (char *) win_ptr->shm_structs[target_rank].addr + 
                  (long) win_ptr->offsets[target_rank] +
                  win_ptr->disp_units[target_rank] * target_disp;

    mpi_errno = MPIR_Localcopy (target_addr, target_count, target_datatype,
                                origin_addr, origin_count, origin_datatype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    /* --END ERROR HANDLING-- */


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_GET);
    return mpi_errno;
}




/*
 * MPIDI_CH3_Accumulate()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *target_addr, *source_addr, *tmp_buf=NULL;
    MPI_Aint true_lb, true_extent, extent;
    MPI_User_function *uop;
    MPIDU_Process_lock_t *locks_base_addr;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ACCUMULATE);

    target_addr = (char *) win_ptr->shm_structs[target_rank].addr + 
                  (long) win_ptr->offsets[target_rank] +
                  win_ptr->disp_units[target_rank] * target_disp;
    source_addr = origin_addr;

    locks_base_addr = win_ptr->locks->addr;

    if (op == MPI_REPLACE) {
        /* simply do a memcpy */

        /* all accumulate operations need to be done atomically. If the process does 
           not already have an exclusive lock, acquire it */

        if (win_ptr->pt_rma_excl_lock == 0)
            MPIDU_Process_lock( &locks_base_addr[target_rank] );

        mpi_errno = MPIR_Localcopy (origin_addr, origin_count, origin_datatype, 
                                    target_addr, target_count, target_datatype);

        if (win_ptr->pt_rma_excl_lock == 0)
            MPIDU_Process_unlock( &locks_base_addr[target_rank] );

	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    else {

	MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN), mpi_errno, MPI_ERR_OP, "**opnotpredefined",
			     "**opnotpredefined %d", op );

	/* get the function by indexing into the op table */
	uop = MPIR_Op_table[op%16 - 1];
    
        if ((HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) &&
            (HANDLE_GET_KIND(origin_datatype) == HANDLE_KIND_BUILTIN)) {
                /* basic datatype on origin and target */
            if (win_ptr->pt_rma_excl_lock == 0)
                MPIDU_Process_lock( &locks_base_addr[target_rank] );

            (*uop)(origin_addr, target_addr, &origin_count, &origin_datatype);

            if (win_ptr->pt_rma_excl_lock == 0)
                MPIDU_Process_unlock( &locks_base_addr[target_rank] );
        }
        else {

	    if (origin_datatype != target_datatype) {
            /* allocate a tmp buffer of extent equal to extent of target buf */

		MPIR_Nest_incr();
		mpi_errno = NMPI_Type_get_true_extent(target_datatype, 
						      &true_lb, &true_extent);
		MPIR_Nest_decr();
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

		MPID_Datatype_get_extent_macro(target_datatype, extent); 

		MPIU_CHKLMEM_MALLOC(tmp_buf, void *, target_count * (MPIR_MAX(extent,true_extent)), 
				    mpi_errno, "temporary buffer");

		/* adjust for potential negative lower bound in datatype */
		tmp_buf = (void *)((char*)tmp_buf - true_lb);

		/*  copy origin buffer into tmp_buf with same datatype as target. */
		mpi_errno = MPIR_Localcopy (origin_addr, origin_count, origin_datatype, 
					    tmp_buf, target_count, target_datatype);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

		source_addr = tmp_buf;
	    }
            
            if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
                /* basic datatype on target. call uop. */
                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_lock( &locks_base_addr[target_rank] );

                (*uop)(source_addr, target_addr, &target_count, &target_datatype);

                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_unlock( &locks_base_addr[target_rank] );
            }
            else
            {
                /* derived datatype on target */
                MPID_Segment *segp;
                DLOOP_VECTOR *dloop_vec;
                MPI_Aint first, last;
                int vec_len, i, type_size, count;
                MPI_Datatype type;
                MPID_Datatype *dtp;
                
                segp = MPID_Segment_alloc();
		MPIU_ERR_CHKANDJUMP((!segp), mpi_errno, MPI_ERR_OTHER, "**nomem"); 

                MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
                first = 0;
                last  = SEGMENT_IGNORE_LAST;
                
                MPID_Datatype_get_ptr(target_datatype, dtp);
                vec_len = dtp->n_contig_blocks * target_count + 1; 
                /* +1 needed because Rob says so */
		MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *, vec_len * sizeof(DLOOP_VECTOR), mpi_errno, "dloop vector");
                
                MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);
                
                type = dtp->eltype;
                type_size = MPID_Datatype_get_basic_size(type);

                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_lock( &locks_base_addr[target_rank] );
                for (i=0; i<vec_len; i++)
                {
                    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                    (*uop)((char *)source_addr + MPIU_PtrToAint( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           (char *)target_addr + MPIU_PtrToAint( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           &count, &type);
                }
                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_unlock( &locks_base_addr[target_rank] );
                
                MPID_Segment_free(segp);
            }
	}
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ACCUMULATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

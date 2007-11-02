/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

    /* wait for all operations from other processes to finish */
    while (win_ptr->my_counter) {
        MPID_Progress_start();
        if (win_ptr->my_counter) {
            mpi_errno = MPID_Progress_wait();
            if (mpi_errno != MPI_SUCCESS) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
                return mpi_errno;
            }
        }
        else 
            MPID_Progress_end();
    } 

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
}









#ifdef USE_OLDSTUFF

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

#   ifdef MPICH_SINGLE_THREADED
    {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
    }
#   else
    {
	int err;
	
#       ifdef HAVE_PTHREAD_H
	{
	    pthread_join(win_ptr->wait_thread_id, (void **) &err);
	}
#       elif defined(HAVE_WINTHREADS)
	{
	    if (WaitForSingleObject(win_ptr->wait_thread_id, INFINITE) == WAIT_OBJECT_0)
	    { 
		err = GetExitCodeThread(win_ptr->wait_thread_id, &err);
	    }
	    else
	    { 
		err = GetLastError();
	    }
	}
#       else
#           error Error: No thread package specified.
#       endif
	
	mpi_errno = err;
    }
#   endif /* defined(MPICH_SINGLE_THREADED) */    

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
}


#ifdef FOO
    int mpi_errno = MPI_SUCCESS, comm_size, src;
    int *nops_from_proc, rank, i, j, *tags;
    MPI_Comm comm;
    typedef struct MPIDI_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        MPI_Op op;
    } MPIDI_RMA_op_info;
    MPIDI_RMA_op_info rma_op_info;
    MPI_Request *reqs;
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent;
    MPI_Group win_grp, post_grp;
    int post_grp_size, *ranks_in_post_grp, *ranks_in_win_grp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

    MPIR_Nest_incr();
    
    comm = win_ptr->comm;
    NMPI_Comm_size(comm, &comm_size);
    NMPI_Comm_rank(comm, &rank);

    /* For each process in post_group, find out how many RMA ops from
       that process is this process a target for */

    nops_from_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!nops_from_proc) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    /* We need to translate the ranks of the processes in
       post_group to ranks in win_ptr->comm, so that we
       can do communication */

    NMPI_Comm_group(win_ptr->comm, &win_grp);
    post_grp_size = win_ptr->post_group_ptr->size;

    ranks_in_post_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
    if (!ranks_in_post_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    ranks_in_win_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
    if (!ranks_in_win_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    for (i=0; i<post_grp_size; i++)
        ranks_in_post_grp[i] = i;

    post_grp = win_ptr->post_group_ptr->handle;
    NMPI_Group_translate_ranks(post_grp, post_grp_size,
                               ranks_in_post_grp, win_grp, ranks_in_win_grp);

    reqs = (MPI_Request *)  MPIU_Malloc(post_grp_size*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    
    tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
    if (!tags) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    for (i=0; i<post_grp_size; i++) {
        src = ranks_in_win_grp[i];
        mpi_errno = NMPI_Irecv(&nops_from_proc[src], 1, MPI_INT, src,
                               tags[src], comm, &reqs[i]);
        if (mpi_errno) return mpi_errno;
        tags[src]++;
    }

    MPIU_Free(ranks_in_win_grp);
    MPIU_Free(ranks_in_post_grp);

    mpi_errno = NMPI_Waitall(post_grp_size, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) return mpi_errno;

    /* Now for each op for which this process is a target, first
       get the info regarding that op and then post an isend or
       irecv to perform the operation. */

    for (i=0; i<comm_size; i++) {
        /* instead of having everyone start receiving from 0,
           stagger the recvs a bit */ 
        src = (rank + i) % comm_size;
        
        for (j=0; j<nops_from_proc[src]; j++) {
            mpi_errno = NMPI_Recv(&rma_op_info,
                                  sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                  src, tags[src], comm,
                                  MPI_STATUS_IGNORE);
            if (mpi_errno) return mpi_errno;
            tags[src]++;
            
            switch (rma_op_info.type) {
            case MPIDI_RMA_PUT:
                /* recv the put */
                mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                      win_ptr->disp_unit *
                                      rma_op_info.disp,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return mpi_errno;
                break;
            case MPIDI_RMA_GET:
                /* send the get */
                mpi_errno = NMPI_Send((char *) win_ptr->base +
                                      win_ptr->disp_unit *
                                      rma_op_info.disp,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm);
                if (mpi_errno) return mpi_errno;
                break;
            case MPIDI_RMA_ACCUMULATE:
                /* recv the data into a temp buffer and perform
                   the reduction operation */
                NMPI_Type_extent(rma_op_info.datatype, 
                                 &extent); 
                tmp_buf = MPIU_Malloc(extent * 
                                      rma_op_info.count);
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code(
                        MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
                    return mpi_errno;
                }
                mpi_errno = NMPI_Recv(tmp_buf,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return mpi_errno;
                
                op = rma_op_info.op;
                if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                    /* get the function by indexing into the op table */
                    uop = MPIR_Op_table[op%16 - 1];
                }
                else {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                      "**opnotpredefined", "**opnotpredefined %d", op );
                    return mpi_errno;
                }
                (*uop)(tmp_buf, (char *) win_ptr->base +
                       win_ptr->disp_unit *
                       rma_op_info.disp,
                       &(rma_op_info.count),
                       &(rma_op_info.datatype));
                MPIU_Free(tmp_buf);
                break;
            default:
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                  "**opnotpredefined", "**opnotpredefined %d", rma_op_info.type );
                return mpi_errno;
            }
            
            tags[src]++;
        }
    }
    
    MPIR_Nest_decr();
    
    MPIU_Free(tags);
    MPIU_Free(reqs);
    MPIU_Free(nops_from_proc);
    NMPI_Group_free(&win_grp);
    
    MPIR_Group_release_ref(win_ptr->post_group_ptr,&i);
    if (!i) {
        /* Only if refcount is 0 do we actually free. */
        MPIU_Free( win_ptr->post_group_ptr->lrank_to_lpid );
        MPIU_Handle_obj_free( &MPID_Group_mem, win_ptr->post_group_ptr );
    }

    win_ptr->post_group_ptr = NULL; 

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);

    return mpi_errno;
#endif
#endif

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, nops_to_proc;
    int i, tag, req_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIDI_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
        int lock_type;
    } MPIDI_RMA_op_info;
    MPIDI_RMA_op_info *rma_op_infos;
    MPIDI_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPID_Datatype *dtp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

#ifdef MPICH_SINGLE_THREADED
    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
#endif

    MPIR_Nest_incr();

    comm = win_ptr->comm;

    /* First inform target process how many
       RMA ops from this process is it the target for.
       There could be ops destined for other processes in
       MPIDI_RMA_ops_list because there could be multiple MPI_Win_locks
       outstanding */

    nops_to_proc = 0;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        if (curr_ptr->target_rank == dest) nops_to_proc++;
        curr_ptr = curr_ptr->next;
    }

    reqs = (MPI_Request *) MPIU_Malloc((4*nops_to_proc+1)*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    
    tag = MPIDI_PASSIVE_TARGET_RMA_TAG;
    mpi_errno = NMPI_Isend(&nops_to_proc, 1, MPI_INT, dest,
                           tag, comm, reqs);
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }

    /* For each RMA op, first send the type (put or get), target
       displ, count, datatype. Then issue an isend for a 
       put or irecv for a get. */
    
    rma_op_infos = (MPIDI_RMA_op_info *) 
        MPIU_Malloc((nops_to_proc+1) * sizeof(MPIDI_RMA_op_info));
    /* allocate one extra to avoid 0 size malloc */ 
    if (!rma_op_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }

    dtype_infos = (MPIDI_RMA_dtype_info *)
        MPIU_Malloc((nops_to_proc+1)*sizeof(MPIDI_RMA_dtype_info));
    /* allocate one extra to avoid 0 size malloc */ 
    if (!dtype_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    
    dataloops = (void **) MPIU_Malloc((nops_to_proc+1)*sizeof(void*));
    /* allocate one extra to avoid 0 size malloc */ 
    if (!dataloops) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    for (i=0; i<nops_to_proc; i++)
        dataloops[i] = NULL;

    i = 0;
    tag = 234;
    req_cnt = 1;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        rma_op_infos[i].type = curr_ptr->type;
        rma_op_infos[i].disp = curr_ptr->target_disp;
        rma_op_infos[i].count = curr_ptr->target_count;
        rma_op_infos[i].datatype = curr_ptr->target_datatype;
        rma_op_infos[i].op = curr_ptr->op;
        rma_op_infos[i].lock_type = curr_ptr->lock_type;

        if (rma_op_infos[i].type == MPIDI_RMA_LOCK) {
            rma_op_infos[i].datatype_kind = -1; /* undefined */
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            req_cnt++;
            tag++;
        }
        else if (HANDLE_GET_KIND(curr_ptr->target_datatype) ==
                 HANDLE_KIND_BUILTIN) {
            /* basic datatype. send only the rma_op_info struct */
            rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_BASIC;
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            req_cnt++;
            tag++;
        }
        else {
            /* derived datatype. send rma_op_info_struct as well
               as derived datatype information */
            
            rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_DERIVED; 
            /* fill derived datatype info */
            MPID_Datatype_get_ptr(curr_ptr->target_datatype, dtp);
            dtype_infos[i].is_contig = dtp->is_contig;
            dtype_infos[i].n_contig_blocks = dtp->n_contig_blocks;
            dtype_infos[i].size = dtp->size;
            dtype_infos[i].extent = dtp->extent;
            dtype_infos[i].dataloop_size = dtp->dataloop_size;
            dtype_infos[i].dataloop_depth = dtp->dataloop_depth;
            dtype_infos[i].eltype = dtp->eltype;
            dtype_infos[i].dataloop = dtp->dataloop;
            dtype_infos[i].ub = dtp->ub;
            dtype_infos[i].lb = dtp->lb;
            dtype_infos[i].true_ub = dtp->true_ub;
            dtype_infos[i].true_lb = dtp->true_lb;
            dtype_infos[i].has_sticky_ub = dtp->has_sticky_ub;
            dtype_infos[i].has_sticky_lb = dtp->has_sticky_lb;
            
            dataloops[i] = MPIU_Malloc(dtp->dataloop_size);
            if (!dataloops[i]) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            memcpy(dataloops[i], dtp->dataloop, dtp->dataloop_size);
            
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            req_cnt++;
            tag++;
            
            /* send the datatype info */
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&dtype_infos[i],
                                   sizeof(MPIDI_RMA_dtype_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            req_cnt++;
            tag++;
            
            mpi_errno = NMPI_Isend(dataloops[i],
                                   dtp->dataloop_size, MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
            req_cnt++;
            tag++;
            
            /* release the target dataype */
            MPID_Datatype_release(dtp);
        }

        /* now send or recv the data */
        if ((curr_ptr->type == MPIDI_RMA_PUT) ||
            (curr_ptr->type == MPIDI_RMA_ACCUMULATE)) {
            mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) !=
                HANDLE_KIND_BUILTIN) {  
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }
            req_cnt++;
            tag++;
        }
        else if (curr_ptr->type == MPIDI_RMA_GET) {
            mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) !=
                HANDLE_KIND_BUILTIN) {  
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }
            req_cnt++;
            tag++;
        }
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
            return mpi_errno;
        }
        
        curr_ptr = curr_ptr->next;
        i++;
    }        

    mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }

    /* Passive target RMA must be complete at target when unlock
       returns. Therefore we need ack from target that it is done. */
    mpi_errno = NMPI_Recv(&i, 0, MPI_INT, dest,
                          MPIDI_PASSIVE_TARGET_DONE_TAG, comm,
                          MPI_STATUS_IGNORE); 
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }

    MPIR_Nest_decr();

    MPIU_Free(reqs);
    MPIU_Free(rma_op_infos);
    MPIU_Free(dtype_infos);
    for (i=0; i<nops_to_proc; i++)
        if (dataloops[i] != NULL) 
            MPIU_Free(dataloops[i]);
    MPIU_Free(dataloops);

    /* free MPIDI_RMA_ops_list */
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    MPIDI_RMA_ops_list = NULL;
    
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);

    return mpi_errno;
}

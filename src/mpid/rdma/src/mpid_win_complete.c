/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, src;
    int i, j, dst, done, total_op_count, *curr_ops_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests; /* array of requests */
    int *decr_addr, new_total_op_count;
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */
    MPI_Group win_grp, start_grp;
    int start_grp_size, *ranks_in_start_grp, *ranks_in_win_grp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    /* Translate the ranks of the processes in
       start_group to ranks in win_ptr->comm */

    MPIR_Nest_incr();
    
    NMPI_Comm_group(win_ptr->comm, &win_grp);

    start_grp_size = win_ptr->start_group_ptr->size;
    
    ranks_in_start_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_start_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    ranks_in_win_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_win_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    for (i=0; i<start_grp_size; i++)
        ranks_in_start_grp[i] = i;
    
    start_grp = win_ptr->start_group_ptr->handle;
    NMPI_Group_translate_ranks(start_grp, start_grp_size,
                               ranks_in_start_grp, win_grp, ranks_in_win_grp);
    

    /* If MPI_MODE_NOCHECK was not specified, we need to check if
       Win_post was called on the target processes. Wait for a 0-byte sync
       message from each target process */
    if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0) {
        for (i=0; i<start_grp_size; i++) {
            src = ranks_in_win_grp[i];
            mpi_errno = NMPI_Recv(NULL, 0, MPI_INT, src, 100,
                                  win_ptr->comm, MPI_STATUS_IGNORE);
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
        }
    }

    MPIR_Nest_decr();

    /* keep track of no. of ops to each proc. Needed for knowing
       whether or not to decrement the completion counter. The
       completion counter is decremented only on the last
       operation. */

    nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!nops_to_proc) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    total_op_count = 0;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        nops_to_proc[curr_ptr->target_rank]++;
        total_op_count++;
        curr_ptr = curr_ptr->next;
    }

    requests = (MPID_Request **) MPIU_Malloc((total_op_count+start_grp_size) *
                                             sizeof(MPID_Request*));
    /* We allocate a few extra requests because if there are no RMA
       ops to a target process, we need to send a 0-byte message just
       to decrement the completion counter. */

    if (!requests) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    curr_ops_cnt = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!curr_ops_cnt) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }

    if (total_op_count != 0) {
        dtype_infos = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc(total_op_count*sizeof(MPIDI_RMA_dtype_info));
        if (!dtype_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
            return mpi_errno;
        }
        
        dataloops = (void **) MPIU_Malloc(total_op_count*sizeof(void*));
        /* allocate one extra for use when receiving data. see below */
        if (!dataloops) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
            return mpi_errno;
        }
        for (i=0; i<total_op_count; i++)
            dataloops[i] = NULL;
    }

    i = 0;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        /* The completion counter at the target is decremented
           only on the last operation on the target. Otherwise, we
           pass NULL */
        if (curr_ops_cnt[curr_ptr->target_rank] ==
            nops_to_proc[curr_ptr->target_rank] - 1) 
            decr_addr = win_ptr->all_counters[curr_ptr->target_rank];
        else 
            decr_addr = NULL;

        switch (curr_ptr->type) {
        case (MPIDI_RMA_PUT):
        case (MPIDI_RMA_ACCUMULATE):
            mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
                     decr_addr, &dtype_infos[i], &dataloops[i], &requests[i]); 
            if (mpi_errno != MPI_SUCCESS) return mpi_errno;
            break;
        case (MPIDI_RMA_GET):
            mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
                     decr_addr, &dtype_infos[i], &dataloops[i], &requests[i]);
            if (mpi_errno != MPI_SUCCESS) return mpi_errno;
            break;
        default:
            /* FIXME - return some error code here */
            break;
        }
        i++;
        curr_ops_cnt[curr_ptr->target_rank]++;
        curr_ptr = curr_ptr->next;
    }

    /* If the start_group included some processes that did not end up
       becoming targets of  RMA operations from this process, we need
       to send a dummy message to those processes just to decrement
       the completion counter */

    j = i;
    new_total_op_count = total_op_count;
    for (i=0; i<start_grp_size; i++) {
        dst = ranks_in_win_grp[i];
        if (nops_to_proc[dst] == 0) {
            MPIDI_CH3_Pkt_t upkt;
            MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
            MPIDI_VC *vc;

            put_pkt->type = MPIDI_CH3_PKT_PUT;
            put_pkt->addr = NULL;
            put_pkt->count = 0;
            put_pkt->datatype = MPI_INT;
            put_pkt->decr_ctr = win_ptr->all_counters[dst];

            vc = comm_ptr->vcr[dst];

            mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt,
                                            sizeof(*put_pkt),
                                            &requests[j]); 
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                return mpi_errno;
            }
            j++;
            new_total_op_count++;
        }
    }

    MPIU_Free(ranks_in_win_grp);
    MPIU_Free(ranks_in_start_grp);
    MPIU_Free(nops_to_proc);
    MPIU_Free(curr_ops_cnt);

    done = 1;
    while (new_total_op_count) {
        MPID_Progress_start();
        for (i=0; i<new_total_op_count; i++) {
            if (requests[i] != NULL) {
                if (*(requests[i]->cc_ptr) != 0)
                    done = 0;
                else {
                        mpi_errno = requests[i]->status.MPI_ERROR;
                        if (mpi_errno != MPI_SUCCESS) {
                            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                            return mpi_errno;
                        }
                        MPID_Request_release(requests[i]);
                        requests[i] = NULL;
                }
            }
        }
        if (!done) {
            mpi_errno = MPID_Progress_wait();
            done = 1;
        }
        else {
            MPID_Progress_end();
            break;
        }
    } 

    MPIU_Free(requests);
    if (total_op_count != 0) {
        MPIU_Free(dtype_infos);
        for (i=0; i<total_op_count; i++)
            if (dataloops[i] != NULL) 
                MPIU_Free(dataloops[i]);
        MPIU_Free(dataloops);
    }

    /* free MPIDI_RMA_ops_list */
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    MPIDI_RMA_ops_list = NULL;
    
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
}



#ifdef USE_OLDSTUFF
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, dest;
    int i, *tags, total_op_count, req_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIDI_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
    } MPIDI_RMA_op_info;
    MPIDI_RMA_op_info *rma_op_infos;
    typedef struct MPIDI_RMA_dtype_info { /* for derived datatypes */
        int           is_contig; 
        int           n_contig_blocks;
        int           size;     
        MPI_Aint      extent;   
        int           dataloop_size; 
        void          *dataloop;  /* pointer needed to update pointers
                                     within dataloop on remote side */
        int           dataloop_depth; 
        int           eltype;
        MPI_Aint ub, lb, true_ub, true_lb;
        int has_sticky_ub, has_sticky_lb;
    } MPIDI_RMA_dtype_info;
    MPIDI_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPI_Group win_grp, start_grp;
    int start_grp_size, *ranks_in_start_grp, *ranks_in_win_grp;
    MPID_Datatype *dtp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

#ifdef MPICH_SINGLE_THREADED
    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
#endif

    MPIR_Nest_incr();

    comm = win_ptr->comm;
    NMPI_Comm_size(comm, &comm_size);

    /* First inform every process in start_group how many
       RMA ops from this process is it the target for. */

    nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!nops_to_proc) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    total_op_count = 0;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        nops_to_proc[curr_ptr->target_rank]++;
        total_op_count++;
        curr_ptr = curr_ptr->next;
    }

    /* We need to translate the ranks of the processes in
       start_group to ranks in win_ptr->comm, so that we
       can do communication */

    NMPI_Comm_group(win_ptr->comm, &win_grp);
    start_grp_size = win_ptr->start_group_ptr->size;

    ranks_in_start_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_start_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    ranks_in_win_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_win_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }

    for (i=0; i<start_grp_size; i++)
        ranks_in_start_grp[i] = i;

    start_grp = win_ptr->start_group_ptr->handle;
    NMPI_Group_translate_ranks(start_grp, start_grp_size,
                               ranks_in_start_grp, win_grp, ranks_in_win_grp);

    reqs = (MPI_Request *)
        MPIU_Malloc((start_grp_size+4*total_op_count)*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
    if (!tags) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }

    for (i=0; i<start_grp_size; i++) {
        dest = ranks_in_win_grp[i];
        mpi_errno = NMPI_Isend(&nops_to_proc[dest], 1, MPI_INT, dest,
                               tags[dest], comm, &reqs[i]);
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
            return mpi_errno;
        }
        tags[dest]++;
    }

    MPIU_Free(ranks_in_win_grp);
    MPIU_Free(ranks_in_start_grp);

    /* For each RMA op, first send the type (put or get), target
       displ, count, datatype. Then issue an isend for a 
       put or irecv for a get. */
    
    rma_op_infos = (MPIDI_RMA_op_info *) 
        MPIU_Malloc((total_op_count+1) * sizeof(MPIDI_RMA_op_info));
    /* allocate one extra to prevent 0 size malloc */
    if (!rma_op_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }

    dtype_infos = (MPIDI_RMA_dtype_info *)
        MPIU_Malloc((total_op_count+1)*sizeof(MPIDI_RMA_dtype_info));
    /* allocate one extra to prevent 0 size malloc */
    if (!dtype_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    
    dataloops = (void **) MPIU_Malloc((total_op_count+1)*sizeof(void*));
    /* allocate one extra to prevent 0 size malloc */
    if (!dataloops) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }
    for (i=0; i<total_op_count; i++)
        dataloops[i] = NULL;

    i = 0;
    req_cnt = start_grp_size;
    curr_ptr = MPIDI_RMA_ops_list;
    while (curr_ptr != NULL) {
        rma_op_infos[i].type = curr_ptr->type;
        rma_op_infos[i].disp = curr_ptr->target_disp;
        rma_op_infos[i].count = curr_ptr->target_count;
        rma_op_infos[i].datatype = curr_ptr->target_datatype;
        rma_op_infos[i].op = curr_ptr->op;
        dest = curr_ptr->target_rank;
        
        if (HANDLE_GET_KIND(curr_ptr->target_datatype) ==
            HANDLE_KIND_BUILTIN) {
            /* basic datatype. send only the rma_op_info struct */
            rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_BASIC;
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
            req_cnt++;
            tags[dest]++;
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
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
            memcpy(dataloops[i], dtp->dataloop, dtp->dataloop_size);

            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
            req_cnt++;
            tags[dest]++;

            /* send the datatype info */
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&dtype_infos[i],
                                   sizeof(MPIDI_RMA_dtype_info), MPI_BYTE, 
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
            req_cnt++;
            tags[dest]++;
            
            mpi_errno = NMPI_Isend(dataloops[i],
                                   dtp->dataloop_size, MPI_BYTE, 
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
                return mpi_errno;
            }
            req_cnt++;
            tags[dest]++;
            
            /* release the target dataype */
            MPID_Datatype_release(dtp);
        }

        /* now send or recv the data */
        if ((curr_ptr->type == MPIDI_RMA_PUT) ||
            (curr_ptr->type == MPIDI_RMA_ACCUMULATE))
            mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
        else
            mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
            return mpi_errno;
        }
        if (HANDLE_GET_KIND(curr_ptr->origin_datatype) != HANDLE_KIND_BUILTIN) {
            MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
            MPID_Datatype_release(dtp);
        }
        req_cnt++;
        tags[dest]++;
        
        curr_ptr = curr_ptr->next;
        i++;
    }        

    mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;
    }

    MPIR_Nest_decr();

    MPIU_Free(tags);
    MPIU_Free(reqs);
    MPIU_Free(rma_op_infos);
    MPIU_Free(nops_to_proc);
    NMPI_Group_free(&win_grp);
    MPIU_Free(dtype_infos);
    for (i=0; i<total_op_count; i++)
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

    /* free the group stored in window */
    MPIR_Group_release(win_ptr->start_group_ptr);
    win_ptr->start_group_ptr = NULL; 

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);

    return mpi_errno;
}
#endif

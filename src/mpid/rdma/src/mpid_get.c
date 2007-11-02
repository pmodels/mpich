/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    MPIDI_msg_sz_t data_sz;
    int mpi_errno = MPI_SUCCESS, dt_contig, rank;
    MPIDI_RMA_ops *curr_ptr, *prev_ptr, *new_ptr;
    MPID_Datatype *dtp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_GET);

    MPIDI_CH3U_Datatype_get_info(origin_count, origin_datatype,
                                 dt_contig, data_sz, dtp); 

    if ((data_sz == 0) || (target_rank == MPI_PROC_NULL)) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_GET);    
        return mpi_errno;
    }

    /* FIXME: It makes sense to save the rank (and size) of the
       communicator in the window structure to speed up these operations */
    MPIR_Nest_incr();
    NMPI_Comm_rank(win_ptr->comm, &rank);
    MPIR_Nest_decr();

    /* If the get is a local operation, do it here */
    if (target_rank == rank) {
        mpi_errno = MPIR_Localcopy((char *) win_ptr->base +
                                   win_ptr->disp_unit * target_disp,
                                   target_count, target_datatype,
                                   origin_addr, origin_count,
                                   origin_datatype);  
    }
    else {  /* queue it up */
        curr_ptr = MPIDI_RMA_ops_list;
        prev_ptr = MPIDI_RMA_ops_list;
        while (curr_ptr != NULL) {
            prev_ptr = curr_ptr;
            curr_ptr = curr_ptr->next;
        }
        
        new_ptr = (MPIDI_RMA_ops *) MPIU_Malloc(sizeof(MPIDI_RMA_ops));
        if (!new_ptr) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_GET);
            return mpi_errno;
        }
        if (prev_ptr != NULL)
            prev_ptr->next = new_ptr;
        else 
            MPIDI_RMA_ops_list = new_ptr;
        
        new_ptr->next = NULL;  
        new_ptr->type = MPIDI_RMA_GET;
        new_ptr->origin_addr = origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = target_count;
        new_ptr->target_datatype = target_datatype;
        
        /* if source or target datatypes are derived, increment their
           reference counts */ 
        if (HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
        if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_GET);

    return mpi_errno;
}

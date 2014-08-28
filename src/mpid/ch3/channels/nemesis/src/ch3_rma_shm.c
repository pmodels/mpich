/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_shared_query
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_SHM_Win_shared_query(MPID_Win *win_ptr, int target_rank, MPI_Aint *size, int *disp_unit, void *baseptr)
{
    int comm_size;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_SHARED_QUERY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_SHARED_QUERY);

    comm_size = win_ptr->comm_ptr->local_size;

    /* Scan the sizes to locate the first process that allocated a nonzero
     * amount of space */
    if (target_rank == MPI_PROC_NULL) {
        int i;

        /* Default, if no processes have size > 0. */
        *size               = 0;
        *disp_unit          = 0;
        *((void**) baseptr) = NULL;

        for (i = 0; i < comm_size; i++) {
            if (win_ptr->sizes[i] > 0) {
                *size               = win_ptr->sizes[i];
                *disp_unit          = win_ptr->disp_units[i];
                *((void**) baseptr) = win_ptr->shm_base_addrs[i];
                break;
            }
        }

    } else {
        *size               = win_ptr->sizes[target_rank];
        *disp_unit          = win_ptr->disp_units[target_rank];
        *((void**) baseptr) = win_ptr->shm_base_addrs[target_rank];
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_SHARED_QUERY);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_SHM_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_SHM_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_SHM_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_SHM_WIN_FREE);

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        mpi_errno = MPIDI_Win_free(win_ptr);
        goto fn_exit;
    }

    mpi_errno = MPIDI_CH3I_Wait_for_pt_ops_finish(*win_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Free shared memory region */
    if ((*win_ptr)->shm_allocated) {
        /* free shm_base_addrs that's only used for shared memory windows */
        MPIU_Free((*win_ptr)->shm_base_addrs);

        /* Only allocate and allocate_shared allocate new shared segments */
        if (((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
             (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) &&
            (*win_ptr)->shm_segment_len > 0) {
            /* detach from shared memory segment */
            mpi_errno = MPIU_SHMW_Seg_detach((*win_ptr)->shm_segment_handle, (char **)&(*win_ptr)->shm_base_addr,
                                         (*win_ptr)->shm_segment_len);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            MPIU_SHMW_Hnd_finalize(&(*win_ptr)->shm_segment_handle);
        }
    }

    /* Free shared process mutex memory region */
    /* Only allocate and allocate_shared allocate new shared mutex.
     * FIXME: it causes unnecessary synchronization when using the same mutex.  */
    if (((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
         (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) &&
        (*win_ptr)->shm_mutex && (*win_ptr)->shm_segment_len > 0) {
        MPID_Comm *node_comm_ptr = NULL;

        /* When allocating shared memory region segment, we need comm of processes
           that are on the same node as this process (node_comm).
           If node_comm == NULL, this process is the only one on this node, therefore
           we use comm_self as node comm. */
        if ((*win_ptr)->comm_ptr->node_comm != NULL)
            node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
        else
            node_comm_ptr = MPIR_Process.comm_self;
        MPIU_Assert(node_comm_ptr != NULL);

        if (node_comm_ptr->rank == 0) {
            MPIDI_CH3I_SHM_MUTEX_DESTROY(*win_ptr);
        }

        /* detach from shared memory segment */
        mpi_errno = MPIU_SHMW_Seg_detach((*win_ptr)->shm_mutex_segment_handle, (char **)&(*win_ptr)->shm_mutex,
                                         sizeof(MPIDI_CH3I_SHM_MUTEX));
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_SHMW_Hnd_finalize(&(*win_ptr)->shm_mutex_segment_handle);
    }

    /* Unlink from global SHM window list if it is original shared window */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) {
        MPIDI_CH3I_SHM_Wins_unlink(&shm_wins_list, (*win_ptr));
    }

    mpi_errno = MPIDI_Win_free(win_ptr);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_SHM_WIN_FREE);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

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
        *((void**) baseptr) = NULL;

        for (i = 0; i < comm_size; i++) {
            if (win_ptr->sizes[i] > 0) {
                *size               = win_ptr->sizes[i];
                *((void**) baseptr) = win_ptr->shm_base_addrs[i];
                break;
            }
        }

    } else {
        *size               = win_ptr->sizes[target_rank];
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

    /* Free shared memory region */
    if ((*win_ptr)->shm_allocated) {
        /* free shm_base_addrs that's only used for shared memory windows */
        MPIU_Free((*win_ptr)->shm_base_addrs);
        /* detach from shared memory segment */
        mpi_errno = MPIU_SHMW_Seg_detach((*win_ptr)->shm_segment_handle, (char **)&(*win_ptr)->shm_base_addr,
                                         (*win_ptr)->shm_segment_len);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_SHMW_Hnd_finalize(&(*win_ptr)->shm_segment_handle);
    }

    /* Free shared process mutex memory region */
    if ((*win_ptr)->shm_mutex) {

        if ((*win_ptr)->myrank == 0) {
            MPIDI_CH3I_SHM_MUTEX_DESTROY(*win_ptr);
        }

        /* detach from shared memory segment */
        mpi_errno = MPIU_SHMW_Seg_detach((*win_ptr)->shm_mutex_segment_handle, (char **)&(*win_ptr)->shm_mutex,
                                         sizeof(MPIDI_CH3I_SHM_MUTEX));
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_SHMW_Hnd_finalize(&(*win_ptr)->shm_mutex_segment_handle);
    }

    mpi_errno = MPIDI_Win_free(win_ptr);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_SHM_WIN_FREE);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

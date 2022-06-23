/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"
#include <utarray.h>

int MPIDI_CH3_SHM_Win_shared_query(MPIR_Win * win_ptr, int target_rank, MPI_Aint * size,
                                   int *disp_unit, void *baseptr)
{
    int comm_size;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (win_ptr->comm_ptr->node_comm == NULL) {
        mpi_errno = MPIDI_CH3U_Win_shared_query(win_ptr, target_rank, size, disp_unit, baseptr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    comm_size = win_ptr->comm_ptr->local_size;

    /* Scan the sizes to locate the first process that allocated a nonzero
     * amount of space */
    if (target_rank == MPI_PROC_NULL) {
        int i;

        /* Default, if no processes have size > 0. */
        *size = 0;
        *disp_unit = 0;
        *((void **) baseptr) = NULL;

        for (i = 0; i < comm_size; i++) {
            if (win_ptr->basic_info_table[i].size > 0) {
                int local_i = win_ptr->comm_ptr->intranode_table[i];
                MPIR_Assert(local_i >= 0 && local_i < win_ptr->comm_ptr->node_comm->local_size);
                *size = win_ptr->basic_info_table[i].size;
                *disp_unit = win_ptr->basic_info_table[i].disp_unit;
                *((void **) baseptr) = win_ptr->shm_base_addrs[local_i];
                break;
            }
        }

    }
    else {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
        MPIR_Assert(local_target_rank >= 0 &&
                    local_target_rank < win_ptr->comm_ptr->node_comm->local_size);
        *size = win_ptr->basic_info_table[target_rank].size;
        *disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;
        *((void **) baseptr) = win_ptr->shm_base_addrs[local_target_rank];
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

struct shm_mutex_entry {
    int rank;
    MPL_shm_hnd_t shm_hnd;
    MPIDI_CH3I_SHM_MUTEX *shm_mutex;
};

static UT_icd shm_mutex_icd = {sizeof(struct shm_mutex_entry), NULL, NULL, NULL};
static UT_array *shm_mutex_free_list;

int MPIDI_CH3_SHM_Init(void)
{
    utarray_new(shm_mutex_free_list, &shm_mutex_icd, MPL_MEM_OTHER);
    return 0;
}

int MPIDI_CH3_SHM_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int mpl_err = 0;
    struct shm_mutex_entry *p;

    for (p = (struct shm_mutex_entry *) utarray_front(shm_mutex_free_list); p != NULL;
         p = (struct shm_mutex_entry *) utarray_next(shm_mutex_free_list, p)) {
        if (p->rank == 0) {
            MPIDI_CH3I_SHM_MUTEX_DESTROY_DIRECT(p->shm_mutex);
        }

        /* detach from shared memory segment */
        mpl_err = MPL_shm_seg_detach(p->shm_hnd, (void **) &p->shm_mutex,
                                     sizeof(MPIDI_CH3I_SHM_MUTEX));
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

        MPL_shm_hnd_finalize(&p->shm_hnd);
    }
    utarray_free(shm_mutex_free_list);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int delay_shm_mutex_destroy(int rank, MPIR_Win *win_ptr)
{
#ifdef DELAY_SHM_MUTEX_DESTROY
    /* On FreeBSD (tested on ver 12.2) destroying the mutex and recreate the mutex,
     * which may result in the same address, the new mutex will not work for inter-
     * process. To work around, we delay the destroy of mutex until finalize. */
    struct shm_mutex_entry entry;
    entry.rank = rank;
    entry.shm_hnd = win_ptr->shm_mutex_segment_handle;
    entry.shm_mutex = win_ptr->shm_mutex;
    utarray_push_back(shm_mutex_free_list, &entry, MPL_MEM_OTHER);
    return 0;
#else
    int mpi_errno = MPI_SUCCESS;
    int mpl_err = 0;

    if (rank == 0) {
        MPIDI_CH3I_SHM_MUTEX_DESTROY_DIRECT(win_ptr->shm_mutex);
    }

    /* detach from shared memory segment */
    mpl_err = MPL_shm_seg_detach(win_ptr->shm_mutex_segment_handle, (void **) &win_ptr->shm_mutex,
                                 sizeof(MPIDI_CH3I_SHM_MUTEX));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");

    MPL_shm_hnd_finalize(&win_ptr->shm_mutex_segment_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}

int MPIDI_CH3_SHM_Win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int mpl_err = 0;

    MPIR_FUNC_ENTER;

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        goto fn_exit;
    }

    /* Free shared memory region */
    if ((*win_ptr)->shm_allocated) {
        /* free shm_base_addrs that's only used for shared memory windows */
        MPL_free((*win_ptr)->shm_base_addrs);

        /* Only allocate and allocate_shared allocate new shared segments */
        if (((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
             (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) &&
            (*win_ptr)->shm_segment_len > 0) {
            /* detach from shared memory segment */
            mpl_err = MPL_shm_seg_detach((*win_ptr)->shm_segment_handle,
                                         &(*win_ptr)->shm_base_addr,
                                         (*win_ptr)->shm_segment_len);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

            MPL_shm_hnd_finalize(&(*win_ptr)->shm_segment_handle);
        }
    }

    /* Free shared process mutex memory region */
    /* Only allocate and allocate_shared allocate new shared mutex.
     * FIXME: it causes unnecessary synchronization when using the same mutex.  */
    if (((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
         (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) &&
        (*win_ptr)->shm_mutex && (*win_ptr)->shm_segment_len > 0) {
        MPIR_Comm *node_comm_ptr = NULL;

        /* When allocating shared memory region segment, we need comm of processes
         * that are on the same node as this process (node_comm).
         * If node_comm == NULL, this process is the only one on this node, therefore
         * we use comm_self as node comm. */
        node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
        MPIR_Assert(node_comm_ptr != NULL);

        delay_shm_mutex_destroy(node_comm_ptr->rank, *win_ptr);
    }

    /* Free shared memory region for window info */
    if ((*win_ptr)->info_shm_base_addr != NULL) {
        mpl_err = MPL_shm_seg_detach((*win_ptr)->info_shm_segment_handle,
                                     &(*win_ptr)->info_shm_base_addr,
                                     (*win_ptr)->info_shm_segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

        MPL_shm_hnd_finalize(&(*win_ptr)->info_shm_segment_handle);

        (*win_ptr)->basic_info_table = NULL;
    }

    /* Unlink from global SHM window list if it is original shared window */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) {
        MPIDI_CH3I_SHM_Wins_unlink(&shm_wins_list, (*win_ptr));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

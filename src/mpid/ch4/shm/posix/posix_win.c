/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_noinline.h"

int MPIDI_POSIX_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);

    mpi_errno = MPIDIG_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);

    mpi_errno = MPIDIG_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);

    mpi_errno = MPIDIG_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                               MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);

    mpi_errno = MPIDIG_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);

    mpi_errno = MPIDIG_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                        MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDIG_mpi_win_allocate_shared(size, disp_unit, info_ptr,
                                               comm_ptr, base_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);

    mpi_errno = MPIDIG_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                 void *baseptr, MPIR_Win ** win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDIG_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDIG_mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);

    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_HOOK);

    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    /* No optimization */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_HOOK);
    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    bool mapfail_flag = false;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_HOOK);

    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    /* Enable shm RMA only when interprocess mutex is supported,
     * more than 1 processes exist on the node, and shm buffer has been successfully allocated. */
    if (shm_comm_ptr == NULL || !MPL_proc_mutex_enabled() || !MPIDIG_WIN(win, mmap_addr))
        goto fn_exit;

    posix_win = &win->dev.shm.posix;

    /* allocate interprocess mutex for RMA atomics over shared memory */
    mpi_errno =
        MPIDU_shm_alloc(shm_comm_ptr, sizeof(MPL_proc_mutex_t), (void **) &posix_win->shm_mutex_ptr,
                        &mapfail_flag);

    /* disable SHM_ALLOCATED optimization if mutex allocation fails */
    if (!mapfail_flag) {
        if (shm_comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_INIT(posix_win->shm_mutex_ptr);
        MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_SHM_ALLOCATED;
    }

    /* No barrier is needed here, because the CH4 generic routine does it */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = NULL;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    bool mapfail_flag = false;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED_HOOK);

    /* Enable shm RMA only when interprocess mutex is supported and
     * more than 1 processes exist on the node. */
    if (!shm_comm_ptr || !MPL_proc_mutex_enabled())
        goto fn_exit;

    posix_win = &win->dev.shm.posix;

    /* allocate interprocess mutex for RMA atomics over shared memory */
    mpi_errno =
        MPIDU_shm_alloc(win->comm_ptr, sizeof(MPL_proc_mutex_t),
                        (void **) &posix_win->shm_mutex_ptr, &mapfail_flag);

    /* disable SHM_ALLOCATED optimization if mutex allocation fails */
    if (!mapfail_flag) {
        if (win->comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_INIT(posix_win->shm_mutex_ptr);
        MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_SHM_ALLOCATED;
    }

    /* No barrier is needed here, because the CH4 generic routine does it */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC_HOOK);

    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    /* No optimization */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC_HOOK);
    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_attach_hook(MPIR_Win * win ATTRIBUTE((unused)),
                                    void *base ATTRIBUTE((unused)),
                                    MPI_Aint size ATTRIBUTE((unused)))
{
    /* No optimization */
    return MPI_SUCCESS;
}

int MPIDI_POSIX_mpi_win_detach_hook(MPIR_Win * win ATTRIBUTE((unused)),
                                    const void *base ATTRIBUTE((unused)))
{
    /* No optimization */
    return MPI_SUCCESS;
}

/* Initialize internal components for POSIX based SHM RMA.
 * It is called by another shmmod (e.g., xpmem) who can enable memory sharing
 * for the window but want to reuse the POSIX RMA operations. */
int MPIDI_POSIX_shm_win_init_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    bool mapfail_flag = false;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_SHM_WIN_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_SHM_WIN_INIT_HOOK);

    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    if (shm_comm_ptr == NULL || !MPL_proc_mutex_enabled())
        goto fn_exit;

    /* allocate interprocess mutex for RMA atomics over shared memory */
    mpi_errno =
        MPIDU_shm_alloc(shm_comm_ptr, sizeof(MPL_proc_mutex_t), (void **) &posix_win->shm_mutex_ptr,
                        &mapfail_flag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!mapfail_flag) {
        if (shm_comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_INIT(posix_win->shm_mutex_ptr);

        MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_SHM_ALLOCATED;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_SHM_WIN_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE_HOOK);

    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
        MPIR_Assert(posix_win->shm_mutex_ptr != NULL);

        /* destroy and detach shared mutex */
        if (win->comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_DESTROY(posix_win->shm_mutex_ptr);

        mpi_errno = MPIDU_shm_free(posix_win->shm_mutex_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

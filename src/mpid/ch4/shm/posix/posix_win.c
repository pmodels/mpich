/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "posix_noinline.h"

int MPIDI_POSIX_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_set_info(win, info);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_get_info(win, info_p_p);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_free(win_ptr);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                               MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_attach(win, base, size);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                        MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_allocate_shared(size, disp_unit, info_ptr,
                                               comm_ptr, base_ptr, win_ptr);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_detach(win, base);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                 void *baseptr, MPIR_Win ** win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_create_dynamic(info, comm, win);



    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;



    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    /* No optimization */


    return mpi_errno;
}

int MPIDI_POSIX_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    bool mapfail_flag = false;



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

    /* disable shm_allocated optimization if mutex allocation fails */
    if (!mapfail_flag) {
        if (shm_comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_INIT(posix_win->shm_mutex_ptr);
        MPIDIG_WIN(win, shm_allocated) = 1;
    }

    /* No barrier is needed here, because the CH4 generic routine does it */

  fn_exit:

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



    /* Enable shm RMA only when interprocess mutex is supported and
     * more than 1 processes exist on the node. */
    if (!shm_comm_ptr || !MPL_proc_mutex_enabled())
        goto fn_exit;

    posix_win = &win->dev.shm.posix;

    /* allocate interprocess mutex for RMA atomics over shared memory */
    mpi_errno =
        MPIDU_shm_alloc(win->comm_ptr, sizeof(MPL_proc_mutex_t),
                        (void **) &posix_win->shm_mutex_ptr, &mapfail_flag);

    /* disable shm_allocated optimization if mutex allocation fails */
    if (!mapfail_flag) {
        if (win->comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_INIT(posix_win->shm_mutex_ptr);
        MPIDIG_WIN(win, shm_allocated) = 1;
    }

    /* No barrier is needed here, because the CH4 generic routine does it */

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win ATTRIBUTE((unused)) = NULL;



    posix_win = &win->dev.shm.posix;
    posix_win->shm_mutex_ptr = NULL;

    /* No optimization */


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

        MPIDIG_WIN(win, shm_allocated) = 1;
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    if (MPIDIG_WIN(win, shm_allocated)) {
        MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
        MPIR_Assert(posix_win->shm_mutex_ptr != NULL);

        /* destroy and detach shared mutex */
        if (win->comm_ptr->rank == 0)
            MPIDI_POSIX_RMA_MUTEX_DESTROY(posix_win->shm_mutex_ptr);

        mpi_errno = MPIDU_shm_free(posix_win->shm_mutex_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

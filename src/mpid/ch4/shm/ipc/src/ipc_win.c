/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_noinline.h"
#include "ipc_types.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"

typedef struct win_shared_info {
    uint32_t disp_unit;
    size_t size;
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_ipc_handle_t ipc_handle;
} win_shared_info_t;

int MPIDI_IPC_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    int i;
    size_t total_shm_size = 0;
    MPIDIG_win_shared_info_t *shared_table = NULL;
    win_shared_info_t *ipc_shared_table = NULL; /* temporary exchange buffer */
    MPIDI_IPCI_ipc_attr_t ipc_attr;

    MPIR_FUNC_ENTER;
    MPIR_CHKPMEM_DECL();
    MPIR_CHKLMEM_DECL();

    /* Skip IPC initialization if no local process */
    if (!shm_comm_ptr)
        goto fn_exit;

    /* Determine IPC type based on buffer type and submodule availability.
     * We always exchange in case any remote buffer can be shared by IPC. */
    ipc_attr.ipc_type = MPIDI_IPCI_TYPE__NONE;
    bool done = false;

    /* Disable local IPC for zero buffer */
    if (win->size == 0) {
        done = true;
    }
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (!done) {
        /* FIXME: the rank should be remote rank for tracking caching, not local rank
         *        Here we can skip the tracking, e.g. just use MPI_PROC_NULL
         */
        mpi_errno = MPIDI_GPU_get_ipc_attr(win->base, win->size, MPIR_BYTE_INTERNAL,
                                           MPI_PROC_NULL, shm_comm_ptr, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        if (ipc_attr.ipc_type == MPIDI_IPCI_TYPE__SKIP) {
            ipc_attr.ipc_type = MPIDI_IPCI_TYPE__NONE;
            done = true;
        } else {
            done = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
        }
    }
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (!done) {
        mpi_errno = MPIDI_XPMEM_get_ipc_attr(win->base, win->size, MPIR_BYTE_INTERNAL, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        done = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
    }
#endif

    /* suppress -Wunused-but-set-variable warnings */
    ((void) done);

    /* Exchange shared memory region information */
    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);

    /* Create shared table for later RMA access. Note that shared_table is
     * a CH4 level structure used also for other window types. CH4 always
     * initializes shared table for win_allocate and win_allocate_shared because
     * their shm region are ensured by POSIX. The other window types can only
     * optionally initialize it in shmmod .*/
    MPIR_CHKPMEM_CALLOC(MPIDIG_WIN(win, shared_table),
                        sizeof(MPIDIG_win_shared_info_t) * shm_comm_ptr->local_size, MPL_MEM_RMA);
    shared_table = MPIDIG_WIN(win, shared_table);

    MPIR_CHKLMEM_MALLOC(ipc_shared_table, sizeof(win_shared_info_t) * shm_comm_ptr->local_size);

    memset(&ipc_shared_table[shm_comm_ptr->rank], 0, sizeof(win_shared_info_t));
    ipc_shared_table[shm_comm_ptr->rank].size = win->size;
    ipc_shared_table[shm_comm_ptr->rank].disp_unit = win->disp_unit;
    ipc_shared_table[shm_comm_ptr->rank].ipc_type = ipc_attr.ipc_type;

    switch (ipc_attr.ipc_type) {
#define IPC_HANDLE ipc_shared_table[shm_comm_ptr->rank].ipc_handle
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_IPCI_TYPE__XPMEM:
            MPIDI_XPMEM_fill_ipc_handle(&ipc_attr, &(IPC_HANDLE));
            break;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
        case MPIDI_IPCI_TYPE__GPU:
            MPIDI_GPU_fill_ipc_handle(&ipc_attr, &(IPC_HANDLE), NULL);
            break;
#endif
        default:
            break;
#undef IPC_HANDLE
    }

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                               0,
                               MPI_DATATYPE_NULL,
                               ipc_shared_table,
                               sizeof(win_shared_info_t), MPIR_BYTE_INTERNAL, shm_comm_ptr,
                               MPIR_COLL_ATTR_SYNC);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
    MPIR_ERR_CHECK(mpi_errno);

    /* Skip shared_table initialization if:
     *  1. All local processes are zero size, or
     *  2. Any process has a non-zero buffer but the IPC type is NONE (i.e.
     *     no submodule supports that buffer) */
    for (i = 0; i < shm_comm_ptr->local_size; i++) {
        total_shm_size += ipc_shared_table[i].size;
        if (ipc_shared_table[i].size > 0 && ipc_shared_table[i].ipc_type == MPIDI_IPCI_TYPE__NONE) {
            total_shm_size = 0;
            break;
        }
    }
    if (total_shm_size == 0) {
        MPL_free(MPIDIG_WIN(win, shared_table));
        MPIDIG_WIN(win, shared_table) = NULL;
        goto fn_exit;
    }

    /* Attach remote memory regions based on its IPC type */
    for (i = 0; i < shm_comm_ptr->local_size; i++) {
        shared_table[i].size = ipc_shared_table[i].size;
        shared_table[i].disp_unit = ipc_shared_table[i].disp_unit;
        shared_table[i].shm_base_addr = NULL;
        shared_table[i].ipc_mapped_device = -1;
        shared_table[i].ipc_type = ipc_shared_table[i].ipc_type;
        shared_table[i].global_dev_id = -1;

        if (i == shm_comm_ptr->rank) {
            shared_table[i].shm_base_addr = win->base;
        } else {
            /* attach remote buffer */
            switch (ipc_shared_table[i].ipc_type) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
                case MPIDI_IPCI_TYPE__XPMEM:
                    mpi_errno =
                        MPIDI_XPMEM_ipc_handle_map(ipc_shared_table[i].ipc_handle.xpmem,
                                                   &shared_table[i].shm_base_addr);
                    MPIR_ERR_CHECK(mpi_errno);
                    shared_table[i].mapped_type = 2;
                    break;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
                case MPIDI_IPCI_TYPE__GPU:
                    /* FIXME: remote win buffer should be mapped to each of their corresponding
                     * local GPU device. */
                    {
                        MPIDI_GPU_ipc_handle_t handle = ipc_shared_table[i].ipc_handle.gpu;
                        shared_table[i].ipc_handle = handle;
                        int dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr.u.gpu.gpu_attr);
                        int map_dev_id = MPIDI_GPU_ipc_get_map_dev(handle.global_dev_id, dev_id,
                                                                   MPIR_BYTE_INTERNAL);
                        int fast_copy = 0;
                        if (shared_table[i].size <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE) {
                            mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_shared_table[i].ipc_handle.gpu,
                                                                 map_dev_id,
                                                                 &shared_table[i].shm_base_addr,
                                                                 true);
                            if (mpi_errno == MPI_SUCCESS) {
                                fast_copy = 1;
                                shared_table[i].mapped_type = 1;
                            }
                        }
                        if (!fast_copy) {
                            mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_shared_table[i].ipc_handle.gpu,
                                                                 map_dev_id,
                                                                 &shared_table[i].shm_base_addr,
                                                                 false);
                            MPIR_ERR_CHECK(mpi_errno);
                            shared_table[i].mapped_type = 0;
                            shared_table[i].global_dev_id = handle.global_dev_id;
                        }
                        shared_table[i].ipc_mapped_device = map_dev_id;
                    }
                    break;
#endif
                case MPIDI_IPCI_TYPE__NONE:
                    /* no-op */
                    break;
                default:
                    /* Unknown IPC type */
                    MPIR_Assert(0);
                    break;
            }
        }
        IPC_TRACE("shared_table[%d]: size=0x%lx, disp_unit=0x%x, shm_base_addr=%p (ipc_type=%d)\n",
                  i, shared_table[i].size, shared_table[i].disp_unit,
                  shared_table[i].shm_base_addr, ipc_shared_table[i].ipc_type);
    }

    /* Initialize POSIX shm window components (e.g., shared mutex), thus
     * we can reuse POSIX operation routines. */
    mpi_errno = MPIDI_POSIX_shm_win_init_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_IPC_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    MPIR_FUNC_ENTER;

    if (win->create_flavor != MPI_WIN_FLAVOR_CREATE ||
        !shm_comm_ptr || !MPIDIG_WIN(win, shared_table))
        goto fn_exit;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
    for (int i = 0; i < shm_comm_ptr->local_size; i++) {
        if (i == shm_comm_ptr->rank)
            continue;
        if (shared_table[i].ipc_type == MPIDI_IPCI_TYPE__GPU) {
            mpi_errno = MPIDI_GPU_ipc_handle_unmap(shared_table[i].shm_base_addr,
                                                   shared_table[i].ipc_handle,
                                                   shared_table[i].mapped_type);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
#endif

    MPL_free(MPIDIG_WIN(win, shared_table));
    /* extra just to silence potential unused-label warnings */
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

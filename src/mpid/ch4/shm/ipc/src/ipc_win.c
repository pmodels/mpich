/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_noinline.h"
#include "ipc_types.h"
#include "ipc_mem.h"

/* Return global node rank of each process in the shared communicator.
 * I.e., rank in MPIR_Process.comm_world->node_comm. The caller routine
 * must allocate/free each buffer. */
static int get_node_ranks(MPIR_Comm * shm_comm_ptr, int *shm_ranks, int *node_ranks)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *shm_group_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_NODE_RANKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_NODE_RANKS);

    for (i = 0; i < shm_comm_ptr->local_size; i++)
        shm_ranks[i] = i;

    mpi_errno = MPIR_Comm_group_impl(shm_comm_ptr, &shm_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Get node group if it is not yet initialized */
    if (!MPIDI_IPCI_global.node_group_ptr) {
        mpi_errno = MPIR_Comm_group_impl(MPIR_Process.comm_world->node_comm,
                                         &MPIDI_IPCI_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Group_translate_ranks_impl(shm_group_ptr, shm_comm_ptr->local_size,
                                                shm_ranks, MPIDI_IPCI_global.node_group_ptr,
                                                node_ranks);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(shm_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_NODE_RANKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


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
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int i;
    size_t total_shm_size = 0;
    MPIDIG_win_shared_info_t *shared_table = NULL;
    win_shared_info_t *ipc_shared_table = NULL; /* temporary exchange buffer */
    int *ranks_in_shm_grp = NULL;
    MPIDI_IPCI_ipc_attr_t ipc_attr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_MPI_WIN_CREATE_HOOK);
    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(2);

    /* Skip IPC initialization if no local process */
    if (!shm_comm_ptr)
        goto fn_exit;

    /* Determine IPC type based on buffer type and submodule availability.
     * We always exchange in case any remote buffer can be shared by IPC. */
    MPIR_GPU_query_pointer_attr(win->base, &ipc_attr.gpu_attr);

    if (ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(win->base, shm_comm_ptr->rank, shm_comm_ptr, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIDI_XPMEM_get_ipc_attr(win->base, win->size, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Disable local IPC for zero buffer */
    if (win->size == 0)
        ipc_attr.ipc_type = MPIDI_IPCI_TYPE__NONE;

    /* Exchange shared memory region information */
    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);

    /* Create shared table for later RMA access. Note that shared_table is
     * a CH4 level structure used also for other window types. CH4 always
     * initializes shared table for win_allocate and win_allocate_shared because
     * their shm region are ensured by POSIX. The other window types can only
     * optionally initialize it in shmmod .*/
    MPIR_CHKPMEM_CALLOC(MPIDIG_WIN(win, shared_table), MPIDIG_win_shared_info_t *,
                        sizeof(MPIDIG_win_shared_info_t) * shm_comm_ptr->local_size,
                        mpi_errno, "shared table", MPL_MEM_RMA);
    shared_table = MPIDIG_WIN(win, shared_table);

    MPIR_CHKLMEM_MALLOC(ipc_shared_table, win_shared_info_t *,
                        sizeof(win_shared_info_t) * shm_comm_ptr->local_size,
                        mpi_errno, "IPC temporary shared table", MPL_MEM_RMA);

    memset(&ipc_shared_table[shm_comm_ptr->rank], 0, sizeof(win_shared_info_t));
    ipc_shared_table[shm_comm_ptr->rank].size = win->size;
    ipc_shared_table[shm_comm_ptr->rank].disp_unit = win->disp_unit;
    ipc_shared_table[shm_comm_ptr->rank].ipc_type = ipc_attr.ipc_type;
    ipc_shared_table[shm_comm_ptr->rank].ipc_handle = ipc_attr.ipc_handle;

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                               0,
                               MPI_DATATYPE_NULL,
                               ipc_shared_table,
                               sizeof(win_shared_info_t), MPI_BYTE, shm_comm_ptr, &errflag);
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
    MPIR_CHKLMEM_MALLOC(ranks_in_shm_grp, int *, shm_comm_ptr->local_size * sizeof(int) * 2,
                        mpi_errno, "ranks in shm group", MPL_MEM_RMA);
    mpi_errno = get_node_ranks(shm_comm_ptr, ranks_in_shm_grp,
                               &ranks_in_shm_grp[shm_comm_ptr->local_size]);
    MPIR_ERR_CHECK(mpi_errno);

    for (i = 0; i < shm_comm_ptr->local_size; i++) {
        shared_table[i].size = ipc_shared_table[i].size;
        shared_table[i].disp_unit = ipc_shared_table[i].disp_unit;
        shared_table[i].shm_base_addr = NULL;

        if (i == shm_comm_ptr->rank) {
            shared_table[i].shm_base_addr = win->base;
        } else {
            /* attach remote buffer */
            switch (ipc_shared_table[i].ipc_type) {
                case MPIDI_IPCI_TYPE__XPMEM:
                    mpi_errno =
                        MPIDI_XPMEM_ipc_handle_map(ipc_shared_table[i].ipc_handle.xpmem,
                                                   &shared_table[i].shm_base_addr);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
                case MPIDI_IPCI_TYPE__GPU:
                    /* FIXME: remote win buffer should be mapped to each of their corresponding
                     * local GPU device. */
                    mpi_errno =
                        MPIDI_GPU_ipc_handle_map(ipc_shared_table[i].ipc_handle.gpu,
                                                 ipc_attr.gpu_attr.device, MPI_BYTE,
                                                 &shared_table[i].shm_base_addr);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_WIN_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_IPC_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_MPI_WIN_FREE_HOOK);

    if (win->create_flavor != MPI_WIN_FLAVOR_CREATE ||
        !shm_comm_ptr || !MPIDIG_WIN(win, shared_table))
        goto fn_exit;

    MPL_free(MPIDIG_WIN(win, shared_table));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_WIN_FREE_HOOK);
    return mpi_errno;
}

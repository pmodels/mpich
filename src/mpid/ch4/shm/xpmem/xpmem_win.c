/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "xpmem_impl.h"
#include "xpmem_seg.h"
#include "xpmem_noinline.h"
#include "../posix/posix_noinline.h"

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
    if (!MPIDI_XPMEM_global.node_group_ptr) {
        mpi_errno = MPIR_Comm_group_impl(MPIR_Process.comm_world->node_comm,
                                         &MPIDI_XPMEM_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Group_translate_ranks_impl(shm_group_ptr, shm_comm_ptr->local_size,
                                                shm_ranks, MPIDI_XPMEM_global.node_group_ptr,
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

int MPIDI_XPMEM_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_win_t *xpmem_win = NULL;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int i;
    size_t total_shm_size = 0;
    MPIDIG_win_shared_info_t *shared_table = NULL;
    int *ranks_in_shm_grp = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_MPI_WIN_CREATE_HOOK);
    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(1);

    if (!shm_comm_ptr)
        goto fn_exit;

    xpmem_win = &win->dev.shm.xpmem;
    xpmem_win->regist_segs = NULL;

    /* Exchange shared memory region information */
    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);

    /* Create shared table for later RMA access. Note that shared_table is
     * a CH4 level structure used also for other window types. CH4 always
     * initializes shared table for win_allocate and win_allocate_shared because
     * their shm region are ensured by POSIX. The other window types can only
     * optionally initialize it in shmmod .*/
    MPIR_CHKPMEM_MALLOC(MPIDIG_WIN(win, shared_table), MPIDIG_win_shared_info_t *,
                        sizeof(MPIDIG_win_shared_info_t) * shm_comm_ptr->local_size,
                        mpi_errno, "shared table", MPL_MEM_RMA);
    shared_table = MPIDIG_WIN(win, shared_table);

    shared_table[shm_comm_ptr->rank].size = win->size;
    shared_table[shm_comm_ptr->rank].disp_unit = win->disp_unit;
    shared_table[shm_comm_ptr->rank].shm_base_addr = win->base;
    mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                               0,
                               MPI_DATATYPE_NULL,
                               shared_table,
                               sizeof(MPIDIG_win_shared_info_t), MPI_BYTE, shm_comm_ptr, &errflag);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
    MPIR_ERR_CHECK(mpi_errno);

    /* Skip shared_table initialization if all local processes are zero size. */
    for (i = 0; i < shm_comm_ptr->local_size; i++)
        total_shm_size += shared_table[i].size;
    if (total_shm_size == 0) {
        MPL_free(MPIDIG_WIN(win, shared_table));
        MPIDIG_WIN(win, shared_table) = NULL;
        goto fn_exit;
    }

    MPIR_CHKPMEM_MALLOC(xpmem_win->regist_segs, MPIDI_XPMEM_seg_t **,
                        sizeof(MPIDI_XPMEM_seg_t) * shm_comm_ptr->local_size,
                        mpi_errno, "xpmem regist segments", MPL_MEM_RMA);

    /* Register remote memory regions */
    MPIR_CHKLMEM_MALLOC(ranks_in_shm_grp, int *, shm_comm_ptr->local_size * sizeof(int) * 2,
                        mpi_errno, "ranks in shm group", MPL_MEM_RMA);
    mpi_errno = get_node_ranks(shm_comm_ptr, ranks_in_shm_grp,
                               &ranks_in_shm_grp[shm_comm_ptr->local_size]);
    MPIR_ERR_CHECK(mpi_errno);

    for (i = 0; i < shm_comm_ptr->local_size; i++) {
        if (shared_table[i].size > 0 && i != shm_comm_ptr->rank) {
            int node_rank = ranks_in_shm_grp[shm_comm_ptr->local_size + i];
            void *remote_vaddr = shared_table[i].shm_base_addr;
            mpi_errno = MPIDI_XPMEM_seg_regist(node_rank, shared_table[i].size,
                                               remote_vaddr,
                                               &xpmem_win->regist_segs[i],
                                               &shared_table[i].shm_base_addr,
                                               &MPIDI_XPMEM_global.
                                               segmaps[node_rank].segcache_ubuf);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (shared_table[i].size == 0)
            shared_table[i].shm_base_addr = NULL;
    }

    /* Initialize POSIX shm window components (e.g., shared mutex), thus
     * we can reuse POSIX operation routines. */
    mpi_errno = MPIDI_POSIX_shm_win_init_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_MPI_WIN_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_XPMEM_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_XPMEM_win_t *xpmem_win = &win->dev.shm.xpmem;
    MPIR_Comm *shm_comm_ptr = win->comm_ptr->node_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_MPI_WIN_FREE_HOOK);

    if (!shm_comm_ptr || win->create_flavor != MPI_WIN_FLAVOR_CREATE ||
        !MPIDIG_WIN(win, shared_table))
        goto fn_exit;

    /* Deregister segments for remote processes */
    for (i = 0; i < shm_comm_ptr->local_size; i++) {
        if (MPIDIG_WIN(win, shared_table)[i].size > 0 && i != shm_comm_ptr->rank) {
            mpi_errno = MPIDI_XPMEM_seg_deregist(xpmem_win->regist_segs[i]);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPL_free(MPIDIG_WIN(win, shared_table));
    MPL_free(xpmem_win->regist_segs);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_MPI_WIN_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

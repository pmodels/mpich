/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_WIN_H_INCLUDED
#define UCX_WIN_H_INCLUDED

#include "ucx_impl.h"
struct _UCX_share {
    int disp;
    MPI_Aint addr;
};

static inline int MPIDI_UCX_Win_allgather(MPIR_Win * win, size_t length,
                                          uint32_t disp_unit, void **base_ptr)
{

    MPIR_Errflag_t err = MPIR_ERR_NONE;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    ucp_mem_h mem_h;
    int cntr = 0;
    size_t rkey_size = 0;
    int *rkey_sizes = NULL, *recv_disps = NULL, i;
    char *rkey_buffer = NULL, *rkey_recv_buff = NULL;
    struct _UCX_share *share_data = NULL;
    ucp_mem_map_params_t mem_map_params;
    ucp_mem_attr_t mem_attr;
    MPIR_Comm *comm_ptr = win->comm_ptr;

    ucp_context_h ucp_context = MPIDI_UCX_global.context;

    MPIDI_UCX_WIN(win).info_table =
        MPL_malloc(sizeof(MPIDI_UCX_win_info_t) * comm_ptr->local_size, MPL_MEM_OTHER);

    /* Only non-zero region maps to device. */
    rkey_size = 0;
    if (length > 0) {
        mem_map_params.field_mask = UCP_MEM_MAP_PARAM_FIELD_ADDRESS |
            UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_FLAGS;

        mem_map_params.address = *base_ptr;
        mem_map_params.length = length;
        mem_map_params.flags = 0;

        if (*base_ptr == NULL)
            mem_map_params.flags |= UCP_MEM_MAP_ALLOCATE;

        status = ucp_mem_map(MPIDI_UCX_global.context, &mem_map_params, &mem_h);
        MPIDI_UCX_CHK_STATUS(status);

        /* query allocated address. */
        mem_attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS | UCP_MEM_ATTR_FIELD_LENGTH;
        status = ucp_mem_query(mem_h, &mem_attr);
        MPIDI_UCX_CHK_STATUS(status);

        *base_ptr = mem_attr.address;
        MPIR_Assert(mem_attr.length >= length);

        MPIDI_UCX_WIN(win).mem_h = mem_h;

        /* pack the key */
        status = ucp_rkey_pack(ucp_context, mem_h, (void **) &rkey_buffer, &rkey_size);

        MPIDI_UCX_CHK_STATUS(status);
    }

    rkey_sizes = (int *) MPL_malloc(sizeof(int) * comm_ptr->local_size, MPL_MEM_OTHER);
    rkey_sizes[comm_ptr->rank] = (int) rkey_size;
    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 1, MPI_INT, rkey_sizes, 1, MPI_INT, comm_ptr, &err);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    recv_disps = (int *) MPL_malloc(sizeof(int) * comm_ptr->local_size, MPL_MEM_OTHER);


    for (i = 0; i < comm_ptr->local_size; i++) {
        recv_disps[i] = cntr;
        cntr += rkey_sizes[i];
    }

    rkey_recv_buff = MPL_malloc(cntr, MPL_MEM_OTHER);

    /* allgather */
    mpi_errno = MPIR_Allgatherv(rkey_buffer, rkey_size, MPI_BYTE,
                                rkey_recv_buff, rkey_sizes, recv_disps, MPI_BYTE, comm_ptr, &err);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* If we use the shared memory support in UCX, we have to distinguish between local
     * and remote windows (at least now). If win_create is used, the key cannot be unpackt -
     * then we need our fallback-solution */

    for (i = 0; i < comm_ptr->local_size; i++) {
        /* Skip zero-size remote region. */
        if (rkey_sizes[i] == 0) {
            MPIDI_UCX_WIN_INFO(win, i).rkey = NULL;
            continue;
        }

        status = ucp_ep_rkey_unpack(MPIDI_UCX_COMM_TO_EP(comm_ptr, i),
                                    &rkey_recv_buff[recv_disps[i]],
                                    &(MPIDI_UCX_WIN_INFO(win, i).rkey));
        if (status == UCS_ERR_UNREACHABLE) {
            MPIDI_UCX_WIN_INFO(win, i).rkey = NULL;
        } else
            MPIDI_UCX_CHK_STATUS(status);
    }
    share_data = MPL_malloc(comm_ptr->local_size * sizeof(struct _UCX_share), MPL_MEM_OTHER);

    share_data[comm_ptr->rank].disp = disp_unit;
    share_data[comm_ptr->rank].addr = (MPI_Aint) * base_ptr;

    mpi_errno =
        MPIR_Allgather(MPI_IN_PLACE, sizeof(struct _UCX_share), MPI_BYTE, share_data,
                       sizeof(struct _UCX_share), MPI_BYTE, comm_ptr, &err);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    for (i = 0; i < comm_ptr->local_size; i++) {
        MPIDI_UCX_WIN_INFO(win, i).disp = share_data[i].disp;
        MPIDI_UCX_WIN_INFO(win, i).addr = share_data[i].addr;
    }
    MPIDI_UCX_WIN(win).need_local_flush = 0;
  fn_exit:
    /* buffer release */
    if (rkey_buffer)
        ucp_rkey_buffer_release(rkey_buffer);
    /* free temps */
    MPL_free(share_data);
    MPL_free(rkey_sizes);
    MPL_free(recv_disps);
    MPL_free(rkey_recv_buff);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_Win_init(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_UCX_WIN_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_UCX_WIN_INIT);

    memset(&MPIDI_UCX_WIN(win), 0, sizeof(MPIDI_UCX_win_t));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_UCX_PROGRESS_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDI_CH4R_mpi_win_set_info(win, info);
}


static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_start(group, assert, win);
}


static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_complete(win);
}

static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{

    return MPIDI_CH4R_mpi_win_post(group, assert, win);
}


static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_wait(win);
}


static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDI_CH4R_mpi_win_test(win, flag);
}

static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                        MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);
}


static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4R_mpi_win_unlock(rank, win);
}

static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_CH4R_mpi_win_get_info(win, info_p_p);
}


static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_free(win_ptr);
}

static inline int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_fence(assert, win);
}

static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
}

static inline int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDI_CH4R_mpi_win_attach(win, base, size);
}

static inline int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size,
                                                   int disp_unit,
                                                   MPIR_Info * info_ptr,
                                                   MPIR_Comm * comm_ptr,
                                                   void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                              win_ptr);
}

static inline int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDI_CH4R_mpi_win_detach(win, base);
}

static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    return MPIDI_CH4R_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
}

static inline int MPIDI_NM_mpi_win_allocate(MPI_Aint length,
                                            int disp_unit,
                                            MPIR_Info * info,
                                            MPIR_Comm * comm_ptr, void *baseptr,
                                            MPIR_Win ** win_ptr)
{

    return MPIDI_CH4R_mpi_win_allocate(length, disp_unit, info, comm_ptr, baseptr, win_ptr);
}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4R_mpi_win_flush(rank, win);
}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_flush_local_all(win);
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_unlock_all(win);
}

static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                  MPIR_Win ** win)
{
    return MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win);
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4R_mpi_win_flush_local(rank, win);
}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_sync(win);
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_flush_all(win);
}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_lock_all(assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_CREATE_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_UCX_Win_init(win);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDI_UCX_Win_allgather(win, win->size, win->disp_unit, &win->base);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_ALLOCATE_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_UCX_Win_init(win);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDI_UCX_Win_allgather(win, win->size, win->disp_unit, &win->base);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_ALLOCATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_FREE_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    if (win->create_flavor != MPI_WIN_FLAVOR_SHARED && win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC) {
        if (win->size > 0)
            ucp_mem_unmap(MPIDI_UCX_global.context, MPIDI_UCX_WIN(win).mem_h);
        MPL_free(MPIDI_UCX_WIN(win).info_table);
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_FREE_HOOK);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    /* maybe we just flush all eps here? More efficient for smaller communicators... */
    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
    MPIDI_UCX_CHK_STATUS(ucp_status);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    /* currently, UCP does not support local flush, so we have to call
     * a global flush. This is not good for performance - but OK for now */
    if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
        ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).need_local_flush = 0;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    if (rank != MPI_PROC_NULL) {
        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
        /* only flush the endpoint */
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_CHK_STATUS(ucp_status);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    if (rank != MPI_PROC_NULL) {
        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */

        if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
            ucp_status = ucp_ep_flush(ep);
            MPIDI_UCX_CHK_STATUS(ucp_status);
            MPIDI_UCX_WIN(win).need_local_flush = 0;
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* UCX_WIN_H_INCLUDED */

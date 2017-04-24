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

char ucx_dummy_buffer[4096];

static inline int MPIDI_UCX_Win_allgather(MPIR_Win * win, size_t length,
                                          uint32_t disp_unit, void **base_ptr)
{

    MPIR_Errflag_t err = MPIR_ERR_NONE;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    ucp_mem_h mem_h;
    int cntr = 0;
    size_t rkey_size;
    int *rkey_sizes, *recv_disps, i;
    char *rkey_buffer, *rkey_recv_buff = NULL;
    struct _UCX_share *share_data;
    size_t size;

    ucp_mem_map_params_t mem_map_params;

    if (length == 0)
        size = 1024;
    else
        size = length;
    MPIR_Comm *comm_ptr = win->comm_ptr;

    ucp_context_h ucp_context = MPIDI_UCX_global.context;

    MPIDI_UCX_WIN(win).info_table = MPL_malloc(sizeof(MPIDI_UCX_win_info_t) * comm_ptr->local_size);
    mem_map_params.field_mask = UCP_MEM_MAP_PARAM_FIELD_ADDRESS |
                                UCP_MEM_MAP_PARAM_FIELD_LENGTH|
                                UCP_MEM_MAP_PARAM_FIELD_FLAGS;
    if (length == 0)
        mem_map_params.address = &ucx_dummy_buffer;
    else
        mem_map_params.address = *base_ptr;
     mem_map_params.length = size;
     mem_map_params.flags = 0 ;

    status = ucp_mem_map(MPIDI_UCX_global.context, &mem_map_params , &mem_h);
    MPIDI_UCX_CHK_STATUS(status);

   if (length > 0)
        *base_ptr = mem_map_params.address;

    MPIDI_UCX_WIN(win).mem_h = mem_h;

    /* pack the key */
    status = ucp_rkey_pack(ucp_context, mem_h, (void **) &rkey_buffer, &rkey_size);

    MPIDI_UCX_CHK_STATUS(status);

    rkey_sizes = (int *) MPL_malloc(sizeof(int) * comm_ptr->local_size);
    rkey_sizes[comm_ptr->rank] = (int) rkey_size;
    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 1, MPI_INT,
                                    rkey_sizes, 1, MPI_INT, comm_ptr, &err);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    recv_disps = (int *) MPL_malloc(sizeof(int) * comm_ptr->local_size);


    for (i = 0; i < comm_ptr->local_size; i++) {
        recv_disps[i] = cntr;
        cntr += rkey_sizes[i];
    }

    rkey_recv_buff = MPL_malloc(cntr);

    /* allgather */
    mpi_errno = MPIR_Allgatherv_impl(rkey_buffer, rkey_size, MPI_BYTE,
                                     rkey_recv_buff, rkey_sizes, recv_disps, MPI_BYTE,
                                     comm_ptr, &err);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

/* If we use the shared memory support in UCX, we have to distinguish between local
    and remote windows (at least now). If win_create is used, the key cannot be unpackt -
    then we need our fallback-solution */

    for (i = 0; i < comm_ptr->local_size; i++) {
        status = ucp_ep_rkey_unpack(MPIDI_UCX_COMM_TO_EP(comm_ptr, i),
                                    &rkey_recv_buff[recv_disps[i]],
                                    &(MPIDI_UCX_WIN_INFO(win, i).rkey));
        if (status == UCS_ERR_UNREACHABLE) {
            MPIDI_UCX_WIN_INFO(win, i).rkey = NULL;
        }
        else
            MPIDI_UCX_CHK_STATUS(status);
    }
    share_data = MPL_malloc(comm_ptr->local_size * sizeof(struct _UCX_share));

    share_data[comm_ptr->rank].disp = disp_unit;
    share_data[comm_ptr->rank].addr = (MPI_Aint) *base_ptr;

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
static inline int MPIDI_UCX_Win_init(MPI_Aint length,
                                     int disp_unit,
                                     MPIR_Win ** win_ptr,
                                     MPIR_Info * info,
                                     MPIR_Comm * comm_ptr, int create_flavor, int model)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_UCX_WIN_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_UCX_WIN_INIT);

    mpi_errno = MPIDI_CH4R_win_init(length, disp_unit, &win, info, comm_ptr, create_flavor, model);
    MPIR_ERR_CHKANDSTMT(mpi_errno != MPI_SUCCESS,
                        mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

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

    ucs_status_t ucp_status;
    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
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
                                        MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    return MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);
}


static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{

    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucp_status;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
    /* make sure all operations are completed  */
    ucp_status = ucp_ep_flush(ep);

    MPIDI_UCX_CHK_STATUS(ucp_status);
    mpi_errno = MPIDI_CH4R_mpi_win_unlock(rank, win);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_CH4R_mpi_win_get_info(win, info_p_p);
}


static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = *win_ptr;

    MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);
    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);

    mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    if (win->create_flavor != MPI_WIN_FLAVOR_SHARED && win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC) {
        ucp_mem_unmap(MPIDI_UCX_global.context, MPIDI_UCX_WIN(win).mem_h);
        MPL_free(MPIDI_UCX_WIN(win).info_table);
    }
    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE)
        win->base = NULL;
    MPIDI_CH4R_win_finalize(win_ptr);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_WIN_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;
    ucs_status_t ucp_status;
    /*keep this for now to fence all none-natice operations */
/* make sure all local and remote operations are completed */
    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);


    mpi_errno = MPIDI_CH4R_mpi_win_fence(assert, win);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_UCX_CHK_STATUS(ucp_status);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_CREATE);

    MPIDI_UCX_Win_init(length, disp_unit, win_ptr, info,
                       comm_ptr, MPI_WIN_FLAVOR_CREATE, MPI_WIN_UNIFIED);

    win = *win_ptr;

    mpi_errno = MPIDI_UCX_Win_allgather(win, length, disp_unit, &base);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win->base = base;



    mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;


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

    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;
    void *base = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_WIN_ALLOCATE);

    MPIDI_UCX_Win_init(length, disp_unit, win_ptr, info,
                       comm_ptr, MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED);
    win = *win_ptr;
    mpi_errno = MPIDI_UCX_Win_allgather(win, length, disp_unit, &base);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    win->base = base;


    *(void **) baseptr = (void *) base;


    mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;


  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;


}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win,
                                         MPIDI_av_entry_t *addr)
{

    int mpi_errno;
    ucs_status_t ucp_status;

    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);

    mpi_errno = MPIDI_CH4R_mpi_win_flush(rank, win);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
/* only flush the endpoint */
    ucp_status = ucp_ep_flush(ep);

    MPIDI_UCX_CHK_STATUS(ucp_status);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucp_status;
    mpi_errno = MPIDI_CH4R_mpi_win_flush_local_all(win);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* currently, UCP does not support local flush, so we have to call
     * a global flush. This is not good for performance - but OK for now */
    if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
        ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).need_local_flush = 0;
    }


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucp_status;

    /*first we have to make sure that all operations are completed */
    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
    MPIDI_UCX_CHK_STATUS(ucp_status);
    mpi_errno = MPIDI_CH4R_mpi_win_unlock_all(win);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                  MPIR_Win ** win)
{
    return MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win);
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucp_status;
    mpi_errno = MPIDI_CH4R_mpi_win_flush_local(rank, win);

    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* currently, UCP does not support local flush, so we have to call
     * a global flush. This is not good for performance - but OK for now */

    if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).need_local_flush = 0;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_sync(win);
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{

/*maybe we just flush all eps here? More efficient for smaller communicators...*/
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucp_status;
    mpi_errno = MPIDI_CH4R_mpi_win_flush_all(win);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);

    MPIDI_UCX_CHK_STATUS(ucp_status);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_lock_all(assert, win);
}


#endif /* UCX_WIN_H_INCLUDED */

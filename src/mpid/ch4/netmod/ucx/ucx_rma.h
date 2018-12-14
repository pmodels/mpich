/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_RMA_H_INCLUDED
#define UCX_RMA_H_INCLUDED

#include "ucx_impl.h"

static inline int MPIDI_UCX_contig_put(const void *origin_addr,
                                       size_t size,
                                       int target_rank,
                                       MPI_Aint target_disp, MPI_Aint true_lb,
                                       MPIR_Win * win, MPIDI_av_entry_t * addr)
{

    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    size_t offset;
    uint64_t base;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;

    status = ucp_put_nbi(ep, origin_addr, size, base + offset, win_info->rkey);
    if (status == UCS_INPROGRESS)
        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    else if (status == UCS_OK)
        /* UCX 1.4 spec: completed immediately if returns UCS_OK.
         * FIXME: is it local completion or remote ? Now we assume local,
         * so we need flush to ensure remote completion.*/
        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH;
    else
        MPIDI_UCX_CHK_STATUS(status);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_UCX_noncontig_put(const void *origin_addr,
                                          int origin_count, MPI_Datatype origin_datatype,
                                          int target_rank, size_t size,
                                          MPI_Aint target_disp, MPI_Aint true_lb,
                                          MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPI_Aint segment_first, last;
    size_t base, offset;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    struct MPIR_Segment *segment_ptr;
    char *buffer = NULL;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    segment_ptr = MPIR_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                         MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIR_Segment_alloc");
    MPIR_Segment_init(origin_addr, origin_count, origin_datatype, segment_ptr);
    segment_first = 0;
    last = size;

    buffer = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(buffer);
    MPIR_Segment_pack(segment_ptr, segment_first, &last, buffer);
    MPIR_Segment_free(segment_ptr);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;
    /* We use the blocking put here - should be faster than send/recv - ucp_put returns when it is
     * locally completed. In reality this means, when the data are copied to the internal UCP-buffer */
    status = ucp_put(ep, buffer, size, base + offset, win_info->rkey);
    MPIDI_UCX_CHK_STATUS(status);

    /* Only need remote flush */
    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH;

  fn_exit:
    MPL_free(buffer);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_UCX_contig_get(void *origin_addr,
                                       size_t size,
                                       int target_rank,
                                       MPI_Aint target_disp, MPI_Aint true_lb,
                                       MPIR_Win * win, MPIDI_av_entry_t * addr)
{

    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    size_t base, offset;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;

    status = ucp_get_nbi(ep, origin_addr, size, base + offset, win_info->rkey);
    MPIDI_UCX_CHK_STATUS(status);

    /* UCX 1.4 spec: ucp_get_nbi always returns immediately and does not
     * guarantee completion */
    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;



}

static inline int MPIDI_NM_mpi_put(const void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_PUT);
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
#else

    int target_contig, origin_contig;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb;
    size_t offset;
    if (!MPIDI_UCX_is_reachable_target(target_rank, win))
        return MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count, target_datatype, win);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count,
                                        target_contig, target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count,
                                        origin_contig, origin_bytes, origin_true_lb);

    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

    if (unlikely(origin_bytes == 0))
        goto fn_exit;
    if (!target_contig)
        return MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count, target_datatype, win);

    if (!origin_contig)
        return MPIDI_UCX_noncontig_put(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_bytes, target_disp, target_true_lb, win, addr);

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        return MPIR_Localcopy(origin_addr,
                              origin_count,
                              origin_datatype,
                              (char *) win->base + offset, target_count, target_datatype);
    }


    mpi_errno = MPIDI_UCX_contig_put((char *) origin_addr + origin_true_lb, origin_bytes,
                                     target_rank, target_disp, target_true_lb, win, addr);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_get(void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
#else
    int origin_contig, target_contig;

    size_t origin_bytes, target_bytes;
    size_t offset;

    MPI_Aint origin_true_lb, target_true_lb;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_GET);

    if (!MPIDI_UCX_is_reachable_target(target_rank, win))
        return MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count, target_datatype, win);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count,
                                        target_contig, target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count,
                                        origin_contig, origin_bytes, origin_true_lb);

    if (unlikely(origin_bytes == 0))
        goto fn_exit;


    if (!origin_contig || !target_contig)
        return MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count, target_datatype, win);

    if (target_rank == win->comm_ptr->rank) {
        offset = target_disp * win->disp_unit;
        return MPIR_Localcopy((char *) win->base + offset,
                              target_count,
                              target_datatype, origin_addr, origin_count, origin_datatype);
    }


    return MPIDI_UCX_contig_get((char *) origin_addr + origin_true_lb, origin_bytes,
                                target_rank, target_disp, target_true_lb, win, addr);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_rput(const void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rput(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win,
                               request);
}


static inline int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                const void *compare_addr,
                                                void *result_addr,
                                                MPI_Datatype datatype,
                                                int target_rank, MPI_Aint target_disp,
                                                MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                           datatype, target_rank, target_disp, win);
}

static inline int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                           int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank,
                                           MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype,
                                           MPI_Op op, MPIR_Win * win,
                                           MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, op, win, request);
}

static inline int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               void *result_addr,
                                               int result_count,
                                               MPI_Datatype result_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPI_Op op, MPIR_Win * win,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win, request);
}

static inline int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                            void *result_addr,
                                            MPI_Datatype datatype,
                                            int target_rank,
                                            MPI_Aint target_disp, MPI_Op op,
                                            MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                       target_rank, target_disp, op, win);
}


static inline int MPIDI_NM_mpi_rget(void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rget(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win,
                               request);
}


static inline int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              void *result_addr,
                                              int result_count,
                                              MPI_Datatype result_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype, MPI_Op op,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win);
}

static inline int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                          int origin_count,
                                          MPI_Datatype origin_datatype,
                                          int target_rank,
                                          MPI_Aint target_disp,
                                          int target_count,
                                          MPI_Datatype target_datatype, MPI_Op op,
                                          MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win);
}

#endif /* UCX_RMA_H_INCLUDED */

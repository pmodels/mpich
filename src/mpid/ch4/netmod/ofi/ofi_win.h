/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_WIN_H_INCLUDED
#define OFI_WIN_H_INCLUDED

#include "ofi_impl.h"
#include <opa_primitives.h>


#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_allgather(MPIR_Win * win, void *base, int disp_unit)
{
    int i, same_disp, mpi_errno = MPI_SUCCESS;
    uint32_t first;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm_ptr = win->comm_ptr;
    int raw_prefix, idx, bitpos;
    unsigned gen_id;
    MPIDI_OFI_win_targetinfo_t *winfo;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);

    /* Calculate a canonical context id */
    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, comm_ptr->context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;
    gen_id = (idx * MPIR_CONTEXT_INT_BITS) + (31 - bitpos);

    int total_bits_avail = MPIDI_Global.max_mr_key_size * 8;
    uint64_t window_instance = (uint64_t) (MPIDI_OFI_WIN(win).win_id) >> 32;
    int bits_for_instance_id = MPIDI_Global.max_windows_bits;
    int bits_for_context_id;
    uint64_t max_contexts_allowed;
    uint64_t max_instances_allowed;

    bits_for_context_id = total_bits_avail -
        MPIDI_Global.max_windows_bits - MPIDI_Global.max_huge_rma_bits;
    max_contexts_allowed = (uint64_t) 1 << (bits_for_context_id);
    max_instances_allowed = (uint64_t) 1 << (bits_for_instance_id);
    MPIR_ERR_CHKANDSTMT(gen_id >= max_contexts_allowed, mpi_errno, MPI_ERR_OTHER,
                        goto fn_fail, "**ofid_mr_reg");
    MPIR_ERR_CHKANDSTMT(window_instance >= max_instances_allowed, mpi_errno, MPI_ERR_OTHER,
                        goto fn_fail, "**ofid_mr_reg");

    if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
        /* Context id in lower bits, instance in upper bits */
        MPIDI_OFI_WIN(win).mr_key = (gen_id << MPIDI_Global.context_shift) | window_instance;
    }
    else {
        MPIDI_OFI_WIN(win).mr_key = 0;
    }

    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_Global.domain,       /* In:  Domain Object       */
                             base,      /* In:  Lower memory address */
                             win->size, /* In:  Length              */
                             FI_REMOTE_READ | FI_REMOTE_WRITE,  /* In:  Expose MR for read  */
                             0ULL,      /* In:  offset(not used)    */
                             MPIDI_OFI_WIN(win).mr_key, /* In:  requested key       */
                             0ULL,      /* In:  flags               */
                             &MPIDI_OFI_WIN(win).mr,    /* Out: memregion object    */
                             NULL), mr_reg);    /* In:  context             */

    MPIDI_OFI_WIN(win).winfo = MPL_malloc(sizeof(*winfo) * comm_ptr->local_size, MPL_MEM_RMA);

    winfo = MPIDI_OFI_WIN(win).winfo;
    winfo[comm_ptr->rank].disp_unit = disp_unit;

    if (!MPIDI_OFI_ENABLE_MR_SCALABLE) {
        /* MR_BASIC */
        MPIDI_OFI_WIN(win).mr_key = fi_mr_key(MPIDI_OFI_WIN(win).mr);
        winfo[comm_ptr->rank].mr_key = MPIDI_OFI_WIN(win).mr_key;
        winfo[comm_ptr->rank].base = (uintptr_t) base;
    }

    mpi_errno = MPID_Allgather(MPI_IN_PLACE, 0,
                               MPI_DATATYPE_NULL,
                               winfo, sizeof(*winfo), MPI_BYTE, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
        first = winfo[0].disp_unit;
        same_disp = 1;
        for (i = 1; i < comm_ptr->local_size; i++) {
            if (winfo[i].disp_unit != first) {
                same_disp = 0;
                break;
            }
        }
        if (same_disp) {
            MPL_free(MPIDI_OFI_WIN(win).winfo);
            MPIDI_OFI_WIN(win).winfo = NULL;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_init(MPI_Aint length,
                                     int disp_unit,
                                     MPIR_Win ** win_ptr,
                                     MPIR_Info * info,
                                     MPIR_Comm * comm_ptr,
                                     int create_flavor,
                                     int model)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t window_instance;
    MPIR_Win *win;
    struct fi_info *finfo;
    struct fi_cntr_attr cntr_attr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT);

    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devwin_t) >= sizeof(MPIDI_OFI_win_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devdt_t) >= sizeof(MPIDI_OFI_datatype_t));

    /* Note: MPIDI_CH4U_win_init will interpret the info object */
    mpi_errno = MPIDI_CH4R_win_init(length, disp_unit, &win, info, comm_ptr, create_flavor, model);
    MPIR_ERR_CHKANDSTMT(mpi_errno != MPI_SUCCESS,
                        mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

    memset(&MPIDI_OFI_WIN(win), 0, sizeof(MPIDI_OFI_win_t));

    /* context id lower bits, window instance upper bits */
    window_instance =
        MPIDI_OFI_index_allocator_alloc(MPIDI_OFI_COMM(win->comm_ptr).win_id_allocator, MPL_MEM_RMA);
    MPIDI_OFI_WIN(win).win_id = ((uint64_t) comm_ptr->context_id) | (window_instance << 32);
    MPIDI_CH4U_map_set(MPIDI_Global.win_map, MPIDI_OFI_WIN(win).win_id, win, MPL_MEM_RMA);

    if (MPIDI_OFI_ENABLE_STX_RMA && MPIDI_Global.stx_ctx != NULL) {
        /* Activate per-window EP/counter */
        int ret;

        finfo = fi_dupinfo(MPIDI_Global.prov_use);
        MPIR_Assert(finfo);
        finfo->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
        finfo->tx_attr->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
        finfo->rx_attr->caps = 0ULL; /* RX capabilities not needed */

        finfo->ep_attr->tx_ctx_cnt = FI_SHARED_CONTEXT; /* Request a shared context */
        MPIDI_OFI_CALL_RETURN(fi_endpoint(MPIDI_Global.domain,
                                          finfo, &MPIDI_OFI_WIN(win).ep, NULL), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to create per-window EP (with completion), "
                        "falling back to global EP/counter scheme");
            fi_freeinfo(finfo);
            goto fallback_global;
        }

        memset(&cntr_attr, 0, sizeof(cntr_attr));
        cntr_attr.events = FI_CNTR_EVENTS_COMP;
        MPIDI_OFI_CALL(fi_cntr_open(MPIDI_Global.domain,        /* In:  Domain Object        */
                                    &cntr_attr, /* In:  Configuration object */
                                    &MPIDI_OFI_WIN(win).cmpl_cntr,      /* Out: Counter Object       */
                                    NULL), openct);     /* Context: counter events   */
        MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_OFI_WIN(win).issued_cntr_v;

        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_WIN(win).ep, &MPIDI_Global.stx_ctx->fid, 0), bind);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                  &MPIDI_Global.ctx[0].cq->fid, FI_TRANSMIT | FI_SELECTIVE_COMPLETION),
                       bind);
        MPIDI_OFI_CALL(fi_ep_bind
                       (MPIDI_OFI_WIN(win).ep, &MPIDI_OFI_WIN(win).cmpl_cntr->fid,
                        FI_READ | FI_WRITE), bind);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_WIN(win).ep, &MPIDI_Global.av->fid, 0), bind);

        fi_freeinfo(finfo);

        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_WIN(win).ep), ep_enable);
    }
    else {
      fallback_global:

        /* Fallback for the traditional global EP/counter model */
        MPIDI_OFI_WIN(win).ep = MPIDI_Global.ctx[0].tx;
        MPIDI_OFI_WIN(win).cmpl_cntr = MPIDI_Global.rma_cmpl_cntr;
        MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_Global.rma_issued_cntr;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_progress_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_progress_fence(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int itercount = 0;
    int ret;
    uint64_t tcount, donecount;
    MPIDI_OFI_win_request_t *r;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    tcount = *MPIDI_OFI_WIN(win).issued_cntr;
    donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);

    MPIR_Assert(donecount <= tcount);

    while (tcount > donecount) {
        MPIR_Assert(donecount <= tcount);
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
        MPIDI_OFI_PROGRESS();
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
        donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);
        itercount++;

        if (itercount == 1000) {
            ret = fi_cntr_wait(MPIDI_OFI_WIN(win).cmpl_cntr, tcount, 0);
            MPIDI_OFI_ERR(ret < 0 && ret != -FI_ETIMEDOUT,
                          mpi_errno,
                          MPI_ERR_RMA_RANGE,
                          "**ofid_cntr_wait",
                          "**ofid_cntr_wait %s %d %s %s",
                          __SHORT_FILE__, __LINE__, FCNAME, fi_strerror(-ret));
            itercount = 0;
        }
    }

    r = MPIDI_OFI_WIN(win).syncQ;

    while (r) {
        MPIDI_OFI_win_request_t *next = r->next;
        MPIDI_OFI_rma_done_event(NULL, (MPIR_Request *) r);
        r = next;
    }

    MPIDI_OFI_WIN(win).syncQ = NULL;
  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);

    mpi_errno = MPIDI_CH4R_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_START);

    mpi_errno = MPIDI_CH4R_mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_complete(win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_POST);

    mpi_errno = MPIDI_CH4R_mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);

    mpi_errno = MPIDI_CH4R_mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);

    mpi_errno = MPIDI_CH4R_mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win,
                                        MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);

    mpi_errno = MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_unlock(rank, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);

    mpi_errno = MPIDI_CH4R_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = *win_ptr;
    uint32_t window_instance;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4R_mpi_win_free(win_ptr);
        goto fn_exit;
    }

    MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);
    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);

    mpi_errno = MPID_Barrier(win->comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    window_instance = (uint32_t) (MPIDI_OFI_WIN(win).win_id >> 32);

    MPIDI_OFI_index_allocator_free(MPIDI_OFI_COMM(win->comm_ptr).win_id_allocator, window_instance);
    MPIDI_CH4U_map_erase(MPIDI_Global.win_map, MPIDI_OFI_WIN(win).win_id);
    if (MPIDI_OFI_WIN(win).ep != MPIDI_Global.ctx[0].tx)
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
    if (MPIDI_OFI_WIN(win).cmpl_cntr != MPIDI_Global.rma_cmpl_cntr)
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cqclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).mr->fid), mr_unreg);
    if (MPIDI_OFI_WIN(win).winfo) {
        MPL_free(MPIDI_OFI_WIN(win).winfo);
        MPIDI_OFI_WIN(win).winfo = NULL;
    }

    MPIDI_CH4R_win_finalize(win_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_fence(massert, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4R_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_win_init(length,
                                   disp_unit,
                                   win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_CREATE, MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = base;

    mpi_errno = MPIDI_OFI_win_allgather(win, base, disp_unit);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPID_Barrier(win->comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);

    mpi_errno = MPIDI_CH4R_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size,
                                                   int disp_unit,
                                                   MPIR_Info * info_ptr,
                                                   MPIR_Comm * comm_ptr,
                                                   void **base_ptr, MPIR_Win ** win_ptr)
{
    int i = 0, fd = -1, rc, first = 0, mpi_errno = MPI_SUCCESS, shm_key_size;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    void *baseP = NULL;
    MPIR_Win *win = NULL;
    ssize_t total_size = 0LL;
    MPI_Aint size_out = 0;
    char *shm_key = NULL;
    void *map_ptr;
    MPIDI_CH4U_win_shared_info_t *shared_table = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4R_mpi_win_allocate_shared(size, disp_unit, info_ptr,
                                                       comm_ptr, base_ptr, win_ptr);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_win_init(size, disp_unit, win_ptr, info_ptr, comm_ptr,
                                   MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    win = *win_ptr;
    MPIDI_CH4U_WIN(win, shared_table) =
        (MPIDI_CH4U_win_shared_info_t *) MPL_malloc(sizeof(MPIDI_CH4U_win_shared_info_t) *
                                                    comm_ptr->local_size, MPL_MEM_RMA);
    shared_table = MPIDI_CH4U_WIN(win, shared_table);

    shared_table[comm_ptr->rank].size = size;
    shared_table[comm_ptr->rank].disp_unit = disp_unit;

    mpi_errno = MPID_Allgather(MPI_IN_PLACE,
                               0,
                               MPI_DATATYPE_NULL,
                               shared_table,
                               sizeof(MPIDI_CH4U_win_shared_info_t),
                               MPI_BYTE, comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* No allreduce here because this is a shared memory domain
     * and should be a relatively small number of processes
     * and a non performance sensitive API.
     */
    for (i = 0; i < comm_ptr->local_size; i++)
        total_size += shared_table[i].size;

    if (total_size == 0)
        goto fn_zero;

    /* Note that two distinct process groups may share the same context id,
     * thus the window id may be duplicated on a node if windows are owned
     * by distinct process groups. Therefore, we use [jobid + root_rank + win_id]
     * as unique shm_key for window, where root_rank is the world rank of the
     * first process in the group. */
    int root_rank = 0;

    if (comm_ptr->rank == 0)
        root_rank = MPIR_Process.comm_world->rank;
    mpi_errno = MPID_Bcast(&root_rank, 1, MPI_INT, 0, comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    shm_key = (char *) MPL_malloc(sizeof(char), MPL_MEM_SHM);
    shm_key_size = snprintf(shm_key, 1, "/mpi-%s-%X-%" PRIx64,
                            MPIDI_CH4_Global.jobid, root_rank, MPIDI_OFI_WIN(win).win_id);
    shm_key = (char *) MPL_realloc(shm_key, shm_key_size, MPL_MEM_SHM);
    MPIR_Assert(shm_key);
    snprintf(shm_key, shm_key_size, "/mpi-%s-%X-%" PRIx64,
             MPIDI_CH4_Global.jobid, root_rank, MPIDI_OFI_WIN(win).win_id);

    rc = shm_open(shm_key, O_CREAT | O_EXCL | O_RDWR, 0600);
    first = (rc != -1);

    if (!first) {
        rc = shm_open(shm_key, O_RDWR, 0);

        if (rc == -1) {
            shm_unlink(shm_key);
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
        }
    }

    /* Make the addresses symmetric by using MAP_FIXED */
    size_t page_sz, mapsize;

    mapsize = MPIDI_CH4R_get_mapsize(total_size, &page_sz);
    fd = rc;
    rc = ftruncate(fd, mapsize);

    if (rc == -1) {
        close(fd);

        if (first)
            shm_unlink(shm_key);

        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    }

    int iter = MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY;
    unsigned anyfail = 1;

    while (anyfail && --iter > 0) {
        if (comm_ptr->rank == 0) {
            int map_flags = MAP_SHARED;

            map_ptr = MPIDI_CH4R_generate_random_addr(mapsize);
#ifdef USE_SYM_HEAP
            if (MPIDI_CH4R_is_valid_mapaddr(map_ptr))
                map_flags |= MAP_FIXED; /* Set fixed only when generated a valid address.
                                         * Otherwise we allow system to pick up one. */
#endif
            map_ptr = mmap(map_ptr, mapsize, PROT_READ | PROT_WRITE, map_flags, fd, 0);

            if (map_ptr == NULL || map_ptr == MAP_FAILED) {
                close(fd);

                if (first)
                    shm_unlink(shm_key);

                MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
            }
        }

        mpi_errno = MPID_Bcast(&map_ptr, 1, MPI_UNSIGNED_LONG, 0, comm_ptr, &errflag);

        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        unsigned map_fail = 0;
        if (comm_ptr->rank != 0) {
            int map_flags = MAP_SHARED;

#ifdef USE_SYM_HEAP
            rc = MPIDI_CH4R_check_maprange_ok(map_ptr, mapsize);
            map_fail = (rc == 1) ? 0 : 1;
            map_flags |= MAP_FIXED;     /* Set fixed only when symmetric heap is enabled. */
#endif

            if (map_fail == 0) {
                map_ptr = mmap(map_ptr, mapsize, PROT_READ | PROT_WRITE, map_flags, fd, 0);
                if (map_ptr == NULL || map_ptr == MAP_FAILED)
                    map_fail = 1;
            }
        }

        /* If any local process fails to sync range or mmap, then try more
         * addresses on rank 0. */
        mpi_errno = MPID_Allreduce(&map_fail,
                                   &anyfail, 1, MPI_UNSIGNED, MPI_BOR, comm_ptr, &errflag);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        if (anyfail && map_ptr != NULL && map_ptr != MAP_FAILED)
            MPL_munmap(map_ptr, mapsize, MPL_MEM_RMA);
    }

    if (anyfail) {      /* Still fails after retry, report error. */
        close(fd);

        if (first)
            shm_unlink(shm_key);

        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    }

    MPIDI_CH4U_WIN(win, mmap_addr) = map_ptr;
    MPIDI_CH4U_WIN(win, mmap_sz) = mapsize;

    /* Scan for my offset into the buffer             */
    /* Could use exscan if this is expensive at scale */
    for (i = 0; i < comm_ptr->rank; i++)
        size_out += shared_table[i].size;

  fn_zero:

    baseP = (total_size == 0) ? NULL : (void *) ((char *) map_ptr + size_out);
    win->base = baseP;
    win->size = size;
    mpi_errno = MPIDI_OFI_win_allgather(win, baseP, disp_unit);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    *(void **) base_ptr = (void *) win->base;
    mpi_errno = MPID_Barrier(comm_ptr, &errflag);

    if (fd >= 0)
        close(fd);

    if (first)
        shm_unlink(shm_key);

  fn_exit:
    if (shm_key != NULL)
        MPL_free(shm_key);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);

    mpi_errno = MPIDI_CH4R_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);

    mpi_errno = MPIDI_CH4R_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_allocate(MPI_Aint size,
                                            int disp_unit,
                                            MPIR_Info * info,
                                            MPIR_Comm * comm, void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    void *baseP;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4R_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win_ptr);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_win_init(size, disp_unit, win_ptr, info, comm,
                                   MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDI_CH4R_get_symmetric_heap(size, comm, &baseP, *win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = baseP;
    mpi_errno = MPIDI_OFI_win_allgather(win, baseP, disp_unit);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    *(void **) baseptr = (void *) win->base;
    mpi_errno = MPID_Barrier(comm, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_flush(rank, win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_flush_local_all(win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_unlock_all(win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info,
                                                  MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win_ptr);
        goto fn_exit;
    }

    rc = MPIDI_OFI_win_init((uintptr_t) UINTPTR_MAX - (uintptr_t) MPI_BOTTOM,
                            1, win_ptr, info, comm, MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED);

    if (rc != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = MPI_BOTTOM;

    rc = MPIDI_OFI_win_allgather(win, win->base, 1);

    if (rc != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPID_Barrier(comm, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    mpi_errno = MPIDI_CH4R_mpi_win_flush_local(rank, win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);

    mpi_errno = MPIDI_CH4R_mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);

    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);

    /* AM completion */
    MPIDI_OFI_MPI_CALL_POP(MPIDI_CH4R_mpi_win_flush_all(win));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);

    mpi_errno = MPIDI_CH4R_mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    return mpi_errno;
}


#endif /* OFI_WIN_H_INCLUDED */

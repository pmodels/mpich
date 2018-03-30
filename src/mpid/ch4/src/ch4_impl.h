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
#ifndef CH4_IMPL_H_INCLUDED
#define CH4_IMPL_H_INCLUDED

#include "ch4_types.h"
#include <mpidch4.h>
#include "mpidig.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Progress_test(int flags);

/* Static inlines */
static inline int MPIDI_CH4U_get_context_index(uint64_t context_id)
{
    int raw_prefix, idx, bitpos, gen_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_GET_CONTEXT_INDEX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_GET_CONTEXT_INDEX);

    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;
    gen_id = (idx * MPIR_CONTEXT_INT_BITS) + (31 - bitpos);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_GET_CONTEXT_INDEX);
    return gen_id;
}

static inline int MPIDI_CH4U_request_get_context_offset(MPIR_Request * req)
{
    int context_offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_REQUEST_GET_CONTEXT_OFFSET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_REQUEST_GET_CONTEXT_OFFSET);

    context_offset = MPIDI_CH4U_REQUEST(req, context_id) - req->comm->context_id;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_REQUEST_GET_CONTEXT_OFFSET);

    return context_offset;
}

static inline MPIR_Comm *MPIDI_CH4U_context_id_to_comm(uint64_t context_id)
{
    int comm_idx = MPIDI_CH4U_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIR_Comm *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_COMM);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);
    ret = MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_COMM);
    return ret;
}

static inline MPIDI_CH4U_rreq_t **MPIDI_CH4U_context_id_to_uelist(uint64_t context_id)
{
    int comm_idx = MPIDI_CH4U_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIDI_CH4U_rreq_t **ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_UELIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_UELIST);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);

    ret = &MPIDI_CH4_Global.comm_req_lists[comm_idx].uelist[is_localcomm][subcomm_type];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_CONTEXT_ID_TO_UELIST);
    return ret;
}

static inline uint64_t MPIDI_CH4U_generate_win_id(MPIR_Comm * comm_ptr)
{
    uint64_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_GENERATE_WIN_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_GENERATE_WIN_ID);

    /* context id lower bits, window instance upper bits */
    ret = 1 + (((uint64_t) comm_ptr->context_id) |
               ((uint64_t) ((MPIDI_CH4U_COMM(comm_ptr, window_instance))++) << 32));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_GENERATE_WIN_ID);
    return ret;
}

static inline MPIR_Context_id_t MPIDI_CH4U_win_id_to_context(uint64_t win_id)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_ID_TO_CONTEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_ID_TO_CONTEXT);

    /* pick the lower 32-bit to extract context id */
    ret = (win_id - 1) & 0xffffffff;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_ID_TO_CONTEXT);
    return ret;
}

static inline MPIR_Context_id_t MPIDI_CH4U_win_to_context(const MPIR_Win * win)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_TO_CONTEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_TO_CONTEXT);

    ret = MPIDI_CH4U_win_id_to_context(MPIDI_CH4U_WIN(win, win_id));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_TO_CONTEXT);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_request_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_request_complete(MPIR_Request * req)
{
    int incomplete;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_REQUEST_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_REQUEST_COMPLETE);

    MPIR_cc_decr(req->cc_ptr, &incomplete);
    if (!incomplete)
        MPIR_Request_free(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_REQUEST_COMPLETE);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_target_add
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDI_CH4U_win_target_t *MPIDI_CH4U_win_target_add(MPIR_Win * win,
                                                                            int rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_TARGET_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_TARGET_ADD);

    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    target_ptr =
        (MPIDI_CH4U_win_target_t *) MPL_malloc(sizeof(MPIDI_CH4U_win_target_t), MPL_MEM_RMA);
    target_ptr->rank = rank;
    MPIR_cc_set(&target_ptr->local_cmpl_cnts, 0);
    MPIR_cc_set(&target_ptr->remote_cmpl_cnts, 0);
    MPIR_cc_set(&target_ptr->remote_acc_cmpl_cnts, 0);
    target_ptr->sync.lock.locked = 0;
    target_ptr->sync.access_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    target_ptr->sync.assert_mode = 0;

    HASH_ADD(hash_handle, MPIDI_CH4U_WIN(win, targets), rank, sizeof(int), target_ptr, MPL_MEM_RMA);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_TARGET_ADD);
    return target_ptr;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_target_find
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDI_CH4U_win_target_t *MPIDI_CH4U_win_target_find(MPIR_Win * win,
                                                                             int rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_TARGET_FIND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_TARGET_FIND);

    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    HASH_FIND(hash_handle, MPIDI_CH4U_WIN(win, targets), &rank, sizeof(int), target_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_TARGET_FIND);
    return target_ptr;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_target_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDI_CH4U_win_target_t *MPIDI_CH4U_win_target_get(MPIR_Win * win,
                                                                            int rank)
{
    MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, rank);
    if (!target_ptr)
        target_ptr = MPIDI_CH4U_win_target_add(win, rank);
    return target_ptr;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_target_delete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_win_target_delete(MPIR_Win * win,
                                                           MPIDI_CH4U_win_target_t * target_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_TARGET_DELETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_TARGET_DELETE);

    HASH_DELETE(hash_handle, MPIDI_CH4U_WIN(win, targets), target_ptr);
    MPL_free(target_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_TARGET_DELETE);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_target_cleanall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_win_target_cleanall(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_TARGET_CLEANALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_TARGET_CLEANALL);

    MPIDI_CH4U_win_target_t *target_ptr, *tmp;
    HASH_ITER(hash_handle, MPIDI_CH4U_WIN(win, targets), target_ptr, tmp) {
        HASH_DELETE(hash_handle, MPIDI_CH4U_WIN(win, targets), target_ptr);
        MPL_free(target_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_TARGET_CLEANALL);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_hash_clear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_win_hash_clear(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_WIN_HASH_CLEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_WIN_HASH_CLEAR);

    HASH_CLEAR(hash_handle, MPIDI_CH4U_WIN(win, targets));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_WIN_HASH_CLEAR);
}

#define MPIDI_Datatype_get_info(_count, _datatype,              \
                                _dt_contig_out, _data_sz_out,   \
                                _dt_ptr, _dt_true_lb)           \
    do {                                                        \
        if (IS_BUILTIN(_datatype))                              \
        {                                                       \
            (_dt_ptr)        = NULL;                            \
            (_dt_contig_out) = TRUE;                            \
            (_dt_true_lb)    = 0;                               \
            (_data_sz_out)   = (size_t)(_count) *               \
                MPIR_Datatype_get_basic_size(_datatype);        \
        }                                                       \
        else                                                    \
        {                                                       \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));      \
            if (_dt_ptr)                                        \
            {                                                   \
                (_dt_contig_out) = (_dt_ptr)->is_contig;        \
                (_dt_true_lb)    = (_dt_ptr)->true_lb;          \
                (_data_sz_out)   = (size_t)(_count) *           \
                    (_dt_ptr)->size;                            \
            }                                                   \
            else                                                \
            {                                                   \
                (_dt_contig_out) = 1;                           \
                (_dt_true_lb)    = 0;                           \
                (_data_sz_out)   = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_get_size_dt_ptr(_count, _datatype,       \
                                       _data_sz_out, _dt_ptr)   \
    do {                                                        \
        if (IS_BUILTIN(_datatype))                              \
        {                                                       \
            (_dt_ptr)        = NULL;                            \
            (_data_sz_out)   = (size_t)(_count) *               \
                MPIR_Datatype_get_basic_size(_datatype);        \
        }                                                       \
        else                                                    \
        {                                                       \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));      \
            (_data_sz_out)   = (_dt_ptr) ? (size_t)(_count) *   \
                (_dt_ptr)->size : 0;                            \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_check_contig(_datatype,_dt_contig_out)           \
    do {                                                                \
        if (IS_BUILTIN(_datatype))                                      \
        {                                                               \
            (_dt_contig_out) = TRUE;                                    \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIR_Datatype *_dt_ptr;                                     \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));              \
            (_dt_contig_out) = (_dt_ptr) ? (_dt_ptr)->is_contig : 1;    \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_contig_size(_datatype,_count,      \
                                         _dt_contig_out,        \
                                         _data_sz_out)          \
    do {                                                        \
        if (IS_BUILTIN(_datatype))                              \
        {                                                       \
            (_dt_contig_out) = TRUE;                            \
            (_data_sz_out)   = (size_t)(_count) *               \
                MPIR_Datatype_get_basic_size(_datatype);        \
        }                                                       \
        else                                                    \
        {                                                       \
            MPIR_Datatype *_dt_ptr;                             \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));      \
            if (_dt_ptr)                                        \
            {                                                   \
                (_dt_contig_out) = (_dt_ptr)->is_contig;        \
                (_data_sz_out)   = (size_t)(_count) *           \
                    (_dt_ptr)->size;                            \
            }                                                   \
            else                                                \
            {                                                   \
                (_dt_contig_out) = 1;                           \
                (_data_sz_out)   = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_check_size(_datatype,_count,_data_sz_out)        \
    do {                                                                \
        if (IS_BUILTIN(_datatype))                                      \
        {                                                               \
            (_data_sz_out)   = (size_t)(_count) *                       \
                MPIR_Datatype_get_basic_size(_datatype);                \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIR_Datatype *_dt_ptr;                                     \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));              \
            (_data_sz_out)   = (_dt_ptr) ? (size_t)(_count) *           \
                (_dt_ptr)->size : 0;                                    \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_size_lb(_datatype,_count,_data_sz_out,     \
                                     _dt_true_lb)                       \
    do {                                                                \
        if (IS_BUILTIN(_datatype)) {                                    \
            (_data_sz_out)   = (size_t)(_count) *                       \
                MPIR_Datatype_get_basic_size(_datatype);                \
            (_dt_true_lb)    = 0;                                       \
        } else {                                                        \
            MPIR_Datatype *_dt_ptr;                                     \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));              \
            (_data_sz_out)   = (_dt_ptr) ? (size_t)(_count) *           \
                (_dt_ptr)->size : 0;                                    \
            (_dt_true_lb)    = (_dt_ptr) ? (_dt_ptr)->true_lb : 0;      \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_contig_size_lb(_datatype,_count,   \
                                            _dt_contig_out,     \
                                            _data_sz_out,       \
                                            _dt_true_lb)        \
    do {                                                        \
        if (IS_BUILTIN(_datatype))                              \
        {                                                       \
            (_dt_contig_out) = TRUE;                            \
            (_data_sz_out)   = (size_t)(_count) *               \
                MPIR_Datatype_get_basic_size(_datatype);        \
            (_dt_true_lb)    = 0;                               \
        }                                                       \
        else                                                    \
        {                                                       \
            MPIR_Datatype *_dt_ptr;                             \
            MPIR_Datatype_get_ptr((_datatype), (_dt_ptr));      \
            if (_dt_ptr)                                        \
            {                                                   \
                (_dt_contig_out) = (_dt_ptr)->is_contig;        \
                (_data_sz_out)   = (size_t)(_count) *           \
                    (_dt_ptr)->size;                            \
                (_dt_true_lb)    = (_dt_ptr)->true_lb;          \
            }                                                   \
            else                                                \
            {                                                   \
                (_dt_contig_out) = 1;                           \
                (_data_sz_out)   = 0;                           \
                (_dt_true_lb)    = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

#define MPIDI_Request_create_null_rreq(rreq_, mpi_errno_, FAIL_)        \
    do {                                                                \
        (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);         \
        if ((rreq_) != NULL) {                                          \
            MPIR_cc_set(&(rreq_)->cc, 0);                               \
            MPIR_Status_set_procnull(&(rreq_)->status);                 \
        }                                                               \
        else {                                                          \
            MPIR_ERR_SETANDJUMP(mpi_errno_,MPIX_ERR_NOREQ,"**nomemreq"); \
        }                                                               \
    } while (0)

#define IS_BUILTIN(_datatype)                           \
    (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_valid_group_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_valid_group_rank(MPIR_Comm * comm, int rank, MPIR_Group * grp)
{
    int lpid;
    int size = grp->size;
    int z;
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_VALID_GROUP_RANK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_VALID_GROUP_RANK);

    if (unlikely(rank == MPI_PROC_NULL)) {
        /* Treat PROC_NULL as always valid */
        ret = 1;
        goto fn_exit;
    }

    MPIDI_NM_comm_get_lpid(comm, rank, &lpid, FALSE);

    for (z = 0; z < size && lpid != grp->lrank_to_lpid[z].lpid; ++z) {
    }

    ret = (z < size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_VALID_GROUP_RANK);
    return ret;
}

#define MPIDI_CH4R_PROGRESS()                                   \
    do {                                                        \
        mpi_errno = MPID_Progress_test();                       \
        if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);  \
    } while (0)

#define MPIDI_CH4R_PROGRESS_WHILE(cond)         \
    do {                                        \
        while (cond)                            \
            MPIDI_CH4R_PROGRESS();              \
    } while (0)

#ifdef HAVE_ERROR_CHECKING
#define MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_NONE || \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_POST) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* Checking per-target sync status for pscw or lock epoch. */
#define MPIDI_CH4U_EPOCH_CHECK_TARGET_SYNC(win,target_rank,mpi_errno,stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, target_rank); \
        if ((MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_START && \
             !MPIDI_CH4I_valid_group_rank(win->comm_ptr, target_rank,   \
                                          MPIDI_CH4U_WIN(win, sync).sc.group)) || \
            (target_ptr != NULL &&                                      \
             MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK && \
             target_ptr->sync.access_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK)) \
            MPIR_ERR_SETANDSTMT(mpi_errno,                              \
                                MPI_ERR_RMA_SYNC,                       \
                                stmt,                                   \
                                "**rmasync");                           \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win,mpi_errno,stmt)              \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if ((MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK) && \
            (MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK_ALL)) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* NOTE: unlock/flush/flush_local needs to check per-target passive epoch (lock) */
#define MPIDI_CH4U_EPOCH_CHECK_TARGET_LOCK(target_ptr,mpi_errno,stmt)   \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (target_ptr->sync.access_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win,mpi_errno,stmt)          \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win,mpi_errno,stmt)        \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
            MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* NOTE: multiple lock access epochs can occur simultaneously, as long as
 * target to different processes */
#define MPIDI_CH4U_LOCK_EPOCH_CHECK_NONE(win,rank,mpi_errno,stmt)       \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, rank); \
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE && \
            !(MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK && \
              (target_ptr == NULL || (!MPIR_CVAR_CH4_RMA_MEM_EFFICIENT && \
                                      target_ptr->sync.lock.locked == 0)))) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_FENCE_EPOCH_CHECK(win,mpi_errno,stmt)                \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if ((MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != MPIDI_CH4U_EPOTYPE_FENCE && \
             MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE && \
             MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != MPIDI_CH4U_EPOTYPE_NONE) || \
            (MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_FENCE && \
             MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE && \
             MPIDI_CH4U_WIN(win, sync).access_epoch_type != MPIDI_CH4U_EPOTYPE_NONE)) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_ACCESS_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type != epoch_type)  \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EXPOSURE_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).exposure_epoch_type != epoch_type) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#else /* HAVE_ERROR_CHECKING */
#define MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_CHECK_TARGET_SYNC(win, target_rank, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, stmt)            if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, stmt)  if (0) goto fn_fail;
#define MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, stmt)        if (0) goto fn_fail;
#define MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, stmt)           if (0) goto fn_fail;
#define MPIDI_CH4U_LOCK_EPOCH_CHECK_NONE(win,rank,mpi_errno,stmt)       if (0) goto fn_fail;
#define MPIDI_CH4U_FENCE_EPOCH_CHECK(win, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDI_CH4U_ACCESS_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) if (0) goto fn_fail;
#define MPIDI_CH4U_EXPOSURE_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt)    if (0) goto fn_fail;
#endif /* HAVE_ERROR_CHECKING */

#define MPIDI_CH4U_EPOCH_FENCE_EVENT(win, massert)                      \
    do {                                                                \
        if (massert & MPI_MODE_NOSUCCEED)                               \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
            MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
            MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
        }                                                               \
    } while (0)

#define MPIDI_CH4U_EPOCH_OP_REFENCE(win)                                \
    do {                                                                \
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_REFENCE && \
            MPIDI_CH4U_WIN(win, sync).exposure_epoch_type == MPIDI_CH4U_EPOTYPE_REFENCE) \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_FENCE; \
            MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_FENCE; \
        }                                                               \
    } while (0)

/*
  Calculate base address of the target window at the origin side
  Return zero to let the target side calculate the actual address
  (only offset from window base is given to the target in this case)
*/
static inline uintptr_t MPIDI_CH4I_win_base_at_origin(const MPIR_Win * win, int target_rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_ORIGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_ORIGIN);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_ORIGIN);

    /* TODO: In future we may want to calculate the full virtual address
     * in the target at the origin side. It can be done by looking at
     * MPIDI_CH4U_WINFO(win, target_rank)->base_addr */
    return 0;
}

/*
  Calculate base address of the window at the target side
  If MPIDI_CH4I_win_base_at_origin calculates the full virtual address
  this function must return zero
*/
static inline uintptr_t MPIDI_CH4I_win_base_at_target(const MPIR_Win * win)
{
    uintptr_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_TARGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_TARGET);

    ret = (uintptr_t) win->base;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_WIN_BASE_AT_TARGET);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_cmpl_cnts_incr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_win_cmpl_cnts_incr(MPIR_Win * win, int target_rank,
                                            MPIR_cc_t ** local_cmpl_cnts_ptr)
{
    int c = 0;

    /* Increase per-window counters for fence, and per-target counters for
     * all other synchronization. */
    switch (MPIDI_CH4U_WIN(win, sync).access_epoch_type) {
        case MPIDI_CH4U_EPOTYPE_LOCK:
            /* FIXME: now we simply set per-target counters for lockall in case
             * user flushes per target, but this should be optimized. */
        case MPIDI_CH4U_EPOTYPE_LOCK_ALL:
            /* FIXME: now we simply set per-target counters for PSCW, can it be optimized ? */
        case MPIDI_CH4U_EPOTYPE_START:
            {
                MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_get(win, target_rank);

                MPIR_cc_incr(&target_ptr->local_cmpl_cnts, &c);
                MPIR_cc_incr(&target_ptr->remote_cmpl_cnts, &c);

                *local_cmpl_cnts_ptr = &target_ptr->local_cmpl_cnts;
                break;
            }
        default:
            MPIR_cc_incr(&MPIDI_CH4U_WIN(win, local_cmpl_cnts), &c);
            MPIR_cc_incr(&MPIDI_CH4U_WIN(win, remote_cmpl_cnts), &c);

            *local_cmpl_cnts_ptr = &MPIDI_CH4U_WIN(win, local_cmpl_cnts);
            break;
    }
}

/* Increase counter for active message acc ops. */
#undef FUNCNAME
#define FUNCNAME MPIDI_win_remote_acc_cmpl_cnt_incr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_win_remote_acc_cmpl_cnt_incr(MPIR_Win * win, int target_rank)
{
    int c = 0;
    switch (MPIDI_CH4U_WIN(win, sync).access_epoch_type) {
        case MPIDI_CH4U_EPOTYPE_LOCK:
        case MPIDI_CH4U_EPOTYPE_LOCK_ALL:
        case MPIDI_CH4U_EPOTYPE_START:
            {
                MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_get(win, target_rank);
                MPIR_cc_incr(&target_ptr->remote_acc_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_incr(&MPIDI_CH4U_WIN(win, remote_acc_cmpl_cnts), &c);
            break;
    }
}

/* Decrease counter for active message acc ops. */
#undef FUNCNAME
#define FUNCNAME MPIDI_win_remote_acc_cmpl_cnt_decr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_win_remote_acc_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    int c = 0;
    switch (MPIDI_CH4U_WIN(win, sync).access_epoch_type) {
        case MPIDI_CH4U_EPOTYPE_LOCK:
        case MPIDI_CH4U_EPOTYPE_LOCK_ALL:
        case MPIDI_CH4U_EPOTYPE_START:
            {
                MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_decr(&target_ptr->remote_acc_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_decr(&MPIDI_CH4U_WIN(win, remote_acc_cmpl_cnts), &c);
            break;
    }

}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_remote_cmpl_cnt_decr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_win_remote_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    int c = 0;

    /* Decrease per-window counter for fence, and per-target counters for
     * all other synchronization. */
    switch (MPIDI_CH4U_WIN(win, sync).access_epoch_type) {
        case MPIDI_CH4U_EPOTYPE_LOCK:
        case MPIDI_CH4U_EPOTYPE_LOCK_ALL:
        case MPIDI_CH4U_EPOTYPE_START:
            {
                MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_decr(&target_ptr->remote_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_decr(&MPIDI_CH4U_WIN(win, remote_cmpl_cnts), &c);
            break;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_check_all_targets_remote_completed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_win_check_all_targets_remote_completed(MPIR_Win * win, int *allcompleted)
{
    int rank = 0;

    *allcompleted = 1;
    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDI_CH4U_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_check_all_targets_local_completed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_win_check_all_targets_local_completed(MPIR_Win * win, int *allcompleted)
{
    int rank = 0;

    *allcompleted = 1;
    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDI_CH4U_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_check_group_local_completed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_win_check_group_local_completed(MPIR_Win * win,
                                                         int *ranks_in_win_grp,
                                                         int grp_siz, int *allcompleted)
{
    int i = 0;

    *allcompleted = 1;
    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    for (i = 0; i < grp_siz; i++) {
        int rank = ranks_in_win_grp[i];
        target_ptr = MPIDI_CH4U_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

/* Map function interfaces in CH4 level */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_map_create(void **out_map, MPL_memory_class class)
{
    MPIDI_CH4U_map_t *map;
    map = MPL_malloc(sizeof(MPIDI_CH4U_map_t), class);
    MPIR_Assert(map != NULL);
    map->head = NULL;
    *out_map = map;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_map_destroy(void *in_map)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
    MPIDI_CH4U_map_t *map = in_map;
    HASH_CLEAR(hh, map->head);
    MPL_free(map);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_map_set(void *in_map, uint64_t id, void *val,
                                                 MPL_memory_class class)
{
    MPIDI_CH4U_map_t *map;
    MPIDI_CH4U_map_entry_t *map_entry;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
    map = (MPIDI_CH4U_map_t *) in_map;
    map_entry = MPL_malloc(sizeof(MPIDI_CH4U_map_entry_t), class);
    MPIR_Assert(map_entry != NULL);
    map_entry->key = id;
    map_entry->value = val;
    HASH_ADD(hh, map->head, key, sizeof(uint64_t), map_entry, class);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_erase
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_map_erase(void *in_map, uint64_t id)
{
    MPIDI_CH4U_map_t *map;
    MPIDI_CH4U_map_entry_t *map_entry;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
    map = (MPIDI_CH4U_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    MPIR_Assert(map_entry != NULL);
    HASH_DELETE(hh, map->head, map_entry);
    MPL_free(map_entry);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_lookup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_CH4U_map_lookup(void *in_map, uint64_t id)
{
    void *rc;
    MPIDI_CH4U_map_t *map;
    MPIDI_CH4U_map_entry_t *map_entry;

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
    map = (MPIDI_CH4U_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    if (map_entry == NULL)
        rc = MPIDI_CH4U_MAP_NOT_FOUND;
    else
        rc = map_entry->value;
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_UTIL_MUTEX);
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_map_lookup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Wait until active message acc ops are done. */
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_wait_am_acc(MPIR_Win * win, int target_rank,
                                                    int order_needed)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & order_needed) {
        MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, target_rank);
        while ((target_ptr && MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0) ||
               MPIR_cc_get(MPIDI_CH4U_WIN(win, remote_acc_cmpl_cnts)) != 0) {
            MPIDI_CH4R_PROGRESS();
        }
    }
  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
#endif /* CH4_IMPL_H_INCLUDED */

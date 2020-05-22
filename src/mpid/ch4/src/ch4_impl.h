/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_IMPL_H_INCLUDED
#define CH4_IMPL_H_INCLUDED

#include "ch4_types.h"
#include "mpidig_am.h"
#include "mpidu_shm.h"

int MPIDI_Progress_test(int flags);
int MPIDIG_get_context_index(uint64_t context_id);
uint64_t MPIDIG_generate_win_id(MPIR_Comm * comm_ptr);

/* Static inlines */

/* Reconstruct context offset associated with a persistent request.
 * Input must be a persistent request. */
static inline int MPIDI_prequest_get_context_offset(MPIR_Request * preq)
{
    int context_offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PREQUEST_GET_CONTEXT_OFFSET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PREQUEST_GET_CONTEXT_OFFSET);

    MPIR_Assert(preq->kind == MPIR_REQUEST_KIND__PREQUEST_SEND ||
                preq->kind == MPIR_REQUEST_KIND__PREQUEST_RECV);

    context_offset = MPIDI_PREQUEST(preq, context_id) - preq->comm->context_id;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PREQUEST_GET_CONTEXT_OFFSET);

    return context_offset;
}

static inline MPIR_Comm *MPIDIG_context_id_to_comm(uint64_t context_id)
{
    int comm_idx = MPIDIG_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIR_Comm *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CONTEXT_ID_TO_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CONTEXT_ID_TO_COMM);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);
    ret = MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CONTEXT_ID_TO_COMM);
    return ret;
}

static inline MPIDIG_rreq_t **MPIDIG_context_id_to_uelist(uint64_t context_id)
{
    int comm_idx = MPIDIG_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIDIG_rreq_t **ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CONTEXT_ID_TO_UELIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CONTEXT_ID_TO_UELIST);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);

    ret = &MPIDI_global.comm_req_lists[comm_idx].uelist[is_localcomm][subcomm_type];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CONTEXT_ID_TO_UELIST);
    return ret;
}

static inline MPIR_Context_id_t MPIDIG_win_id_to_context(uint64_t win_id)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_ID_TO_CONTEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_ID_TO_CONTEXT);

    /* pick the lower 32-bit to extract context id */
    ret = (win_id - 1) & 0xffffffff;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_ID_TO_CONTEXT);
    return ret;
}

static inline MPIR_Context_id_t MPIDIG_win_to_context(const MPIR_Win * win)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_TO_CONTEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_TO_CONTEXT);

    ret = MPIDIG_win_id_to_context(MPIDIG_WIN(win, win_id));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_TO_CONTEXT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDIU_request_complete(MPIR_Request * req)
{
    int incomplete;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_REQUEST_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_REQUEST_COMPLETE);

    MPIR_cc_decr(req->cc_ptr, &incomplete);
    if (!incomplete) {
        MPIR_Request_free(req);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_REQUEST_COMPLETE);
}

MPL_STATIC_INLINE_PREFIX MPIDIG_win_target_t *MPIDIG_win_target_add(MPIR_Win * win, int rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_TARGET_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_TARGET_ADD);

    MPIDIG_win_target_t *target_ptr = NULL;
    target_ptr = (MPIDIG_win_target_t *) MPL_malloc(sizeof(MPIDIG_win_target_t), MPL_MEM_RMA);
    MPIR_Assert(target_ptr);
    target_ptr->rank = rank;
    MPIR_cc_set(&target_ptr->local_cmpl_cnts, 0);
    MPIR_cc_set(&target_ptr->remote_cmpl_cnts, 0);
    MPIR_cc_set(&target_ptr->remote_acc_cmpl_cnts, 0);
    target_ptr->sync.lock.locked = 0;
    target_ptr->sync.access_epoch_type = MPIDIG_EPOTYPE_NONE;
    target_ptr->sync.assert_mode = 0;

    HASH_ADD(hash_handle, MPIDIG_WIN(win, targets), rank, sizeof(int), target_ptr, MPL_MEM_RMA);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_TARGET_ADD);
    return target_ptr;
}

MPL_STATIC_INLINE_PREFIX MPIDIG_win_target_t *MPIDIG_win_target_find(MPIR_Win * win, int rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_TARGET_FIND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_TARGET_FIND);

    MPIDIG_win_target_t *target_ptr = NULL;
    HASH_FIND(hash_handle, MPIDIG_WIN(win, targets), &rank, sizeof(int), target_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_TARGET_FIND);
    return target_ptr;
}

MPL_STATIC_INLINE_PREFIX MPIDIG_win_target_t *MPIDIG_win_target_get(MPIR_Win * win, int rank)
{
    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, rank);
    if (!target_ptr)
        target_ptr = MPIDIG_win_target_add(win, rank);
    return target_ptr;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_target_delete(MPIR_Win * win,
                                                       MPIDIG_win_target_t * target_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_TARGET_DELETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_TARGET_DELETE);

    HASH_DELETE(hash_handle, MPIDIG_WIN(win, targets), target_ptr);
    MPL_free(target_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_TARGET_DELETE);
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_target_cleanall(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_TARGET_CLEANALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_TARGET_CLEANALL);

    MPIDIG_win_target_t *target_ptr, *tmp;
    HASH_ITER(hash_handle, MPIDIG_WIN(win, targets), target_ptr, tmp) {
        HASH_DELETE(hash_handle, MPIDIG_WIN(win, targets), target_ptr);
        MPL_free(target_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_TARGET_CLEANALL);
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_hash_clear(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_HASH_CLEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_HASH_CLEAR);

    HASH_CLEAR(hash_handle, MPIDIG_WIN(win, targets));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_HASH_CLEAR);
}

#define MPIDI_Datatype_get_info(count_, datatype_,              \
                                dt_contig_out_, data_sz_out_,   \
                                dt_ptr_, dt_true_lb_)           \
    do {                                                        \
        if (IS_BUILTIN(datatype_)) {                            \
            (dt_ptr_)        = NULL;                            \
            (dt_contig_out_) = TRUE;                            \
            (dt_true_lb_)    = 0;                               \
            (data_sz_out_)   = (size_t)(count_) *               \
                MPIR_Datatype_get_basic_size(datatype_);        \
        } else {                                                \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));      \
            if (dt_ptr_)                                        \
            {                                                   \
                (dt_contig_out_) = (dt_ptr_)->is_contig;        \
                (dt_true_lb_)    = (dt_ptr_)->true_lb;          \
                (data_sz_out_)   = (size_t)(count_) *           \
                    (dt_ptr_)->size;                            \
            }                                                   \
            else                                                \
            {                                                   \
                (dt_contig_out_) = 1;                           \
                (dt_true_lb_)    = 0;                           \
                (data_sz_out_)   = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_get_size_dt_ptr(count_, datatype_,       \
                                       data_sz_out_, dt_ptr_)   \
    do {                                                        \
        if (IS_BUILTIN(datatype_)) {                            \
            (dt_ptr_)        = NULL;                            \
            (data_sz_out_)   = (size_t)(count_) *               \
                MPIR_Datatype_get_basic_size(datatype_);        \
        } else {                                                \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));      \
            (data_sz_out_)   = (dt_ptr_) ? (size_t)(count_) *   \
                (dt_ptr_)->size : 0;                            \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_check_contig(datatype_,dt_contig_out_)           \
    do {                                                                \
        if (IS_BUILTIN(datatype_)) {                                    \
            (dt_contig_out_) = TRUE;                                    \
        } else {                                                        \
            MPIR_Datatype *dt_ptr_;                                     \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));              \
            (dt_contig_out_) = (dt_ptr_) ? (dt_ptr_)->is_contig : 1;    \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_contig_size(datatype_,count_,      \
                                         dt_contig_out_,        \
                                         data_sz_out_)          \
    do {                                                        \
        if (IS_BUILTIN(datatype_)) {                            \
            (dt_contig_out_) = TRUE;                            \
            (data_sz_out_)   = (size_t)(count_) *               \
                MPIR_Datatype_get_basic_size(datatype_);        \
        } else {                                                \
            MPIR_Datatype *dt_ptr_;                             \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));      \
            if (dt_ptr_) {                                      \
                (dt_contig_out_) = (dt_ptr_)->is_contig;        \
                (data_sz_out_)   = (size_t)(count_) *           \
                    (dt_ptr_)->size;                            \
            } else {                                            \
                (dt_contig_out_) = 1;                           \
                (data_sz_out_)   = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

#define MPIDI_Datatype_check_size(datatype_,count_,data_sz_out_)        \
    do {                                                                \
        if (IS_BUILTIN(datatype_)) {                                    \
            (data_sz_out_)   = (size_t)(count_) *                       \
                MPIR_Datatype_get_basic_size(datatype_);                \
        } else {                                                        \
            MPIR_Datatype *dt_ptr_;                                     \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));              \
            (data_sz_out_)   = (dt_ptr_) ? (size_t)(count_) *           \
                (dt_ptr_)->size : 0;                                    \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_size_lb(datatype_,count_,data_sz_out_,     \
                                     dt_true_lb_)                       \
    do {                                                                \
        if (IS_BUILTIN(datatype_)) {                                    \
            (data_sz_out_)   = (size_t)(count_) *                       \
                MPIR_Datatype_get_basic_size(datatype_);                \
            (dt_true_lb_)    = 0;                                       \
        } else {                                                        \
            MPIR_Datatype *dt_ptr_;                                     \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));              \
            (data_sz_out_)   = (dt_ptr_) ? (size_t)(count_) *           \
                (dt_ptr_)->size : 0;                                    \
            (dt_true_lb_)    = (dt_ptr_) ? (dt_ptr_)->true_lb : 0;      \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_contig_size_lb(datatype_,count_,   \
                                            dt_contig_out_,     \
                                            data_sz_out_,       \
                                            dt_true_lb_)        \
    do {                                                        \
        if (IS_BUILTIN(datatype_)) {                            \
            (dt_contig_out_) = TRUE;                            \
            (data_sz_out_)   = (size_t)(count_) *               \
                MPIR_Datatype_get_basic_size(datatype_);        \
            (dt_true_lb_)    = 0;                               \
        } else {                                                \
            MPIR_Datatype *dt_ptr_;                             \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));      \
            if (dt_ptr_) {                                      \
                (dt_contig_out_) = (dt_ptr_)->is_contig;        \
                (data_sz_out_)   = (size_t)(count_) *           \
                    (dt_ptr_)->size;                            \
                (dt_true_lb_)    = (dt_ptr_)->true_lb;          \
            } else {                                            \
                (dt_contig_out_) = 1;                           \
                (data_sz_out_)   = 0;                           \
                (dt_true_lb_)    = 0;                           \
            }                                                   \
        }                                                       \
    } while (0)

/* Check both origin|target buffers' size. */
#define MPIDI_Datatype_check_origin_target_size(o_datatype_, t_datatype_,         \
                                                o_count_, t_count_,               \
                                                o_data_sz_out_, t_data_sz_out_)   \
    do {                                                                          \
        MPIDI_Datatype_check_size(o_datatype_, o_count_, o_data_sz_out_);         \
        if (t_datatype_ == o_datatype_ && t_count_ == o_count_) {                 \
            t_data_sz_out_ = o_data_sz_out_;                                      \
        } else {                                                                  \
            MPIDI_Datatype_check_size(t_datatype_, t_count_, t_data_sz_out_);     \
        }                                                                         \
    } while (0)

/* Check both origin|target buffers' size, contig and lb. */
#define MPIDI_Datatype_check_origin_target_contig_size_lb(o_datatype_, t_datatype_,             \
                                                          o_count_, t_count_,                   \
                                                          o_dt_contig_out_, t_dt_contig_out_,   \
                                                          o_data_sz_out_, t_data_sz_out_,       \
                                                          o_dt_true_lb_, t_dt_true_lb_)         \
    do {                                                                                        \
        MPIDI_Datatype_check_contig_size_lb(o_datatype_, o_count_, o_dt_contig_out_,            \
                                            o_data_sz_out_, o_dt_true_lb_);                     \
        if (t_datatype_ == o_datatype_ && t_count_ == o_count_) {                               \
            t_dt_contig_out_ = o_dt_contig_out_;                                                \
            t_data_sz_out_ = o_data_sz_out_;                                                    \
            t_dt_true_lb_ = o_dt_true_lb_;                                                      \
        }                                                                                       \
        else {                                                                                  \
            MPIDI_Datatype_check_contig_size_lb(t_datatype_, t_count_, t_dt_contig_out_,        \
                                                t_data_sz_out_, t_dt_true_lb_);                 \
        }                                                                                       \
    } while (0)

#define MPIDI_Request_create_null_rreq(rreq_, mpi_errno_, FAIL_)        \
    do {                                                                \
        (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV, 0);         \
        if ((rreq_) != NULL) {                                          \
            MPIR_cc_set(&(rreq_)->cc, 0);                               \
            MPIR_Status_set_procnull(&(rreq_)->status);                 \
        }                                                               \
        else {                                                          \
            MPIR_ERR_SETANDJUMP(mpi_errno_,MPIX_ERR_NOREQ,"**nomemreq"); \
        }                                                               \
    } while (0)

#define IS_BUILTIN(_datatype)                           \
    (HANDLE_IS_BUILTIN(_datatype))

/* We assume this routine is never called with rank=MPI_PROC_NULL. */
static inline int MPIDIU_valid_group_rank(MPIR_Comm * comm, int rank, MPIR_Group * grp)
{
    int lpid;
    int size = grp->size;
    int z;
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_VALID_GROUP_RANK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_VALID_GROUP_RANK);

    MPIDI_NM_comm_get_lpid(comm, rank, &lpid, FALSE);

    for (z = 0; z < size && lpid != grp->lrank_to_lpid[z].lpid; ++z) {
    }

    ret = (z < size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_VALID_GROUP_RANK);
    return ret;
}

/* TODO: Several unbounded loops call this macro. One way to avoid holding the
 * ALLFUNC_MUTEX lock forever is to insert YIELD in each loop. We choose to
 * insert it here for simplicity, but this might not be the best place. One
 * needs to investigate the appropriate place to yield the lock. */
/* NOTE: Taking off VCI lock is necessary to avoid recursive locking and allow
 * more granular per-vci locks */
/* TODO: MPIDI_global.vci_lock probably will be changed into granular generic lock
 */

#define MPIDIU_PROGRESS()                                   \
    do {                                                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock); \
        mpi_errno = MPID_Progress_test();                       \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock); \
        MPIR_ERR_CHECK(mpi_errno);  \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

/* Optimized versions to avoid exessive locking/unlocking */
/* FIXME: use inline function rather macros for cleaner semantics */

#define MPIDIU_PROGRESS_WHILE(cond)         \
    do {                                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock); \
        while (cond) {                          \
            mpi_errno = MPID_Progress_test();   \
            if (mpi_errno) break;               \
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
        } \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock); \
        MPIR_ERR_CHECK(mpi_errno);              \
    } while (0)

/* This macro is refactored for original code that progress in a do-while loop
 * NOTE: it's already inside the progress lock and it is calling progress again.
 *       To avoid recursive locking, we yield the lock here.
 * TODO: Can we consolidate with previous macro? Double check the reasoning.
 */
#define MPIDIU_PROGRESS_DO_WHILE(cond) \
    do {                                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock); \
        do {                          \
            mpi_errno = MPID_Progress_test();   \
            if (mpi_errno) break;               \
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
        } while (cond); \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock); \
        MPIR_ERR_CHECK(mpi_errno);              \
    } while (0)

#ifdef HAVE_ERROR_CHECKING
#define MPIDIG_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_NONE || \
            MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_POST) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* Checking per-target sync status for pscw or lock epoch. */
#define MPIDIG_EPOCH_CHECK_TARGET_SYNC(win,target_rank,mpi_errno,stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank); \
        if ((MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_START && \
             !MPIDIU_valid_group_rank(win->comm_ptr, target_rank,   \
                                          MPIDIG_WIN(win, sync).sc.group)) || \
            (target_ptr != NULL &&                                      \
             MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK && \
             target_ptr->sync.access_epoch_type != MPIDIG_EPOTYPE_LOCK)) \
            MPIR_ERR_SETANDSTMT(mpi_errno,                              \
                                MPI_ERR_RMA_SYNC,                       \
                                stmt,                                   \
                                "**rmasync");                           \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_EPOCH_CHECK_PASSIVE(win,mpi_errno,stmt)              \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if ((MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_LOCK) && \
            (MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_LOCK_ALL)) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* NOTE: unlock/flush/flush_local needs to check per-target passive epoch (lock) */
#define MPIDIG_EPOCH_CHECK_TARGET_LOCK(target_ptr,mpi_errno,stmt)   \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (target_ptr->sync.access_epoch_type != MPIDIG_EPOTYPE_LOCK) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_ACCESS_EPOCH_CHECK_NONE(win,mpi_errno,stmt)          \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_NONE && \
            MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_EXPOSURE_EPOCH_CHECK_NONE(win,mpi_errno,stmt)        \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDIG_WIN(win, sync).exposure_epoch_type != MPIDIG_EPOTYPE_NONE && \
            MPIDIG_WIN(win, sync).exposure_epoch_type != MPIDIG_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

/* NOTE: multiple lock access epochs can occur simultaneously, as long as
 * target to different processes */
#define MPIDIG_LOCK_EPOCH_CHECK_NONE(win,rank,mpi_errno,stmt)       \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, rank); \
        if (MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_NONE && \
            MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_REFENCE && \
            !(MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK && \
              (target_ptr == NULL || (!MPIR_CVAR_CH4_RMA_MEM_EFFICIENT && \
                                      target_ptr->sync.lock.locked == 0)))) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_FENCE_EPOCH_CHECK(win,mpi_errno,stmt)                \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if ((MPIDIG_WIN(win, sync).exposure_epoch_type != MPIDIG_EPOTYPE_FENCE && \
             MPIDIG_WIN(win, sync).exposure_epoch_type != MPIDIG_EPOTYPE_REFENCE && \
             MPIDIG_WIN(win, sync).exposure_epoch_type != MPIDIG_EPOTYPE_NONE) || \
            (MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_FENCE && \
             MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_REFENCE && \
             MPIDIG_WIN(win, sync).access_epoch_type != MPIDIG_EPOTYPE_NONE)) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_ACCESS_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDIG_WIN(win, sync).access_epoch_type != epoch_type)  \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDIG_EXPOSURE_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDIG_WIN(win, sync).exposure_epoch_type != epoch_type) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#else /* HAVE_ERROR_CHECKING */
#define MPIDIG_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDIG_EPOCH_CHECK_TARGET_SYNC(win, target_rank, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, stmt)            if (0) goto fn_fail;
#define MPIDIG_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, stmt)  if (0) goto fn_fail;
#define MPIDIG_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, stmt)        if (0) goto fn_fail;
#define MPIDIG_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, stmt)           if (0) goto fn_fail;
#define MPIDIG_LOCK_EPOCH_CHECK_NONE(win,rank,mpi_errno,stmt)       if (0) goto fn_fail;
#define MPIDIG_FENCE_EPOCH_CHECK(win, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDIG_ACCESS_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt) if (0) goto fn_fail;
#define MPIDIG_EXPOSURE_EPOCH_CHECK(win, epoch_type, mpi_errno, stmt)    if (0) goto fn_fail;
#endif /* HAVE_ERROR_CHECKING */

#define MPIDIG_EPOCH_FENCE_EVENT(win, massert)                      \
    do {                                                                \
        if (massert & MPI_MODE_NOSUCCEED)                               \
        {                                                               \
            MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_NONE; \
            MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_NONE; \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_REFENCE; \
            MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_REFENCE; \
        }                                                               \
    } while (0)

#define MPIDIG_EPOCH_OP_REFENCE(win)                                \
    do {                                                                \
        if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_REFENCE && \
            MPIDIG_WIN(win, sync).exposure_epoch_type == MPIDIG_EPOTYPE_REFENCE) \
        {                                                               \
            MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_FENCE; \
            MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_FENCE; \
        }                                                               \
    } while (0)

/* Generic routine for checking synchronization at every RMA operation.
 * Assuming no RMA operation with target_rank=PROC_NULL will call it. */
#define MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win)                                 \
    do {                                                                               \
        MPIDIG_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);                     \
        MPIDIG_EPOCH_OP_REFENCE(win);                                              \
        /* Check target sync status for target_rank. */       \
        MPIDIG_EPOCH_CHECK_TARGET_SYNC(win, target_rank, mpi_errno, goto fn_fail); \
    } while (0);

/*
  Calculate base address of the target window at the origin side
  Return zero to let the target side calculate the actual address
  (only offset from window base is given to the target in this case)
*/
static inline uintptr_t MPIDIG_win_base_at_origin(const MPIR_Win * win, int target_rank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_BASE_AT_ORIGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_BASE_AT_ORIGIN);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_BASE_AT_ORIGIN);

    /* TODO: In future we may want to calculate the full virtual address
     * in the target at the origin side. It can be done by looking at
     * MPIDIG_WINFO(win, target_rank)->base_addr */
    return 0;
}

/*
  Calculate base address of the window at the target side
  If MPIDIG_win_base_at_origin calculates the full virtual address
  this function must return zero
*/
static inline uintptr_t MPIDIG_win_base_at_target(const MPIR_Win * win)
{
    uintptr_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_BASE_AT_TARGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_BASE_AT_TARGET);

    ret = (uintptr_t) win->base;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_BASE_AT_TARGET);
    return ret;
}

static inline void MPIDIG_win_cmpl_cnts_incr(MPIR_Win * win, int target_rank,
                                             MPIR_cc_t ** local_cmpl_cnts_ptr)
{
    int c = 0;

    /* Increase per-window counters for fence, and per-target counters for
     * all other synchronization. */
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
            /* FIXME: now we simply set per-target counters for lockall in case
             * user flushes per target, but this should be optimized. */
        case MPIDIG_EPOTYPE_LOCK_ALL:
            /* FIXME: now we simply set per-target counters for PSCW, can it be optimized ? */
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_get(win, target_rank);

                MPIR_cc_incr(&target_ptr->local_cmpl_cnts, &c);
                MPIR_cc_incr(&target_ptr->remote_cmpl_cnts, &c);

                *local_cmpl_cnts_ptr = &target_ptr->local_cmpl_cnts;
                break;
            }
        default:
            MPIR_cc_incr(&MPIDIG_WIN(win, local_cmpl_cnts), &c);
            MPIR_cc_incr(&MPIDIG_WIN(win, remote_cmpl_cnts), &c);

            *local_cmpl_cnts_ptr = &MPIDIG_WIN(win, local_cmpl_cnts);
            break;
    }
}

/* Increase counter for active message acc ops. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_win_remote_acc_cmpl_cnt_incr(MPIR_Win * win, int target_rank)
{
    int c = 0;
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_get(win, target_rank);
                MPIR_cc_incr(&target_ptr->remote_acc_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_incr(&MPIDIG_WIN(win, remote_acc_cmpl_cnts), &c);
            break;
    }
}

/* Decrease counter for active message acc ops. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_win_remote_acc_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    int c = 0;
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_decr(&target_ptr->remote_acc_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_decr(&MPIDIG_WIN(win, remote_acc_cmpl_cnts), &c);
            break;
    }

}

static inline void MPIDIG_win_remote_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    int c = 0;

    /* Decrease per-window counter for fence, and per-target counters for
     * all other synchronization. */
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_decr(&target_ptr->remote_cmpl_cnts, &c);
                break;
            }
        default:
            MPIR_cc_decr(&MPIDIG_WIN(win, remote_cmpl_cnts), &c);
            break;
    }
}

static inline void MPIDIG_win_check_all_targets_remote_completed(MPIR_Win * win, int *allcompleted)
{
    int rank = 0;

    *allcompleted = 1;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0 ||
            MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

static inline void MPIDIG_win_check_all_targets_local_completed(MPIR_Win * win, int *allcompleted)
{
    int rank = 0;

    *allcompleted = 1;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

static inline void MPIDIG_win_check_group_local_completed(MPIR_Win * win,
                                                          int *ranks_in_win_grp,
                                                          int grp_siz, int *allcompleted)
{
    int i = 0;

    *allcompleted = 1;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (i = 0; i < grp_siz; i++) {
        int rank = ranks_in_win_grp[i];
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            *allcompleted = 0;
            break;
        }
    }
}

/* Map function interfaces in CH4 level */
MPL_STATIC_INLINE_PREFIX void MPIDIU_map_create(void **out_map, MPL_memory_class class)
{
    MPIDIU_map_t *map;
    map = MPL_malloc(sizeof(MPIDIU_map_t), class);
    MPIR_Assert(map != NULL);
    map->head = NULL;
    *out_map = map;
}

MPL_STATIC_INLINE_PREFIX void MPIDIU_map_destroy(void *in_map)
{
    MPIDIU_map_t *map = in_map;
    MPIDIU_map_entry_t *e, *etmp;
    HASH_ITER(hh, map->head, e, etmp) {
        /* Free all remaining entries in the hash */
        HASH_DELETE(hh, map->head, e);
        MPL_free(e);
    }
    HASH_CLEAR(hh, map->head);
    MPL_free(map);
}

MPL_STATIC_INLINE_PREFIX void MPIDIU_map_set_unsafe(void *in_map, uint64_t id, void *val,
                                                    MPL_memory_class class)
{
    MPIDIU_map_t *map;
    MPIDIU_map_entry_t *map_entry;
    /* MPIDIU_MAP_NOT_FOUND may be used as a special value to indicate an error. */
    MPIR_Assert(val != MPIDIU_MAP_NOT_FOUND);
    map = (MPIDIU_map_t *) in_map;
    map_entry = MPL_malloc(sizeof(MPIDIU_map_entry_t), class);
    MPIR_Assert(map_entry != NULL);
    map_entry->key = id;
    map_entry->value = val;
    HASH_ADD(hh, map->head, key, sizeof(uint64_t), map_entry, class);
}

/* Sets a (id -> val) pair into the map, assuming there's no entry with `id`. */
MPL_STATIC_INLINE_PREFIX void MPIDIU_map_set(void *in_map, uint64_t id, void *val,
                                             MPL_memory_class class)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIDIU_THREAD_UTIL_MUTEX);
    MPIDIU_map_set_unsafe(in_map, id, val, class);
    MPID_THREAD_CS_EXIT(POBJ, MPIDIU_THREAD_UTIL_MUTEX);
}

MPL_STATIC_INLINE_PREFIX void MPIDIU_map_erase(void *in_map, uint64_t id)
{
    MPIDIU_map_t *map;
    MPIDIU_map_entry_t *map_entry;
    map = (MPIDIU_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    MPIR_Assert(map_entry != NULL);
    HASH_DELETE(hh, map->head, map_entry);
    MPL_free(map_entry);
}

MPL_STATIC_INLINE_PREFIX void *MPIDIU_map_lookup(void *in_map, uint64_t id)
{
    void *rc;
    MPIDIU_map_t *map;
    MPIDIU_map_entry_t *map_entry;

    map = (MPIDIU_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    if (map_entry == NULL)
        rc = MPIDIU_MAP_NOT_FOUND;
    else
        rc = map_entry->value;
    return rc;
}

/* Updates a value in the map which has `id` as a key.
   If `id` does not exist in the map, it will be added. Returns the old value. */
MPL_STATIC_INLINE_PREFIX void *MPIDIU_map_update(void *in_map, uint64_t id, void *new_val,
                                                 MPL_memory_class class)
{
    void *rc;
    MPIDIU_map_t *map;
    MPIDIU_map_entry_t *map_entry;

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_THREAD_UTIL_MUTEX);
    map = (MPIDIU_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    if (map_entry == NULL) {
        rc = MPIDIU_MAP_NOT_FOUND;
        MPIDIU_map_set_unsafe(in_map, id, new_val, class);
    } else {
        rc = map_entry->value;
        map_entry->value = new_val;
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_THREAD_UTIL_MUTEX);
    return rc;
}

/* Wait until active message acc ops are done. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_wait_am_acc(MPIR_Win * win, int target_rank, int order_needed)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIDIG_WIN(win, info_args).accumulate_ordering & order_needed) {
        MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
        while ((target_ptr && MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0) ||
               MPIR_cc_get(MPIDIG_WIN(win, remote_acc_cmpl_cnts)) != 0) {
            MPIDIU_PROGRESS();
        }
    }
  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Compute accumulate operation.
 * The source datatype can be only predefined; the target datatype can be
 * predefined or derived. If the source buffer has been packed by the caller,
 * src_kind must be set to MPIDIG_ACC_SRCBUF_PACKED.*/
MPL_STATIC_INLINE_PREFIX int MPIDIG_compute_acc_op(void *source_buf, int source_count,
                                                   MPI_Datatype source_dtp, void *target_buf,
                                                   int target_count, MPI_Datatype target_dtp,
                                                   MPI_Op acc_op, int src_kind)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop = NULL;
    MPI_Aint source_dtp_size = 0, source_dtp_extent = 0;
    int is_empty_source = FALSE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMPUTE_ACC_OP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMPUTE_ACC_OP);

    /* first Judge if source buffer is empty */
    if (acc_op == MPI_NO_OP)
        is_empty_source = TRUE;

    if (is_empty_source == FALSE) {
        MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));
        MPIR_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPIR_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
    }

    if ((HANDLE_IS_BUILTIN(acc_op))
        && ((*MPIR_OP_HDL_TO_DTYPE_FN(acc_op)) (source_dtp) == MPI_SUCCESS)) {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(acc_op);
    } else {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OP,
                                         "**opnotpredefined", "**opnotpredefined %d", acc_op);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }


    if (is_empty_source == TRUE || MPIR_DATATYPE_IS_PREDEFINED(target_dtp)) {
        /* directly apply op if target dtp is predefined dtp OR source buffer is empty */
        (*uop) (source_buf, target_buf, &source_count, &source_dtp);
    } else {
        /* derived datatype */
        struct iovec *typerep_vec;
        int i, count;
        MPI_Aint vec_len, type_extent, type_size, src_type_stride;
        MPI_Datatype type;
        MPIR_Datatype *dtp;
        MPI_Aint curr_len;
        void *curr_loc;
        int accumulated_count;

        MPIR_Datatype_get_ptr(target_dtp, dtp);
        MPIR_Assert(dtp != NULL);
        vec_len = dtp->max_contig_blocks * target_count + 1;
        /* +1 needed because Rob says so */
        typerep_vec = (struct iovec *)
            MPL_malloc(vec_len * sizeof(struct iovec), MPL_MEM_RMA);
        /* --BEGIN ERROR HANDLING-- */
        if (!typerep_vec) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        MPI_Aint actual_iov_len, actual_iov_bytes;
        MPIR_Typerep_to_iov(NULL, target_count, target_dtp, 0, typerep_vec, vec_len,
                            source_count * source_dtp_size, &actual_iov_len, &actual_iov_bytes);
        vec_len = actual_iov_len;

        type = dtp->basic_type;
        MPIR_Assert(type != MPI_DATATYPE_NULL);

        MPIR_Assert(type == source_dtp);
        type_size = source_dtp_size;
        type_extent = source_dtp_extent;
        /* If the source buffer has been packed by the caller, the distance between
         * two elements can be smaller than extent. E.g., predefined pairtype may
         * have larger extent than size.*/
        if (src_kind == MPIDIG_ACC_SRCBUF_PACKED)
            src_type_stride = source_dtp_size;
        else
            src_type_stride = source_dtp_extent;

        i = 0;
        curr_loc = typerep_vec[0].iov_base;
        curr_len = typerep_vec[0].iov_len;
        accumulated_count = 0;
        while (i != vec_len) {
            if (curr_len < type_size) {
                MPIR_Assert(i != vec_len);
                i++;
                curr_len += typerep_vec[i].iov_len;
                continue;
            }

            MPIR_Assign_trunc(count, curr_len / type_size, int);

            (*uop) ((char *) source_buf + src_type_stride * accumulated_count,
                    (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), &count, &type);

            if (curr_len % type_size == 0) {
                i++;
                if (i != vec_len) {
                    curr_loc = typerep_vec[i].iov_base;
                    curr_len = typerep_vec[i].iov_len;
                }
            } else {
                curr_loc = (void *) ((char *) curr_loc + type_extent * count);
                curr_len -= type_size * count;
            }

            accumulated_count += count;
        }

        MPL_free(typerep_vec);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMPUTE_ACC_OP);
    return mpi_errno;
}

#endif /* CH4_IMPL_H_INCLUDED */

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_IMPL_H_INCLUDED
#define CH4_IMPL_H_INCLUDED

#include "ch4_types.h"
#include "mpidig.h"
#include "mpidu_shm.h"
#include "ch4_proc.h"
#include "ch4_self.h"
#include "ch4_vci.h"

int MPIDIU_Intercomm_map_bcast_intra(MPIR_Comm * local_comm, int local_leader, int *remote_size,
                                     int *is_low_group, int pure_intracomm,
                                     int *remote_upid_size, char *remote_upids,
                                     uint64_t ** remote_gpids);
int MPIDIG_get_context_index(uint64_t context_id);
uint64_t MPIDIG_generate_win_id(MPIR_Comm * comm_ptr);

/* define CH4_CALL to call netmod or shm API based on is_local */
#ifdef MPIDI_CH4_DIRECT_NETMOD
#define CH4_CALL(FUNC, is_local_, err_) \
    do { \
        err_ = MPIDI_NM_ ## FUNC; \
    } while (0)
#else
#define CH4_CALL(FUNC, is_local_, err_) \
    do { \
        if (is_local_) { \
            err_ = MPIDI_SHM_ ## FUNC; \
        } else { \
            err_ = MPIDI_NM_ ## FUNC; \
        } \
    } while (0)
#endif

/* Request creation with locking for LOCKLESS MT model */
#define MPIDI_CH4_REQUEST_CREATE(req, kind, pool, ref_count)            \
    do {                                                                \
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS)                \
            (req) = MPIR_Request_create_from_pool_safe(kind, pool, ref_count); \
        else                                                            \
            (req) = MPIR_Request_create_from_pool(kind, pool, ref_count); \
    } while (0)

#define MPIDI_CH4_REQUEST_FREE(req)                                \
    do {                                                           \
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS)           \
            MPIR_Request_free_safe(req);                           \
        else                                                       \
            MPIR_Request_free_unsafe(req);                         \
    } while (0)

/* Static inlines */

/* Reconstruct context offset associated with a persistent request.
 * Input must be a persistent request. */
MPL_STATIC_INLINE_PREFIX int MPIDI_prequest_get_context_offset(MPIR_Request * preq)
{
    int context_offset;

    MPIR_FUNC_ENTER;

    MPIR_Assert(preq->kind == MPIR_REQUEST_KIND__PREQUEST_SEND ||
                preq->kind == MPIR_REQUEST_KIND__PREQUEST_RECV);

    context_offset = MPIDI_PREQUEST(preq, context_id) - preq->comm->context_id;

    MPIR_FUNC_EXIT;

    return context_offset;
}

MPL_STATIC_INLINE_PREFIX MPIR_Context_id_t MPIDIG_win_id_to_context(uint64_t win_id)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_ENTER;

    /* pick the lower 32-bit to extract context id */
    ret = (MPIR_Context_id_t) ((win_id - 1) & 0xffffffff);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX MPIR_Context_id_t MPIDIG_win_to_context(const MPIR_Win * win)
{
    MPIR_Context_id_t ret;

    MPIR_FUNC_ENTER;

    ret = MPIDIG_win_id_to_context(MPIDIG_WIN(win, win_id));

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX MPIDIG_win_target_t *MPIDIG_win_target_add(MPIR_Win * win, int rank)
{
    MPIR_FUNC_ENTER;

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

    MPIR_FUNC_EXIT;
    return target_ptr;
}

MPL_STATIC_INLINE_PREFIX MPIDIG_win_target_t *MPIDIG_win_target_find(MPIR_Win * win, int rank)
{
    MPIR_FUNC_ENTER;

    MPIDIG_win_target_t *target_ptr = NULL;
    HASH_FIND(hash_handle, MPIDIG_WIN(win, targets), &rank, sizeof(int), target_ptr);

    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    HASH_DELETE(hash_handle, MPIDIG_WIN(win, targets), target_ptr);
    MPL_free(target_ptr);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_target_cleanall(MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    MPIDIG_win_target_t *target_ptr, *tmp;
    HASH_ITER(hash_handle, MPIDIG_WIN(win, targets), target_ptr, tmp) {
        HASH_DELETE(hash_handle, MPIDIG_WIN(win, targets), target_ptr);
        MPL_free(target_ptr);
    }

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_hash_clear(MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    HASH_CLEAR(hash_handle, MPIDIG_WIN(win, targets));

    MPIR_FUNC_EXIT;
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

#define MPIDI_Datatype_check_contig_lb(datatype_, dt_contig_out_, dt_true_lb_) \
    do {                                                                       \
        if (IS_BUILTIN(datatype_)) {                                           \
            (dt_contig_out_) = TRUE;                                           \
            (dt_true_lb_)    = 0;                                              \
        } else {                                                               \
            MPIR_Datatype *dt_ptr_;                                            \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));                     \
            if (dt_ptr_) {                                                     \
                (dt_contig_out_) = (dt_ptr_)->is_contig;                       \
                (dt_true_lb_)    = (dt_ptr_)->true_lb;                         \
            } else {                                                           \
                (dt_contig_out_) = 1;                                          \
                (dt_true_lb_)    = 0;                                          \
            }                                                                  \
        }                                                                      \
    } while (0)

#define MPIDI_Datatype_check_lb(datatype_, dt_true_lb_)    \
    do {                                                   \
        if (IS_BUILTIN(datatype_)) {                       \
            (dt_true_lb_)    = 0;                          \
        } else {                                           \
            MPIR_Datatype *dt_ptr_;                        \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_)); \
            if (dt_ptr_) {                                 \
                (dt_true_lb_)    = (dt_ptr_)->true_lb;     \
            } else {                                       \
                (dt_true_lb_)    = 0;                      \
            }                                              \
        }                                                  \
    } while (0)

#define MPIDI_Datatype_check_contig_size_extent_lb(datatype_,count_,   \
                                                   dt_contig_out_,     \
                                                   data_sz_out_,       \
                                                   dt_extent_out_,     \
                                                   dt_true_lb_)        \
    do {                                                        \
        if (IS_BUILTIN(datatype_)) {                            \
            (dt_contig_out_) = TRUE;                            \
            (data_sz_out_) = (size_t)(count_) * MPIR_Datatype_get_basic_size(datatype_);  \
            (dt_extent_out_) = (data_sz_out_);                  \
            (dt_true_lb_) = 0;                                  \
        } else {                                                \
            MPIR_Datatype *dt_ptr_;                             \
            MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));      \
            MPIR_Assert(dt_ptr_);                               \
            (dt_contig_out_) = (dt_ptr_)->is_contig;            \
            (data_sz_out_) = (size_t)(count_) * (dt_ptr_)->size;    \
            (dt_extent_out_) = (size_t)(count_) * (dt_ptr_)->extent;\
            (dt_true_lb_) = (dt_ptr_)->true_lb;                 \
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

#define IS_BUILTIN(_datatype)                           \
    (HANDLE_IS_BUILTIN(_datatype))

/* We assume this routine is never called with rank=MPI_PROC_NULL. */
MPL_STATIC_INLINE_PREFIX int MPIDIU_valid_group_rank(MPIR_Comm * comm, int rank, MPIR_Group * grp)
{
    uint64_t gpid;
    int size = grp->size;
    int z;
    int ret;

    MPIR_FUNC_ENTER;

    MPIDI_NM_comm_get_gpid(comm, rank, &gpid, FALSE);

    for (z = 0; z < size && gpid != grp->lrank_to_lpid[z].lpid; ++z) {
    }

    ret = (z < size);

    MPIR_FUNC_EXIT;
    return ret;
}

/* Following progress macros are currently used by window synchronization calls.
 *
 * CAUTION: the macro uses MPIR_ERR_CHECK, be careful of it escaping the
 * critical section.
 *
 * NOTE: when used in a loop, we insert a yield of global lock to prevent
 * blocking other progress (under global granularity).
 */

/* declare to avoid header order dance */
MPL_STATIC_INLINE_PREFIX int MPIDI_progress_test_vci(int vci);

#define MPIDIU_PROGRESS_WHILE(cond, vci)         \
    while (cond) {                          \
        mpi_errno = MPIDI_progress_test_vci(vci);   \
        MPIR_ERR_CHECK(mpi_errno); \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    }

#define MPIDIU_PROGRESS_DO_WHILE(cond, vci) \
    do { \
        mpi_errno = MPIDI_progress_test_vci(vci); \
        MPIR_ERR_CHECK(mpi_errno); \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (cond)

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

#define MPIDIG_EPOCH_OP_REFENCE(win)                                \
    do {                                                                \
        if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_REFENCE && \
            MPIDIG_WIN(win, sync).exposure_epoch_type == MPIDIG_EPOTYPE_REFENCE) \
        {                                                               \
            MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_FENCE; \
            MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_FENCE; \
        }                                                               \
    } while (0)

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

/* Generic routine for checking synchronization at every RMA operation.
 * Assuming no RMA operation with target_rank=PROC_NULL will call it. */
#define MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win)                                 \
    do {                                                                               \
        MPIDIG_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);                     \
        MPIDIG_EPOCH_OP_REFENCE(win);                                              \
        /* Check target sync status for target_rank. */       \
        MPIDIG_EPOCH_CHECK_TARGET_SYNC(win, target_rank, mpi_errno, goto fn_fail); \
    } while (0);

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
#define MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win) if (0) goto fn_fail;
#define MPIDIG_EPOCH_FENCE_EVENT(win, massert) do {} while (0)
#endif /* HAVE_ERROR_CHECKING */

#define MPIDI_WIN_TARGET_VCI(win, rank) \
    (MPIDI_WIN(win, vci_table) ? MPIDI_WIN(win, vci_table)[rank] : MPIDI_WIN(win, am_vci))

/*
  Calculate base address of the target window at the origin side
  Return zero to let the target side calculate the actual address
  (only offset from window base is given to the target in this case)
*/
MPL_STATIC_INLINE_PREFIX uintptr_t MPIDIG_win_base_at_origin(const MPIR_Win * win, int target_rank)
{
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;

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
MPL_STATIC_INLINE_PREFIX uintptr_t MPIDIG_win_base_at_target(const MPIR_Win * win)
{
    uintptr_t ret;

    MPIR_FUNC_ENTER;

    ret = (uintptr_t) win->base;

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_cmpl_cnts_incr(MPIR_Win * win, int target_rank,
                                                        MPIR_cc_t ** local_cmpl_cnts_ptr)
{
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

                MPIR_cc_inc(&target_ptr->local_cmpl_cnts);
                MPIR_cc_inc(&target_ptr->remote_cmpl_cnts);

                *local_cmpl_cnts_ptr = &target_ptr->local_cmpl_cnts;
                break;
            }
        default:
            MPIR_cc_inc(&MPIDIG_WIN(win, local_cmpl_cnts));
            MPIR_cc_inc(&MPIDIG_WIN(win, remote_cmpl_cnts));

            *local_cmpl_cnts_ptr = &MPIDIG_WIN(win, local_cmpl_cnts);
            break;
    }
}

/* Increase counter for active message acc ops. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_win_remote_acc_cmpl_cnt_incr(MPIR_Win * win, int target_rank)
{
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_get(win, target_rank);
                MPIR_cc_inc(&target_ptr->remote_acc_cmpl_cnts);
                break;
            }
        default:
            MPIR_cc_inc(&MPIDIG_WIN(win, remote_acc_cmpl_cnts));
            break;
    }
}

/* Decrease counter for active message acc ops. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_win_remote_acc_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_dec(&target_ptr->remote_acc_cmpl_cnts);
                break;
            }
        default:
            MPIR_cc_dec(&MPIDIG_WIN(win, remote_acc_cmpl_cnts));
            break;
    }

}

MPL_STATIC_INLINE_PREFIX void MPIDIG_win_remote_cmpl_cnt_decr(MPIR_Win * win, int target_rank)
{
    /* Decrease per-window counter for fence, and per-target counters for
     * all other synchronization. */
    switch (MPIDIG_WIN(win, sync).access_epoch_type) {
        case MPIDIG_EPOTYPE_LOCK:
        case MPIDIG_EPOTYPE_LOCK_ALL:
        case MPIDIG_EPOTYPE_START:
            {
                MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
                MPIR_Assert(target_ptr);
                MPIR_cc_dec(&target_ptr->remote_cmpl_cnts);
                break;
            }
        default:
            MPIR_cc_dec(&MPIDIG_WIN(win, remote_cmpl_cnts));
            break;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_win_check_all_targets_remote_completed(MPIR_Win * win)
{
    int rank = 0;

    if (!MPIDIG_WIN(win, targets))
        return true;

    bool allcompleted = true;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0 ||
            MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0) {
            allcompleted = false;
            break;
        }
    }
    return allcompleted;
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_win_check_all_targets_local_completed(MPIR_Win * win)
{
    int rank = 0;

    if (!MPIDIG_WIN(win, targets))
        return true;

    bool allcompleted = true;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++) {
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            allcompleted = false;
            break;
        }
    }
    return allcompleted;
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_win_check_group_local_completed(MPIR_Win * win,
                                                                     int *ranks_in_win_grp,
                                                                     int grp_siz)
{
    int i = 0;

    if (!MPIDIG_WIN(win, targets))
        return true;

    bool allcompleted = true;
    MPIDIG_win_target_t *target_ptr = NULL;
    for (i = 0; i < grp_siz; i++) {
        int rank = ranks_in_win_grp[i];
        target_ptr = MPIDIG_win_target_find(win, rank);
        if (!target_ptr)
            continue;
        if (MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) {
            allcompleted = false;
            break;
        }
    }
    return allcompleted;
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
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_UTIL_MUTEX);
    MPIDIU_map_set_unsafe(in_map, id, val, class);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_UTIL_MUTEX);
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

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_UTIL_MUTEX);
    map = (MPIDIU_map_t *) in_map;
    HASH_FIND(hh, map->head, &id, sizeof(uint64_t), map_entry);
    if (map_entry == NULL) {
        rc = MPIDIU_MAP_NOT_FOUND;
        MPIDIU_map_set_unsafe(in_map, id, new_val, class);
    } else {
        rc = map_entry->value;
        map_entry->value = new_val;
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_UTIL_MUTEX);
    return rc;
}

/* Return the associated av for a RMA target.
 * This is an optimized path for direct intra comm (comm_world or dup from comm_world) by
 * eliminating pointer dereferences into dynamic allocated objects (i.e., win->comm_ptr).*/
MPL_STATIC_INLINE_PREFIX MPIDI_av_entry_t *MPIDIU_win_rank_to_av(MPIR_Win * win, int rank,
                                                                 MPIDI_winattr_t winattr)
{
    MPIDI_av_entry_t *av = NULL;

    if (winattr & MPIDI_WINATTR_DIRECT_INTRA_COMM) {
        av = &MPIDI_global.avt_mgr.av_table0->table[rank];
    } else
        av = MPIDIU_comm_rank_to_av(win->comm_ptr, rank);
    return av;
}

/* Return the local process's rank in the window.
 * This is an optimized path for direct intra comm (comm_world or dup from comm_world) by
 * eliminating pointer dereferences into dynamic allocated objects (i.e., win->comm_ptr).*/
MPL_STATIC_INLINE_PREFIX int MPIDIU_win_comm_rank(MPIR_Win * win, MPIDI_winattr_t winattr)
{
    if (winattr & MPIDI_WINATTR_DIRECT_INTRA_COMM)
        return MPIR_Process.rank;
    else
        return win->comm_ptr->rank;
}

/* Return the corresponding rank in intranode for a RMA target.
 * This is an optimized path for direct intra comm (comm_world or dup from comm_world) by
 * eliminating pointer dereferences into dynamic allocated objects (i.e., win->comm_ptr).*/
MPL_STATIC_INLINE_PREFIX int MPIDIU_win_rank_to_intra_rank(MPIR_Win * win, int rank,
                                                           MPIDI_winattr_t winattr)
{
    if (winattr & MPIDI_WINATTR_DIRECT_INTRA_COMM)
        return MPIR_Process.comm_world->intranode_table[rank];
    else
        return win->comm_ptr->intranode_table[rank];
}

/* Wait until active message acc ops are done. */
/* NOTE: this function is currently only called from ofi_rma.h, it is being called
 * outside per-vci critical section */
MPL_STATIC_INLINE_PREFIX int MPIDIG_wait_am_acc(MPIR_Win * win, int target_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, target_rank);
    MPID_Progress_state state;
    state.vci_count = 1;
    state.vci[0] = 0;   /* MPIDIG only uses vci 0 for now */
    state.flag = MPIDI_PROGRESS_ALL;
    /* skip other state fields for MPID_Progress_test */
    while ((target_ptr && MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0) ||
           MPIR_cc_get(MPIDIG_WIN(win, remote_acc_cmpl_cnts)) != 0) {
        mpi_errno = MPID_Progress_test(&state);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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
MPL_STATIC_INLINE_PREFIX int MPIDIG_compute_acc_op(void *source_buf, MPI_Aint source_count,
                                                   MPI_Datatype source_dtp, void *target_buf,
                                                   MPI_Aint target_count, MPI_Datatype target_dtp,
                                                   MPI_Op acc_op, int src_kind)
{
    int mpi_errno = MPI_SUCCESS;


    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Typerep_op(source_buf, source_count, source_dtp,
                                target_buf, target_count, target_dtp,
                                acc_op, src_kind == MPIDIG_ACC_SRCBUF_PACKED, -1);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* NOTE: the slot for MPI_OP_NULL refers to RMA cswap */
MPL_STATIC_INLINE_PREFIX int MPIDIU_win_acc_op_get_index(MPI_Op op)
{
    return MPIR_Op_builtin_get_index(op);
}

MPL_STATIC_INLINE_PREFIX MPI_Op MPIDIU_win_acc_get_op(int index)
{
    return MPIR_Op_builtin_get_op(index);
}

/* Determine whether need poll progress for RMA target-side active message.
 * The polling interval is set globally as we don't distinguish target-side
 * AM handling per-window.  */
MPL_STATIC_INLINE_PREFIX bool MPIDIG_rma_need_poll_am(void)
{
    bool poll_flag = false;

    if (MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS) {
        int interval;
        MPIR_cc_incr(&MPIDIG_global.rma_am_poll_cntr, &interval);

        /* Always poll if any RMA target-side AM has arrived because
         * we expect more incoming AM now. */
        if (MPL_atomic_load_int(&MPIDIG_global.rma_am_flag)) {
            poll_flag = true;
        } else {
            /* Otherwise poll with low frequency to reduce latency */
            poll_flag = ((interval + 1) % MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL
                         == 0) ? true : false;
        }
    } else if (MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL > 1) {
        int interval;
        MPIR_cc_incr(&MPIDIG_global.rma_am_poll_cntr, &interval);

        /* User explicitly controls the polling frequency */
        poll_flag = ((interval + 1) % MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL == 0) ? true : false;
    } else if (MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL == 1) {
        /* Skip cntr update when interval == 1, as we always poll (default)  */
        poll_flag = true;
    } else {
        /* User explicitly disables polling */
        poll_flag = false;
    }

    return poll_flag;
}

/* Set flag to indicate a target-side AM has arrived. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_rma_set_am_flag(void)
{
    MPL_atomic_store_int(&MPIDIG_global.rma_am_flag, 1);
}

MPL_STATIC_INLINE_PREFIX void update_sender_vci(MPIR_Comm * comm, int value)
{
    comm->hints[MPIR_COMM_HINT_SENDER_VCI] = value % MPIDI_global.n_vcis;
}

MPL_STATIC_INLINE_PREFIX void update_receiver_vci(MPIR_Comm * comm, int value)
{
    comm->hints[MPIR_COMM_HINT_RECEIVER_VCI] = value % MPIDI_global.n_vcis;
}

MPL_STATIC_INLINE_PREFIX void update_comm_vci(MPIR_Comm * comm, int value)
{
    /* update the comm hints vci, sender_vci, and receiver_vci */
    comm->hints[MPIR_COMM_HINT_VCI] = value % MPIDI_global.n_vcis;
    update_sender_vci(comm, value);
    update_receiver_vci(comm, value);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_sender_vci(MPIR_Comm * comm, int type, int value)
{
    /* if the generic hint "vci" is set, update all of "vci", "sender_vci", and "receiver_vci"
     * for consistency */
    if (comm->hints[MPIR_COMM_HINT_VCI] != MPIDI_VCI_INVALID) {
        update_comm_vci(comm, value);
    } else {
        update_sender_vci(comm, value);
    }
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_receiver_vci(MPIR_Comm * comm, int type, int value)
{
    /* if the generic hint "vci" is set, update all of "vci", "sender_vci", and "receiver_vci"
     * for consistency */
    if (comm->hints[MPIR_COMM_HINT_VCI] != MPIDI_VCI_INVALID) {
        update_comm_vci(comm, value);
    } else {
        update_receiver_vci(comm, value);
    }
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_vci(MPIR_Comm * comm, int type, int value)
{
    update_comm_vci(comm, value);
    return MPI_SUCCESS;
}

#endif /* CH4_IMPL_H_INCLUDED */

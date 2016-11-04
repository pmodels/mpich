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

/* Static inlines */
static inline int MPIDI_CH4U_get_tag(uint64_t match_bits)
{
    int tag = (match_bits & MPIDI_CH4U_TAG_MASK);
    /* Left shift and right shift by MPIDI_CH4U_TAG_SHIFT_UNPACK is to make sure the sign of tag is retained */
    return ((tag << MPIDI_CH4U_TAG_SHIFT_UNPACK) >> MPIDI_CH4U_TAG_SHIFT_UNPACK);
}

static inline int MPIDI_CH4U_get_context(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_CH4U_CONTEXT_MASK) >>
                   (MPIDI_CH4U_TAG_SHIFT + MPIDI_CH4U_SOURCE_SHIFT)));
}

static inline int MPIDI_CH4U_get_context_index(uint64_t context_id)
{
    int raw_prefix, idx, bitpos, gen_id;
    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;
    gen_id = (idx * MPIR_CONTEXT_INT_BITS) + (31 - bitpos);
    return gen_id;
}

static inline MPIR_Comm *MPIDI_CH4U_context_id_to_comm(uint64_t context_id)
{
    int comm_idx = MPIDI_CH4U_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);
    return MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type];
}

static inline MPIDI_CH4U_rreq_t **MPIDI_CH4U_context_id_to_uelist(uint64_t context_id)
{
    int comm_idx = MPIDI_CH4U_get_context_index(context_id);
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    int is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id);
    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 2);
    return &MPIDI_CH4_Global.comm_req_lists[comm_idx].uelist[is_localcomm][subcomm_type];
}

static inline uint64_t MPIDI_CH4U_generate_win_id(MPIR_Comm * comm_ptr)
{
    /* context id lower bits, window instance upper bits */
    return 1 + (((uint64_t) comm_ptr->context_id) |
                ((uint64_t) ((MPIDI_CH4U_COMM(comm_ptr, window_instance))++) << 32));
}

static inline MPIR_Context_id_t MPIDI_CH4U_win_id_to_context(uint64_t win_id)
{
    /* pick the lower 32-bit to extract context id */
    return (win_id - 1) & 0xffffffff;
}

static inline MPIR_Context_id_t MPIDI_CH4U_win_to_context(const MPIR_Win * win)
{
    return MPIDI_CH4U_win_id_to_context(MPIDI_CH4U_WIN(win, win_id));
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_request_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_request_complete(MPIR_Request * req)
{
    int incomplete;
    MPIR_cc_decr(req->cc_ptr, &incomplete);
    if (!incomplete)
        MPIR_Request_free(req);
}

#ifndef dtype_add_ref_if_not_builtin
#define dtype_add_ref_if_not_builtin(datatype_)                         \
    do {								\
	if ((datatype_) != MPI_DATATYPE_NULL &&				\
	    HANDLE_GET_KIND((datatype_)) != HANDLE_KIND_BUILTIN)	\
	{								\
	    MPIR_Datatype *dtp_ = NULL;					\
	    MPID_Datatype_get_ptr((datatype_), dtp_);			\
	    MPID_Datatype_add_ref(dtp_);				\
	}								\
    } while (0)
#endif

#ifndef dtype_release_if_not_builtin
#define dtype_release_if_not_builtin(datatype_)				\
    do {								\
	if ((datatype_) != MPI_DATATYPE_NULL &&				\
	    HANDLE_GET_KIND((datatype_)) != HANDLE_KIND_BUILTIN)	\
	{								\
	    MPIR_Datatype *dtp_ = NULL;					\
	    MPID_Datatype_get_ptr((datatype_), dtp_);			\
	    MPID_Datatype_release(dtp_);				\
	}								\
    } while (0)
#endif

#define MPIDI_Datatype_get_info(_count, _datatype,                      \
                                _dt_contig_out, _data_sz_out,           \
                                _dt_ptr, _dt_true_lb)                   \
    do {								\
	if (IS_BUILTIN(_datatype))					\
	{								\
	    (_dt_ptr)        = NULL;					\
	    (_dt_contig_out) = TRUE;					\
	    (_dt_true_lb)    = 0;					\
	    (_data_sz_out)   = (size_t)(_count) *		\
		MPID_Datatype_get_basic_size(_datatype);		\
	}								\
	else								\
	{								\
	    MPID_Datatype_get_ptr((_datatype), (_dt_ptr));		\
            if (_dt_ptr)                                                \
            {                                                           \
                (_dt_contig_out) = (_dt_ptr)->is_contig;                \
                (_dt_true_lb)    = (_dt_ptr)->true_lb;                  \
                (_data_sz_out)   = (size_t)(_count) *           \
                    (_dt_ptr)->size;                                    \
            }                                                           \
            else                                                        \
            {                                                           \
                (_dt_contig_out) = 1;                                   \
                (_dt_true_lb)    = 0;                                   \
                (_data_sz_out)   = 0;                                   \
            }								\
        }                                                               \
    } while (0)

#define MPIDI_Datatype_get_size_dt_ptr(_count, _datatype,               \
                                       _data_sz_out, _dt_ptr)           \
    do {								\
	if (IS_BUILTIN(_datatype))					\
	{								\
	    (_dt_ptr)        = NULL;					\
	    (_data_sz_out)   = (size_t)(_count) *		\
		MPID_Datatype_get_basic_size(_datatype);		\
	}								\
	else								\
	{								\
	    MPID_Datatype_get_ptr((_datatype), (_dt_ptr));		\
	    (_data_sz_out)   = (_dt_ptr) ? (size_t)(_count) *   \
                (_dt_ptr)->size : 0;                                    \
	}								\
    } while (0)

#define MPIDI_Datatype_check_contig(_datatype,_dt_contig_out)	\
    do {							\
      if (IS_BUILTIN(_datatype))				\
      {								\
       (_dt_contig_out) = TRUE;					\
       }							\
      else							\
      {								\
       MPIR_Datatype *_dt_ptr;					\
       MPID_Datatype_get_ptr((_datatype), (_dt_ptr));		\
       (_dt_contig_out) = (_dt_ptr) ? (_dt_ptr)->is_contig : 1; \
      }                                                         \
    } while (0)

#define MPIDI_Datatype_check_contig_size(_datatype,_count,              \
                                         _dt_contig_out,                \
                                         _data_sz_out)                  \
    do {								\
      if (IS_BUILTIN(_datatype))					\
      {                                                                 \
	  (_dt_contig_out) = TRUE;					\
	  (_data_sz_out)   = (size_t)(_count) *			\
	      MPID_Datatype_get_basic_size(_datatype);			\
      }                                                                 \
      else								\
      {                                                                 \
	  MPIR_Datatype *_dt_ptr;					\
	  MPID_Datatype_get_ptr((_datatype), (_dt_ptr));		\
          if (_dt_ptr)                                                  \
          {                                                             \
              (_dt_contig_out) = (_dt_ptr)->is_contig;                  \
              (_data_sz_out)   = (size_t)(_count) *             \
                  (_dt_ptr)->size;                                      \
          }                                                             \
          else                                                          \
          {                                                             \
              (_dt_contig_out) = 1;                                     \
              (_data_sz_out)   = 0;                                     \
          }                                                             \
      }                                                                 \
    } while (0)

#define MPIDI_Datatype_check_size(_datatype,_count,_data_sz_out)        \
    do {								\
        if (IS_BUILTIN(_datatype))                                      \
        {                                                               \
            (_data_sz_out)   = (size_t)(_count) *               \
                MPID_Datatype_get_basic_size(_datatype);                \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIR_Datatype *_dt_ptr;                                     \
            MPID_Datatype_get_ptr((_datatype), (_dt_ptr));              \
            (_data_sz_out)   = (_dt_ptr) ? (size_t)(_count) *   \
                (_dt_ptr)->size : 0;                                    \
        }                                                               \
    } while (0)

#define MPIDI_Datatype_check_contig_size_lb(_datatype,_count,           \
                                            _dt_contig_out,             \
                                            _data_sz_out,               \
                                            _dt_true_lb)                \
    do {								\
	if (IS_BUILTIN(_datatype))					\
	{								\
	    (_dt_contig_out) = TRUE;					\
	    (_data_sz_out)   = (size_t)(_count) *		\
		MPID_Datatype_get_basic_size(_datatype);		\
	    (_dt_true_lb)    = 0;					\
	}								\
	else								\
	{								\
	    MPIR_Datatype *_dt_ptr;					\
	    MPID_Datatype_get_ptr((_datatype), (_dt_ptr));		\
            if (_dt_ptr)                                                \
            {                                                           \
                (_dt_contig_out) = (_dt_ptr)->is_contig;                \
                (_data_sz_out)   = (size_t)(_count) *           \
                    (_dt_ptr)->size;                                    \
                (_dt_true_lb)    = (_dt_ptr)->true_lb;                  \
            }                                                           \
            else                                                        \
            {                                                           \
                (_dt_contig_out) = 1;                                   \
                (_data_sz_out)   = 0;                                   \
                (_dt_true_lb)    = 0;                                   \
            }                                                           \
	}								\
    } while (0)

#define MPIDI_Request_create_null_rreq(rreq_, mpi_errno_, FAIL_)        \
  do {                                                                  \
    (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);             \
    if ((rreq_) != NULL) {                                              \
      MPIR_cc_set(&(rreq_)->cc, 0);                                     \
      MPIR_Status_set_procnull(&(rreq_)->status);                       \
    }                                                                   \
    else {                                                              \
      MPIR_ERR_SETANDJUMP(mpi_errno_,MPI_ERR_OTHER,"**nomemreq");       \
    }                                                                   \
  } while (0)

#define IS_BUILTIN(_datatype)				\
    (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)

#ifndef container_of
#define container_of(ptr, type, field)			\
    ((type *) ((char *)ptr - offsetof(type, field)))
#endif

static inline uint64_t MPIDI_CH4U_init_send_tag(MPIR_Context_id_t contextid, int source, int tag)
{
    uint64_t match_bits;
    match_bits = contextid;
    match_bits = (match_bits << MPIDI_CH4U_SOURCE_SHIFT);
    match_bits |= (source & (MPIDI_CH4U_SOURCE_MASK >> MPIDI_CH4U_TAG_SHIFT));
    match_bits = (match_bits << MPIDI_CH4U_TAG_SHIFT);
    match_bits |= (MPIDI_CH4U_TAG_MASK & tag);
    return match_bits;
}

static inline uint64_t MPIDI_CH4U_init_recvtag(uint64_t * mask_bits,
                                               MPIR_Context_id_t contextid, int source, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPIDI_CH4U_PROTOCOL_MASK;
    match_bits = contextid;
    match_bits = (match_bits << MPIDI_CH4U_SOURCE_SHIFT);

    if (MPI_ANY_SOURCE == source) {
        match_bits = (match_bits << MPIDI_CH4U_TAG_SHIFT);
        *mask_bits |= MPIDI_CH4U_SOURCE_MASK;
    }
    else {
        match_bits |= source;
        match_bits = (match_bits << MPIDI_CH4U_TAG_SHIFT);
    }

    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPIDI_CH4U_TAG_MASK;
    else
        match_bits |= (MPIDI_CH4U_TAG_MASK & tag);

    return match_bits;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_valid_group_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_valid_group_rank(MPIR_Comm * comm, int rank, MPIR_Group * grp)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4I_VALID_GROUP_RANK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4I_VALID_GROUP_RANK);

    int lpid;
    int size = grp->size;
    int z;
    int ret;

    if (unlikely(rank == MPI_PROC_NULL)) {
        /* Treat PROC_NULL as always valid */
        ret = 1;
        goto fn_exit;
    }

    MPIDI_NM_comm_get_lpid(comm, rank, &lpid, FALSE);

    for (z = 0; z < size && lpid != grp->lrank_to_lpid[z].lpid; ++z) {
    }

    ret = (z < size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4I_VALID_GROUP_RANK);
  fn_exit:
    return ret;
}

#define MPIDI_CH4R_PROGRESS()                                   \
    do {							\
	mpi_errno = MPIDI_Progress_test();			\
	if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);	\
    } while (0)

#define MPIDI_CH4R_PROGRESS_WHILE(cond)         \
    do {					\
	while (cond)				\
	    MPIDI_CH4R_PROGRESS();              \
    } while (0)

#ifdef HAVE_ERROR_CHECKING
#define MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_WIN(win, sync).target_epoch_type && \
           MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_REFENCE) \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_FENCE; \
            MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_FENCE; \
        }                                                               \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_NONE || \
           MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_POST) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_CHECK_TYPE(win,mpi_errno,stmt)            \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
           MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_START_CHECK(win,mpi_errno,stmt)                \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_START && \
            !MPIDI_CH4I_valid_group_rank(win->comm_ptr, target_rank,    \
                                         MPIDI_CH4U_WIN(win, sync).sc.group)) \
            MPIR_ERR_SETANDSTMT(mpi_errno,                              \
                                MPI_ERR_RMA_SYNC,                       \
                                stmt,                                   \
                                "**rmasync");                           \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_START_CHECK2(win,mpi_errno,stmt)               \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_START) { \
            MPIR_ERR_SETANDSTMT(mpi_errno,                              \
                                MPI_ERR_RMA_SYNC,                       \
                                stmt,                                   \
                                "**rmasync");                           \
        }                                                               \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_FENCE_CHECK(win,mpi_errno,stmt)                \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_WIN(win, sync).target_epoch_type) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        if (!(massert & MPI_MODE_NOPRECEDE) &&                          \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_FENCE && \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE && \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_NONE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_POST_CHECK(win,mpi_errno,stmt)            \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).target_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
           MPIDI_CH4U_WIN(win, sync).target_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_LOCK_CHECK(win,mpi_errno,stmt)               \
do {                                                                  \
    MPID_BEGIN_ERROR_CHECKS;                                      \
    if ((MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK) && \
       (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_LOCK_ALL)) \
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,                \
                            stmt, "**rmasync");                 \
    MPID_END_ERROR_CHECKS;                                              \
} while (0)

#define MPIDI_CH4U_EPOCH_FREE_CHECK(win,mpi_errno,stmt)                 \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_WIN(win, sync).target_epoch_type || \
           (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_NONE && \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type != MPIDI_CH4U_EPOTYPE_REFENCE)) \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, stmt, "**rmasync"); \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_ORIGIN_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type != epoch_type)    \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");                     \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#define MPIDI_CH4U_EPOCH_TARGET_CHECK(win, epoch_type, mpi_errno, stmt) \
    do {                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                        \
        if (MPIDI_CH4U_WIN(win, sync).target_epoch_type != epoch_type)    \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,            \
                                stmt, "**rmasync");         \
        MPID_END_ERROR_CHECKS;                                          \
    } while (0)

#else /* HAVE_ERROR_CHECKING */
#define MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_CHECK_TYPE(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_START_CHECK2(win, mpi_errno, stmt)             if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_FENCE_CHECK(win, mpi_errno, stmt)              if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_POST_CHECK(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_FREE_CHECK(win, mpi_errno, stmt)               if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_ORIGIN_CHECK(win, epoch_type, mpi_errno, stmt) if (0) goto fn_fail;
#define MPIDI_CH4U_EPOCH_TARGET_CHECK(win, epoch_type, mpi_errno, stmt) if (0) goto fn_fail;
#endif /* HAVE_ERROR_CHECKING */

#define MPIDI_CH4U_EPOCH_FENCE_EVENT(win, massert)                 \
    do {                                                                \
        if (massert & MPI_MODE_NOSUCCEED)                           \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
            MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
        }                                                               \
        else                                                            \
        {                                                               \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
            MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
        }                                                               \
    } while (0)

#define MPIDI_CH4U_EPOCH_TARGET_EVENT(win)                              \
    do {                                                            \
        if (MPIDI_CH4U_WIN(win, sync).target_epoch_type == MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
        else                                                            \
            MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
    } while (0)

#define MPIDI_CH4U_EPOCH_ORIGIN_EVENT(Win)                              \
    do {                                                                \
        if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_REFENCE) \
            MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_REFENCE; \
        else                                                            \
            MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_NONE; \
    } while (0)

/*
  Calculate base address of the target window at the origin side
  Return zero to let the target side calculate the actual address
  (only offset from window base is given to the target in this case)
*/
static inline uintptr_t MPIDI_CH4I_win_base_at_origin(const MPIR_Win * win, int target_rank)
{
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
    return (uintptr_t) win->base;
}

#endif /* CH4_IMPL_H_INCLUDED */

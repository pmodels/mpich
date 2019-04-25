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
#ifndef OFI_IMPL_H_INCLUDED
#define OFI_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "ofi_types.h"
#include "mpidch4r.h"
#include "mpidig.h"
#include "ch4_impl.h"
#include "ofi_iovec_util.h"

#define MPIDI_OFI_ENAVAIL   -1  /* OFI resource not available */
#define MPIDI_OFI_EPERROR   -2  /* OFI endpoint error */

#define MPIDI_OFI_DT(dt)         ((dt)->dev.netmod.ofi)
#define MPIDI_OFI_OP(op)         ((op)->dev.netmod.ofi)
#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)
#define MPIDI_OFI_COMM_TO_INDEX(comm,rank) \
    MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_OFI_AV_TO_PHYS(av) (MPIDI_OFI_AV(av).dest)
#define MPIDI_OFI_COMM_TO_PHYS(comm,rank)                       \
    MPIDI_OFI_AV(MPIDIU_comm_rank_to_av((comm), (rank))).dest
#define MPIDI_OFI_TO_PHYS(avtid, lpid)                                 \
    MPIDI_OFI_AV(&MPIDIU_get_av((avtid), (lpid))).dest

#define MPIDI_OFI_WIN(win)     ((win)->dev.netmod.ofi)

/* Get op index.
 * TODO: OP_NULL is the oddball. Change configure to table this correctly */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_mpi_acc_op_index(int op)
{
    int op_index;
    if (op == MPI_OP_NULL)
        op_index = MPIDI_OFI_OP_SIZES - 1;
    else
        op_index = (0x000000FFU & op) - 1;
    return op_index;
}

/*
 * Helper routines and macros for request completion
 */
#define MPIDI_OFI_PROGRESS()                                      \
    do {                                                          \
        mpi_errno = MPIDI_NM_progress(0, 0);                      \
        if (mpi_errno!=MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);      \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_PROGRESS_WHILE(cond)                 \
    while (cond) MPIDI_OFI_PROGRESS()

#define MPIDI_OFI_ERR  MPIR_ERR_CHKANDJUMP4
#define MPIDI_OFI_CALL(FUNC,STR)                                     \
    do {                                                    \
        ssize_t _ret = FUNC;                                \
        MPIDI_OFI_ERR(_ret<0,                       \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                       \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_CALL_NOLOCK(FUNC,STR)                              \
    do {                                                    \
        ssize_t _ret = FUNC;                                \
        MPIDI_OFI_ERR(_ret<0,                       \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                       \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_CALL_LOCK 1
#define MPIDI_OFI_CALL_NO_LOCK 0
#define MPIDI_OFI_CALL_RETRY(FUNC,STR,LOCK,EAGAIN)          \
    do {                                                    \
    ssize_t _ret;                                           \
    int _retry = MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY;        \
    do {                                                    \
        _ret = FUNC;                                        \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                       \
                              fi_strerror(-_ret));          \
        MPIR_ERR_CHKANDJUMP(_retry == 0 && EAGAIN,          \
                            mpi_errno,                      \
                            MPIX_ERR_EAGAIN,                \
                            "**eagain");                    \
        /* FIXME: by fixing the recursive locking interface to account
         * for recursive locking in more than one lock (currently limited
         * to one due to scalar TLS counter), this lock yielding
         * operation can be avoided since we are inside a finite loop. */\
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);         \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);        \
        if (mpi_errno != MPI_SUCCESS)                                \
            MPIR_ERR_POP(mpi_errno);                                 \
        _retry--;                                           \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETRY2(FUNC1,FUNC2,STR)                       \
    do {                                                    \
    ssize_t _ret;                                           \
    FUNC1;                                                  \
    do {                                                    \
        _ret = FUNC2;                                       \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                       \
                              fi_strerror(-_ret));          \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);         \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);        \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);\
        if (mpi_errno != MPI_SUCCESS)                                \
            MPIR_ERR_POP(mpi_errno);                                 \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETURN(FUNC, _ret)                               \
        do {                                                            \
            (_ret) = FUNC;                                              \
        } while (0)

#define MPIDI_OFI_PMI_CALL_POP(FUNC,STR)                    \
  do                                                          \
    {                                                         \
      pmi_errno  = FUNC;                                      \
      MPIDI_OFI_ERR(pmi_errno!=PMI_SUCCESS,           \
                            mpi_errno,                        \
                            MPI_ERR_OTHER,                    \
                            "**ofid_"#STR,                    \
                            "**ofid_"#STR" %s %d %s %s",      \
                            __SHORT_FILE__,                   \
                            __LINE__,                         \
                            __func__,                           \
                            #STR);                            \
    } while (0)

#define MPIDI_OFI_MPI_CALL_POP(FUNC)                               \
  do                                                                 \
    {                                                                \
      mpi_errno = FUNC;                                              \
      if (unlikely(mpi_errno!=MPI_SUCCESS)) MPIR_ERR_POP(mpi_errno); \
    } while (0)

#define MPIDI_OFI_STR_CALL(FUNC,STR)                                   \
  do                                                            \
    {                                                           \
      str_errno = FUNC;                                         \
      MPIDI_OFI_ERR(str_errno!=MPL_STR_SUCCESS,        \
                            mpi_errno,                          \
                            MPI_ERR_OTHER,                      \
                            "**"#STR,                           \
                            "**"#STR" %s %d %s %s",             \
                            __SHORT_FILE__,                     \
                            __LINE__,                           \
                            __func__,                             \
                            #STR);                              \
    } while (0)

#define MPIDI_OFI_REQUEST_CREATE(req, kind)                 \
    do {                                                      \
        (req) = MPIR_Request_create(kind);  \
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq"); \
        MPIR_Request_add_ref((req));                                \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_need_request_creation(const MPIR_Request * req)
{
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_TRYLOCK) {
        return (req == NULL);   /* Depends on upper layer */
    } else if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_DIRECT) {
        return 1;       /* Always allocated by netmod */
    } else if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF) {
        return (req == NULL);
    } else {
        /* Invalid MT model */
        MPIR_Assert(0);
        return -1;
    }
}

/* Initial value of the completion counter of request objects for lightweight (injection) operations */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_lw_request_cc_val(void)
{
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_TRYLOCK) {
        /* Note on CC initialization: this might be overkill when trylock succeeds
         * and the main thread directly issues injection.
         * However, at this moment we assume trylock always goes through progress thread
         * to simplify implementation. */
        return 1;
    } else if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_DIRECT) {
        return 0;
    } else if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF) {
        return 1;
    } else {
        /* Invalid MT model */
        MPIR_Assert(0);
        return -1;
    }
}

#define MPIDI_OFI_REQUEST_CREATE_CONDITIONAL(req, kind)                 \
      do {                                                              \
          if (MPIDI_OFI_need_request_creation(req)) {                   \
              MPIR_Assert(MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_DIRECT ||  \
                          (req) == NULL);                               \
              (req) = MPIR_Request_create(kind);                        \
              MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, \
                                  "**nomemreq");                        \
          }                                                             \
          /* At this line we should always have a valid request */      \
          MPIR_Assert((req) != NULL);                                   \
          MPIR_Request_add_ref((req));                                  \
      } while (0)

#define MPIDI_OFI_SEND_REQUEST_CREATE_LW_CONDITIONAL(req)               \
    do {                                                                \
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_DIRECT) {                \
            (req) = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND); \
        } else {                                                        \
            if (MPIDI_OFI_need_request_creation(req)) {                 \
                MPIR_Assert((req) == NULL);                             \
                (req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);   \
                MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, \
                                    "**nomemreq");                      \
            }                                                           \
            /* At this line we should always have a valid request */    \
            MPIR_Assert((req) != NULL);                                 \
            /* Completing lightweight (injection) requests:             \
               If a progess thread is issuing injection, we need to     \
               keep CC>0 until the actual injection completes. */       \
            MPIR_cc_set(&(req)->cc, MPIDI_OFI_lw_request_cc_val());     \
        }                                                               \
    } while (0)

/* If we set CC>0 in case of injection, we need to decrement the CC
   to tell the main thread we completed the injection. */
#define MPIDI_OFI_SEND_REQUEST_COMPLETE_LW_CONDITIONAL(req) \
    do {                                                    \
        if (MPIDI_OFI_lw_request_cc_val()) {                \
            int incomplete_;                                \
            MPIR_cc_decr(&(req)->cc, &incomplete_);         \
        }                                                   \
    } while (0)

MPL_STATIC_INLINE_PREFIX uintptr_t MPIDI_OFI_winfo_base(MPIR_Win * w, int rank)
{
    if (MPIDI_OFI_ENABLE_MR_SCALABLE)
        return 0;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].base;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_winfo_mr_key(MPIR_Win * w, int rank)
{
    if (MPIDI_OFI_ENABLE_MR_SCALABLE)
        return MPIDI_OFI_WIN(w).mr_key;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].mr_key;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_cntr_incr(MPIR_Win * win)
{
    (*MPIDI_OFI_WIN(win).issued_cntr)++;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_incr()
{
    MPIDI_OFI_global.rma_issued_cntr++;
}

/* Externs:  see util.c for definition */
int MPIDI_OFI_handle_cq_error_util(int ep_idx, ssize_t ret);
int MPIDI_OFI_retry_progress(void);
int MPIDI_OFI_control_handler(int handler_id, void *am_hdr,
                              void **data, size_t * data_sz, int is_local, int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_OFI_control_dispatch(void *buf);
void MPIDI_OFI_index_datatypes(void);
void MPIDI_OFI_mr_key_allocator_init(void);
uint64_t MPIDI_OFI_mr_key_alloc(void);
void MPIDI_OFI_mr_key_free(uint64_t index);
void MPIDI_OFI_mr_key_allocator_destroy(void);

/* Common Utility functions used by the
 * C and C++ components
 */
/* Set max size based on OFI acc ordering limit. */
MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_check_acc_order_size(MPIR_Win * win, size_t max_size)
{
    /* Check ordering limit, a value of -1 guarantees ordering for any data size. */
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAR)
        && MPIDI_OFI_global.max_order_war != -1) {
        /* An order size value of 0 indicates that ordering is not guaranteed. */
        MPIR_Assert(MPIDI_OFI_global.max_order_war != 0);
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_war);
    }
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAW)
        && MPIDI_OFI_global.max_order_waw != -1) {
        MPIR_Assert(MPIDI_OFI_global.max_order_waw != 0);
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_waw);
    }
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW)
        && MPIDI_OFI_global.max_order_raw != -1) {
        MPIR_Assert(MPIDI_OFI_global.max_order_raw != 0);
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_raw);
    }
    return max_size;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_request_complete(MPIDI_OFI_win_request_t * req)
{
    int in_use;
    MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST);
    MPIR_Object_release_ref(req, &in_use);
    if (!in_use) {
        MPL_free(req->noncontig);
        MPIR_Handle_obj_free(&MPIR_Request_mem, (req));
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_comm_to_phys(MPIR_Comm * comm, int rank)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm, rank));
        int ep_num = MPIDI_OFI_av_to_ep(av);
        int rx_idx = ep_num;
        return fi_rx_addr(av->dest, rx_idx, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_COMM_TO_PHYS(comm, rank);
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_av_to_phys(MPIDI_av_entry_t * av)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int ep_num = MPIDI_OFI_av_to_ep(&MPIDI_OFI_AV(av));
        return fi_rx_addr(MPIDI_OFI_AV_TO_PHYS(av), ep_num, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_AV_TO_PHYS(av);
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_to_phys(int rank)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int ep_num = 0;
        int rx_idx = ep_num;
        return fi_rx_addr(MPIDI_OFI_TO_PHYS(0, rank), rx_idx, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_TO_PHYS(0, rank);
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_is_tag_sync(uint64_t match_bits)
{
    return (0 != (MPIDI_OFI_SYNC_SEND & match_bits));
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_sendtag(MPIR_Context_id_t contextid,
                                                         int source, int tag, uint64_t type)
{
    uint64_t match_bits;
    match_bits = contextid;

    match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
    match_bits |= (MPIDI_OFI_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_recvtag(uint64_t * mask_bits,
                                                         MPIR_Context_id_t contextid,
                                                         int source, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPIDI_OFI_PROTOCOL_MASK;
    match_bits = contextid;

    match_bits = (match_bits << MPIDI_OFI_TAG_BITS);

    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPIDI_OFI_TAG_MASK;
    else
        match_bits |= (MPIDI_OFI_TAG_MASK & tag);

    return match_bits;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_init_get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPIDI_OFI_TAG_MASK));
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_OFI_context_to_request(void *context)
{
    char *base = (char *) context;
    return (MPIR_Request *) MPL_container_of(base, MPIR_Request, dev.ch4.netmod);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_handler(struct fid_ep *ep, const void *buf, size_t len,
                                                    void *desc, uint32_t src, fi_addr_t dest_addr,
                                                    uint64_t tag, void *context, int is_inject,
                                                    int do_lock, int do_eagain)
{
    int mpi_errno = MPI_SUCCESS;

    if (is_inject) {
        MPIDI_OFI_CALL_RETRY(fi_tinjectdata(ep, buf, len, src, dest_addr, tag), tinjectdata,
                             do_lock, do_eagain);
    } else {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(ep, buf, len, desc, src, dest_addr, tag, context),
                             tsenddata, do_lock, do_eagain);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct MPIDI_OFI_contig_blocks_params {
    size_t max_pipe;
    MPI_Aint count;
    MPI_Aint last_loc;
    MPI_Aint start_loc;
    size_t last_chunk;
};

MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_count_iov(int dt_count,       /* number of data elements in dt_datatype */
                                                    MPI_Datatype dt_datatype, size_t total_bytes,       /* total byte size, passed in here for reusing */
                                                    size_t max_pipe)
{
    MPIR_Segment *dt_seg;
    ssize_t rem_size = total_bytes;
    size_t num_iov, total_iov = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOV);

    if (dt_datatype == MPI_DATATYPE_NULL)
        goto fn_exit;

    dt_seg = MPIR_Segment_alloc(NULL, dt_count, dt_datatype);

    do {
        size_t tmp_size = (rem_size > max_pipe) ? max_pipe : rem_size;

        MPIR_Segment_count_contig_blocks(dt_seg, 0, &tmp_size, &num_iov);
        total_iov += num_iov;

        rem_size -= tmp_size;
    } while (rem_size);
    MPIR_Segment_free(dt_seg);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    return total_iov;
}

/* Find the nearest length of iov that meets alignment requirement */
MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_align_iov_len(size_t len)
{
    size_t pad = MPIDI_OFI_IOVEC_ALIGN - 1;
    size_t mask = ~pad;

    return (len + pad) & mask;
}

/* Find the minimum address that is >= ptr && meets alignment requirement */
MPL_STATIC_INLINE_PREFIX void *MPIDI_OFI_aligned_next_iov(void *ptr)
{
    size_t aligned_iov = MPIDI_OFI_align_iov_len((size_t) ptr);
    return (void *) (uintptr_t) aligned_iov;
}

MPL_STATIC_INLINE_PREFIX struct iovec *MPIDI_OFI_request_util_iov(MPIR_Request * req)
{
#if defined (MPL_HAVE_VAR_ATTRIBUTE_ALIGNED)
    return &MPIDI_OFI_REQUEST(req, util.iov);
#else
    return (struct iovec *) MPIDI_OFI_aligned_next_iov(&MPIDI_OFI_REQUEST(req, util.iov_store));
#endif
}

/* Make sure `p` is properly aligned */
#define MPIDI_OFI_ASSERT_IOVEC_ALIGN(p)                                 \
    do {                                                                \
        MPIR_Assert((((uintptr_t)(void *) p) & (MPIDI_OFI_IOVEC_ALIGN - 1)) == 0); \
    } while (0)

#endif /* OFI_IMPL_H_INCLUDED */

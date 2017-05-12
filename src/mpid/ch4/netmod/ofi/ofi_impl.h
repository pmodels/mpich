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
#define MPIDI_OFI_MPI_ACCU_OP_INDEX(op, op_index) do {   \
    if (op == MPI_OP_NULL) op_index = 14;                \
    else op_index = (0x000000FFU & op) - 1;              \
} while (0);

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
        MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
        ssize_t _ret = FUNC;                                \
        MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        MPIDI_OFI_ERR(_ret<0,                       \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
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
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_CALL_LOCK 1
#define MPIDI_OFI_CALL_NO_LOCK 0
#define MPIDI_OFI_CALL_RETRY(FUNC,STR,LOCK,EAGAIN)          \
    do {                                                    \
    ssize_t _ret;                                           \
    int _retry = MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY;        \
    do {                                                    \
        if (LOCK == MPIDI_OFI_CALL_LOCK)                    \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
        _ret = FUNC;                                        \
        if (LOCK == MPIDI_OFI_CALL_LOCK)                    \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
        MPIR_ERR_CHKANDJUMP(_retry == 0 && EAGAIN,          \
                            mpi_errno,                      \
                            MPIX_ERR_EAGAIN,                \
                            "**eagain");                    \
        if (LOCK == MPIDI_OFI_CALL_NO_LOCK)                 \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);     \
        /* FIXME: by fixing the recursive locking interface to account
         * for recursive locking in more than one lock (currently limited
         * to one due to scalar TLS counter), this lock yielding
         * operation can be avoided since we are inside a finite loop. */\
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);         \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);        \
        if (mpi_errno != MPI_SUCCESS)                                \
            MPIR_ERR_POP(mpi_errno);                                 \
        if (LOCK == MPIDI_OFI_CALL_NO_LOCK)                 \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        _retry--;                                           \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETRY2(FUNC1,FUNC2,STR)                       \
    do {                                                    \
    ssize_t _ret;                                           \
    MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);       \
    FUNC1;                                                  \
    do {                                                    \
        _ret = FUNC2;                                       \
        MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);         \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);        \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);\
        if (mpi_errno != MPI_SUCCESS)                                \
            MPIR_ERR_POP(mpi_errno);                                 \
        MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETURN(FUNC, _ret)                               \
        do {                                                            \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);       \
            (_ret) = FUNC;                                              \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);        \
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
                            FCNAME,                           \
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
                            FCNAME,                             \
                            #STR);                              \
    } while (0)

#define MPIDI_OFI_REQUEST_CREATE(req, kind)                 \
    do {                                                      \
        (req) = MPIR_Request_create(kind);  \
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq"); \
        MPIR_Request_add_ref((req));                                \
    } while (0)

#ifndef HAVE_DEBUGGER_SUPPORT
#define MPIDI_OFI_SEND_REQUEST_CREATE_LW(req)                   \
    do {                                                                \
        (req) = MPIDI_Global.lw_send_req;                               \
        MPIR_Request_add_ref((req));                                    \
    } while (0)
#else
#define MPIDI_OFI_SEND_REQUEST_CREATE_LW(req)                   \
    do {                                                                \
        (req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);           \
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq"); \
        MPIR_cc_set(&(req)->cc, 0);                                     \
    } while (0)
#endif

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
            MPIDI_OFI_SEND_REQUEST_CREATE_LW(req);                      \
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

#define WINFO(w,rank) MPIDI_CH4U_WINFO(w,rank)

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
    MPIDI_Global.rma_issued_cntr++;
}

/* Externs:  see util.c for definition */
int MPIDI_OFI_handle_cq_error_util(int ep_idx, ssize_t ret);
int MPIDI_OFI_retry_progress(void);
int MPIDI_OFI_control_handler(int handler_id, void *am_hdr,
                              void **data, size_t * data_sz, int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_OFI_control_dispatch(void *buf);
void MPIDI_OFI_index_datatypes(void);
void MPIDI_OFI_index_allocator_create(void **_indexmap, int start, MPL_memory_class class);
int MPIDI_OFI_index_allocator_alloc(void *_indexmap, MPL_memory_class class);
void MPIDI_OFI_index_allocator_free(void *_indexmap, int index);
void MPIDI_OFI_index_allocator_destroy(void *_indexmap);

/* Common Utility functions used by the
 * C and C++ components
 */
/* Set max size based on OFI acc ordering limit. */
MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_check_acc_order_size(MPIR_Win * win, size_t max_size)
{
    /* Check ordering limit, a value of -1 guarantees ordering for any data size. */
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAR)
        && MPIDI_Global.max_order_war != -1) {
        /* An order size value of 0 indicates that ordering is not guaranteed. */
        MPIR_Assert(MPIDI_Global.max_order_war != 0);
        max_size = MPL_MIN(max_size, MPIDI_Global.max_order_war);
    }
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAW)
        && MPIDI_Global.max_order_waw != -1) {
        MPIR_Assert(MPIDI_Global.max_order_waw != 0);
        max_size = MPL_MIN(max_size, MPIDI_Global.max_order_waw);
    }
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAW)
        && MPIDI_Global.max_order_raw != -1) {
        MPIR_Assert(MPIDI_Global.max_order_raw != 0);
        max_size = MPL_MIN(max_size, MPIDI_Global.max_order_raw);
    }
    return max_size;
}

/* Set OFI attributes and capabilities for RMA. */
MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_set_rma_fi_info(MPIR_Win * win, struct fi_info *finfo)
{
    finfo->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
    finfo->tx_attr->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;

    /* Update msg_order by accumulate ordering in window info.
     * Accumulate ordering cannot easily be changed once the window has been created.
     * OFI implementation ignores acc ordering hints issued in MPI_WIN_SET_INFO()
     * after window is created. */
    finfo->tx_attr->msg_order = FI_ORDER_NONE;  /* FI_ORDER_NONE is an alias for the value 0 */
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAR) ==
        MPIDI_CH4I_ACCU_ORDER_RAR)
        finfo->tx_attr->msg_order |= FI_ORDER_RAR;
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAW) ==
        MPIDI_CH4I_ACCU_ORDER_RAW)
        finfo->tx_attr->msg_order |= FI_ORDER_RAW;
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAR) ==
        MPIDI_CH4I_ACCU_ORDER_WAR)
        finfo->tx_attr->msg_order |= FI_ORDER_WAR;
    if ((MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAW) ==
        MPIDI_CH4I_ACCU_ORDER_WAW)
        finfo->tx_attr->msg_order |= FI_ORDER_WAW;
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

    if (!MPIDI_OFI_ENABLE_DATA) {
        match_bits = (match_bits << MPIDI_OFI_SOURCE_BITS);
        match_bits |= source;
    }

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

    if (!MPIDI_OFI_ENABLE_DATA) {
        match_bits = (match_bits << MPIDI_OFI_SOURCE_BITS);

        if (MPI_ANY_SOURCE == source) {
            match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
            *mask_bits |= MPIDI_OFI_SOURCE_MASK;
        } else {
            match_bits |= source;
            match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
        }
    } else {
        match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
    }

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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_init_get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_OFI_SOURCE_MASK) >> MPIDI_OFI_TAG_BITS));
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_OFI_context_to_request(void *context)
{
    char *base = (char *) context;
    return (MPIR_Request *) MPL_container_of(base, MPIR_Request, dev.ch4.netmod);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_handler(struct fid_ep *ep, const void *buf, size_t len,
                                                    void *desc, uint32_t src, fi_addr_t dest_addr,
                                                    uint64_t tag, void *context, int is_inject,
                                                    int do_lock, int do_eagain)
{
    int mpi_errno = MPI_SUCCESS;

    if (is_inject) {
        if (MPIDI_OFI_ENABLE_DATA)
            MPIDI_OFI_CALL_RETRY(fi_tinjectdata(ep, buf, len, src, dest_addr, tag), tinjectdata,
                                 do_lock, do_eagain);
        else
            MPIDI_OFI_CALL_RETRY(fi_tinject(ep, buf, len, dest_addr, tag), tinject, do_lock,
                                 do_eagain);
    } else {
        if (MPIDI_OFI_ENABLE_DATA)
            MPIDI_OFI_CALL_RETRY(fi_tsenddata(ep, buf, len, desc, src, dest_addr, tag, context),
                                 tsenddata, do_lock, do_eagain);
        else
            MPIDI_OFI_CALL_RETRY(fi_tsend(ep, buf, len, desc, dest_addr, tag, context), tsend,
                                 do_lock, do_eagain);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_conn_manager_insert_conn(fi_addr_t conn, int rank, int state)
{
    int conn_id = -1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);

    /* We've run out of space in the connection table. Allocate more. */
    if (MPIDI_Global.conn_mgr.next_conn_id == -1) {
        int old_max, new_max, i;
        old_max = MPIDI_Global.conn_mgr.max_n_conn;
        new_max = old_max + 1;
        MPIDI_Global.conn_mgr.free_conn_id =
            (int *) MPL_realloc(MPIDI_Global.conn_mgr.free_conn_id, new_max * sizeof(int),
                                MPL_MEM_ADDRESS);
        for (i = old_max; i < new_max - 1; ++i) {
            MPIDI_Global.conn_mgr.free_conn_id[i] = i + 1;
        }
        MPIDI_Global.conn_mgr.free_conn_id[new_max - 1] = -1;
        MPIDI_Global.conn_mgr.max_n_conn = new_max;
        MPIDI_Global.conn_mgr.next_conn_id = old_max;
    }

    conn_id = MPIDI_Global.conn_mgr.next_conn_id;
    MPIDI_Global.conn_mgr.next_conn_id = MPIDI_Global.conn_mgr.free_conn_id[conn_id];
    MPIDI_Global.conn_mgr.free_conn_id[conn_id] = -1;
    MPIDI_Global.conn_mgr.n_conn++;

    MPIR_Assert(MPIDI_Global.conn_mgr.n_conn <= MPIDI_Global.conn_mgr.max_n_conn);

    MPIDI_Global.conn_mgr.conn_list[conn_id].dest = conn;
    MPIDI_Global.conn_mgr.conn_list[conn_id].rank = rank;
    MPIDI_Global.conn_mgr.conn_list[conn_id].state = state;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_conn_id=%d for conn=%" PRIu64 " rank=%d state=%d",
                     conn_id, conn, rank, MPIDI_Global.conn_mgr.conn_list[conn_id].state));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);
    return conn_id;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_conn_manager_remove_conn(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);

    MPIR_Assert(MPIDI_Global.conn_mgr.n_conn > 0);
    MPIDI_Global.conn_mgr.free_conn_id[conn_id] = MPIDI_Global.conn_mgr.next_conn_id;
    MPIDI_Global.conn_mgr.next_conn_id = conn_id;
    MPIDI_Global.conn_mgr.n_conn--;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " free_conn_id=%d", conn_id));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_dynproc_send_disconnect(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Context_id_t context_id = 0xF000;
    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    int close_msg = 0xcccccccc;
    int rank = MPIDI_Global.conn_mgr.conn_list[conn_id].rank;
    struct fi_msg_tagged msg;
    struct iovec msg_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);

    if (MPIDI_Global.conn_mgr.conn_list[conn_id].state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " send disconnect msg conn_id=%d from child side",
                         conn_id));
        match_bits = MPIDI_OFI_init_sendtag(context_id, rank, 1, MPIDI_OFI_DYNPROC_SEND);

        /* fi_av_map here is not quite right for some providers */
        /* we need to get this connection from the sockname     */
        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        msg_iov.iov_base = &close_msg;
        msg_iov.iov_len = sizeof(close_msg);
        msg.msg_iov = &msg_iov;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = MPIDI_Global.conn_mgr.conn_list[conn_id].dest;
        msg.tag = match_bits;
        msg.ignore = context_id;
        msg.context = (void *) &req.context;
        msg.data = 0;
        MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_Global.ctx[0].tx, &msg,
                                         FI_COMPLETION | FI_TRANSMIT_COMPLETE |
                                         (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0)), tsendmsg,
                             MPIDI_OFI_CALL_LOCK, FALSE);
        MPIDI_OFI_PROGRESS_WHILE(!req.done);
    }

    switch (MPIDI_Global.conn_mgr.conn_list[conn_id].state) {
        case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
            MPIDI_Global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD;
            break;
        case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
            MPIDI_Global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT;
            break;
        default:
            break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " local_disconnected conn_id=%d state=%d",
                     conn_id, MPIDI_Global.conn_mgr.conn_list[conn_id].state));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct MPIDI_OFI_contig_blocks_params {
    size_t max_pipe;
    DLOOP_Count count;
    DLOOP_Offset last_loc;
    DLOOP_Offset start_loc;
    size_t last_chunk;
};

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_contig_count_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_contig_count_block(DLOOP_Offset * blocks_p,
                                     DLOOP_Type el_type,
                                     DLOOP_Offset rel_off, DLOOP_Buffer bufp, void *v_paramp)
{
    DLOOP_Offset size, el_size;
    size_t rem, num;
    struct MPIDI_OFI_contig_blocks_params *paramp = v_paramp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);

    DLOOP_Assert(*blocks_p > 0);

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;
    if (paramp->count > 0 && rel_off == paramp->last_loc) {
        /* this region is adjacent to the last */
        paramp->last_loc += size;
        /* If necessary, recalculate the number of chunks in this block */
        if (paramp->last_loc - paramp->start_loc > paramp->max_pipe) {
            paramp->count -= paramp->last_chunk;
            num = (paramp->last_loc - paramp->start_loc) / paramp->max_pipe;
            rem = (paramp->last_loc - paramp->start_loc) % paramp->max_pipe;
            if (rem)
                num++;
            paramp->last_chunk = num;
            paramp->count += num;
        }
    } else {
        /* new region */
        num = size / paramp->max_pipe;
        rem = size % paramp->max_pipe;
        if (rem)
            num++;

        paramp->last_chunk = num;
        paramp->last_loc = rel_off + size;
        paramp->start_loc = rel_off;
        paramp->count += num;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_count_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    size_t MPIDI_OFI_count_iov(int dt_count, MPI_Datatype dt_datatype, size_t max_pipe)
{
    struct MPIDI_OFI_contig_blocks_params params;
    MPIR_Segment dt_seg;
    ssize_t dt_size, num, rem;
    size_t dtc, count, count1, count2;;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOV);

    count = 0;
    dtc = MPL_MIN(2, dt_count);
    params.max_pipe = max_pipe;
    MPIDI_Datatype_check_size(dt_datatype, dtc, dt_size);
    if (dtc) {
        params.count = 0;
        params.last_loc = 0;
        params.start_loc = 0;
        params.last_chunk = 0;
        MPIR_Segment_init(NULL, 1, dt_datatype, &dt_seg);
        MPIR_Segment_manipulate(&dt_seg, 0, &dt_size,
                                MPIDI_OFI_contig_count_block,
                                NULL, NULL, NULL, NULL, (void *) &params);
        count1 = params.count;
        params.count = 0;
        params.last_loc = 0;
        params.start_loc = 0;
        params.last_chunk = 0;
        MPIR_Segment_init(NULL, dtc, dt_datatype, &dt_seg);
        MPIR_Segment_manipulate(&dt_seg, 0, &dt_size,
                                MPIDI_OFI_contig_count_block,
                                NULL, NULL, NULL, NULL, (void *) &params);
        count2 = params.count;
        if (count2 == 1) {      /* Contiguous */
            num = (dt_size * dt_count) / max_pipe;
            rem = (dt_size * dt_count) % max_pipe;
            if (rem)
                num++;
            count += num;
        } else if (count2 < count1 * 2)
            /* The commented calculation assumes merged blocks  */
            /* The iov processor will not merge adjacent blocks */
            /* When the iov state machine adds this capability  */
            /* we should switch to use the optimized calcultion */
            /* count += (count1*dt_count) - (dt_count-1);       */
            count += count1 * dt_count;
        else
            count += count1 * dt_count;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    return count;
}

/* Find the nearest length of iov that meets alignment requirement */
#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_align_iov_len
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_align_iov_len(size_t len)
{
    size_t pad = MPIDI_OFI_IOVEC_ALIGN - 1;
    size_t mask = ~pad;

    return (len + pad) & mask;
}

/* Find the minimum address that is >= ptr && meets alignment requirement */
#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_aligned_next_iov
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_OFI_aligned_next_iov(void *ptr)
{
    return (void *) (uintptr_t) MPIDI_OFI_align_iov_len((size_t) ptr);
}

#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_request_util_iov
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
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

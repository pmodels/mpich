/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_IMPL_H_INCLUDED
#define OFI_IMPL_H_INCLUDED

#include <mpidimpl.h>
/* NOTE: headers with global struct need be included before ofi_types.h */
#include "ofi_types.h"
#include "mpidch4r.h"
#include "mpidig_am.h"
#include "ch4_impl.h"

extern unsigned long long PVAR_COUNTER_nic_sent_bytes_count[MPIDI_OFI_MAX_NICS] ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_nic_recvd_bytes_count[MPIDI_OFI_MAX_NICS]
ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_striped_nic_sent_bytes_count[MPIDI_OFI_MAX_NICS]
ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_striped_nic_recvd_bytes_count[MPIDI_OFI_MAX_NICS]
ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_rma_pref_phy_nic_put_bytes_count[MPIDI_OFI_MAX_NICS]
ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_rma_pref_phy_nic_get_bytes_count[MPIDI_OFI_MAX_NICS]
ATTRIBUTE((unused));


#define MPIDI_OFI_ENAVAIL   -1  /* OFI resource not available */
#define MPIDI_OFI_EPERROR   -2  /* OFI endpoint error */

#define MPIDI_OFI_DT(dt)         ((dt)->dev.netmod.ofi)
#define MPIDI_OFI_OP(op)         ((op)->dev.netmod.ofi)
#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)
#define MPIDI_OFI_COMM_TO_INDEX(comm,rank) \
    MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_OFI_TO_PHYS(avtid, lpid, _nic) \
    MPIDI_OFI_AV(&MPIDIU_get_av((avtid), (lpid))).dest[_nic][0]

#define MPIDI_OFI_WIN(win)     ((win)->dev.netmod.ofi)

int MPIDI_OFI_progress_uninlined(int vni);
int MPIDI_OFI_handle_cq_error(int vni, int nic, ssize_t ret);

/* vni mapping */
/* NOTE: concerned by the modulo? If we restrict num_vnis to power of 2,
 * we may get away with bit mask */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_vni(int flag, MPIR_Comm * comm_ptr,
                                               int src_rank, int dst_rank, int tag)
{
#if MPIDI_CH4_MAX_VCIS == 1
    return 0;
#else
    return MPIDI_get_vci(flag, comm_ptr, src_rank, dst_rank, tag) % MPIDI_OFI_global.num_vnis;
#endif
}

/* for RMA, vni need be persistent with window */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_win_vni(MPIR_Win * win)
{
    int win_idx = 0;
    return MPIDI_get_vci(SRC_VCI_FROM_SENDER, win->comm_ptr, 0, 0, win_idx) %
        MPIDI_OFI_global.num_vnis;
}

/*
 * Helper routines and macros for request completion
 */
#define MPIDI_OFI_PROGRESS(vni)                                   \
    do {                                                          \
        mpi_errno = MPIDI_NM_progress(vni, 0);                   \
        MPIR_ERR_CHECK(mpi_errno);                                \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_PROGRESS_WHILE(cond, vni) \
    while (cond) MPIDI_OFI_PROGRESS(vni)

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

#define MPIDI_OFI_CALL_RETRY(FUNC,vci_,STR,EAGAIN)      \
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
         * operation can be avoided since we are inside a finite loop. */ \
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_);			  \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_);			     \
        MPIR_ERR_CHECK(mpi_errno);                               \
        _retry--;                                           \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

/* per-vci macros - we'll transition into these macros once the locks are
 * moved down to ofi-layer */
#define MPIDI_OFI_VCI_PROGRESS(vci_)                                    \
    do {                                                                \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                \
        mpi_errno = MPIDI_NM_progress(vci_, 0);                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);                 \
        MPIR_ERR_CHECK(mpi_errno);                                      \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_VCI_PROGRESS_WHILE(vci_, cond)                            \
    do {                                                                    \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                    \
        while (cond) {                                                      \
            mpi_errno = MPIDI_NM_progress(vci_, 0);                        \
            if (mpi_errno) {                                                \
                MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);             \
                MPIR_ERR_POP(mpi_errno);                                    \
            }                                                               \
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
        }                                                                   \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);                     \
    } while (0)

#define MPIDI_OFI_VCI_CALL(FUNC,vci_,STR)                   \
    do {                                                    \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);    \
        ssize_t _ret = FUNC;                                \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);     \
        MPIDI_OFI_ERR(_ret<0,                               \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                     \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_VCI_CALL_RETRY(FUNC,vci_,STR,EAGAIN)      \
    do {                                                    \
    ssize_t _ret;                                           \
    int _retry = MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY;        \
    do {                                                    \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);    \
        _ret = FUNC;                                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);     \
        if (likely(_ret==0)) break;                         \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,                     \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              __func__,                     \
                              fi_strerror(-_ret));          \
        MPIR_ERR_CHKANDJUMP(_retry == 0 && EAGAIN,          \
                            mpi_errno,                      \
                            MPIX_ERR_EAGAIN,                \
                            "**eagain");                    \
        mpi_errno = MPID_Progress_test(NULL);                   \
        MPIR_ERR_CHECK(mpi_errno);                          \
        _retry--;                                           \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_)            \
    do {                                                        \
        if (MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_LOCKLESS) {      \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);    \
        }                                                       \
    } while (0)

#define MPIDI_OFI_THREAD_CS_ENTER_REC_VCI_OPTIONAL(vci_)        \
    do {                                                        \
        if (MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_LOCKLESS) {      \
            MPID_THREAD_CS_ENTER_REC_VCI(MPIDI_VCI(vci_).lock);     \
        }                                                       \
    } while (0)

#define MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_)         \
    do {                                                    \
        if (MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_LOCKLESS) {  \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock); \
        }                                                   \
    } while (0)

#define MPIDI_OFI_CALL_RETURN(FUNC, _ret)                               \
        do {                                                            \
            (_ret) = FUNC;                                              \
        } while (0)

#define MPIDI_OFI_STR_CALL(FUNC,STR)                                   \
  do                                                            \
    {                                                           \
      str_errno = FUNC;                                         \
      MPIDI_OFI_ERR(str_errno!=MPL_SUCCESS,        \
                            mpi_errno,                          \
                            MPI_ERR_OTHER,                      \
                            "**"#STR,                           \
                            "**"#STR" %s %d %s %s",             \
                            __SHORT_FILE__,                     \
                            __LINE__,                           \
                            __func__,                             \
                            #STR);                              \
    } while (0)

#define MPIDI_OFI_REQUEST_CREATE(req, kind, vni) \
    do {                                                      \
        MPIDI_CH4_REQUEST_CREATE(req, kind, vni, 2);                    \
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq"); \
    } while (0)

MPL_STATIC_INLINE_PREFIX uintptr_t MPIDI_OFI_winfo_base(MPIR_Win * w, int rank)
{
    if (!MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS)
        return 0;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].base;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_winfo_mr_key(MPIR_Win * w, int rank)
{
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY)
        return MPIDI_OFI_WIN(w).mr_key;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].mr_key;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_cntr_incr(MPIR_Win * win)
{
#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    /* Lockless mode requires to use atomic operation, in order to make
     * cntrs thread-safe. */
    MPL_atomic_fetch_add_uint64(MPIDI_OFI_WIN(win).issued_cntr, 1);
#else
    (*MPIDI_OFI_WIN(win).issued_cntr)++;
#endif
}

/* Calculate the OFI context index.
 * The total number of OFI contexts will be the number of nics * number of vcis
 * Each nic will contain num_vcis vnis. Each corresponding to their respective vci index. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_ctx_index(MPIR_Comm * comm_ptr, int vni, int nic)
{
    if (comm_ptr == NULL || MPIDI_OFI_COMM(comm_ptr).pref_nic == NULL) {
        return nic * MPIDI_OFI_global.num_vnis + vni;
    } else {
        return MPIDI_OFI_COMM(comm_ptr).pref_nic[comm_ptr->rank] * MPIDI_OFI_global.num_vnis + vni;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_incr(MPIR_Comm * comm, int vni, int nic)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni, nic);
#else
    /* NOTE: shared with ctx[0] */
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, 0, nic);
#endif

#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    MPL_atomic_fetch_add_uint64(&MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr, 1);
#else
    MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr++;
#endif
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_set(int ctx_idx, int val)
{
#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    MPL_atomic_store_uint64(&MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr, val);
#else
    MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr = val;
#endif
}

/* Externs:  see util.c for definition */
#define MPIDI_OFI_LOCAL_MR_KEY 0
#define MPIDI_OFI_COLL_MR_KEY 1
#define MPIDI_OFI_INVALID_MR_KEY 0xFFFFFFFFFFFFFFFFULL
int MPIDI_OFI_retry_progress(void);
int MPIDI_OFI_control_handler(void *am_hdr, void *data, MPI_Aint data_sz,
                              uint32_t attr, MPIR_Request ** req);
int MPIDI_OFI_am_rdma_read_ack_handler(void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDI_OFI_control_dispatch(void *buf);
void MPIDI_OFI_index_datatypes(struct fid_ep *ep);
int MPIDI_OFI_mr_key_allocator_init(void);
uint64_t MPIDI_OFI_mr_key_alloc(int key_type, uint64_t requested_key);
void MPIDI_OFI_mr_key_free(int key_type, uint64_t index);
void MPIDI_OFI_mr_key_allocator_destroy(void);

/* RMA */
#define MPIDI_OFI_INIT_CHUNK_CONTEXT(win,sigreq)                        \
    do {                                                                \
        if (sigreq) {                                                   \
            MPIDI_OFI_chunk_request *creq;                              \
            MPIR_cc_inc((*sigreq)->cc_ptr);                             \
            creq=(MPIDI_OFI_chunk_request*)MPL_malloc(sizeof(*creq), MPL_MEM_BUFFER); \
            MPIR_ERR_CHKANDSTMT(creq == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem"); \
            creq->event_id = MPIDI_OFI_EVENT_CHUNK_DONE;                \
            creq->parent = *(sigreq);                                   \
            msg.context = &creq->context;                               \
        }                                                               \
        MPIDI_OFI_win_cntr_incr(win);                                   \
    } while (0)

MPL_STATIC_INLINE_PREFIX uint32_t MPIDI_OFI_winfo_disp_unit(MPIR_Win * win, int rank)
{
    uint32_t ret;

    MPIR_FUNC_ENTER;

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY || MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        /* Always use winfo[rank].disp_unit if any of PROV_KEY and VIRT_ADDRESS is on.
         * Compiler can eliminate the branch in such a case. */
        ret = MPIDI_OFI_WIN(win).winfo[rank].disp_unit;
    } else if (MPIDI_OFI_WIN(win).winfo) {
        ret = MPIDI_OFI_WIN(win).winfo[rank].disp_unit;
    } else {
        ret = win->disp_unit;
    }

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_sigreq_complete(MPIR_Request ** sigreq)
{
    if (sigreq) {
        /* If sigreq is not NULL, *sigreq should be a valid object now. */
        MPIR_Assert(*sigreq != NULL);
        MPID_Request_complete(*sigreq);
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_load_iov(const void *buffer, int count,
                                                 MPI_Datatype datatype, MPI_Aint max_len,
                                                 MPI_Aint * loaded_iov_offset, struct iovec *iov)
{
    MPI_Aint outlen;
    MPIR_Typerep_to_iov_offset(buffer, count, datatype, *loaded_iov_offset, iov, max_len, &outlen);
    *loaded_iov_offset += outlen;
}

int MPIDI_OFI_issue_deferred_rma(MPIR_Win * win);
void MPIDI_OFI_complete_chunks(MPIDI_OFI_win_request_t * winreq);
int MPIDI_OFI_nopack_putget(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            int target_count, MPI_Datatype target_datatype,
                            MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                            MPIDI_av_entry_t * addr, int rma_type, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_put(const void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       int target_count, MPI_Datatype target_datatype,
                       MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_get(void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       int target_count, MPI_Datatype target_datatype,
                       MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq);

/* Common Utility functions used by the
 * C and C++ components
 */
/* Set max size based on OFI acc ordering limit. */
MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_OFI_check_acc_order_size(MPIR_Win * win, MPI_Aint data_size)
{
    MPI_Aint max_size = data_size;
    /* Check ordering limit:
     * - A value of -1 guarantees ordering for any data size.
     * - An order size value of 0 indicates that ordering is not guaranteed.
     * The check below returns the supported positive max_size, or zero which indicates disabled acc.*/
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAR)
        && MPIDI_OFI_global.max_order_war != -1) {
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_war);
    }
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAW)
        && MPIDI_OFI_global.max_order_waw != -1) {
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_waw);
    }
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW)
        && MPIDI_OFI_global.max_order_raw != -1) {
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_order_raw);
    }
    return max_size;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_win_request_t *MPIDI_OFI_win_request_create(void)
{
    MPIDI_OFI_win_request_t *winreq;
    winreq = MPL_malloc(sizeof(*winreq), MPL_MEM_OTHER);
    return winreq;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_request_complete(MPIDI_OFI_win_request_t * winreq)
{
    MPIDI_OFI_complete_chunks(winreq);
    if (winreq->rma_type == MPIDI_OFI_PUT &&
        winreq->noncontig.put.origin.datatype != MPI_DATATYPE_NULL &&
        winreq->noncontig.put.target.datatype != MPI_DATATYPE_NULL) {
        MPIR_Datatype_release_if_not_builtin(winreq->noncontig.put.origin.datatype);
        MPIR_Datatype_release_if_not_builtin(winreq->noncontig.put.target.datatype);
    } else if (winreq->rma_type == MPIDI_OFI_GET &&
               winreq->noncontig.get.origin.datatype != MPI_DATATYPE_NULL &&
               winreq->noncontig.get.target.datatype != MPI_DATATYPE_NULL) {
        MPIR_Datatype_release_if_not_builtin(winreq->noncontig.get.origin.datatype);
        MPIR_Datatype_release_if_not_builtin(winreq->noncontig.get.target.datatype);
    }
    MPL_free(winreq);
}

/* This function implements netmod vci to vni(context) mapping.
 * Currently, we only support one-to-one mapping.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_vci_to_vni(int vci)
{
    return vci;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_vci_to_vni_assert(int vci)
{
    int vni = MPIDI_OFI_vci_to_vni(vci);
    MPIR_Assert(vni < MPIDI_OFI_global.num_vnis);
    return vni;
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_av_to_phys(MPIDI_av_entry_t * av, int nic,
                                                        int vni_local, int vni_remote)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[nic][vni_remote], 0, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_AV(av).dest[nic][vni_remote];
    }
#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[nic][0], vni_remote, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        MPIR_Assert(vni_remote == 0);
        return MPIDI_OFI_AV(av).dest[nic][0];
    }
#endif
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_rank_to_phys(int rank, int nic,
                                                          int vni_local, int vni_remote)
{
    MPIDI_av_entry_t *av = &MPIDIU_get_av(0, rank);
    return MPIDI_OFI_av_to_phys(av, nic, vni_local, vni_remote);
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_comm_to_phys(MPIR_Comm * comm, int rank, int nic,
                                                          int vni_local, int vni_remote)
{
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, rank);
    return MPIDI_OFI_av_to_phys(av, nic, vni_local, vni_remote);
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_is_tag_sync(uint64_t match_bits)
{
    return (0 != (MPIDI_OFI_SYNC_SEND & match_bits));
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_sendtag(MPIR_Context_id_t contextid,
                                                         int tag, uint64_t type)
{
    uint64_t match_bits;
    match_bits = contextid;

    match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
    match_bits |= (MPIDI_OFI_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_recvtag(uint64_t * mask_bits,
                                                         MPIR_Context_id_t contextid, int tag)
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
    ssize_t rem_size = total_bytes;
    MPI_Aint num_iov, total_iov = 0;

    MPIR_FUNC_ENTER;

    if (dt_datatype == MPI_DATATYPE_NULL)
        goto fn_exit;

    do {
        MPI_Aint tmp_size = (rem_size > max_pipe) ? max_pipe : rem_size;

        MPIR_Typerep_iov_len(dt_count, dt_datatype, tmp_size, &num_iov);
        total_iov += num_iov;

        rem_size -= tmp_size;
    } while (rem_size);

  fn_exit:
    MPIR_FUNC_EXIT;
    return total_iov;
}

/* Calculate the index of the NIC used to send a message from sender_rank to receiver_rank
 *
 * comm - The communicator used to send the message.
 * ctxid_in_effect - The context ID that will be used to send the message.
 *                   On the sender side, this should be comm->context_id.
 *                   On the receiver side, this should be comm->recvcontext_id.
 * receiver_rank - The rank of the receiving process.
 * tag - The tag of the message being sent.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_multx_sender_nic_index(MPIR_Comm * comm,
                                                              MPIR_Context_id_t ctxid_in_effect,
                                                              int receiver_rank, int tag)
{
    int nic_idx = 0;

    /* TODO - If there is a communicator specific mapping, that should be checked/used here. */
    /* TODO - We should use the per-communicator value for the maximum number of NICs in this
     *        calculation once we have a per-communicator value for it. */
    if (MPIDI_OFI_COMM(comm).enable_hashing) {
        nic_idx = ((unsigned int) (MPIR_CONTEXT_READ_FIELD(PREFIX, ctxid_in_effect) +
                                   receiver_rank + tag)) % MPIDI_OFI_global.num_nics;
    }

    return nic_idx;
}

/* Calculate the index of the NIC used to receive a message from sender_rank at receiver_rank
 *
 * comm - The communicator used to receive the message.
 * ctxid_in_effect - The context ID that will be used to receive the message.
 *                   On the sender side, this should be comm->context_id.
 *                   On the receiver side, this should be comm->recvcontext_id.
 * sender_rank - The rank of the sending process.
 * tag - The tag of the message being sent.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_multx_receiver_nic_index(MPIR_Comm * comm,
                                                                MPIR_Context_id_t ctxid_in_effect,
                                                                int sender_rank, int tag)
{
    int nic_idx = 0;

    /* TODO - If there is a communicator specific mapping, that should be checked/used here. */
    /* TODO - We should use the per-communicator value for the maximum number of NICs in this
     *        calculation once we have a per-communicator value for it. */
    if (MPIDI_OFI_COMM(comm).enable_hashing) {
        nic_idx = ((unsigned int) (MPIR_CONTEXT_READ_FIELD(PREFIX, ctxid_in_effect) +
                                   sender_rank + tag)) % MPIDI_OFI_global.num_nics;
    }

    return nic_idx;
}

/* cq bufferring routines --
 * in particular, when we encounter EAGAIN error during progress, such as during
 * active message handling, recursively calling progress may result in unpredictable
 * behaviors (e.g. stack overflow). Thus we need use the cq buffering to avoid
 * process further cq entries during (am-related) calls.
 */

/* local macros to make the code cleaner */
#define CQ_S_LIST MPIDI_OFI_global.per_vni[vni].cq_buffered_static_list
#define CQ_S_HEAD MPIDI_OFI_global.per_vni[vni].cq_buffered_static_head
#define CQ_S_TAIL MPIDI_OFI_global.per_vni[vni].cq_buffered_static_tail
#define CQ_D_HEAD MPIDI_OFI_global.per_vni[vni].cq_buffered_dynamic_head
#define CQ_D_TAIL MPIDI_OFI_global.per_vni[vni].cq_buffered_dynamic_tail

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_has_cq_buffered(int vni)
{
    return (CQ_S_HEAD != CQ_S_TAIL) || (CQ_D_HEAD != NULL);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni)
{
    int mpi_errno = MPI_SUCCESS, ret = 0;
    struct fi_cq_tagged_entry cq_entry;
    MPIR_FUNC_ENTER;

    /* Caller must hold MPIDI_OFI_THREAD_FI_MUTEX */

    for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
        int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);
        ret = fi_cq_read(MPIDI_OFI_global.ctx[ctx_idx].cq, &cq_entry, 1);

        if (unlikely(ret == -FI_EAGAIN))
            goto fn_exit;

        if (ret < 0) {
            mpi_errno = MPIDI_OFI_handle_cq_error(vni, nic, ret);
            goto fn_fail;
        }

        /* If the statically allocated buffered list is full or we've already
         * started using the dynamic list, continue using it. */
        if (((CQ_S_HEAD + 1) % MPIDI_OFI_NUM_CQ_BUFFERED == CQ_S_TAIL) || (CQ_D_HEAD != NULL)) {
            MPIDI_OFI_cq_list_t *list_entry =
                (MPIDI_OFI_cq_list_t *) MPL_malloc(sizeof(MPIDI_OFI_cq_list_t), MPL_MEM_BUFFER);
            MPIR_Assert(list_entry);
            list_entry->cq_entry = cq_entry;
            LL_APPEND(CQ_D_HEAD, CQ_D_TAIL, list_entry);
        } else {
            CQ_S_LIST[CQ_S_HEAD] = cq_entry;
            CQ_S_HEAD = (CQ_S_HEAD + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_buffered(int vni, struct fi_cq_tagged_entry *wc)
{
    int rc = 0;

    if (1) {
        /* If the static list isn't empty, do so first */
        if (CQ_S_HEAD != CQ_S_TAIL) {
            wc[0] = CQ_S_LIST[CQ_S_TAIL];
            CQ_S_TAIL = (CQ_S_TAIL + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
        /* If there's anything in the dynamic list, it goes second. */
        else if (CQ_D_HEAD != NULL) {
            MPIDI_OFI_cq_list_t *cq_list_entry = CQ_D_HEAD;
            LL_DELETE(CQ_D_HEAD, CQ_D_TAIL, cq_list_entry);
            wc[0] = cq_list_entry->cq_entry;
            MPL_free(cq_list_entry);
        }

        rc = 1;
    }

    return rc;
}

#undef CQ_S_LIST
#undef CQ_S_HEAD
#undef CQ_S_TAIL
#undef CQ_D_HEAD
#undef CQ_D_TAIL

#endif /* OFI_IMPL_H_INCLUDED */

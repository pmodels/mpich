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

int MPIDI_OFI_progress_uninlined(int vci);
int MPIDI_OFI_handle_cq_error(int vci, int nic, ssize_t ret);

/*
 * Helper routines and macros for request completion
 */
#define MPIDI_OFI_PROGRESS(vci)                                   \
    do {                                                          \
        int made_progress = 0; \
        mpi_errno = MPIDI_NM_progress(vci, &made_progress); \
        MPIR_ERR_CHECK(mpi_errno);                                \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_PROGRESS_WHILE(cond, vci) \
    while (cond) MPIDI_OFI_PROGRESS(vci)

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

#define MPIDI_OFI_CALL_RETRY(FUNC,vci_,STR)                 \
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
        if (_retry > 0) { \
            _retry--; \
            MPIR_ERR_CHKANDJUMP(_retry == 0, mpi_errno, MPIX_ERR_EAGAIN, "**eagain"); \
        } \
        /* FIXME: by fixing the recursive locking interface to account
         * for recursive locking in more than one lock (currently limited
         * to one due to scalar TLS counter), this lock yielding
         * operation can be avoided since we are inside a finite loop. */ \
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_);			  \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_);			     \
        MPIR_ERR_CHECK(mpi_errno);                               \
    } while (1);                                            \
    } while (0)

#define MPIDI_OFI_CALL_RETRY_RETURN(FUNC,vci_,ret) \
    do { \
        int _retry = MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY; \
        while (1) { \
            ret = FUNC; \
            if (likely(ret != -FI_EAGAIN)) { \
                break; \
            } \
            if (_retry > 0) { \
                _retry--; \
                MPIR_ERR_CHKANDJUMP(_retry == 0, mpi_errno, MPIX_ERR_EAGAIN, "**eagain"); \
            } \
            MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_); \
            mpi_errno = MPIDI_OFI_retry_progress(); \
            MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_); \
        } \
    } while (0)

#define MPIDI_OFI_CALL_RETRY_AM(FUNC,vci_,STR)                          \
    do {                                                                \
        ssize_t _ret;                                                   \
        do {                                                            \
            _ret = FUNC;                                                \
            if (likely(_ret==0)) break;                                  \
            MPIDI_OFI_ERR(_ret != -FI_EAGAIN,                  \
                                   mpi_errno,                           \
                                   MPI_ERR_OTHER,                       \
                                   "**ofid_"#STR,                        \
                                   "**ofid_"#STR" %s %d %s %s",          \
                                   __SHORT_FILE__,                      \
                                   __LINE__,                            \
                                   __func__,                              \
                                   fi_strerror(-_ret));                 \
            mpi_errno = MPIDI_OFI_progress_do_queue(vci_);              \
            if (mpi_errno != MPI_SUCCESS)                                \
                MPIR_ERR_CHECK(mpi_errno);                               \
        } while (_ret == -FI_EAGAIN);                                   \
    } while (0)

/* per-vci macros - we'll transition into these macros once the locks are
 * moved down to ofi-layer */
#define MPIDI_OFI_VCI_PROGRESS(vci_)                                    \
    do {                                                                \
        int made_progress = 0; \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                \
        mpi_errno = MPIDI_NM_progress(vci_, &made_progress); \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);                 \
        MPIR_ERR_CHECK(mpi_errno);                                      \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_VCI_PROGRESS_WHILE(vci_, cond)                            \
    do {                                                                    \
        int made_progress = 0; \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                    \
        while (cond) {                                                      \
            mpi_errno = MPIDI_NM_progress(vci_, &made_progress);                        \
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

#define MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_)            \
    do {                                                        \
        if (!MPIDI_VCI_IS_EXPLICIT(vci_) && MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_LOCKLESS) {      \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);    \
        }                                                       \
    } while (0)

#define MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_)         \
    do {                                                    \
        if (!MPIDI_VCI_IS_EXPLICIT(vci_) && MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_LOCKLESS) {  \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock); \
        }                                                   \
    } while (0)

#define MPIDI_OFI_CALL_RETURN(FUNC, _ret)                               \
        do {                                                            \
            (_ret) = FUNC;                                              \
        } while (0)

#define MPIDI_OFI_REQUEST_CREATE(req, kind, vci) \
    do {                                                      \
        MPIDI_CH4_REQUEST_CREATE(req, kind, vci, 2);                    \
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

/* This is similar to MPIDI_OFI_multx_{sender,receiver}_nic_index, except no hashing */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_pref_nic(MPIR_Comm * comm_ptr, int rank)
{
    if (comm_ptr == NULL || MPIDI_OFI_COMM(comm_ptr).pref_nic == NULL) {
        return 0;
    } else {
        return MPIDI_OFI_COMM(comm_ptr).pref_nic[rank];
    }
}

/* Calculate the OFI context index.
 * The total number of OFI contexts will be the number of nics * number of vcis
 * Each nic will contain num_vcis vcis. Each corresponding to their respective vci index. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_ctx_index(int vci, int nic)
{
    return nic * MPIDI_CH4_MAX_VCIS + vci;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_incr(int vci, int nic)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
#else
    /* NOTE: shared with ctx[0] */
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_mr_bind(struct fi_info *prov, struct fid_mr *mr,
                                               struct fid_ep *ep, struct fid_cntr *cntr)
{
    int mpi_errno = MPI_SUCCESS;

    if (prov->domain_attr->mr_mode & FI_MR_ENDPOINT) {
        /* Bind the memory region to the endpoint */
        MPIDI_OFI_CALL(fi_mr_bind(mr, &ep->fid, 0ULL), mr_bind);
        MPIDI_OFI_CALL(fi_mr_enable(mr), mr_enable);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Externs:  see util.c for definition */
#define MPIDI_OFI_LOCAL_MR_KEY 0
#define MPIDI_OFI_COLL_MR_KEY 1
#define MPIDI_OFI_INVALID_MR_KEY 0xFFFFFFFFFFFFFFFFULL
int MPIDI_OFI_retry_progress(void);
int MPIDI_OFI_recv_huge_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_recv_huge_control(int vci, MPIR_Context_id_t comm_id, int rank, int tag,
                                MPIDI_OFI_huge_remote_info_t * info);
int MPIDI_OFI_peek_huge_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_huge_chunk_done_event(int vci, struct fi_cq_tagged_entry *wc, void *req);
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
int MPIDI_OFI_mpi_to_ofi(MPI_Datatype dt, enum fi_datatype *fi_dt, MPI_Op op, enum fi_op *fi_op);

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
int MPIDI_OFI_nopack_putget(const void *origin_addr, MPI_Aint origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_count, MPI_Datatype target_datatype,
                            MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                            MPIDI_av_entry_t * addr, int rma_type, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_put(const void *origin_addr, MPI_Aint origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_count, MPI_Datatype target_datatype,
                       MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_get(void *origin_addr, MPI_Aint origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_count, MPI_Datatype target_datatype,
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

/* NOTE: in general, the address can be different between any two endpoints. Thus, we need
 *       a 4 dimension tuple of (nic_local, vci_local, nic_remote, vci_remote) for a given addr.
 *       However, libfabric let us to have the same addr for a (nic_remote, vci_remote) pair
 *       on any local endpoints, as long as we are careful in the insertion order). Thus,
 *       we get away with simplified interface using just (nic, vci) pair.
 */
MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_av_to_phys(MPIDI_av_entry_t * av, int nic, int vci)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[nic][vci], 0, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_AV(av).dest[nic][vci];
    }
#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[nic][0], vci, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        MPIR_Assert(vci == 0);
        return MPIDI_OFI_AV(av).dest[nic][0];
    }
#endif
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_comm_to_phys(MPIR_Comm * comm, int rank,
                                                          int nic, int vci)
{
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, rank);
    return MPIDI_OFI_av_to_phys(av, nic, vci);
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

struct MPIDI_OFI_contig_blocks_params {
    size_t max_pipe;
    MPI_Aint count;
    MPI_Aint last_loc;
    MPI_Aint start_loc;
    size_t last_chunk;
};

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
                                                              int sender_rank, int receiver_rank,
                                                              int tag)
{
    int nic_idx = 0;

    if (MPIDI_OFI_COMM(comm).pref_nic) {
        nic_idx = MPIDI_OFI_COMM(comm).pref_nic[sender_rank];
    } else if (MPIDI_OFI_COMM(comm).enable_hashing) {
        /* TODO - We should use the per-communicator value for the maximum number of NICs in this
         *        calculation once we have a per-communicator value for it. */
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
                                                                int sender_rank, int receiver_rank,
                                                                int tag)
{
    int nic_idx = 0;

    if (MPIDI_OFI_COMM(comm).pref_nic) {
        nic_idx = MPIDI_OFI_COMM(comm).pref_nic[receiver_rank];
    } else if (MPIDI_OFI_COMM(comm).enable_hashing) {
        /* TODO - We should use the per-communicator value for the maximum number of NICs in this
         *        calculation once we have a per-communicator value for it. */
        nic_idx = ((unsigned int) (MPIR_CONTEXT_READ_FIELD(PREFIX, ctxid_in_effect) +
                                   sender_rank + tag)) % MPIDI_OFI_global.num_nics;
    }

    return nic_idx;
}

/* cq buffering routines --
 * in particular, when we encounter EAGAIN error during progress, such as during
 * active message handling, recursively calling progress may result in unpredictable
 * behaviors (e.g. stack overflow). Thus we need use the cq buffering to avoid
 * process further cq entries during (am-related) calls.
 */

/* local macros to make the code cleaner */
#define CQ_S_LIST MPIDI_OFI_global.per_vci[vci].cq_buffered_static_list
#define CQ_S_HEAD MPIDI_OFI_global.per_vci[vci].cq_buffered_static_head
#define CQ_S_TAIL MPIDI_OFI_global.per_vci[vci].cq_buffered_static_tail
#define CQ_D_HEAD MPIDI_OFI_global.per_vci[vci].cq_buffered_dynamic_head
#define CQ_D_TAIL MPIDI_OFI_global.per_vci[vci].cq_buffered_dynamic_tail

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_has_cq_buffered(int vci)
{
    return (CQ_S_HEAD != CQ_S_TAIL) || (CQ_D_HEAD != NULL);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vci)
{
    int mpi_errno = MPI_SUCCESS, ret = 0;
    struct fi_cq_tagged_entry cq_entry;
    MPIR_FUNC_ENTER;

    /* Caller must hold MPIDI_OFI_THREAD_FI_MUTEX */

    for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
        int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
        ret = fi_cq_read(MPIDI_OFI_global.ctx[ctx_idx].cq, &cq_entry, 1);

        if (unlikely(ret == -FI_EAGAIN))
            goto fn_exit;

        if (ret < 0) {
            mpi_errno = MPIDI_OFI_handle_cq_error(vci, nic, ret);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_buffered(int vci, struct fi_cq_tagged_entry *wc)
{
    int num = 0;

    while (num < MPIDI_OFI_NUM_CQ_ENTRIES) {
        /* If the static list isn't empty, do so first */
        if (CQ_S_HEAD != CQ_S_TAIL) {
            wc[num] = CQ_S_LIST[CQ_S_TAIL];
            CQ_S_TAIL = (CQ_S_TAIL + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
        /* If there's anything in the dynamic list, it goes second. */
        else if (CQ_D_HEAD != NULL) {
            MPIDI_OFI_cq_list_t *cq_list_entry = CQ_D_HEAD;
            LL_DELETE(CQ_D_HEAD, CQ_D_TAIL, cq_list_entry);
            wc[num] = cq_list_entry->cq_entry;
            MPL_free(cq_list_entry);
        } else {
            break;
        }

        num++;
    }

    return num;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_register_memory(char *send_buf, size_t data_sz,
                                                       MPL_pointer_attr_t * attr, int ctx_idx,
                                                       uint64_t rkey, struct fid_mr **mr)
{
    struct fi_mr_attr mr_attr = { 0 };
    struct iovec iov;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    iov.iov_base = send_buf;
    iov.iov_len = data_sz;
    mr_attr.mr_iov = &iov;
    mr_attr.iov_count = 1;
    mr_attr.access = FI_REMOTE_READ | FI_REMOTE_WRITE;
    mr_attr.requested_key = rkey;
    mr_attr.offset = 0;
    mr_attr.context = NULL;
    if (MPL_gpu_attr_is_strict_dev(attr)) {
#ifdef MPL_HAVE_CUDA
        mr_attr.iface = FI_HMEM_CUDA;
        mr_attr.device.cuda = MPL_gpu_get_dev_id_from_attr(attr);
#elif defined MPL_HAVE_ZE
        /* OFI does not support tiles yet, need to pass the root device. */
        mr_attr.iface = FI_HMEM_ZE;
        mr_attr.device.ze = MPL_gpu_get_root_device(MPL_gpu_get_dev_id_from_attr(attr));
#else
        /* FIXME: add support for MPL_HAVE_HIP (FI_HMEM_ROCR) */
        mr_attr.iface = FI_HMEM_SYSTEM;
#endif
    } else {
        mr_attr.iface = FI_HMEM_SYSTEM;
    }
    MPIDI_OFI_CALL(fi_mr_regattr
                   (MPIDI_OFI_global.ctx[ctx_idx].domain, &mr_attr, 0, mr), mr_regattr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_register_memory_and_bind(char *send_buf, size_t data_sz,
                                                                MPL_pointer_attr_t * attr,
                                                                int ctx_idx, struct fid_mr **mr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_OFI_register_memory(send_buf, data_sz, attr, ctx_idx, 0, mr);
    MPIR_ERR_CHECK(mpi_errno);

    if (*mr != NULL) {
        mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], *mr,
                                      MPIDI_OFI_global.ctx[ctx_idx].ep, NULL);
        MPIR_ERR_CHECK(mpi_errno);
        /* Cache the mrs for closing during Finalize() */
        struct MPIDI_GPU_RDMA_queue_t *new_mr =
            MPL_malloc(sizeof(struct MPIDI_GPU_RDMA_queue_t), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(new_mr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "GPU RDMA MR alloc");
        new_mr->mr = *mr;
        DL_APPEND(MPIDI_OFI_global.gdr_mrs, new_mr);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_register_am_bufs(void)
{
    if (!MPIDI_OFI_global.am_bufs_registered) {
        for (int i = 0; i < MPIDI_OFI_global.num_vcis; i++) {
            MPIR_gpu_register_host(MPIDI_OFI_global.per_vci[i].am_bufs,
                                   MPIDI_OFI_AM_BUFF_SZ * MPIDI_OFI_NUM_AM_BUFFERS);
        }
        MPIDI_OFI_global.am_bufs_registered = true;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_unregister_am_bufs(void)
{
    if (MPIDI_OFI_global.am_bufs_registered) {
        for (int i = 0; i < MPIDI_OFI_global.num_vcis; i++) {
            MPIR_gpu_unregister_host(MPIDI_OFI_global.per_vci[i].am_bufs);
        }
        MPIDI_OFI_global.am_bufs_registered = false;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_gpu_rma_register(const void *buffer, size_t size,
                                                         MPL_pointer_attr_t * attr, MPIR_Win * win,
                                                         int nic, void **desc)
{
    struct fid_mr *mr = NULL;
    MPL_pointer_attr_t attr_tmp;

    *desc = NULL;

    int ctx_idx = MPIDI_OFI_get_ctx_index(MPIDI_WIN(win, am_vci), nic);
    if (!attr) {
        MPIR_GPU_query_pointer_attr(buffer, &attr_tmp);
        attr = &attr_tmp;
    }
    if (MPIDI_OFI_ENABLE_HMEM && MPIDI_OFI_ENABLE_MR_HMEM && MPL_gpu_attr_is_strict_dev(attr)) {
        MPIDI_OFI_register_memory_and_bind((char *) buffer, size, attr, ctx_idx, &mr);
        if (mr != NULL) {
            *desc = fi_mr_desc(mr);
        }
    }
}

#undef CQ_S_LIST
#undef CQ_S_HEAD
#undef CQ_S_TAIL
#undef CQ_D_HEAD
#undef CQ_D_TAIL

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_gpu_malloc_pack_buffer(void **ptr, size_t pack_size)
{
    if (MPIDI_OFI_ENABLE_HMEM) {
        return MPL_gpu_malloc_host(ptr, pack_size);
    } else {
#ifdef MPL_DEFINE_ALIGNED_ALLOC
        *ptr = MPL_aligned_alloc(256, pack_size, MPL_MEM_BUFFER);
#else
        *ptr = MPL_malloc(pack_size, MPL_MEM_BUFFER);
#endif
        return 0;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_gpu_free_pack_buffer(void *ptr)
{
    if (MPIDI_OFI_ENABLE_HMEM) {
        return MPL_gpu_free_host(ptr);
    } else {
        MPL_free(ptr);
        return 0;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_gpu_pipeline_chunk_size(size_t data_sz)
{
    int chunk_size = MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ;
    if (data_sz <= MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ) {
        chunk_size = data_sz;
    }
    return chunk_size;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_gpu_task_t *MPIDI_OFI_create_gpu_task(MPIDI_OFI_pipeline_type_t
                                                                         type, void *buf,
                                                                         size_t len,
                                                                         MPIR_Request * request,
                                                                         MPIR_gpu_req yreq)
{
    MPIDI_OFI_gpu_task_t *task =
        (MPIDI_OFI_gpu_task_t *) MPL_malloc(sizeof(MPIDI_OFI_gpu_task_t), MPL_MEM_OTHER);
    MPIR_Assert(task != NULL);
    task->type = type;
    task->status = MPIDI_OFI_PIPELINE_READY;
    task->buf = buf;
    task->len = len;
    task->request = request;
    task->yreq = yreq;
    task->prev = NULL;
    task->next = NULL;
    return task;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_gpu_pending_recv_t
    * MPIDI_OFI_create_recv_task(MPIDI_OFI_gpu_pipeline_request * req, int idx, int n_chunks)
{
    MPIDI_OFI_gpu_pending_recv_t *task =
        (MPIDI_OFI_gpu_pending_recv_t *) MPL_malloc(sizeof(MPIDI_OFI_gpu_pending_recv_t),
                                                    MPL_MEM_OTHER);
    MPIR_Assert(task);
    task->req = req;
    task->idx = idx;
    task->n_chunks = n_chunks;
    task->prev = NULL;
    task->next = NULL;
    return task;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_gpu_pending_send_t *MPIDI_OFI_create_send_task(MPIR_Request *
                                                                                  req,
                                                                                  void *send_buf,
                                                                                  MPI_Aint count,
                                                                                  MPI_Datatype
                                                                                  datatype,
                                                                                  MPL_pointer_attr_t
                                                                                  attr,
                                                                                  MPI_Aint left_sz,
                                                                                  int dt_contig)
{
    MPIDI_OFI_gpu_pending_send_t *task =
        (MPIDI_OFI_gpu_pending_send_t *) MPL_malloc(sizeof(MPIDI_OFI_gpu_pending_send_t),
                                                    MPL_MEM_OTHER);
    MPIR_Assert(task);
    task->sreq = req;
    task->attr = attr;
    task->send_buf = send_buf;
    task->datatype = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    task->offset = 0;
    task->n_chunks = 0;
    task->left_sz = left_sz;
    task->count = count;
    task->dt_contig = dt_contig;
    task->prev = NULL;
    task->next = NULL;
    return task;
}

static int MPIDI_OFI_gpu_progress_task(MPIDI_OFI_gpu_task_t * gpu_queue[], int vni);

static int MPIDI_OFI_gpu_progress_send(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_gpu_engine_type_t engine_type =
        (MPL_gpu_engine_type_t) MPIR_CVAR_CH4_OFI_GPU_PIPELINE_D2H_ENGINE_TYPE;

    while (MPIDI_OFI_global.gpu_send_queue) {
        char *host_buf = NULL;
        MPI_Aint chunk_sz;
        int vci_local = -1;

        MPIDI_OFI_gpu_pending_send_t *send_task = MPIDI_OFI_global.gpu_send_queue;
        int block_sz = MPIDI_OFI_REQUEST(send_task->sreq, pipeline_info.chunk_sz);
        while (send_task->left_sz > 0) {
            MPIDI_OFI_gpu_task_t *task = NULL;
            chunk_sz = send_task->left_sz > block_sz ? block_sz : send_task->left_sz;
            host_buf = NULL;
            MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.gpu_pipeline_send_pool,
                                               (void **) &host_buf);
            if (host_buf == NULL) {
                goto fn_exit;
            }
            MPI_Aint actual_pack_bytes;
            MPIR_gpu_req yreq;
            int commit = send_task->left_sz <= chunk_sz ? 1 : 0;
            if (!commit &&
                !MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST &&
                send_task->n_chunks % MPIR_CVAR_CH4_OFI_GPU_PIPELINE_NUM_BUFFERS_PER_CHUNK ==
                MPIR_CVAR_CH4_OFI_GPU_PIPELINE_NUM_BUFFERS_PER_CHUNK - 1)
                commit = 1;
            mpi_errno =
                MPIR_Ilocalcopy_gpu((char *) send_task->send_buf, send_task->count,
                                    send_task->datatype, send_task->offset, &send_task->attr,
                                    host_buf, chunk_sz, MPI_BYTE, 0, NULL, MPL_GPU_COPY_D2H,
                                    engine_type, commit, &yreq);
            MPIR_ERR_CHECK(mpi_errno);
            actual_pack_bytes = chunk_sz;
            task =
                MPIDI_OFI_create_gpu_task(MPIDI_OFI_PIPELINE_SEND, host_buf, actual_pack_bytes,
                                          send_task->sreq, yreq);
            send_task->offset += (size_t) actual_pack_bytes;
            send_task->left_sz -= (size_t) actual_pack_bytes;
            vci_local = MPIDI_OFI_REQUEST(send_task->sreq, pipeline_info.vci_local);
            MPIR_Assert(vci_local < MPIDI_CH4_MAX_VCIS);
            DL_APPEND(MPIDI_OFI_global.gpu_send_task_queue[vci_local], task);
            send_task->n_chunks++;
            /* Increase request completion cnt, cc is 1 more than necessary
             * to prevent parent request being freed prematurally. */
            MPIR_cc_inc(send_task->sreq->cc_ptr);
        }
        /* all done, decrease cc by 1 to allow parent request to be freed
         * when complete */
        MPIR_cc_dec(send_task->sreq->cc_ptr);
        /* Update correct number of chunks in immediate data. */
        MPIDI_OFI_idata_set_gpuchunk_bits(&MPIDI_OFI_REQUEST
                                          (send_task->sreq, pipeline_info.cq_data),
                                          send_task->n_chunks);
        DL_DELETE(MPIDI_OFI_global.gpu_send_queue, send_task);
        MPIR_Datatype_release_if_not_builtin(send_task->datatype);
        MPL_free(send_task);

        if (vci_local != -1)
            MPIDI_OFI_gpu_progress_task(MPIDI_OFI_global.gpu_send_task_queue, vci_local);

    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_gpu_progress_recv(void)
{
    int mpi_errno = MPI_SUCCESS;

    while (MPIDI_OFI_global.gpu_recv_queue) {
        MPIDI_OFI_gpu_pending_recv_t *recv_task = MPIDI_OFI_global.gpu_recv_queue;
        MPIDI_OFI_gpu_pipeline_request *chunk_req = recv_task->req;
        MPIR_Request *rreq = chunk_req->parent;
        void *host_buf = chunk_req->buf;
        if (!host_buf) {
            MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool,
                                               (void **) &host_buf);
            if (!host_buf) {
                break;
            }
            chunk_req->buf = host_buf;
        }
        fi_addr_t remote_addr = MPIDI_OFI_REQUEST(rreq, pipeline_info.remote_addr);

        int ret = fi_trecv(MPIDI_OFI_global.ctx[MPIDI_OFI_REQUEST(rreq, pipeline_info.ctx_idx)].rx,
                           (void *) host_buf,
                           MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ, NULL, remote_addr,
                           MPIDI_OFI_REQUEST(rreq,
                                             pipeline_info.match_bits) |
                           MPIDI_OFI_GPU_PIPELINE_SEND,
                           MPIDI_OFI_REQUEST(rreq, pipeline_info.mask_bits),
                           (void *) &chunk_req->context);
        if (ret == 0) {
            DL_DELETE(MPIDI_OFI_global.gpu_recv_queue, recv_task);
            MPL_free(recv_task);
        } else if (ret == -FI_EAGAIN || ret == -FI_ENOMEM) {
            break;
        } else {
            goto fn_fail;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

static int MPIDI_OFI_gpu_progress_task(MPIDI_OFI_gpu_task_t * gpu_queue[], int vni)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_gpu_task_t *task = NULL;
    MPIDI_OFI_gpu_task_t *tmp;

    DL_FOREACH_SAFE(gpu_queue[vni], task, tmp) {
        if (task->status == MPIDI_OFI_PIPELINE_EXEC) {
            /* Avoid the deadlock of re-launching an executing OFI task. */
            goto fn_exit;
        }

        MPIR_gpu_req *yreq = &task->yreq;
        int completed = 0;
        if (yreq->type == MPIR_GPU_REQUEST) {
            mpi_errno = MPL_gpu_test(&yreq->u.gpu_req, &completed);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (yreq->type == MPIR_TYPEREP_REQUEST) {
            MPIR_Typerep_test(yreq->u.y_req, &completed);
        } else {
            completed = 1;
        }
        if (completed == 1) {
            /* GPU transfer completes. */
            task->status = MPIDI_OFI_PIPELINE_EXEC;
            MPIR_Request *request = task->request;

            if (task->type == MPIDI_OFI_PIPELINE_SEND) {
                MPIDI_OFI_gpu_pipeline_request *chunk_req = (MPIDI_OFI_gpu_pipeline_request *)
                    MPL_malloc(sizeof(MPIDI_OFI_gpu_pipeline_request), MPL_MEM_BUFFER);
                MPIR_ERR_CHKANDJUMP1(chunk_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                                     "**nomem %s", "GPU pipelining chunk_req alloc");
                chunk_req->parent = request;
                chunk_req->event_id = MPIDI_OFI_EVENT_SEND_GPU_PIPELINE;
                chunk_req->buf = task->buf;
                MPIDI_OFI_CALL(fi_tsenddata
                               (MPIDI_OFI_global.ctx
                                [MPIDI_OFI_REQUEST(request, pipeline_info.ctx_idx)].tx,
                                task->buf, task->len, NULL /* desc */ ,
                                MPIDI_OFI_REQUEST(request, pipeline_info.cq_data),
                                MPIDI_OFI_REQUEST(request, pipeline_info.remote_addr),
                                MPIDI_OFI_REQUEST(request,
                                                  pipeline_info.match_bits) |
                                MPIDI_OFI_GPU_PIPELINE_SEND, (void *) &chunk_req->context),
                               tsenddata);
                DL_DELETE(gpu_queue[vni], task);
                MPL_free(task);
            } else {
                MPIR_Assert(task->type == MPIDI_OFI_PIPELINE_RECV);
                int c;
                MPIR_cc_decr(request->cc_ptr, &c);
                if (c == 0) {
                    /* If synchronous, ack and complete when the ack is done */
                    if (unlikely(MPIDI_OFI_REQUEST(request, pipeline_info.is_sync))) {
                        MPIR_Comm *comm = request->comm;
                        uint64_t ss_bits =
                            MPIDI_OFI_init_sendtag(MPL_atomic_relaxed_load_int
                                                   (&MPIDI_OFI_REQUEST(request, util_id)),
                                                   MPIR_Comm_rank(comm), request->status.MPI_TAG,
                                                   MPIDI_OFI_SYNC_SEND_ACK);
                        int r = request->status.MPI_SOURCE;
                        int vci_src = MPIDI_get_vci(SRC_VCI_FROM_RECVER, comm, r, comm->rank,
                                                    request->status.MPI_TAG);
                        int vci_dst = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, r, comm->rank,
                                                    request->status.MPI_TAG);
                        int vci_local = vci_dst;
                        int vci_remote = vci_src;
                        int nic = 0;
                        int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic);
                        fi_addr_t dest_addr = MPIDI_OFI_comm_to_phys(comm, r, nic, vci_remote);
                        MPIDI_OFI_CALL_RETRY(fi_tinjectdata
                                             (MPIDI_OFI_global.ctx[ctx_idx].tx, NULL /* buf */ ,
                                              0 /* len */ ,
                                              MPIR_Comm_rank(comm), dest_addr, ss_bits),
                                             vci_local, tinjectdata);
                    }

                    /* Set number of bytes in status. */
                    MPIR_STATUS_SET_COUNT(request->status,
                                          MPIDI_OFI_REQUEST(request, pipeline_info.offset));

                    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST
                                                         (request, noncontig.pack.datatype));
                    MPIR_Request_free(request);
                }

                /* For recv, now task can be deleted from DL. */
                DL_DELETE(gpu_queue[vni], task);
                /* Free host buffer, yaksa request and task. */
                if (task->type == MPIDI_OFI_PIPELINE_RECV)
                    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool,
                                                      task->buf);
                else
                    MPIDI_OFI_gpu_free_pack_buffer(task->buf);
                MPL_free(task);
            }
        } else {
            goto fn_exit;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_gpu_progress(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_OFI_gpu_progress_task(MPIDI_OFI_global.gpu_recv_task_queue, vni);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_OFI_gpu_progress_task(MPIDI_OFI_global.gpu_send_task_queue, vni);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_OFI_gpu_progress_send();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_OFI_gpu_progress_recv();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_IMPL_H_INCLUDED */

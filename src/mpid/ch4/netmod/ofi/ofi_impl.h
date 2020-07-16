/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_IMPL_H_INCLUDED
#define OFI_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "ofi_types.h"
#include "mpidch4r.h"
#include "mpidig_am.h"
#include "ch4_impl.h"

#define MPIDI_OFI_ENAVAIL   -1  /* OFI resource not available */
#define MPIDI_OFI_EPERROR   -2  /* OFI endpoint error */

#define MPIDI_OFI_DT(dt)         ((dt)->dev.netmod.ofi)
#define MPIDI_OFI_OP(op)         ((op)->dev.netmod.ofi)
#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)
#define MPIDI_OFI_COMM_TO_INDEX(comm,rank) \
    MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_OFI_TO_PHYS(avtid, lpid)                                 \
    MPIDI_OFI_AV(&MPIDIU_get_av((avtid), (lpid))).dest[0][0]

#define MPIDI_OFI_WIN(win)     ((win)->dev.netmod.ofi)

int MPIDI_OFI_init_provider(void);

void MPIDI_OFI_init_hints(struct fi_info *hints);
void MPIDI_OFI_init_global_settings(const char *prov_name);
bool MPIDI_OFI_match_global_settings(struct fi_info *prov);
void MPIDI_OFI_update_global_settings(struct fi_info *prov, struct fi_info *hints);
void MPIDI_OFI_dump_global_settings(void);

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_fi_version(void)
{
    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        return FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);
    else
        return FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);
}

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

int MPIDI_OFI_progress(int vci, int blocking);

/* vni mapping */
/* NOTE: concerned by the modulo? If we restrict num_vnis to power of 2,
 * we may get away with bit mask */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_vni_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_get_src(comm_ptr, rank, tag) % MPIDI_OFI_global.num_vnis;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_vni_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_get_dst(comm_ptr, rank, tag) % MPIDI_OFI_global.num_vnis;
}

/*
 * Helper routines and macros for request completion
 */
#define MPIDI_OFI_PROGRESS(vni)                                   \
    do {                                                          \
        mpi_errno = MPIDI_OFI_progress(vni, 0);                   \
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
         * operation can be avoided since we are inside a finite loop. */\
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);     \
        mpi_errno = MPIDI_OFI_retry_progress();                      \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);    \
        MPIR_ERR_CHECK(mpi_errno);                               \
        _retry--;                                           \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

/* per-vci macros - we'll transition into these macros once the locks are
 * moved down to ofi-layer */
#define MPIDI_OFI_VCI_PROGRESS(vci_)                                    \
    do {                                                                \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                \
        mpi_errno = MPIDI_OFI_progress(vci_, 0);                        \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_).lock);                 \
        MPIR_ERR_CHECK(mpi_errno);                                      \
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); \
    } while (0)

#define MPIDI_OFI_VCI_PROGRESS_WHILE(vci_, cond)                            \
    do {                                                                    \
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_).lock);                    \
        while (cond) {                                                      \
            mpi_errno = MPIDI_OFI_progress(vci_, 0);                        \
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
        (req) = MPIR_Request_create_from_pool(kind, vni);  \
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq"); \
        MPIR_Request_add_ref((req));                                \
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
    (*MPIDI_OFI_WIN(win).issued_cntr)++;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_incr()
{
    MPIDI_OFI_global.rma_issued_cntr++;
}

/* Externs:  see util.c for definition */
int MPIDI_OFI_handle_cq_error_util(int ep_idx, ssize_t ret);
int MPIDI_OFI_retry_progress(void);
int MPIDI_OFI_control_handler(int handler_id, void *am_hdr, void *data, MPI_Aint data_sz,
                              int is_local, int is_async, MPIR_Request ** req);
int MPIDI_OFI_control_dispatch(void *buf);
void MPIDI_OFI_index_datatypes(void);
int MPIDI_OFI_mr_key_allocator_init(void);
uint64_t MPIDI_OFI_mr_key_alloc(void);
void MPIDI_OFI_mr_key_free(uint64_t index);
void MPIDI_OFI_mr_key_allocator_destroy(void);

/* RMA */
#define MPIDI_OFI_INIT_CHUNK_CONTEXT(win,sigreq)                        \
    do {                                                                \
        if (sigreq) {                                                   \
            int tmp;                                                    \
            MPIDI_OFI_chunk_request *creq;                              \
            MPIR_cc_incr((*sigreq)->cc_ptr, &tmp);                      \
            creq=(MPIDI_OFI_chunk_request*)MPL_malloc(sizeof(*creq), MPL_MEM_BUFFER); \
            MPIR_ERR_CHKANDSTMT(creq == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem"); \
            creq->event_id = MPIDI_OFI_EVENT_CHUNK_DONE;                \
            creq->parent = *(sigreq);                                   \
            msg.context = &creq->context;                               \
        }                                                               \
        MPIDI_OFI_win_cntr_incr(win);                                   \
    } while (0)

static inline uint32_t MPIDI_OFI_winfo_disp_unit(MPIR_Win * win, int rank)
{
    uint32_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WINFO_DISP_UNIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WINFO_DISP_UNIT);

    if (MPIDI_OFI_WIN(win).winfo)
        ret = MPIDI_OFI_WIN(win).winfo[rank].disp_unit;
    else
        ret = win->disp_unit;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WINFO_DISP_UNIT);
    return ret;
}

static inline void MPIDI_OFI_sigreq_complete(MPIR_Request ** sigreq)
{
    if (sigreq) {
        /* If sigreq is not NULL, *sigreq should be a valid object now. */
        MPIR_Assert(*sigreq != NULL);
        MPID_Request_complete(*sigreq);
    }
}

static inline void MPIDI_OFI_load_iov(const void *buffer, int count, MPI_Datatype datatype,
                                      MPI_Aint max_len,
                                      MPI_Aint * loaded_iov_offset, struct iovec *iov)
{
    MPI_Aint outlen;
    MPIR_Typerep_to_iov_offset(buffer, count, datatype, *loaded_iov_offset, iov, max_len, &outlen);
    *loaded_iov_offset += outlen;
}

int MPIDI_OFI_issue_deferred_rma(MPIR_Win * win);
int MPIDI_OFI_nopack_putget(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_disp, int target_count,
                            MPI_Datatype target_datatype, MPIR_Win * win,
                            MPIDI_av_entry_t * addr, int rma_type, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_put(const void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq);
int MPIDI_OFI_pack_get(void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq);

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

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_win_request_t *MPIDI_OFI_win_request_create(void)
{
    MPIDI_OFI_win_request_t *winreq;
    winreq = MPL_malloc(sizeof(*winreq), MPL_MEM_OTHER);
    return winreq;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_request_complete(MPIDI_OFI_win_request_t * winreq)
{
    if (winreq->rma_type == MPIDI_OFI_PUT)
        MPL_free(winreq->noncontig.put.origin.pack_buffer);

    if (winreq->rma_type == MPIDI_OFI_GET) {
        if (winreq->noncontig.get.origin.pack_buffer) {
            MPI_Aint actual_unpack_bytes;
            MPIR_Typerep_unpack(winreq->noncontig.get.origin.pack_buffer,
                                winreq->noncontig.get.origin.pack_size,
                                winreq->noncontig.get.origin.addr,
                                winreq->noncontig.get.origin.count,
                                winreq->noncontig.get.origin.datatype,
                                winreq->noncontig.get.origin.pack_offset -
                                winreq->noncontig.get.origin.pack_size, &actual_unpack_bytes);
            MPIR_Assert(winreq->noncontig.get.origin.pack_size == actual_unpack_bytes);
            MPL_free(winreq->noncontig.get.origin.pack_buffer);
        }
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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_av_insert(int vni, int rank, void *addrname)
{
    int mpi_errno = MPI_SUCCESS;
    fi_addr_t addr;
    MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[vni].av, addrname, 1, &addr, 0ULL, NULL),
                   avmap);
    MPIDI_OFI_AV(&MPIDIU_get_av(vni, rank)).dest[0][0] = addr;
#if MPIDI_OFI_ENABLE_ENDPOINTS_BITS
    MPIDI_OFI_AV(&MPIDIU_get_av(vni, rank)).ep_idx = 0;
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_av_to_phys(MPIDI_av_entry_t * av,
                                                        int vni_local, int vni_remote)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int ep_num = MPIDI_OFI_av_to_ep(&MPIDI_OFI_AV(av));
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[vni_local][vni_remote], ep_num,
                          MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_AV(av).dest[vni_local][vni_remote];
    }
#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        return fi_rx_addr(MPIDI_OFI_AV(av).dest[0][0], vni_remote, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        MPIR_Assert(vni_remote == 0);
        return MPIDI_OFI_AV(av).dest[0][0];
    }
#endif
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_comm_to_phys(MPIR_Comm * comm, int rank,
                                                          int vni_local, int vni_remote)
{
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, rank);
    return MPIDI_OFI_av_to_phys(av, vni_local, vni_remote);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOV);

    if (dt_datatype == MPI_DATATYPE_NULL)
        goto fn_exit;

    do {
        MPI_Aint tmp_size = (rem_size > max_pipe) ? max_pipe : rem_size;

        MPIR_Typerep_iov_len(dt_count, dt_datatype, tmp_size, &num_iov);
        total_iov += num_iov;

        rem_size -= tmp_size;
    } while (rem_size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    return total_iov;
}

#endif /* OFI_IMPL_H_INCLUDED */

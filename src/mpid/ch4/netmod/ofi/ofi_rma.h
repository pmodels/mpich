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
#ifndef NETMOD_OFI_RMA_H_INCLUDED
#define NETMOD_OFI_RMA_H_INCLUDED

#include "ofi_impl.h"

#define MPIDI_OFI_QUERY_ATOMIC_COUNT         0
#define MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT   1
#define MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT 2

#define MPIDI_OFI_INIT_CHUNK_CONTEXT(win,sigreq)                        \
    do {                                                                \
    if (sigreq) {                                                        \
        int tmp;                                                        \
        MPIDI_OFI_chunk_request *creq;                                  \
        MPIR_cc_incr((*sigreq)->cc_ptr, &tmp);                          \
        creq=(MPIDI_OFI_chunk_request*)MPL_malloc(sizeof(*creq));       \
        creq->event_id = MPIDI_OFI_EVENT_CHUNK_DONE;                    \
        creq->parent   = *sigreq;                                       \
        msg.context    = &creq->context;                                \
    }                                                                   \
    MPIDI_OFI_win_cntr_incr(win);                                       \
    } while (0)

#define MPIDI_OFI_INIT_SIGNAL_REQUEST(win,sigreq,flags,ep)              \
    do {                                                                \
        if (sigreq)                                                      \
        {                                                               \
            MPIDI_OFI_REQUEST_CREATE((*(sigreq)), MPIR_REQUEST_KIND__RMA); \
            MPIR_cc_set((*(sigreq))->cc_ptr, 0);                        \
            *(flags)                    = FI_COMPLETION | FI_DELIVERY_COMPLETE; \
            *(ep)                       = MPIDI_OFI_WIN(win).ep;        \
        }                                                               \
        else {                                                          \
            *(ep) = MPIDI_OFI_WIN(win).ep_nocmpl;                       \
            *(flags)                    = FI_DELIVERY_COMPLETE;         \
        }                                                               \
    } while (0)

#define MPIDI_OFI_GET_BASIC_TYPE(a,b)   \
    do {                                        \
        if (MPIR_DATATYPE_IS_PREDEFINED(a))     \
            b = a;                              \
        else {                                  \
            MPIR_Datatype *dt_ptr;              \
            MPID_Datatype_get_ptr(a,dt_ptr);    \
            b = dt_ptr->basic_type;             \
        }                                       \
    } while (0)

static inline uint32_t MPIDI_OFI_winfo_disp_unit(MPIR_Win * win, int rank)
{
    if (MPIDI_OFI_WIN(win).winfo)
        return MPIDI_OFI_WIN(win).winfo[rank].disp_unit;
    else
        return win->disp_unit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_query_datatype
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_query_datatype(MPI_Datatype dt,
                                           enum fi_datatype *fi_dt,
                                           MPI_Op op,
                                           enum fi_op *fi_op, size_t * count, size_t * dtsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_QUERY_DT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_QUERY_DT);
    MPIR_Datatype *dt_ptr;
    int op_index, dt_index, rc;

    MPID_Datatype_get_ptr(dt, dt_ptr);

    /* OP_NULL is the oddball                          */
    /* todo...change configure to table this correctly */
    dt_index = MPIDI_OFI_DATATYPE(dt_ptr).index;

    if (op == MPI_OP_NULL)
        op_index = 14;
    else
        op_index = (0x000000FFU & op) - 1;

    *fi_dt = (enum fi_datatype) MPIDI_Global.win_op_table[dt_index][op_index].dt;
    *fi_op = (enum fi_op) MPIDI_Global.win_op_table[dt_index][op_index].op;
    *dtsize = MPIDI_Global.win_op_table[dt_index][op_index].dtsize;

    if (*count == MPIDI_OFI_QUERY_ATOMIC_COUNT)
        *count = MPIDI_Global.win_op_table[dt_index][op_index].max_atomic_count;

    if (*count == MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT)
        *count = MPIDI_Global.win_op_table[dt_index][op_index].max_fetch_atomic_count;

    if (*count == MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT)
        *count = MPIDI_Global.win_op_table[dt_index][op_index].max_compare_atomic_count;

    if (((int) *fi_dt) == -1 || ((int) *fi_op) == -1)
        rc = -1;
    else
        rc = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_QUERY_DT);
    return rc;
}


static inline void MPIDI_OFI_win_datatype_basic(int count,
                                                MPI_Datatype datatype,
                                                MPIDI_OFI_win_datatype_t * dt)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_BASIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_BASIC);

    if (datatype != MPI_DATATYPE_NULL)
        MPIDI_Datatype_get_info(dt->count = count,
                                dt->type = datatype,
                                dt->contig, dt->size, dt->pointer, dt->true_lb);
    else
        memset(dt, 0, sizeof(*dt));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_BASIC);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_datatype_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_OFI_win_datatype_map(MPIDI_OFI_win_datatype_t * dt)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_MAP);

    if (dt->contig) {
        dt->num_contig = 1;
        dt->map = &dt->__map;
        dt->map[0].DLOOP_VECTOR_BUF = (void *) (size_t) dt->true_lb;
        dt->map[0].DLOOP_VECTOR_LEN = dt->size;
    }
    else {
        unsigned map_size = dt->pointer->max_contig_blocks * dt->count + 1;
        dt->num_contig = map_size;
        dt->map = (DLOOP_VECTOR *) MPL_malloc(map_size * sizeof(DLOOP_VECTOR));
        MPIR_Assert(dt->map != NULL);

        MPID_Segment seg;
        DLOOP_Offset last = dt->pointer->size * dt->count;
        MPIDU_Segment_init(NULL, dt->count, dt->type, &seg, 0);
        MPIDU_Segment_pack_vector(&seg, 0, &last, dt->map, &dt->num_contig);
        MPIR_Assert((unsigned) dt->num_contig <= map_size);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_WIN_DATATYPE_MAP);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_allocate_win_request_put_get(MPIR_Win * win,
                                                                    int origin_count,
                                                                    int target_count,
                                                                    int target_rank,
                                                                    MPI_Datatype origin_datatype,
                                                                    MPI_Datatype target_datatype,
                                                                    MPIDI_OFI_win_request_t **
                                                                    winreq, uint64_t * flags,
                                                                    struct fid_ep **ep,
                                                                    MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t o_size, t_size;
    MPIDI_OFI_win_request_t *req;

    o_size = sizeof(struct iovec);
    t_size = sizeof(struct fi_rma_iov);
    req = MPIDI_OFI_win_request_alloc_and_init(MPIDI_Global.iov_limit * (o_size + t_size));
    *winreq = req;

    req->noncontig->buf.iov.put_get.originv = (struct iovec *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.put_get.targetv =
        (struct fi_rma_iov *) &req->noncontig->buf.iov_store[o_size * MPIDI_Global.iov_limit];
    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, flags, ep);
    MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &req->noncontig->target_dt);
    MPIR_ERR_CHKANDJUMP((req->noncontig->origin_dt.size != req->noncontig->target_dt.size),
                        mpi_errno, MPI_ERR_SIZE, "**rmasize");

    MPIDI_OFI_win_datatype_map(&req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_map(&req->noncontig->target_dt);
    req->target_rank = target_rank;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_allocate_win_request_accumulate(MPIR_Win * win,
                                                                       int origin_count,
                                                                       int target_count,
                                                                       int target_rank,
                                                                       MPI_Datatype origin_datatype,
                                                                       MPI_Datatype target_datatype,
                                                                       MPIDI_OFI_win_request_t **
                                                                       winreq, uint64_t * flags,
                                                                       struct fid_ep **ep,
                                                                       MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t o_size, t_size;
    MPIDI_OFI_win_request_t *req;

    o_size = sizeof(struct fi_ioc);
    t_size = sizeof(struct fi_rma_ioc);
    req = MPIDI_OFI_win_request_alloc_and_init(MPIDI_Global.iov_limit * (o_size + t_size));
    *winreq = req;

    req->noncontig->buf.iov.accumulate.originv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.accumulate.targetv =
        (struct fi_rma_ioc *) &req->noncontig->buf.iov_store[o_size * MPIDI_Global.iov_limit];
    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, flags, ep);
    MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &req->noncontig->target_dt);
    MPIR_ERR_CHKANDJUMP((req->noncontig->origin_dt.size != req->noncontig->target_dt.size),
                        mpi_errno, MPI_ERR_SIZE, "**rmasize");

    MPIDI_OFI_win_datatype_map(&req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_map(&req->noncontig->target_dt);
    req->target_rank = target_rank;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_allocate_win_request_get_accumulate(MPIR_Win * win,
                                                                           int origin_count,
                                                                           int target_count,
                                                                           int result_count,
                                                                           int target_rank,
                                                                           MPI_Op op,
                                                                           MPI_Datatype
                                                                           origin_datatype,
                                                                           MPI_Datatype
                                                                           target_datatype,
                                                                           MPI_Datatype
                                                                           result_datatype,
                                                                           MPIDI_OFI_win_request_t
                                                                           ** winreq,
                                                                           uint64_t * flags,
                                                                           struct fid_ep **ep,
                                                                           MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t o_size, t_size, r_size;
    MPIDI_OFI_win_request_t *req;

    o_size = sizeof(struct fi_ioc);
    t_size = sizeof(struct fi_rma_ioc);
    r_size = sizeof(struct fi_ioc);
    req = MPIDI_OFI_win_request_alloc_and_init(MPIDI_Global.iov_limit * (o_size + t_size + r_size));
    *winreq = req;

    req->noncontig->buf.iov.get_accumulate.originv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.get_accumulate.targetv =
        (struct fi_rma_ioc *) &req->noncontig->buf.iov_store[o_size * MPIDI_Global.iov_limit];
    req->noncontig->buf.iov.get_accumulate.resultv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[o_size * MPIDI_Global.iov_limit +
                                                         t_size * MPIDI_Global.rma_iov_limit];
    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, flags, ep);
    MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &req->noncontig->target_dt);
    MPIDI_OFI_win_datatype_basic(result_count, result_datatype, &req->noncontig->result_dt);

    MPIR_ERR_CHKANDJUMP((req->noncontig->origin_dt.size != req->noncontig->target_dt.size &&
                         op != MPI_NO_OP), mpi_errno, MPI_ERR_SIZE, "**rmasize");
    MPIR_ERR_CHKANDJUMP((req->noncontig->result_dt.size != req->noncontig->target_dt.size),
                        mpi_errno, MPI_ERR_SIZE, "**rmasize");

    if (op != MPI_NO_OP)
        MPIDI_OFI_win_datatype_map(&req->noncontig->origin_dt);

    MPIDI_OFI_win_datatype_map(&req->noncontig->target_dt);
    MPIDI_OFI_win_datatype_map(&req->noncontig->result_dt);

    req->target_rank = target_rank;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_put(const void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count,
                                   MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIR_Request ** sigreq)
{
    int rc, mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_win_request_t *req;
    size_t offset, omax, tmax, tout, oout;
    uint64_t flags;
    struct fid_ep *ep;
    struct fi_msg_rma msg;
    unsigned i;
    struct iovec *originv;
    struct fi_rma_iov *targetv;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_DO_PUT);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_put_get(win,
                                                                  origin_count,
                                                                  target_count,
                                                                  target_rank,
                                                                  origin_datatype,
                                                                  target_datatype,
                                                                  &req, &flags, &ep, sigreq));

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    req->event_id = MPIDI_OFI_EVENT_ABORT;
    msg.desc = NULL;
    msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, req->target_rank, MPIDI_OFI_API_RMA);
    msg.context = NULL;
    msg.data = 0;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;
    MPIDI_OFI_init_iovec_state(&req->noncontig->iovs,
                               (uintptr_t) origin_addr,
                               MPIDI_OFI_winfo_base(win, req->target_rank) + offset,
                               req->noncontig->origin_dt.num_contig,
                               req->noncontig->target_dt.num_contig,
                               INT64_MAX,
                               req->noncontig->origin_dt.map, req->noncontig->target_dt.map);
    rc = MPIDI_OFI_IOV_EAGAIN;

    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = req->noncontig->buf.iov.put_get.originv;
        targetv = req->noncontig->buf.iov.put_get.targetv;
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, originv,
                                      omax, targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        for (i = 0; i < tout; i++)
            targetv[i].key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIR_Assert(rc != MPIDI_OFI_IOV_ERROR);
        msg.msg_iov = originv;
        msg.iov_count = oout;
        msg.rma_iov = targetv;
        msg.rma_iov_count = tout;
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq),
                              fi_writemsg(ep, &msg, flags), rdma_write);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_put(const void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_PUT);
    int target_contig, origin_contig, mpi_errno = MPI_SUCCESS;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb;
    size_t offset;

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);
    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count,
                                        target_contig, target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count,
                                        origin_contig, origin_bytes, origin_true_lb);

    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL)))
        goto fn_exit;

    if (target_rank == win->comm_ptr->rank) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        goto fn_exit;
    }

    if (origin_contig && target_contig && origin_bytes <= MPIDI_Global.max_buffered_write) {
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_win_cntr_incr(win),
                              fi_inject_write(MPIDI_OFI_WIN(win).ep_nocmpl,
                                              (char *) origin_addr + origin_true_lb, target_bytes,
                                              MPIDI_OFI_comm_to_phys(win->comm_ptr, target_rank,
                                                                     MPIDI_OFI_API_RMA),
                                              (uint64_t) MPIDI_OFI_winfo_base(win, target_rank)
                                              + target_disp * MPIDI_OFI_winfo_disp_unit(win,
                                                                                        target_rank)
                                              + target_true_lb, MPIDI_OFI_winfo_mr_key(win,
                                                                                       target_rank)),
                              rdma_inject_write);
    }
    else {
        mpi_errno = MPIDI_OFI_do_put(origin_addr,
                                     origin_count,
                                     origin_datatype,
                                     target_rank,
                                     target_disp, target_count, target_datatype, win, NULL);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_get(void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count,
                                   MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIR_Request ** sigreq)
{
    int rc, mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_win_request_t *req;
    size_t offset, omax, tmax, tout, oout;
    uint64_t flags;
    struct fid_ep *ep;
    struct fi_msg_rma msg;
    struct iovec *originv;
    struct fi_rma_iov *targetv;
    unsigned i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_DO_GET);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_put_get(win, origin_count, target_count,
                                                                  target_rank,
                                                                  origin_datatype, target_datatype,
                                                                  &req, &flags, &ep, sigreq));

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
    req->event_id = MPIDI_OFI_EVENT_ABORT;
    msg.desc = NULL;
    msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, req->target_rank, MPIDI_OFI_API_RMA);
    msg.context = NULL;
    msg.data = 0;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;
    MPIDI_OFI_init_iovec_state(&req->noncontig->iovs,
                               (uintptr_t) origin_addr,
                               MPIDI_OFI_winfo_base(win, req->target_rank) + offset,
                               req->noncontig->origin_dt.num_contig,
                               req->noncontig->target_dt.num_contig,
                               INT64_MAX,
                               req->noncontig->origin_dt.map, req->noncontig->target_dt.map);
    rc = MPIDI_OFI_IOV_EAGAIN;

    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = req->noncontig->buf.iov.put_get.originv;
        targetv = req->noncontig->buf.iov.put_get.targetv;
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, originv,
                                      omax, targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        MPIR_Assert(rc != MPIDI_OFI_IOV_ERROR);

        for (i = 0; i < tout; i++)
            targetv[i].key = MPIDI_OFI_winfo_mr_key(win, target_rank);

        msg.msg_iov = originv;
        msg.iov_count = oout;
        msg.rma_iov = targetv;
        msg.rma_iov_count = tout;
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq),
                              fi_readmsg(ep, &msg, flags), rdma_write);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_DO_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_get(void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype, MPIR_Win * win)
{
    int origin_contig, target_contig, mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_win_datatype_t origin_dt, target_dt;
    size_t origin_bytes;
    size_t offset;
    struct fi_rma_iov riov;
    struct iovec iov;
    struct fi_msg_rma msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_GET);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);
    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_check_contig_size(origin_datatype, origin_count, origin_contig, origin_bytes);

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL)))
        goto fn_exit;

    if (target_rank == win->comm_ptr->rank) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
    }

    MPIDI_Datatype_check_contig(origin_datatype, origin_contig);
    MPIDI_Datatype_check_contig(target_datatype, target_contig);

    if (origin_contig && target_contig) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &origin_dt);
        MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &target_dt);
        MPIR_ERR_CHKANDJUMP((origin_dt.size != target_dt.size),
                            mpi_errno, MPI_ERR_SIZE, "**rmasize");

        msg.desc = NULL;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, target_rank, MPIDI_OFI_API_RMA);
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        msg.context = NULL;
        msg.data = 0;
        iov.iov_base = (char *) origin_addr + origin_dt.true_lb;
        iov.iov_len = target_dt.size;
        riov.addr =
            (uint64_t) (MPIDI_OFI_winfo_base(win, target_rank) + offset +
                        target_dt.true_lb);
        riov.len = target_dt.size;
        riov.key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_win_cntr_incr(win),
                              fi_readmsg(MPIDI_OFI_WIN(win).ep_nocmpl, &msg, 0), rdma_write);
    }
    else {
        mpi_errno = MPIDI_OFI_do_get(origin_addr,
                                     origin_count,
                                     origin_datatype,
                                     target_rank,
                                     target_disp, target_count, target_datatype, win, NULL);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_rput(const void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RPUT);
    int mpi_errno;
    size_t origin_bytes;
    size_t offset;
    MPIR_Request *rreq;

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL))) {
        mpi_errno = MPI_SUCCESS;
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(rreq);
        MPIDI_CH4U_request_complete(rreq);
        goto fn_exit;
    }

    if (target_rank == win->comm_ptr->rank) {
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(rreq);
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        MPIDI_CH4U_request_complete(rreq);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_put((void *) origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, &rreq);

  fn_exit:
    *request = rreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RPUT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_compare_and_swap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                const void *compare_addr,
                                                void *result_addr,
                                                MPI_Datatype datatype,
                                                int target_rank, MPI_Aint target_disp,
                                                MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    MPIDI_OFI_win_datatype_t origin_dt, target_dt, result_dt;
    size_t offset, max_size, dt_size;
    void *buffer, *tbuffer, *rbuffer;
    struct fi_ioc originv, resultv, comparev;
    struct fi_rma_ioc targetv;
    struct fi_msg_atomic msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_COMPARE_AND_SWAP);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_OFI_win_datatype_basic(1, datatype, &origin_dt);
    MPIDI_OFI_win_datatype_basic(1, datatype, &result_dt);
    MPIDI_OFI_win_datatype_basic(1, datatype, &target_dt);

    if ((origin_dt.size == 0) || (target_rank == MPI_PROC_NULL))
        goto fn_exit;

    buffer = (char *) origin_addr + origin_dt.true_lb;
    rbuffer = (char *) result_addr + result_dt.true_lb;
    tbuffer = (void *) (MPIDI_OFI_winfo_base(win, target_rank) + offset);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    max_size = MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT;
    MPIDI_OFI_query_datatype(datatype, &fi_dt, MPI_OP_NULL, &fi_op, &max_size, &dt_size);

    originv.addr = (void *) buffer;
    originv.count = 1;
    resultv.addr = (void *) rbuffer;
    resultv.count = 1;
    comparev.addr = (void *) compare_addr;
    comparev.count = 1;
    targetv.addr = (uint64_t) tbuffer;
    targetv.count = 1;
    targetv.key = MPIDI_OFI_winfo_mr_key(win, target_rank);;

    msg.msg_iov = &originv;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, target_rank, MPIDI_OFI_API_RMA);
    msg.rma_iov = &targetv;
    msg.rma_iov_count = 1;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    msg.context = NULL;
    msg.data = 0;
    MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_win_cntr_incr(win),
                          fi_compare_atomicmsg(MPIDI_OFI_WIN(win).ep_nocmpl, &msg,
                                               &comparev, NULL, 1, &resultv, NULL, 1, 0), atomicto);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_accumulate(const void *origin_addr,
                                          int origin_count,
                                          MPI_Datatype origin_datatype,
                                          int target_rank,
                                          MPI_Aint target_disp,
                                          int target_count,
                                          MPI_Datatype target_datatype,
                                          MPI_Op op, MPIR_Win * win, MPIR_Request ** sigreq)
{
    int rc, acccheck = 0, mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    MPIDI_OFI_win_request_t *req;
    size_t offset, max_size, dt_size, omax, tmax, tout, oout;
    struct fid_ep *ep;
    MPI_Datatype basic_type;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    struct fi_msg_atomic msg;
    struct fi_ioc *originv;
    struct fi_rma_ioc *targetv;
    unsigned i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_DO_ACCUMULATE);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_accumulate(win,
                                                                     origin_count,
                                                                     target_count,
                                                                     target_rank,
                                                                     origin_datatype,
                                                                     target_datatype,
                                                                     &req, &flags, &ep, sigreq));

    if ((req->noncontig->origin_dt.size == 0) || (target_rank == MPI_PROC_NULL)) {
        MPIDI_OFI_win_request_complete(req);

        if (sigreq)
            MPIDI_CH4U_request_release(*sigreq);

        return MPI_SUCCESS;
    }

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    MPIDI_OFI_GET_BASIC_TYPE(target_datatype, basic_type);

    switch (basic_type) {
        /* 8 byte types */
    case MPI_FLOAT_INT:
    case MPI_2INT:
    case MPI_LONG_INT:
#ifdef HAVE_FORTRAN_BINDING
    case MPI_2REAL:
    case MPI_2INTEGER:
#endif
        {
            basic_type = MPI_LONG_LONG;
            acccheck = 1;
            break;
        }

        /* 16-byte types */
#ifdef HAVE_FORTRAN_BINDING

    case MPI_2DOUBLE_PRECISION:
#endif
#ifdef MPICH_DEFINE_2COMPLEX
    case MPI_2COMPLEX:
#endif
        {
            basic_type = MPI_DOUBLE_COMPLEX;
            acccheck = 1;
            break;
        }

        /* Types with pads or too large to handle */
    case MPI_DATATYPE_NULL:
    case MPI_SHORT_INT:
    case MPI_DOUBLE_INT:
    case MPI_LONG_DOUBLE_INT:
#ifdef MPICH_DEFINE_2COMPLEX
    case MPI_2DOUBLE_COMPLEX:
#endif
        goto am_fallback;
    }

    if (acccheck && op != MPI_REPLACE)
        goto am_fallback;

    max_size = MPIDI_OFI_QUERY_ATOMIC_COUNT;

    MPIDI_OFI_query_datatype(basic_type, &fi_dt, op, &fi_op, &max_size, &dt_size);
    if (max_size == 0)
        goto am_fallback;

    req->event_id = MPIDI_OFI_EVENT_ABORT;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;
    max_size = max_size * dt_size;

    MPIDI_OFI_init_iovec_state(&req->noncontig->iovs,
                               (uintptr_t) origin_addr,
                               MPIDI_OFI_winfo_base(win, req->target_rank) + offset,
                               req->noncontig->origin_dt.num_contig,
                               req->noncontig->target_dt.num_contig,
                               max_size,
                               req->noncontig->origin_dt.map, req->noncontig->target_dt.map);

    msg.desc = NULL;
    msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, req->target_rank, MPIDI_OFI_API_RMA);
    msg.context = NULL;
    msg.data = 0;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    rc = MPIDI_OFI_IOV_EAGAIN;

    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = req->noncontig->buf.iov.accumulate.originv;
        targetv = req->noncontig->buf.iov.accumulate.targetv;
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, (struct iovec *) originv, omax,
                                      (struct fi_rma_iov *) targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        MPIR_Assert(rc != MPIDI_OFI_IOV_ERROR);

        for (i = 0; i < tout; i++)
            targetv[i].key = MPIDI_OFI_winfo_mr_key(win, target_rank);

        for (i = 0; i < oout; i++)
            originv[i].count /= dt_size;

        for (i = 0; i < tout; i++)
            targetv[i].count /= dt_size;

        msg.msg_iov = originv;
        msg.iov_count = oout;
        msg.rma_iov = targetv;
        msg.rma_iov_count = tout;
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq),
                              fi_atomicmsg(ep, &msg, flags), rdma_atomicto);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_DO_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    /* Fall back to active message */
    MPIDI_OFI_win_request_complete(req);
    return MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win, MPIDI_NM);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_get_accumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              void *result_addr,
                                              int result_count,
                                              MPI_Datatype result_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype,
                                              MPI_Op op, MPIR_Win * win, MPIR_Request ** sigreq)
{
    int rc, acccheck = 0, mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    MPIDI_OFI_win_request_t *req;
    size_t offset, max_size, dt_size, omax, rmax, tmax, tout, rout, oout;
    struct fid_ep *ep;
    MPI_Datatype rt, basic_type, basic_type_res;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    struct fi_msg_atomic msg;
    struct fi_ioc *originv, *resultv;
    struct fi_rma_ioc *targetv;
    unsigned i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_get_accumulate(win,
                                                                         origin_count,
                                                                         target_count,
                                                                         result_count,
                                                                         target_rank,
                                                                         op,
                                                                         origin_datatype,
                                                                         target_datatype,
                                                                         result_datatype,
                                                                         &req, &flags, &ep,
                                                                         sigreq));

    if ((req->noncontig->result_dt.size == 0) || (target_rank == MPI_PROC_NULL)) {
        MPIDI_OFI_win_request_complete(req);

        if (sigreq)
            MPIDI_CH4U_request_release(*sigreq);

        goto fn_exit;
    }

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    MPIDI_OFI_GET_BASIC_TYPE(target_datatype, basic_type);
    rt = result_datatype;

    switch (basic_type) {
        /* 8 byte types */
    case MPI_FLOAT_INT:
    case MPI_2INT:
    case MPI_LONG_INT:
#ifdef HAVE_FORTRAN_BINDING
    case MPI_2REAL:
    case MPI_2INTEGER:
#endif
        {
            basic_type = rt = MPI_LONG_LONG;
            acccheck = 1;
            break;
        }

        /* 16-byte types */
#ifdef HAVE_FORTRAN_BINDING

    case MPI_2DOUBLE_PRECISION:
#endif
#ifdef MPICH_DEFINE_2COMPLEX
    case MPI_2COMPLEX:
#endif
        {
            basic_type = rt = MPI_DOUBLE_COMPLEX;
            acccheck = 1;
            break;
        }

        /* Types with pads or too large to handle */
    case MPI_DATATYPE_NULL:
    case MPI_SHORT_INT:
    case MPI_DOUBLE_INT:
    case MPI_LONG_DOUBLE_INT:
#ifdef MPICH_DEFINE_2COMPLEX
    case MPI_2DOUBLE_COMPLEX:
#endif
        goto am_fallback;
        break;
    }

    if (acccheck && op != MPI_REPLACE && op != MPI_NO_OP)
        goto am_fallback;

    MPIDI_OFI_GET_BASIC_TYPE(rt, basic_type_res);
    MPIR_Assert(basic_type_res != MPI_DATATYPE_NULL);

    max_size = MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT;
    MPIDI_OFI_query_datatype(basic_type_res, &fi_dt, op, &fi_op, &max_size, &dt_size);
    max_size = max_size * dt_size;
    if (max_size == 0)
        goto am_fallback;

    req->event_id = MPIDI_OFI_EVENT_RMA_DONE;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;

    if (op != MPI_NO_OP)
        MPIDI_OFI_init_iovec_state2(&req->noncontig->iovs,
                                    (uintptr_t) origin_addr,
                                    (uintptr_t) result_addr,
                                    MPIDI_OFI_winfo_base(win, req->target_rank) + offset,
                                    req->noncontig->origin_dt.num_contig,
                                    req->noncontig->result_dt.num_contig,
                                    req->noncontig->target_dt.num_contig, max_size,
                                    req->noncontig->origin_dt.map, req->noncontig->result_dt.map,
                                    req->noncontig->target_dt.map);
    else
        MPIDI_OFI_init_iovec_state(&req->noncontig->iovs,
                                   (uintptr_t) result_addr,
                                   MPIDI_OFI_winfo_base(win, req->target_rank) + offset,
                                   req->noncontig->result_dt.num_contig,
                                   req->noncontig->target_dt.num_contig,
                                   max_size,
                                   req->noncontig->result_dt.map, req->noncontig->target_dt.map);

    msg.desc = NULL;
    msg.addr = MPIDI_OFI_comm_to_phys(win->comm_ptr, req->target_rank, MPIDI_OFI_API_RMA);
    msg.context = NULL;
    msg.data = 0;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    rc = MPIDI_OFI_IOV_EAGAIN;

    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = req->noncontig->buf.iov.get_accumulate.originv;
        targetv = req->noncontig->buf.iov.get_accumulate.targetv;
        resultv = req->noncontig->buf.iov.get_accumulate.resultv;
        omax = rmax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;

        if (op != MPI_NO_OP)
            rc = MPIDI_OFI_merge_iov_list2(&req->noncontig->iovs, (struct iovec *) originv,
                                           omax, (struct iovec *) resultv, rmax,
                                           (struct fi_rma_iov *) targetv, tmax, &oout, &rout,
                                           &tout);
        else {
            oout = 0;
            rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, (struct iovec *) resultv,
                                          rmax, (struct fi_rma_iov *) targetv, tmax, &rout, &tout);
        }

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        MPIR_Assert(rc != MPIDI_OFI_IOV_ERROR);

        for (i = 0; i < oout; i++)
            originv[i].count /= dt_size;

        for (i = 0; i < rout; i++)
            resultv[i].count /= dt_size;

        for (i = 0; i < tout; i++) {
            targetv[i].count /= dt_size;
            targetv[i].key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        }

        msg.msg_iov = originv;
        msg.iov_count = oout;
        msg.rma_iov = targetv;
        msg.rma_iov_count = tout;
        MPIDI_OFI_CALL_RETRY2(MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq),
                              fi_fetch_atomicmsg(ep, &msg, resultv,
                                                 NULL, rout, flags), rdma_readfrom);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    MPIDI_OFI_win_request_complete(req);
    return MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, MPIDI_NM);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                           int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank,
                                           MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype,
                                           MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RACCUMULATE);
    MPIR_Request *rreq;
    int mpi_errno = MPIDI_OFI_do_accumulate((void *) origin_addr,
                                            origin_count,
                                            origin_datatype,
                                            target_rank,
                                            target_disp,
                                            target_count,
                                            target_datatype,
                                            op,
                                            win,
                                            &rreq);
    *request = rreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RACCUMULATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               void *result_addr,
                                               int result_count,
                                               MPI_Datatype result_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RGET_ACCUMULATE);

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, &rreq);
    *request = rreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RGET_ACCUMULATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_fetch_and_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                            void *result_addr,
                                            MPI_Datatype datatype,
                                            int target_rank,
                                            MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_FETCH_AND_OP);

    /*  This can be optimized by directly calling the fi directly
     *  and avoiding all the datatype processing of the full
     *  MPIDI_Get_accumulate
     */
    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, 1, datatype,
                                            result_addr, 1, datatype,
                                            target_rank, target_disp, 1, datatype, op, win, NULL);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_FETCH_AND_OP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_rget(void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RGET);
    int mpi_errno;
    size_t origin_bytes;
    size_t offset;
    MPIR_Request *rreq;

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL))) {
        mpi_errno = MPI_SUCCESS;
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(rreq);
        MPIDI_CH4U_request_complete(rreq);
        goto fn_exit;
    }

    if (target_rank == win->comm_ptr->rank) {
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(rreq);
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        MPIDI_CH4U_request_complete(rreq);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, &rreq);
  fn_exit:
    *request = rreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RGET);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              void *result_addr,
                                              int result_count,
                                              MPI_Datatype result_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype, MPI_Op op,
                                              MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);
    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, NULL);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_GET_ACCUMULATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                          int origin_count,
                                          MPI_Datatype origin_datatype,
                                          int target_rank,
                                          MPI_Aint target_disp,
                                          int target_count,
                                          MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_ACCUMULATE);
    int mpi_errno = MPIDI_OFI_do_accumulate(origin_addr,
                                            origin_count,
                                            origin_datatype,
                                            target_rank,
                                            target_disp,
                                            target_count,
                                            target_datatype,
                                            op,
                                            win,
                                            NULL);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_ACCUMULATE);
    return mpi_errno;
}

#endif /* NETMOD_OFI_RMA_H_INCLUDED */

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
#ifndef OFI_RMA_H_INCLUDED
#define OFI_RMA_H_INCLUDED

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

struct MPIDI_OFI_contig_blocks_params {
    size_t max_pipe;
    DLOOP_Count count;
    DLOOP_Offset last_loc;
    DLOOP_Offset start_loc;
    size_t       last_chunk;
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
        if(paramp->last_loc - paramp->start_loc > paramp->max_pipe) {
            paramp->count -= paramp->last_chunk;
            num = (paramp->last_loc - paramp->start_loc) / paramp->max_pipe;
            rem = (paramp->last_loc - paramp->start_loc) % paramp->max_pipe;
            if(rem) num++;
            paramp->last_chunk = num;
            paramp->count += num;
        }
    }
    else {
        /* new region */
        num  = size / paramp->max_pipe;
        rem  = size % paramp->max_pipe;
        if(rem) num++;

        paramp->last_chunk = num;
        paramp->last_loc   = rel_off + size;
        paramp->start_loc  = rel_off;
        paramp->count     += num;
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
    MPID_Segment dt_seg;
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
        MPIDU_Segment_init(NULL, 1, dt_datatype, &dt_seg, 0);
        MPIDU_Segment_manipulate(&dt_seg, 0, &dt_size,
                                 MPIDI_OFI_contig_count_block,
                                 NULL, NULL, NULL, NULL, (void *) &params);
        count1 = params.count;
        params.count = 0;
        params.last_loc = 0;
        params.start_loc = 0;
        params.last_chunk = 0;
        MPIDU_Segment_init(NULL, dtc, dt_datatype, &dt_seg, 0);
        MPIDU_Segment_manipulate(&dt_seg, 0, &dt_size,
                                 MPIDI_OFI_contig_count_block,
                                 NULL, NULL, NULL, NULL, (void *) &params);
        count2 = params.count;

        if (count2 == 1) {      /* Contiguous */
            num = (dt_size * dt_count) / max_pipe;
            rem = (dt_size * dt_count) % max_pipe;
            if (rem)
                num++;
            count += num;
        }
        else if (count2 < count1 * 2)
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

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_count_iovecs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_count_iovecs(int origin_count,
                               int target_count,
                               int result_count,
                               MPI_Datatype origin_datatype,
                               MPI_Datatype target_datatype,
                               MPI_Datatype result_datatype, size_t max_pipe, size_t * countp)
{
    /* Count the max number of iovecs that will be generated, given the iovs    */
    /* and maximum data size.  The code adds the iovecs from all three lists    */
    /* which is an upper bound for all three lists.  This is a tradeoff because */
    /* it will be very fast to calculate the estimate;  count_iov() only        */
    /* scans two elements of the datatype to make the estimate                  */
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    *countp = MPIDI_OFI_count_iov(origin_count, origin_datatype, max_pipe);
    *countp += MPIDI_OFI_count_iov(target_count, target_datatype, max_pipe);
    *countp += MPIDI_OFI_count_iov(result_count, result_datatype, max_pipe);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    return MPI_SUCCESS;
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
    MPIR_Datatype *dt_ptr;
    int op_index, dt_index, rc;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_QUERY_DATATYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_QUERY_DATATYPE);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_QUERY_DATATYPE);
    return rc;
}


static inline void MPIDI_OFI_win_datatype_basic(int count,
                                                MPI_Datatype datatype,
                                                MPIDI_OFI_win_datatype_t * dt)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_BASIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_BASIC);

    if (datatype != MPI_DATATYPE_NULL)
        MPIDI_Datatype_get_info(dt->count = count,
                                dt->type = datatype,
                                dt->contig, dt->size, dt->pointer, dt->true_lb);
    else
        memset(dt, 0, sizeof(*dt));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_BASIC);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_datatype_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_OFI_win_datatype_map(MPIDI_OFI_win_datatype_t * dt)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_MAP);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_DATATYPE_MAP);
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
    size_t o_size, t_size, alloc_iovs;
    MPIDI_OFI_win_request_t *req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_PUT_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_PUT_GET);

    o_size = sizeof(struct iovec);
    t_size = sizeof(struct fi_rma_iov);
    /* An upper bound on the iov limit */
    MPIDI_OFI_count_iovecs(origin_count, target_count, 0, origin_datatype,
                           target_datatype, MPI_DATATYPE_NULL, INT64_MAX, &alloc_iovs);

    req = MPIDI_OFI_win_request_alloc_and_init(alloc_iovs * (o_size + t_size));
    *winreq = req;

    req->noncontig->buf.iov.put_get.originv = (struct iovec *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.put_get.targetv =
        (struct fi_rma_iov *) &req->noncontig->buf.iov_store[o_size * alloc_iovs];
    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, flags, ep);
    MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &req->noncontig->target_dt);
    MPIR_ERR_CHKANDJUMP((req->noncontig->origin_dt.size != req->noncontig->target_dt.size),
                        mpi_errno, MPI_ERR_SIZE, "**rmasize");

    MPIDI_OFI_win_datatype_map(&req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_map(&req->noncontig->target_dt);
    req->target_rank = target_rank;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_PUT_GET);
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
                                                                       size_t max_pipe,
                                                                       MPIDI_OFI_win_request_t **
                                                                       winreq, uint64_t * flags,
                                                                       struct fid_ep **ep,
                                                                       MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t o_size, t_size, alloc_iovs;
    MPIDI_OFI_win_request_t *req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_ACCUMULATE);

    o_size = sizeof(struct fi_ioc);
    t_size = sizeof(struct fi_rma_ioc);
    /* An upper bound on the iov limit. */
    MPIDI_OFI_count_iovecs(origin_count, target_count, 0, origin_datatype,
                           target_datatype, MPI_DATATYPE_NULL, max_pipe, &alloc_iovs);
    req = MPIDI_OFI_win_request_alloc_and_init(alloc_iovs * (o_size + t_size));
    *winreq = req;

    req->noncontig->buf.iov.accumulate.originv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.accumulate.targetv =
        (struct fi_rma_ioc *) &req->noncontig->buf.iov_store[o_size * alloc_iovs];
    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, flags, ep);
    MPIDI_OFI_win_datatype_basic(origin_count, origin_datatype, &req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_basic(target_count, target_datatype, &req->noncontig->target_dt);
    MPIR_ERR_CHKANDJUMP((req->noncontig->origin_dt.size != req->noncontig->target_dt.size),
                        mpi_errno, MPI_ERR_SIZE, "**rmasize");

    MPIDI_OFI_win_datatype_map(&req->noncontig->origin_dt);
    MPIDI_OFI_win_datatype_map(&req->noncontig->target_dt);
    req->target_rank = target_rank;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_ACCUMULATE);
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
                                                                           size_t max_pipe,
                                                                           MPIDI_OFI_win_request_t
                                                                           ** winreq,
                                                                           uint64_t * flags,
                                                                           struct fid_ep **ep,
                                                                           MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t o_size, t_size, r_size, alloc_iovs, alloc_rma_iovs;
    MPIDI_OFI_win_request_t *req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_GET_ACCUMULATE);

    o_size = sizeof(struct fi_ioc);
    t_size = sizeof(struct fi_rma_ioc);
    r_size = sizeof(struct fi_ioc);

    /* An upper bound on the iov limit. */
    MPIDI_OFI_count_iovecs(origin_count, target_count, result_count, origin_datatype,
                           target_datatype, result_datatype, max_pipe, &alloc_iovs);
    alloc_rma_iovs = alloc_iovs;

    req = MPIDI_OFI_win_request_alloc_and_init(alloc_iovs * (o_size + t_size + r_size));
    *winreq = req;

    req->noncontig->buf.iov.get_accumulate.originv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[0];
    req->noncontig->buf.iov.get_accumulate.targetv =
        (struct fi_rma_ioc *) &req->noncontig->buf.iov_store[o_size * alloc_iovs];
    req->noncontig->buf.iov.get_accumulate.resultv =
        (struct fi_ioc *) &req->noncontig->buf.iov_store[o_size * alloc_iovs +
                                                         t_size * alloc_rma_iovs];
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_ALLOCATE_WIN_REQUEST_GET_ACCUMULATE);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_PUT);

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

    size_t cur_o = 0, cur_t = 0;
    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = &req->noncontig->buf.iov.put_get.originv[cur_o];
        targetv = &req->noncontig->buf.iov.put_get.targetv[cur_t];
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, originv,
                                      omax, targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        cur_o += oout;
        cur_t += tout;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_PUT);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_PUT);
    int target_contig, origin_contig, mpi_errno = MPI_SUCCESS;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb;
    size_t offset;

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win);
        goto fn_exit;
    }

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_PUT);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_GET);

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

    size_t cur_o = 0, cur_t = 0;
    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = &req->noncontig->buf.iov.put_get.originv[cur_o];
        targetv = &req->noncontig->buf.iov.put_get.targetv[cur_t];
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, originv,
                                      omax, targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        cur_o += oout;
        cur_t += tout;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_GET);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET);

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win);
        goto fn_exit;
    }

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
            (uint64_t) (MPIDI_OFI_winfo_base(win, target_rank) + offset + target_dt.true_lb);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RPUT);
    int mpi_errno;
    size_t origin_bytes;
    size_t offset;

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4U_mpi_rput(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, win, request);
        goto fn_exit;
    }

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL))) {
        mpi_errno = MPI_SUCCESS;
        *request = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(*request);
        MPIDI_CH4U_request_complete(*request);
    }
    else if (target_rank == win->comm_ptr->rank) {
        *request = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(*request);
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        MPIDI_CH4U_request_complete(*request);
    }
    else {
        mpi_errno = MPIDI_OFI_do_put((void *) origin_addr,
                                     origin_count,
                                     origin_datatype,
                                     target_rank,
                                     target_disp, target_count, target_datatype, win, request);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RPUT);
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

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr,
                                                    result_addr, datatype,
                                                    target_rank, target_disp,
                                                    win);
        goto fn_exit;
    }

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_accumulate_get_basic_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline MPI_Datatype MPIDI_OFI_accumulate_get_basic_type(MPI_Datatype target_datatype,
                                                               int *acc_check)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_BASIC_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_BASIC_TYPE);
    MPI_Datatype basic_type;
    *acc_check = 0;
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
            *acc_check = 1;
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
            *acc_check = 1;
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
        goto unsupported;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_BASIC_TYPE);
    return basic_type;
  unsupported:
    basic_type = MPI_DATATYPE_NULL;
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
    int rc, acc_check = 0, mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    MPIDI_OFI_win_request_t *req;
    size_t origin_bytes, offset, max_count, max_size, dt_size, omax, tmax, tout, oout;
    struct fid_ep *ep;
    MPI_Datatype basic_type;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    struct fi_msg_atomic msg;
    struct fi_ioc *originv;
    struct fi_rma_ioc *targetv;
    unsigned i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) goto am_fallback;

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);
    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);
    if ((origin_bytes == 0) || (target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        if (sigreq) {
            *sigreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_Request_add_ref(*sigreq);
            MPIDI_CH4U_request_complete(*sigreq);
        }
        goto fn_exit;
    }

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);

    basic_type = MPIDI_OFI_accumulate_get_basic_type(target_datatype, &acc_check);
    if (basic_type == MPI_DATATYPE_NULL || (acc_check && op != MPI_REPLACE))
        goto am_fallback;

    max_count = MPIDI_OFI_QUERY_ATOMIC_COUNT;

    MPIDI_OFI_query_datatype(basic_type, &fi_dt, op, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;
    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_accumulate
                           (win, origin_count, target_count, target_rank, origin_datatype,
                            target_datatype, max_count, &req, &flags, &ep, sigreq));
    max_size = max_count * dt_size;

    req->event_id = MPIDI_OFI_EVENT_ABORT;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;

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

    size_t cur_o = 0, cur_t = 0;
    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = &req->noncontig->buf.iov.accumulate.originv[cur_o];
        targetv = &req->noncontig->buf.iov.accumulate.targetv[cur_t];
        omax = MPIDI_Global.iov_limit;
        tmax = MPIDI_Global.rma_iov_limit;
        rc = MPIDI_OFI_merge_iov_list(&req->noncontig->iovs, (struct iovec *) originv, omax,
                                      (struct fi_rma_iov *) targetv, tmax, &oout, &tout);

        if (rc == MPIDI_OFI_IOV_DONE)
            break;

        MPIR_Assert(rc != MPIDI_OFI_IOV_ERROR);

        cur_o += oout;
        cur_t += tout;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    return MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win);
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
    int rc, acc_check = 0, mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    MPIDI_OFI_win_request_t *req;
    size_t target_bytes, offset, max_count, max_size, dt_size, omax, rmax, tmax, tout, rout, oout;
    struct fid_ep *ep;
    MPI_Datatype rt, basic_type, basic_type_res;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    struct fi_msg_atomic msg;
    struct fi_ioc *originv, *resultv;
    struct fi_rma_ioc *targetv;
    unsigned i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) goto am_fallback;

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_bytes);
    if ((target_bytes == 0) || (target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        if (sigreq) {
            *sigreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_Request_add_ref(*sigreq);
            MPIDI_CH4U_request_complete(*sigreq);
        }
        goto fn_exit;
    }

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    rt = result_datatype;
    basic_type = MPIDI_OFI_accumulate_get_basic_type(target_datatype, &acc_check);
    if (basic_type == MPI_DATATYPE_NULL || (acc_check && op != MPI_REPLACE && op != MPI_NO_OP))
        goto am_fallback;

    if (acc_check)
        rt = basic_type;
    MPIDI_OFI_GET_BASIC_TYPE(rt, basic_type_res);
    MPIR_Assert(basic_type_res != MPI_DATATYPE_NULL);

    max_count = MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT;
    MPIDI_OFI_query_datatype(basic_type_res, &fi_dt, op, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_allocate_win_request_get_accumulate
                           (win, origin_count, target_count, result_count, target_rank, op,
                            origin_datatype, target_datatype, result_datatype, max_count, &req,
                            &flags, &ep, sigreq));
    max_size = max_count * dt_size;

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

    size_t cur_o = 0, cur_t = 0, cur_r = 0;
    while (rc == MPIDI_OFI_IOV_EAGAIN) {
        originv = &req->noncontig->buf.iov.get_accumulate.originv[cur_o];
        targetv = &req->noncontig->buf.iov.get_accumulate.targetv[cur_t];
        resultv = &req->noncontig->buf.iov.get_accumulate.resultv[cur_r];
        omax = rmax = MPIDI_Global.iov_limit;
        tmax = MPIDI_OFI_FETCH_ATOMIC_IOVECS < 0 ? MPIDI_Global.rma_iov_limit : MPIDI_OFI_FETCH_ATOMIC_IOVECS;

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

        cur_o += oout;
        cur_t += tout;
        cur_r += rout;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    return MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win);
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, request);
    }
    else {
        mpi_errno = MPIDI_OFI_do_accumulate((void *) origin_addr,
                                            origin_count,
                                            origin_datatype,
                                            target_rank,
                                            target_disp,
                                            target_count,
                                            target_datatype,
                                            op,
                                            win,
                                            request);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, win, request);
    }
    else {
        mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, request);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                                target_rank, target_disp, op, win);
        goto fn_exit;
    }

    /*  This can be optimized by directly calling the fi directly
     *  and avoiding all the datatype processing of the full
     *  MPID_Get_accumulate
     */
    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, 1, datatype,
                                            result_addr, 1, datatype,
                                            target_rank, target_disp, 1, datatype, op, win, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET);
    int mpi_errno;
    size_t origin_bytes;
    size_t offset;

    if (!MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_CH4U_mpi_rget(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count, target_datatype, win,
                                        request);
        goto fn_exit;
    }

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);

    if (unlikely((origin_bytes == 0) || (target_rank == MPI_PROC_NULL))) {
        mpi_errno = MPI_SUCCESS;
        *request = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(*request);
        MPIDI_CH4U_request_complete(*request);
        goto fn_exit;
    }

    if (target_rank == win->comm_ptr->rank) {
        *request = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Request_add_ref(*request);
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        MPIDI_CH4U_request_complete(*request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);

    if (!MPIDI_OFI_ENABLE_ATOMICS) {
        mpi_errno = MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count, target_datatype, op,
                                              win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_accumulate(origin_addr,
                                            origin_count,
                                            origin_datatype,
                                            target_rank,
                                            target_disp,
                                            target_count,
                                            target_datatype,
                                            op,
                                            win,
                                            NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    return mpi_errno;
}

#endif /* OFI_RMA_H_INCLUDED */

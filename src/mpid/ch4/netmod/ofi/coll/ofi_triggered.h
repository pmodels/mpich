/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_TRIGGERED_H_INCLUDED
#define OFI_TRIGGERED_H_INCLUDED

#include "ofi_impl.h"

#define MPIDI_OFI_TRIGGERED_WINDOW 2

enum {
    MPIDI_OFI_TRIGGERED_TAGGED_SEND,
    MPIDI_OFI_TRIGGERED_TAGGED_RECV
};

enum {
    MPIDI_OFI_TRIGGERED_RMA_READ,
    MPIDI_OFI_TRIGGERED_RMA_WRITE
};

typedef struct {
    struct fi_deferred_work *works;
    int num_works;
    struct fid_cntr **completion_cntrs; /* counters to check for completion of the collective */
    uint64_t *cntr_val;         /* desired values of counters to check for completion of the collective */
    int len_completion_cntrs;   /* length of the array of completion counters */
    struct fid_cntr **all_cntrs;        /* need to store all counters for freeing them up */
    int len_all_cntrs;          /* length of the array of all counters */
    void **buffers;             /* need to store the buffers for freeing them up */
    int num_buffers;            /* number of buffers */
    struct fid_mr **r_mrs;      /* rma keys */
    int num_keys;               /* number of rma keys */
    bool is_rma;                /* flag to indicate if RMA is used */
    struct fid_domain *domain;
    bool is_persistent;
} MPIDI_OFI_trigger_t;


/* This is a wrapper function to post tagged send/receive using OFI deferred works.
 *
 * Input Parameters:
 * domain - OFI domain
 * ep - end-point
 * work - deferred work for issuing the operation with triggered ops
 * addr - source or destination address
 * cmd_type - operation type SEND or RECV
 * buf - data buffer
 * size - buffer size
 * tag - tag for send/receive
 * thresh - threshold for the operation to be triggered
 * trigger_cntr - trigger counter
 * complete_cntr - completion counter
 * */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_prepare_tagged_control_cmd(struct fid_domain *domain,
                                                                  struct fid_ep *ep,
                                                                  struct fi_deferred_work *work,
                                                                  fi_addr_t addr, int cmd_type,
                                                                  void *buf, int size, uint64_t tag,
                                                                  MPIR_Comm * comm, int peer,
                                                                  int thresh,
                                                                  struct fid_cntr *trigger_cntr,
                                                                  struct fid_cntr *complete_cntr,
                                                                  bool is_persistent)
{
    struct iovec iov;
    struct fi_msg_tagged *msg;
    int mpi_errno = MPI_SUCCESS;
    struct iovec *iovp;
    uint64_t match_bits;
    uint64_t mask_bits;
    int context_offset;

    work->threshold = thresh;
    work->triggering_cntr = trigger_cntr;
    work->completion_cntr = complete_cntr;

    work->op.tagged =
        (struct fi_op_tagged *) MPL_malloc(sizeof(struct fi_op_tagged), MPL_MEM_BUFFER);

    MPIR_Assert(work->op.tagged != NULL);

    memset(work->op.tagged, 0, sizeof(struct fi_op_tagged));
    msg = &work->op.tagged->msg;

    iov.iov_base = buf;
    iov.iov_len = size;

    if (is_persistent) {
        iovp = (struct iovec *) MPL_malloc(sizeof(struct iovec), MPL_MEM_BUFFER);
        MPIR_Assert(iovp != NULL);
        *iovp = iov;
        msg->msg_iov = iovp;
    } else {
        msg->msg_iov = &iov;
    }

    msg->iov_count = 1;
    msg->desc = 0;
    msg->addr = addr;
    msg->data = 0;

    context_offset = MPIR_CONTEXT_COLL_OFFSET;

    if (cmd_type == MPIDI_OFI_TRIGGERED_TAGGED_SEND) {
        match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
        msg->tag = match_bits;
        msg->ignore = 0;
        work->op_type = FI_OP_TSEND;
    } else {
        match_bits =
            MPIDI_OFI_init_recvtag(&mask_bits, comm->context_id + context_offset, peer, tag);
        msg->tag = match_bits;
        msg->ignore = mask_bits;
        work->op_type = FI_OP_TRECV;
    }

    work->op.tagged->ep = ep;
    work->op.tagged->flags = 0;
    work->op.tagged->msg.context = &work->context;

    if (!is_persistent) {
        MPIDI_OFI_CALL_RETRY(fi_control(&domain->fid, FI_QUEUE_WORK, work), 0, control);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

/* This is a wrapper function to post a one-sided send  using OFI deferred works.
 *
 * Input Parameters:
 * domain - OFI domain
 * ep - end-point
 * work - deferred work for issuing the operation with triggered ops
 * addr - destination address
 * inbuf - input buffer
 * key - key for data transfer
 * size - size of the input buffer
 * thresh - threshold for the operation to be triggered
 * trigger_cntr - trigger counter
 * complete_cntr - completion counter
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_prepare_rma_control_cmd(struct fid_domain *domain,
                                                               struct fid_ep *ep,
                                                               struct fi_deferred_work *work,
                                                               fi_addr_t addr, void *inbuf,
                                                               uint64_t key, uint64_t size,
                                                               int thresh,
                                                               struct fid_cntr *trigger_cntr,
                                                               struct fid_cntr *complete_cntr,
                                                               int cmd_type, bool is_persistent)
{
    struct iovec in_iov;
    struct fi_rma_iov rma_iov;
    int mpi_errno = MPI_SUCCESS;
    struct iovec *iovp;
    struct fi_rma_iov *rma_iovp;

    work->threshold = thresh;
    work->triggering_cntr = trigger_cntr;
    work->completion_cntr = complete_cntr;

    work->op.rma = (struct fi_op_rma *) MPL_malloc(sizeof(struct fi_op_rma), MPL_MEM_BUFFER);
    MPIR_Assert(work->op.rma != NULL);

    memset(work->op.rma, 0, sizeof(struct fi_op_rma));
    work->op.rma->ep = ep;
    in_iov.iov_base = inbuf;
    in_iov.iov_len = size;

    if (is_persistent) {
        iovp = (struct iovec *) MPL_malloc(sizeof(struct iovec), MPL_MEM_BUFFER);
        MPIR_Assert(iovp != NULL);
        *iovp = in_iov;
        work->op.rma->msg.msg_iov = iovp;
    } else {
        work->op.rma->msg.msg_iov = &in_iov;
    }

    work->op.rma->msg.desc = 0;
    work->op.rma->msg.iov_count = 1;
    work->op.rma->msg.addr = addr;
    rma_iov.addr = (uint64_t) 0;
    rma_iov.len = size;
    rma_iov.key = key;

    if (is_persistent) {
        rma_iovp = (struct fi_rma_iov *) MPL_malloc(sizeof(struct fi_rma_iov), MPL_MEM_BUFFER);
        MPIR_Assert(rma_iovp != NULL);
        *rma_iovp = rma_iov;
        work->op.rma->msg.rma_iov = rma_iovp;
    } else {
        work->op.rma->msg.rma_iov = &rma_iov;
    }

    work->op.rma->msg.rma_iov_count = 1;
    work->op.rma->msg.context = &work->context;
    work->op.rma->msg.data = 0;
    work->op.rma->flags = 0;
    work->op_type = cmd_type == MPIDI_OFI_TRIGGERED_RMA_WRITE ? FI_OP_WRITE : FI_OP_READ;

    if (!is_persistent) {
        MPIDI_OFI_CALL_RETRY(fi_control(&domain->fid, FI_QUEUE_WORK, work), 0, control);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



/* This is a wrapper function to post a tagged atomic operation using OFI deferred works.
 *
 * Input Parameters:
 * domain - OFI domain
 * ep - end-point
 * work - deferred work for issuing the operation with triggered ops
 * addr - destination address
 * inbuf - input buffer
 * tag - tag for data transfer
 * fi_dt - datatype of the elements in the input buffer
 * fi_op - atomic operation to be performed
 * count - number of elements in the input buffer
 * thresh - threshold for the operation to be triggered
 * trigger_cntr - trigger counter
 * complete_cntr - completion counter
 * is_persistent - flag to indicate if it is a persistent deferred work
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_prepare_atomic_tagged_control_cmd(struct fid_domain *domain,
                                                                         struct fid_ep *ep,
                                                                         struct fi_deferred_work
                                                                         *work, fi_addr_t addr,
                                                                         void *inbuf, int tag,
                                                                         MPIR_Comm * comm,
                                                                         enum fi_datatype fi_dt,
                                                                         enum fi_op fi_op,
                                                                         int count, int thresh,
                                                                         struct fid_cntr
                                                                         *trigger_cntr, struct fid_cntr
                                                                         *complete_cntr,
                                                                         bool is_persistent)
{
    struct fi_ioc in_iov;
    struct fi_rma_ioc rma_iov;
    int mpi_errno = MPI_SUCCESS;
    struct fi_ioc *iovp;
    struct fi_rma_ioc *rma_iovp;
    uint64_t match_bits;
    int context_offset;

    work->threshold = thresh;
    work->triggering_cntr = trigger_cntr;
    work->completion_cntr = complete_cntr;

    work->op.atomic =
        (struct fi_op_atomic *) MPL_malloc(sizeof(struct fi_op_atomic), MPL_MEM_BUFFER);

    MPIR_Assert(work->op.atomic != NULL);

    memset(work->op.atomic, 0, sizeof(struct fi_op_atomic));
    work->op.atomic->ep = ep;
    in_iov.addr = inbuf;
    in_iov.count = count;

    if (is_persistent) {
        iovp = (struct fi_ioc *) MPL_malloc(sizeof(struct fi_ioc), MPL_MEM_BUFFER);
        MPIR_Assert(iovp != NULL);
        *iovp = in_iov;
        work->op.atomic->msg.msg_iov = iovp;
    } else {
        work->op.atomic->msg.msg_iov = &in_iov;
    }
    work->op.atomic->msg.desc = 0;
    work->op.atomic->msg.iov_count = 1;
    work->op.atomic->msg.addr = addr;
    rma_iov.addr = (uint64_t) 0;
    rma_iov.count = count;

    context_offset = MPIR_CONTEXT_COLL_OFFSET;
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
    rma_iov.key = match_bits;

    if (is_persistent) {
        rma_iovp = (struct fi_rma_ioc *) MPL_malloc(sizeof(struct fi_rma_ioc), MPL_MEM_BUFFER);
        MPIR_Assert(rma_iovp != NULL);
        *rma_iovp = rma_iov;
        work->op.atomic->msg.rma_iov = rma_iovp;
    } else {
        work->op.atomic->msg.rma_iov = &rma_iov;
    }
    work->op.atomic->msg.rma_iov_count = 1;
    work->op.atomic->msg.datatype = fi_dt;
    work->op.atomic->msg.op = fi_op;
    work->op.atomic->msg.context = &work->context;
    work->op.atomic->msg.data = 0;
    work->op.atomic->flags = FI_TAGGED;
    work->op_type = FI_OP_ATOMIC;

    if (!is_persistent) {
        MPIDI_OFI_CALL_RETRY(fi_control(&domain->fid, FI_QUEUE_WORK, work), 0, control);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This is a wrapper function to post a one-sided atomic operation using OFI deferred works.
 *
 * Input Parameters:
 * domain - OFI domain
 * ep - end-point
 * work - deferred work for issuing the operation with triggered ops
 * addr - destination address
 * inbuf - input buffer
 * inoutbuf - not used
 * s_mr - key for rma
 * fi_dt - datatype of the elements in the input buffer
 * fi_op - atomic operation to be performed
 * count - number of elements in the input buffer
 * thresh - threshold for the operation to be triggered
 * trigger_cntr - trigger counter
 * complete_cntr - completion counter
 * is_persistent - flag to indicate if it is a persistent deferred work
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_prepare_atomic_control_cmd(struct fid_domain *domain,
                                                                  struct fid_ep *ep,
                                                                  struct fi_deferred_work *work,
                                                                  fi_addr_t addr, void *inbuf,
                                                                  void *inoutbuf,
                                                                  struct fid_mr *s_mr,
                                                                  enum fi_datatype fi_dt,
                                                                  enum fi_op fi_op, int count,
                                                                  int thresh,
                                                                  struct fid_cntr *trigger_cntr,
                                                                  struct fid_cntr *complete_cntr,
                                                                  bool is_persistent)
{
    struct fi_ioc in_iov;
    struct fi_rma_ioc rma_iov;
    int mpi_errno = MPI_SUCCESS;
    struct fi_ioc *iovp;
    struct fi_rma_ioc *rma_iovp;

    work->threshold = thresh;
    work->triggering_cntr = trigger_cntr;
    work->completion_cntr = complete_cntr;

    work->op.atomic =
        (struct fi_op_atomic *) MPL_malloc(sizeof(struct fi_op_atomic), MPL_MEM_BUFFER);

    MPIR_Assert(work->op.atomic != NULL);

    memset(work->op.atomic, 0, sizeof(struct fi_op_atomic));
    work->op.atomic->ep = ep;
    in_iov.addr = inbuf;
    in_iov.count = count;

    if (is_persistent) {
        iovp = (struct fi_ioc *) MPL_malloc(sizeof(struct fi_ioc), MPL_MEM_BUFFER);
        MPIR_Assert(iovp != NULL);
        *iovp = in_iov;
        work->op.atomic->msg.msg_iov = iovp;
    } else {
        work->op.atomic->msg.msg_iov = &in_iov;
    }
    work->op.atomic->msg.desc = 0;
    work->op.atomic->msg.iov_count = 1;
    work->op.atomic->msg.addr = addr;
    rma_iov.addr = (uint64_t) 0;
    rma_iov.count = count;
    rma_iov.key = fi_mr_key(s_mr);

    if (is_persistent) {
        rma_iovp = (struct fi_rma_ioc *) MPL_malloc(sizeof(struct fi_rma_ioc), MPL_MEM_BUFFER);
        MPIR_Assert(rma_iovp != NULL);
        *rma_iovp = rma_iov;
        work->op.atomic->msg.rma_iov = rma_iovp;
    } else {
        work->op.atomic->msg.rma_iov = &rma_iov;
    }
    work->op.atomic->msg.rma_iov_count = 1;
    work->op.atomic->msg.datatype = fi_dt;
    work->op.atomic->msg.op = fi_op;
    work->op.atomic->msg.context = &work->context;
    work->op.atomic->msg.data = 0;
    work->op.atomic->flags = 0;
    work->op_type = FI_OP_ATOMIC;

    if (!is_persistent) {
        MPIDI_OFI_CALL_RETRY(fi_control(&domain->fid, FI_QUEUE_WORK, work), 0, control);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This is a wrapper function to post a counter add operation using OFI deferred works.
 *
 * Input Parameters:
 * domain - OFI domain
 * ep - end-point
 * work - deferred work for issuing the operation with triggered ops
 * addr - destination address
 * cntr - counter to change value
 * value - value to add to the giving counter
 * thresh - threshold for the operation to be triggered
 * trigger_cntr - trigger counter
 * complete_cntr - completion counter
 * is_persistent - flag to indicate if it is a persistent deferred work
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_prepare_counter_add_cmd(struct fid_domain *domain,
                                                               struct fid_ep *ep,
                                                               struct fi_deferred_work *work,
                                                               fi_addr_t addr,
                                                               struct fid_cntr *cntr,
                                                               uint64_t value, int thresh,
                                                               struct fid_cntr *trigger_cntr,
                                                               struct fid_cntr *complete_cntr,
                                                               bool is_persistent)
{
    int mpi_errno = MPI_SUCCESS;

    work->threshold = thresh;
    work->triggering_cntr = trigger_cntr;
    work->completion_cntr = complete_cntr;

    work->op.cntr = (struct fi_op_cntr *) MPL_malloc(sizeof(struct fi_op_cntr), MPL_MEM_BUFFER);
    work->op.cntr->cntr = cntr;
    work->op.cntr->value = value;
    work->op_type = FI_OP_CNTR_ADD;

    if (!is_persistent) {
        MPIDI_OFI_CALL_RETRY(fi_control(&domain->fid, FI_QUEUE_WORK, work), 0, control);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This is a wrapper function to free the deferred works.
 * Input Parameters:
 * works - Array of deferred works
 * num_works - NUmber of deferred works
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_free_deferred_works(struct fi_deferred_work *works,
                                                           int num_works, int need_cancel)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    for (i = 0; i < num_works; i++) {
        struct fi_deferred_work *work = works + i;
        if (work->op_type == FI_OP_TSEND)
            MPL_free(work->op.tagged);
        else if (work->op_type == FI_OP_TRECV) {
            /* cancel the pre-posted triggered receive */
            /* TODO: when FI_ECANCELED is handled in function
             * MPIDI_OFI_handle_cq_error, it can not tell whether the
             * context is from triggered op or a normal message */
            /* FIXME: need to cancel */
            if (need_cancel)
                fi_cancel((fid_t) MPIDI_OFI_global.ctx[0].rx, &work->context);
            MPL_free(work->op.tagged);
        } else if (work->op_type == FI_OP_WRITE || work->op_type == FI_OP_READ)
            MPL_free(work->op.rma);
        else if (work->op_type == FI_OP_ATOMIC)
            MPL_free(work->op.atomic);
        else if (work->op_type == FI_OP_CNTR_ADD)
            MPL_free(work->op.cntr);
        else {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**fail");
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_free_trigger(MPIDI_OFI_trigger_t * trig)
{
    int i;
    int mpi_errno = MPI_SUCCESS;

    /* Close all the counters */
    for (i = 0; i < trig->len_all_cntrs; i++) {
        MPIDI_OFI_CALL(fi_close(&trig->all_cntrs[i]->fid), cntrclose);
    }
    MPL_free(trig->all_cntrs);

    /* Free the completion counter arrays */
    MPL_free(trig->completion_cntrs);
    MPL_free(trig->cntr_val);

    /* Free the buffers */
    for (i = 0; i < trig->num_buffers; i++) {
        MPL_free(trig->buffers[i]);
    }
    MPL_free(trig->buffers);

    /* Free the rma keys */
    for (i = 0; i < trig->num_keys; i++) {
        MPIDI_OFI_CALL(fi_close(&trig->r_mrs[i]->fid), mr_unreg);
    }
    MPL_free(trig->r_mrs);

    /* Free deferred works */
    MPIDI_OFI_free_deferred_works(trig->works, trig->num_works, false);
    MPL_free(trig->works);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

/* This is a helper function to build a k-ary tree.
 *
 * Input Parameters:
 * myrank - current rank
 * nranks - total number of ranks
 * root - root rank
 * k - branching factor
 * first_child - first child rank of the current rank in the tree
 * parent - parent rank of the current rank in the tree
 * num_children - number of children of the current rank in the tree
 * is_root - flag to indicate if the current rank is the root
 * leaf - flag to indicate if the current rank is a leaf
 * intermediate - flag to indicate if the current rank is an intermediate rank in the tree
 */

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_build_kary_tree(int myrank, int nranks, int root, int k,
                                                       int *first_child, int *parent,
                                                       int *num_children, int *is_root, int *leaf,
                                                       int *intermediate)
{
    int kth_child;
    int mpi_errno = MPI_SUCCESS;

    *first_child = k * myrank + 1;
    kth_child = *first_child + k - 1;

    if (myrank == root)
        *is_root = 1;
    else if (*first_child >= nranks)
        *leaf = 1;
    else
        *intermediate = 1;

    if (!(*is_root))
        *parent = (myrank - 1) / k;
    else
        *parent = -1;

    if (!(*leaf)) {
        if (*first_child == (nranks - 1))
            *num_children = 1;
        else if (kth_child >= nranks)
            *num_children = nranks - *first_child;
        else
            *num_children = k;

    }

    return mpi_errno;
}

#endif /* OFI_TRIGGERED_H_INCLUDED */

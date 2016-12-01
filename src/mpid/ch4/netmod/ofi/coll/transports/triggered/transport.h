/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef TRIGGEREDTRANSPORT_H_INCLUDED
#define TRIGGEREDTRANSPORT_H_INCLUDED

#include "../../include/tsp_namespace_pre.h"
#include "../../../ofi_impl.h"
#include "../../../ofi_send.h"
#include "../../../ofi_recv.h"

#ifndef TSP_NAMESPACE
#error "TSP_NAMESPACE must be defined before including a collective transport"
#endif

static inline int TSP_init(struct fid_ep *send_ep,
                           struct fid_ep *recv_ep)
{
    TSP_global.send_ep = send_ep;
    TSP_global.recv_ep = recv_ep;
    return 0;
}

static inline int TSP_sched_init(TSP_sched_t *sched)
{
    struct fi_cntr_attr cntr_attr;
    int mpi_errno = MPI_SUCCESS;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    /* TODO:  This should use an allocator an a set of preallocated counters
       since it is on the critical path.
    */
    MPIDI_OFI_CALL(fi_cntr_open(MPIDI_Global.domain,   /* In:  Domain Object        */
                                &cntr_attr,            /* In:  Configuration object */
                                &sched->cntr,          /* Out: Counter Object       */
                                NULL), openct);        /* Context: counter events   */
    fi_cntr_set(sched->cntr, 0);
    sched->cur_thresh = 0;
    sched->posted     = 0;
    sched->completed  = 0;
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_Assert(0);
    goto fn_exit;
    return 0;
}

static inline int TSP_sched_commit(TSP_sched_t *sched)
{
    return 0;
}

static inline int TSP_sched_start(TSP_sched_t *sched)
{
    return 0;
}

static inline int TSP_sched_finalize(TSP_sched_t *sched)
{
    fi_close(&sched->cntr->fid);
    return 0;
}

static inline int TSP_init_control_dt(TSP_dt_t *dt)
{
    dt->mpi_dt = MPI_CHAR;
    return 0;
}

static inline void TSP_opinfo(TSP_op_t  *op,
                              int       *is_commutative)
{
    MPIR_Op *op_ptr;

    if(HANDLE_GET_KIND(op->mpi_op) == HANDLE_KIND_BUILTIN)
        *is_commutative=1;
    else {
        MPIR_Op_get_ptr(op->mpi_op, op_ptr);

        if(op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            *is_commutative = 0;
        else
            *is_commutative = 1;
    }
}

static inline int TSP_isinplace(const void *buf)
{
    if(buf == MPI_IN_PLACE) return 1;

    return 0;
}
static inline void TSP_dtinfo(TSP_dt_t *dt,
                              int      *iscontig,
                              size_t   *size,
                              size_t   *out_extent,
                              size_t   *lower_bound)
{
    MPI_Aint true_lb,extent,true_extent,type_size;
    MPID_Datatype_get_size_macro(dt->mpi_dt, type_size);
    MPIR_Type_get_true_extent_impl(dt->mpi_dt,&true_lb,&true_extent);
    MPID_Datatype_get_extent_macro(dt->mpi_dt,extent);
    MPIDI_Datatype_check_contig(dt->mpi_dt,*iscontig);
    *size = type_size;
    *out_extent  = MPL_MAX(extent,true_extent);
    *lower_bound = true_lb;
}

static inline int TSP_wait(TSP_sched_t *sched)
{
}

static inline int TSP_fence(TSP_sched_t *sched)
{
    int i;
    sched->cur_thresh+=sched->posted;
    int cur = sched->total - sched->posted;

    if(0) fprintf(stderr, "TSP(trigger) [fence]: sched(%p)  posted=%ld total=%ld cur_thresh=%ld\n",
                      sched,sched->posted, sched->total, sched->cur_thresh);

    for(i=cur; i<sched->total; i++) {
        if(0) fprintf(stderr, "  --> [fence] TSP(trigger) (%p) setting thresh=%d to %ld\n",
                          &sched->requests[i], i, sched->cur_thresh);

        sched->requests[i].done_thresh = sched->cur_thresh;
    }

    sched->posted = 0;
    return 0;
}

static inline void TSP_addref_dt(TSP_dt_t *dt,
                                 int       up)
{
    MPIR_Datatype *dt_ptr;

    if(HANDLE_GET_KIND(dt->mpi_dt) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype_get_ptr(dt->mpi_dt, dt_ptr);

        if(up)
            MPID_Datatype_add_ref(dt_ptr);
        else
            MPID_Datatype_release(dt_ptr);
    }
}

static inline void TSP_addref_op(TSP_op_t *op,
                                 int       up)
{
    MPIR_Op     *op_ptr;

    if(HANDLE_GET_KIND(op->mpi_op) != HANDLE_KIND_BUILTIN) {
        MPIR_Op_get_ptr(op->mpi_op, op_ptr);

        if(up)
            MPIR_Op_add_ref(op_ptr);
        else
            MPIR_Op_release(op_ptr);
    }
}

static inline int TSP_addref_dt_nb(TSP_dt_t    *dt,
                                    int          up,
                                    TSP_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_ADDREF_DT;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    req->nbargs.addref_dt.dt = dt;
    req->nbargs.addref_dt.up = up;

    if(0) fprintf(stderr, "TSP(trigger) [addref_dt]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline int TSP_addref_op_nb(TSP_op_t    *op,
                                    int          up,
                                    TSP_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_ADDREF_OP;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    req->nbargs.addref_op.op = op;
    req->nbargs.addref_op.up = up;

    if(0) fprintf(stderr, "TSP(trigger) [addref_op]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline void TSP_handle_send(TSP_sched_t  *sched,
                                   MPIR_Request *mpid_req)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_HANDLE_SEND;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;
    req->nbargs.handle_send.mpid_req  = mpid_req;
    sched->posted++;
    sched->total++;

    if(0) fprintf(stderr, "TSP(trigger) [handle_send]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline void TSP_handle_recv(TSP_sched_t  *sched,
                                   MPIR_Request *mpid_req,
                                   size_t        recv_size)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_HANDLE_RECV;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    req->nbargs.handle_recv.mpid_req  = mpid_req;
    req->nbargs.handle_recv.recv_size = recv_size;

    sched->posted++;
    sched->total++;

    if(0) fprintf(stderr, "TSP(trigger) [handle_recv]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline int TSP_send(const void  *buf,
                            int          count,
                            TSP_dt_t    *dt,
                            int          dest,
                            int          tag,
                            TSP_comm_t  *comm,
                            TSP_sched_t *sched,
                            int         n_invtcs,
                            int         *invtcs)
{
    int            mpi_errno;
    size_t         data_sz;
    MPI_Aint       dt_true_lb;
    MPIR_Datatype *dt_ptr;
    TSP_req_t     *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_SEND;
    req->state               = TSP_STATE_ISSUED;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    MPIDI_Datatype_get_info(count, dt->mpi_dt, req->is_contig, data_sz, dt_ptr, dt_true_lb);

    if(0) fprintf(stderr, "TSP(trigger) [send]: sched(%p) [%ld](%p) curThresh=%ld buf=%p\n",
                      sched,sched->total, req, req->thresh.threshold, buf);

    mpi_errno  = MPIDI_OFI_send_normal(buf,
                                       count,
                                       dt->mpi_dt,
                                       dest,
                                       tag,
                                       comm->mpid_comm,
                                       MPIR_CONTEXT_INTRA_COLL,
                                       &req->mpid_req[0],
                                       req->is_contig, data_sz,
                                       dt_ptr, dt_true_lb,0ULL,
                                       TSP_global.send_ep,
                                       &req->thresh);

    if(!req->is_contig) {
        TSP_fence(sched);
        TSP_handle_send(sched,req->mpid_req[0]);
        TSP_fence(sched);
    }

    MPIR_Assert(mpi_errno == MPI_SUCCESS);
}

static inline int TSP_send_accumulate(const void  *buf,
                                       int          count,
                                       TSP_dt_t    *dt,
                                       TSP_op_t    *op,
                                       int          dest,
                                       int          tag,
                                       TSP_comm_t  *comm,
                                       TSP_sched_t *sched,
                                       int          n_invtcs,
                                       int          *invtcs)
{
    int            mpi_errno;
    size_t         data_sz;
    MPI_Aint       dt_true_lb;
    MPIR_Datatype *dt_ptr;
    TSP_req_t     *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_SEND;
    req->state               = TSP_STATE_ISSUED;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    MPIDI_Datatype_get_info(count, dt->mpi_dt, req->is_contig, data_sz, dt_ptr, dt_true_lb);

    if(0) fprintf(stderr, "TSP(trigger) [send_accumulate]: sched(%p) [%ld](%p) curThresh=%ld buf=%p\n",
                      sched,sched->total, req, req->thresh.threshold, buf);

    mpi_errno  = MPIDI_OFI_send_normal(buf,
                                       count,
                                       dt->mpi_dt,
                                       dest,
                                       tag,
                                       comm->mpid_comm,
                                       MPIR_CONTEXT_INTRA_COLL,
                                       &req->mpid_req[0],
                                       req->is_contig, data_sz,
                                       dt_ptr, dt_true_lb,0ULL,
                                       TSP_global.send_ep,
                                       &req->thresh);

    if(!req->is_contig) {
        TSP_fence(sched);
        TSP_handle_send(sched,req->mpid_req[0]);
        TSP_fence(sched);
    }

    MPIR_Assert(mpi_errno == MPI_SUCCESS);
}

static inline int TSP_recv(void        *buf,
                            int          count,
                            TSP_dt_t    *dt,
                            int          source,
                            int          tag,
                            TSP_comm_t  *comm,
                            TSP_sched_t *sched,
                            int         n_invtcs,
                            int         *invtcs)
{
    int mpi_errno;
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_RECV;
    req->state               = TSP_STATE_ISSUED;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    MPIDI_Datatype_check_contig_size(dt->mpi_dt,count,req->is_contig,req->recv_size);
    mpi_errno = MPIDI_OFI_do_irecv(buf,
                                   count,
                                   dt->mpi_dt,
                                   source,
                                   tag,
                                   comm->mpid_comm,
                                   MPIR_CONTEXT_INTRA_COLL,
                                   &req->mpid_req[0],
                                   MPIDI_OFI_ON_HEAP,
                                   0ULL,
                                   TSP_global.recv_ep,
                                   &req->thresh);

    if(0) fprintf(stderr, "TSP(trigger) [recv]: sched(%p) [%ld](%p) buf=%p\n",
                      sched,sched->total, req, buf);

    if(!req->is_contig) {
        TSP_fence(sched);
        TSP_handle_recv(sched,req->mpid_req[0],req->recv_size);
        TSP_fence(sched);
    }

    MPIR_Assert(mpi_errno == MPI_SUCCESS);
}

static inline int TSP_recv_reduce(void        *buf,
                                   int          count,
                                   TSP_dt_t    *datatype,
                                   TSP_op_t    *op,
                                   int          source,
                                   int          tag,
                                   TSP_comm_t  *comm,
                                   uint64_t     flags,
                                   TSP_sched_t *sched,
                                   int          n_invtcs,
                                   int          *invtcs)
{
    int                     iscontig, mpi_errno;
    size_t                  type_size, out_extent, lower_bound;
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];

    TSP_dtinfo(datatype,&iscontig,&type_size,&out_extent,&lower_bound);
    req->nbargs.recv_reduce.inbuf    = MPL_malloc(count*out_extent);
    req->nbargs.recv_reduce.inoutbuf = buf;
    req->nbargs.recv_reduce.count    = count;
    req->nbargs.recv_reduce.datatype = datatype;
    req->nbargs.recv_reduce.op       = op;
    req->nbargs.recv_reduce.source   = source;
    req->nbargs.recv_reduce.tag      = tag;
    req->nbargs.recv_reduce.comm     = comm;
    req->nbargs.recv_reduce.req      = req;
    req->nbargs.recv_reduce.done     = 0;

    req->kind                = TSP_KIND_RECV_REDUCE;
    req->state               = TSP_STATE_ISSUED;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    mpi_errno = MPIDI_OFI_do_irecv(req->nbargs.recv_reduce.inbuf,
                                   count,
                                   datatype->mpi_dt,
                                   source,
                                   tag,
                                   comm->mpid_comm,
                                   MPIR_CONTEXT_INTRA_COLL,
                                   &req->mpid_req[0],
                                   MPIDI_OFI_ON_HEAP,
                                   0ULL,
                                   TSP_global.recv_ep,
                                   &req->thresh);

    if(0) fprintf(stderr, "TSP(trigger) [recv_reduce]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);

    MPIR_Assert(mpi_errno == MPI_SUCCESS);
}

static inline int TSP_rank(TSP_comm_t  *comm)
{
    return comm->mpid_comm->rank;
}

static inline int TSP_size(TSP_comm_t *comm)
{
    return comm->mpid_comm->local_size;
}

static inline void TSP_reduce_local(const void  *inbuf,
                                    void        *inoutbuf,
                                    int          count,
                                    TSP_dt_t    *datatype,
                                    TSP_op_t    *operation,
                                    TSP_sched_t *sched,
                                    int         n_invtcs,
                                    int         *invtcs)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_REDUCE_LOCAL;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    req->nbargs.reduce_local.inbuf    = inbuf;
    req->nbargs.reduce_local.inoutbuf = inoutbuf;
    req->nbargs.reduce_local.count    = count;
    req->nbargs.reduce_local.dt       = datatype;
    req->nbargs.reduce_local.op       = operation;

    if(0) fprintf(stderr, "TSP(trigger) [reduce_local]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline int TSP_dtcopy(void       *tobuf,
                             int         tocount,
                             TSP_dt_t   *totype,
                             const void *frombuf,
                             int         fromcount,
                             TSP_dt_t   *fromtype)
{
    return MPIR_Localcopy(frombuf,   /* yes, parameters are reversed        */
                          fromcount, /* MPICH forgot what memcpy looks like */
                          fromtype->mpi_dt,
                          tobuf,
                          tocount,
                          totype->mpi_dt);
}

static inline int TSP_dtcopy_nb(void        *tobuf,
                                 int          tocount,
                                 TSP_dt_t    *totype,
                                 const void  *frombuf,
                                 int          fromcount,
                                 TSP_dt_t    *fromtype,
                                 TSP_sched_t *sched,
                                 int         n_invtcs,
                                 int          *invtcs)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_DTCOPY;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    req->nbargs.dtcopy.tobuf      = tobuf;
    req->nbargs.dtcopy.tocount    = tocount;
    req->nbargs.dtcopy.totype     = totype;
    req->nbargs.dtcopy.frombuf    = frombuf;
    req->nbargs.dtcopy.fromcount  = fromcount;
    req->nbargs.dtcopy.fromtype   = fromtype;

    if(0) fprintf(stderr, "TSP(trigger) [dtcopy]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);
}

static inline void *TSP_allocate_mem(size_t  size)
{
    return MPL_malloc(size);

}

static inline void TSP_free_mem(void *ptr)
{
    MPL_free(ptr);
}

static inline int TSP_free_mem_nb(void        *ptr,
                                   TSP_sched_t *sched,
                                   int          n_invtcs,
                                   int          *invtcs)
{
    TSP_req_t *req;
    MPIR_Assert(sched->total < 32);
    req                      = &sched->requests[sched->total];
    req->kind                = TSP_KIND_FREE_MEM;
    req->state               = TSP_STATE_INIT;
    req->thresh.trig_cntr    = sched->cntr;
    req->thresh.cmp_cntr     = sched->cntr;
    req->thresh.threshold    = sched->cur_thresh;

    sched->posted++;
    sched->total++;

    if(0) fprintf(stderr, "TSP(trigger) [free_mem]: sched(%p) [%ld](%p)\n",
                      sched,sched->total,req);

    req->nbargs.free_mem.ptr = ptr;
}


static inline int TSP_test(TSP_sched_t *sched)
{
    uint64_t   cntr_val;
    TSP_req_t *req,*rp;
    int i;

    req = &sched->requests[0];
    int consecutiveComplete=1;

    /* First check for ops that need to be issued */
    for(i=sched->completed; i<sched->total; i++) {
        rp = &req[i];

        if(rp->state == TSP_STATE_COMPLETE && consecutiveComplete) {
            sched->completed++;
            continue;
        } else
            consecutiveComplete=0;

        cntr_val = fi_cntr_read(rp->thresh.trig_cntr);

        if(rp->state == TSP_STATE_INIT && cntr_val >= rp->thresh.threshold) {
            switch(rp->kind) {
                case TSP_KIND_ADDREF_DT:
                    TSP_addref_dt(rp->nbargs.addref_dt.dt,
                                  rp->nbargs.addref_dt.up);

                    if(0) fprintf(stderr, "  --> TSP(trigger) [addref dt](%p) complete\n", rp);

                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_ADDREF_OP:
                    TSP_addref_op(rp->nbargs.addref_op.op,
                                  rp->nbargs.addref_op.up);

                    if(0) fprintf(stderr, "  --> TSP(trigger) [addref op](%p) complete\n", rp);

                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_DTCOPY:
                    TSP_dtcopy(rp->nbargs.dtcopy.tobuf,
                               rp->nbargs.dtcopy.tocount,
                               rp->nbargs.dtcopy.totype,
                               rp->nbargs.dtcopy.frombuf,
                               rp->nbargs.dtcopy.fromcount,
                               rp->nbargs.dtcopy.fromtype);

                    if(0) fprintf(stderr, "  --> TSP(trigger) [dtcopy](%p) complete\n",rp);

                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_FREE_MEM:
                    if(0) fprintf(stderr, "  --> TSP(trigger) [freemem](%p) complete\n",rp);

                    TSP_free_mem(rp->nbargs.free_mem.ptr);
                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_REDUCE_LOCAL:
                    MPIR_Reduce_local_impl(rp->nbargs.reduce_local.inbuf,
                                           rp->nbargs.reduce_local.inoutbuf,
                                           rp->nbargs.reduce_local.count,
                                           rp->nbargs.reduce_local.dt->mpi_dt,
                                           rp->nbargs.reduce_local.op->mpi_op);

                    if(0) fprintf(stderr, "  --> TSP(trigger) [reduce local](%p) complete\n",rp);

                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_HANDLE_SEND:
                    if(0) fprintf(stderr, "  --> TSP(trigger) [handle_send](%p) complete\n",rp);

                    MPIDI_OFI_send_event(NULL, rp->nbargs.handle_send.mpid_req);
                    MPIR_Request_free(rp->nbargs.handle_send.mpid_req);
                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;

                case TSP_KIND_HANDLE_RECV: {
                    struct fi_cq_tagged_entry wc;

                    if(0) fprintf(stderr, "  --> TSP(trigger) [handle_recv](%p) complete\n", rp);

                    wc.tag=0;
                    wc.len=rp->nbargs.handle_recv.recv_size;
                    MPIDI_OFI_recv_event(&wc, rp->nbargs.handle_recv.mpid_req);
                    MPIR_Request_free(rp->nbargs.handle_recv.mpid_req);
                    rp->state = TSP_STATE_COMPLETE;
                    fi_cntr_add(rp->thresh.cmp_cntr,1);
                    break;
                }

                default:
                    break;
            }
        }
    }

    /* Now check for issued ops that have been completed */
    for(i=sched->completed; i<sched->total; i++) {
        rp       = &req[i];
        cntr_val = fi_cntr_read(rp->thresh.cmp_cntr);

        if(rp->state == TSP_STATE_ISSUED && cntr_val >= rp->done_thresh) {
            MPIR_Request  *mpid_req0 = rp->mpid_req[0];

            switch(rp->kind) {
                case TSP_KIND_SEND:
                    if(0) fprintf(stderr, "  --> TSP(trigger) [send](%p) complete done_thresh i=%d %ld >= %ld ctxt=%p\n",
                                      rp,i,cntr_val, rp->done_thresh,
                                      &(MPIDI_OFI_REQUEST(mpid_req0, context)));

                    if(rp->is_contig) {
                        MPIDI_OFI_send_event(NULL, mpid_req0);
                        MPIR_Request_free(mpid_req0);
                    }

                    rp->state = TSP_STATE_COMPLETE;
                    break;

                case TSP_KIND_RECV: {
                    struct fi_cq_tagged_entry wc;

                    if(0) fprintf(stderr, "  --> TSP(trigger) [recv](%p) complete i=%d done_thresh %ld >= %ld ctxt=%p\n",
                                      rp, i,cntr_val, rp->done_thresh,
                                      &(MPIDI_OFI_REQUEST(mpid_req0, context)));

                    wc.tag=0;
                    wc.len=rp->recv_size;

                    if(rp->is_contig) {
                        MPIDI_OFI_recv_event(&wc, mpid_req0);
                        MPIR_Request_free(mpid_req0);
                    }

                    rp->state = TSP_STATE_COMPLETE;
                }
                break;

                case TSP_KIND_RECV_REDUCE:
                    MPIR_Request_free(mpid_req0);
                    rp->state = TSP_STATE_COMPLETE;
                    assert(0);
                    break;

                default:
                    break;
            }
        }
    }

    if(fi_cntr_read(sched->cntr) >= sched->cur_thresh && sched->completed == sched->total) {
        if(0) fprintf(stderr, "  --> TSP(trigger) [test](%p) complete: threshold=%ld\n",
                          sched,sched->cur_thresh);

        return 1;
    } else
        return 0;

}

#endif

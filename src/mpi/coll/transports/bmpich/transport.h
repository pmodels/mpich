/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef BMPICHTRANSPORT_H_INCLUDED
#define BMPICHTRANSPORT_H_INCLUDED


static inline void MPIC_BMPICH_issue_request(int,MPIC_BMPICH_req_t*,MPIC_BMPICH_sched_t*);

static inline int MPIC_BMPICH_init()
{
    MPIC_global_instance.tsp_bmpich.control_dt = MPI_BYTE;
    return 0;
}

static inline int MPIC_BMPICH_comm_init(MPIC_BMPICH_comm_t * comm, void* base)
{
    MPIR_Comm * mpi_comm = container_of(base, MPIR_Comm, ch4_coll);
    comm->mpid_comm = mpi_comm;
}

static inline int MPIC_BMPICH_comm_cleanup(MPIC_BMPICH_comm_t * comm)
{
        comm->mpid_comm = NULL;
}

static inline void MPIC_BMPICH_sched_init(MPIC_BMPICH_sched_t *sched, int tag)
{
    sched->total     = 0;
    sched->nbufs     = 0;
    sched->tag       = tag;
}

static inline void MPIC_BMPICH_sched_reset(MPIC_BMPICH_sched_t *sched, int tag){
    sched->tag = tag;
}

static inline void MPIC_BMPICH_sched_commit(MPIC_BMPICH_sched_t *sched)
{

}

static inline void MPIC_BMPICH_sched_start(MPIC_BMPICH_sched_t *sched)
{

}

static inline void MPIC_BMPICH_sched_finalize(MPIC_BMPICH_sched_t *sched)
{

}


static inline void MPIC_BMPICH_opinfo(MPIC_BMPICH_op_t  op,
                              int       *is_commutative)
{
    MPIR_Op *op_ptr;

    if(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
        *is_commutative=1;
    else {
        MPIR_Op_get_ptr(op, op_ptr);

        if(op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            *is_commutative = 0;
        else
            *is_commutative = 1 ;
    }
}

static inline int MPIC_BMPICH_isinplace(const void *buf)
{
    if(buf == MPI_IN_PLACE) return 1;

    return 0;
}
static inline void MPIC_BMPICH_dtinfo(MPIC_BMPICH_dt_t dt,
                              int      *iscontig,
                              size_t   *size,
                              size_t   *out_extent,
                              size_t   *lower_bound)
{
    MPI_Aint true_lb,extent,true_extent,type_size;
    MPID_Datatype_get_size_macro(dt, type_size);
    MPIR_Type_get_true_extent_impl(dt,&true_lb,&true_extent);
    MPID_Datatype_get_extent_macro(dt,extent);
    MPIDI_Datatype_check_contig(dt,*iscontig);
    *size = type_size;
    *out_extent  = MPL_MAX(extent,true_extent);
    *lower_bound = true_lb;
}

static inline void MPIC_BMPICH_addref_dt(MPIC_BMPICH_dt_t dt,
                                 int       up)
{
    MPIR_Datatype *dt_ptr;

    if(HANDLE_GET_KIND(dt) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype_get_ptr(dt, dt_ptr);

        if(up)
            MPID_Datatype_add_ref(dt_ptr);
        else
            MPID_Datatype_release(dt_ptr);
    }
}

/*this vertex sets the incoming edges (inedges) to vtx
and also add vtx to the outgoing edge list of vertices in inedges*/
static inline void MPIC_BMPICH_add_vtx_dependencies(MPIC_BMPICH_sched_t *sched, int vtx, int n_invtcs, int *invtcs){
}

/*This function should go away, keeping it there for smooth transition
from phase array to completely DAG based collectives*/
static inline int MPIC_BMPICH_fence(MPIC_BMPICH_sched_t *sched)
{
    return 0;
}

/*MPIC_BMPICH_wait waits for all the operations posted before it to complete
before issuing any operations posted after it. This is useful in composing
multiple schedules, for example, allreduce can be written as
COLL_sched_reduce(s)
MPIC_BMPICH_wait(s)
COLL_sched_bcast(s)
This is different from the fence operation in the sense that fence requires
any vertex to post dependencies on it while MPIC_BMPICH_wait is used internally
by the transport to add it as a dependency to any operations poster after it
*/

static inline int MPIC_BMPICH_wait(MPIC_BMPICH_sched_t *sched)
{
    return 0;
}

static inline void MPIC_BMPICH_addref_op(MPIC_BMPICH_op_t op,
                                 int       up)
{
    MPIR_Op     *op_ptr;

    if(HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
        MPIR_Op_get_ptr(op, op_ptr);

        if(up)
            MPIR_Op_add_ref(op_ptr);
        else
            MPIR_Op_release(op_ptr);
    }
}

static inline int MPIC_BMPICH_addref_dt_nb(MPIC_BMPICH_dt_t    dt,
                                    int          up,
                                    MPIC_BMPICH_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_BMPICH_KIND_ADDREF_DT;
    req->nbargs.addref_dt.dt = dt;
    req->nbargs.addref_dt.up = up;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [addref_dt]\n",sched->total);
    return sched->total++;
}

static inline int MPIC_BMPICH_addref_op_nb(MPIC_BMPICH_op_t    op,
                                    int          up,
                                    MPIC_BMPICH_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_BMPICH_KIND_ADDREF_OP;
    req->nbargs.addref_op.op = op;
    req->nbargs.addref_op.up = up;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [addref_op]\n",sched->total);
    return sched->total++;
}


static inline int MPIC_BMPICH_test(MPIC_BMPICH_sched_t *sched);
static inline int MPIC_BMPICH_send(const void  *buf,
                            int          count,
                            MPIC_BMPICH_dt_t    dt,
                            int          dest,
                            int          tag,
                            MPIC_BMPICH_comm_t  *comm,
                            MPIC_BMPICH_sched_t *sched,
                            int          n_invtcs,
                            int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_BMPICH_KIND_SEND;
    req->nbargs.sendrecv.buf   = (void *)buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = dest;
    req->nbargs.sendrecv.tag   = tag;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [send]\n",vtx);
    sched->total++;
    return vtx;
}

static inline int MPIC_BMPICH_send_accumulate(const void  *buf,
                                       int          count,
                                       MPIC_BMPICH_dt_t    dt,
                                       MPIC_BMPICH_op_t    op,
                                       int          dest,
                                       int          tag,
                                       MPIC_BMPICH_comm_t  *comm,
                                       MPIC_BMPICH_sched_t *sched,
                                       int          n_invtcs,
                                       int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_BMPICH_KIND_SEND;
    req->nbargs.sendrecv.buf   = (void *)buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = dest;
    req->nbargs.sendrecv.tag   = tag;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [send_accumulate]\n",sched->total);
    sched->total++;
    return vtx;
}

static inline int MPIC_BMPICH_recv(void        *buf,
                            int          count,
                            MPIC_BMPICH_dt_t    dt,
                            int          source,
                            int          tag,
                            MPIC_BMPICH_comm_t  *comm,
                            MPIC_BMPICH_sched_t *sched,
                            int          n_invtcs,
                            int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_BMPICH_KIND_RECV;
    req->nbargs.sendrecv.buf   = buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = source;
    req->nbargs.sendrecv.tag   = tag;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [recv]\n",sched->total);
    sched->total++;
    return vtx;
}

static inline int MPIC_BMPICH_queryfcn(
    void       *data,
    MPI_Status *status)
{
    MPIC_BMPICH_recv_reduce_arg_t *rr
        = (MPIC_BMPICH_recv_reduce_arg_t *)data;
    if(rr->req->mpid_req[0] == NULL && !rr->done) {
        MPI_Datatype dt = rr->datatype;
        MPI_Op       op = rr->op;

        if(rr->flags==0 || rr->flags & MPIC_FLAG_REDUCE_L) {
            if(0) fprintf(stderr, "  --> MPICH transport (recv_reduce L) complete to %p\n",
                              rr->inoutbuf);

            MPIR_Reduce_local_impl(rr->inbuf,rr->inoutbuf,rr->count,dt,op);
        } else {
            if(0) fprintf(stderr, "  --> MPICH transport (recv_reduce R) complete to %p\n",
                              rr->inoutbuf);

            MPIR_Reduce_local_impl(rr->inoutbuf,rr->inbuf,rr->count,dt,op);
            MPIR_Localcopy(rr->inbuf,rr->count,dt,
                           rr->inoutbuf,rr->count,dt);
        }

        //MPL_free(rr->inbuf);
        MPIR_Grequest_complete_impl(rr->req->mpid_req[1]);
        rr->done = 1;
    }

    status->MPI_SOURCE = MPI_UNDEFINED;
    status->MPI_TAG    = MPI_UNDEFINED;
    MPI_Status_set_cancelled(status, 0);
    MPI_Status_set_elements(status, MPI_BYTE, 0);
    return MPI_SUCCESS;
}

static inline int MPIC_BMPICH_recv_reduce(void        *buf,
                                   int          count,
                                   MPIC_BMPICH_dt_t    datatype,
                                   MPIC_BMPICH_op_t    op,
                                   int          source,
                                   int          tag,
                                   MPIC_BMPICH_comm_t  *comm,
                                   uint64_t     flags,
                                   MPIC_BMPICH_sched_t *sched,
                                   int          n_invtcs,
                                   int         *invtcs)
{
    int        iscontig;
    size_t     type_size, out_extent, lower_bound;
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    req = &sched->requests[vtx];
    req->kind                = MPIC_BMPICH_KIND_RECV_REDUCE;

    MPIC_BMPICH_dtinfo(datatype,&iscontig,&type_size,&out_extent,&lower_bound);
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
    req->nbargs.recv_reduce.flags    = flags;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [recv_reduce]\n",sched->total);
    
    sched->total++;
    return vtx;
}

static inline int MPIC_BMPICH_rank(MPIC_BMPICH_comm_t  *comm)
{
    return comm->mpid_comm->rank;
}

static inline int MPIC_BMPICH_size(MPIC_BMPICH_comm_t *comm)
{
    return comm->mpid_comm->local_size;
}

static inline int MPIC_BMPICH_reduce_local(const void  *inbuf,
                                    void        *inoutbuf,
                                    int          count,
                                    MPIC_BMPICH_dt_t    datatype,
                                    MPIC_BMPICH_op_t    operation,
                                    uint64_t     flags,
                                    MPIC_BMPICH_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    req = &sched->requests[vtx];
    req->kind                         = MPIC_BMPICH_KIND_REDUCE_LOCAL;
    sched->total++;
    req->nbargs.reduce_local.inbuf    = inbuf;
    req->nbargs.reduce_local.inoutbuf = inoutbuf;
    req->nbargs.reduce_local.count    = count;
    req->nbargs.reduce_local.dt       = datatype;
    req->nbargs.reduce_local.op       = operation;
    req->nbargs.reduce_local.flags    = flags;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [reduce_local]\n",sched->total);
    return vtx;
}

static inline int MPIC_BMPICH_dtcopy(void       *tobuf,
                             int         tocount,
                             MPIC_BMPICH_dt_t   totype,
                             const void *frombuf,
                             int         fromcount,
                             MPIC_BMPICH_dt_t   fromtype)
{
    return MPIR_Localcopy(frombuf,   /* yes, parameters are reversed        */
                          fromcount, /* MPICH forgot what memcpy looks like */
                          fromtype,
                          tobuf,
                          tocount,
                          totype);
}

static inline int MPIC_BMPICH_dtcopy_nb(void        *tobuf,
                                 int          tocount,
                                 MPIC_BMPICH_dt_t    totype,
                                 const void  *frombuf,
                                 int          fromcount,
                                 MPIC_BMPICH_dt_t    fromtype,
                                 MPIC_BMPICH_sched_t *sched,
                                 int          n_invtcs,
                                 int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind   = MPIC_BMPICH_KIND_DTCOPY;

    req->nbargs.dtcopy.tobuf      = tobuf;
    req->nbargs.dtcopy.tocount    = tocount;
    req->nbargs.dtcopy.totype     = totype;
    req->nbargs.dtcopy.frombuf    = frombuf;
    req->nbargs.dtcopy.fromcount  = fromcount;
    req->nbargs.dtcopy.fromtype   = fromtype;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [dt_copy]\n",sched->total);
    return sched->total++;
}

static inline void *MPIC_BMPICH_allocate_mem(size_t size)
{
    return MPL_malloc(size);
}

/*This function allocates memory required for schedule execution.
 *This is recorded in the schedule so that this memory can be 
 *freed when the schedule is destroyed
 */
static inline void *MPIC_BMPICH_allocate_buffer(size_t size, MPIC_BMPICH_sched_t *s){
    void *buf = MPIC_BMPICH_allocate_mem(size);
    /*record memory allocation*/
    assert(s->nbufs<MPIC_BMPICH_MAX_REQUESTS);
    s->buf[s->nbufs] = buf;
    s->nbufs += 1;
    return buf;
}

static inline void MPIC_BMPICH_free_mem(void *ptr)
{
    MPL_free(ptr);
}

static inline int MPIC_BMPICH_free_mem_nb(void        *ptr,
                                   MPIC_BMPICH_sched_t *sched,
                                   int          n_invtcs,
                                   int         *invtcs)
{
    MPIC_BMPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_BMPICH_KIND_FREE_MEM;
    req->nbargs.free_mem.ptr = ptr;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [free_mem]\n",sched->total);
    return sched->total++;
}

static inline void MPIC_BMPICH_issue_request(int vtxid, MPIC_BMPICH_req_t *rp, MPIC_BMPICH_sched_t *sched){
    if(0) fprintf(stderr, "issuing request: %d\n", vtxid);
    switch(rp->kind) {
        case MPIC_BMPICH_KIND_SEND: {
            if(0) fprintf(stderr, "  --> MPICH transport (isend) issued\n");

            MPIR_Errflag_t  errflag  = MPIR_ERR_NONE;
            MPIC_Send(rp->nbargs.sendrecv.buf,
                       rp->nbargs.sendrecv.count,
                       rp->nbargs.sendrecv.dt,
                       rp->nbargs.sendrecv.dest,
                       sched->tag,
                       rp->nbargs.sendrecv.comm->mpid_comm,
                       &errflag);
        }
        break;

        case MPIC_BMPICH_KIND_RECV: {
            if(0) fprintf(stderr, "  --> MPICH transport (irecv) issued\n");
            MPIR_Errflag_t  errflag  = MPIR_ERR_NONE;
            MPI_Status status;
            MPIC_Recv(rp->nbargs.sendrecv.buf,
                       rp->nbargs.sendrecv.count,
                       rp->nbargs.sendrecv.dt,
                       rp->nbargs.sendrecv.dest,
                       sched->tag,
                       rp->nbargs.sendrecv.comm->mpid_comm,
                       &status, &errflag);
        }
        break;

        case MPIC_BMPICH_KIND_ADDREF_DT:
            MPIC_BMPICH_addref_dt(rp->nbargs.addref_dt.dt,
                          rp->nbargs.addref_dt.up);

            if(0) fprintf(stderr, "  --> MPICH transport (addref dt) complete\n");
            break;

        case MPIC_BMPICH_KIND_ADDREF_OP:
            MPIC_BMPICH_addref_op(rp->nbargs.addref_op.op,
                          rp->nbargs.addref_op.up);

            if(0) fprintf(stderr, "  --> MPICH transport (addref op) complete\n");

            break;

        case MPIC_BMPICH_KIND_DTCOPY:
            MPIC_BMPICH_dtcopy(rp->nbargs.dtcopy.tobuf,
                       rp->nbargs.dtcopy.tocount,
                       rp->nbargs.dtcopy.totype,
                       rp->nbargs.dtcopy.frombuf,
                       rp->nbargs.dtcopy.fromcount,
                       rp->nbargs.dtcopy.fromtype);

            if(0) fprintf(stderr, "  --> MPICH transport (dtcopy) complete\n");

            break;

        case MPIC_BMPICH_KIND_FREE_MEM:
            if(0) fprintf(stderr, "  --> MPICH transport (freemem) complete\n");

            MPIC_BMPICH_free_mem(rp->nbargs.free_mem.ptr);
            break;

        case MPIC_BMPICH_KIND_NOOP:
            if(0) fprintf(stderr, "  --> MPICH transport (noop) complete\n");

            break;

        case MPIC_BMPICH_KIND_RECV_REDUCE: {
            MPIR_Errflag_t  errflag  = MPIR_ERR_NONE;
            MPI_Status status;
            MPIR_Request  *mpid_req1 = rp->mpid_req[1];
            MPIC_Recv(rp->nbargs.recv_reduce.inbuf,
                       rp->nbargs.recv_reduce.count,
                       rp->nbargs.recv_reduce.datatype,
                       rp->nbargs.recv_reduce.source,
                       sched->tag,
                       rp->nbargs.recv_reduce.comm->mpid_comm,
                       &status, &errflag);
            MPIR_Grequest_start_impl(MPIC_BMPICH_queryfcn,
                                     NULL,
                                     NULL,
                                     &rp->nbargs.recv_reduce,
                                     &rp->mpid_req[1]);
            (mpid_req1->u.ureq.greq_fns->query_fn)
            (mpid_req1->u.ureq.greq_fns->grequest_extra_state,
             &status);
            MPIR_Request_free(rp->mpid_req[1]);
            if(0) fprintf(stderr, "  --> MPICH transport (recv_reduce) issued\n");
        }
        break;

        case MPIC_BMPICH_KIND_REDUCE_LOCAL:
                  if(rp->nbargs.reduce_local.flags==0 || rp->nbargs.reduce_local.flags & MPIC_FLAG_REDUCE_L) {
                          MPIR_Reduce_local_impl(rp->nbargs.reduce_local.inbuf,
                                                 rp->nbargs.reduce_local.inoutbuf,
                                                 rp->nbargs.reduce_local.count,
                                                 rp->nbargs.reduce_local.dt,
                                                 rp->nbargs.reduce_local.op);
                          if(1) fprintf(stderr, "  --> MPICH transport (reduce local_L) complete\n");
                   } else {
                          MPIR_Reduce_local_impl(rp->nbargs.reduce_local.inoutbuf,
                                                 rp->nbargs.reduce_local.inbuf,
                                                 rp->nbargs.reduce_local.count,
                                                 rp->nbargs.reduce_local.dt,
                                                 rp->nbargs.reduce_local.op);

                          MPIR_Localcopy(rp->nbargs.reduce_local.inbuf,
                                         rp->nbargs.reduce_local.count,
                                         rp->nbargs.reduce_local.dt,
                                         rp->nbargs.reduce_local.inoutbuf,
                                         rp->nbargs.reduce_local.count,
                                         rp->nbargs.reduce_local.dt);
                          if(1) fprintf(stderr, "  --> MPICH transport (reduce local_R) complete\n");
                   }

            break;
    }
}



static inline int MPIC_BMPICH_test(MPIC_BMPICH_sched_t *sched)
{
    MPIC_BMPICH_req_t *req;
    int i;
    req = &sched->requests[0];
    /*if issued list is empty, generate it*/
    if(sched->total > 0){
        for(i=0; i<sched->total; i++)
            MPIC_BMPICH_issue_request(i, &sched->requests[i], sched);
    }
    return 1;
}

 /*frees any memory allocated for execution of this schedule*/
static inline void MPIC_BMPICH_free_buffers(MPIC_BMPICH_sched_t *s){
    int i;
    for(i=0; i<s->total; i++){
        /*free the temporary memory allocated by recv_reduce call*/
        if(s->requests[i].kind == MPIC_BMPICH_KIND_RECV_REDUCE){
            MPIC_BMPICH_free_mem(s->requests[i].nbargs.recv_reduce.inbuf);
        }
    }
    /*free temporary buffers*/
    for(i=0; i<s->nbufs; i++){
        MPIC_BMPICH_free_mem(s->buf[i]);
    }
}

#endif

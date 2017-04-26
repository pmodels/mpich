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
#ifndef MPICHTRANSPORT_H_INCLUDED
#define MPICHTRANSPORT_H_INCLUDED



static inline void MPIC_MPICH_issue_request(int,MPIC_MPICH_req_t*,MPIC_MPICH_sched_t*);

static inline void MPIC_MPICH_decrement_num_unfinished_dependecies(MPIC_MPICH_req_t *rp, MPIC_MPICH_sched_t *sched){
    /*for each outgoing vertex of request rp, decrement number of unfinished dependencies*/
    MPIC_MPICH_IntArray *outvtcs = &rp->outvtcs;
    if(0) fprintf(stderr,"num outvtcs of %d = %d\n", rp->kind, outvtcs->used);
    int i;
    for(i=0; i<outvtcs->used; i++){
        int num_unfinished_dependencies = --(sched->requests[outvtcs->array[i]].num_unfinished_dependencies);
        /*if all dependencies are complete, issue the op*/
        if(num_unfinished_dependencies == 0){
            if(0) fprintf(stderr,"issuing request number: %d\n",outvtcs->array[i]);
            MPIC_MPICH_issue_request(outvtcs->array[i], &sched->requests[outvtcs->array[i]], sched);
        }
    }
}

static inline void MPIC_MPICH_record_request_completion(MPIC_MPICH_req_t *rp, MPIC_MPICH_sched_t *sched){
    rp->state = MPIC_MPICH_STATE_COMPLETE;
    /*update the dependencies*/
    sched->num_completed++;
    if(0) fprintf(stderr, "num completed vertices: %d\n", sched->num_completed);
    MPIC_MPICH_decrement_num_unfinished_dependecies(rp,sched);
}

static inline void MPIC_MPICH_record_request_issue(MPIC_MPICH_req_t *rp, MPIC_MPICH_sched_t *sched){
    rp->state = MPIC_MPICH_STATE_ISSUED;
    
    if(sched->issued_head == NULL){
        sched->issued_head = sched->last_issued = rp;
        sched->last_issued->next_issued = sched->req_iter;
    }else if(sched->last_issued->next_issued==rp){
        sched->last_issued = rp;
    }
    else{
        sched->last_issued->next_issued = rp;
        rp->next_issued = sched->req_iter;
        sched->last_issued = rp;
    }

#if 0
    if(sched->issued_head==NULL){/*this is the first issued task*/
        sched->issued_head = sched->last_issued = rp;
        if(0) fprintf(stderr, "sched->last_issued->next_issued = %p\n", sched->last_issued->next_issued);
        if(sched->req_iter)/*if this is reiteration over the issued list, point to the next task already issued*/
            sched->last_issued->next_issued = sched->req_iter->next_issued;
        else
            sched->last_issued->next_issued=NULL;
        
    }else if(sched->last_issued->next_issued == rp){
        sched->last_issued = rp;
    }else{
        if(0)fprintf(stderr, "insert task in between\n");
        /*insert the task between last_issued and last_issed->next_issued*/
        MPIC_MPICH_req_t *last_issued = sched->last_issued;
        rp->next_issued = last_issued->next_issued;
        last_issued->next_issued = rp;
        sched->last_issued = rp;
    }
    sched->req_iter = sched->last_issued;
#endif
    /*print issued task list*/
    if(0){
        MPIC_MPICH_req_t *req = sched->issued_head;
        fprintf(stderr, "issued list: ");
        while(req){
            fprintf(stderr, "%d ", req->id);
            req = req->next_issued;
        }
        if(sched->req_iter)
            fprintf(stderr, ", req_iter: %d\n", sched->req_iter->id);
        else
            fprintf(stderr,"\n");
    }
}

static inline int MPIC_MPICH_init()
{
    MPIC_global_instance.tsp_mpich.control_dt = MPI_BYTE;
    return 0;
}

static inline int MPIC_MPICH_comm_init(MPIC_MPICH_comm_t * comm, void* base)
{
    MPIR_Comm * mpi_comm = container_of(base, MPIR_Comm, ch4_coll);
    comm->mpid_comm = mpi_comm;
}

static inline int MPIC_MPICH_comm_cleanup(MPIC_MPICH_comm_t * comm)
{
        comm->mpid_comm = NULL;
}

static inline void MPIC_MPICH_reset_issued_list(MPIC_MPICH_sched_t *sched){
    sched->issued_head = NULL;
    int i;
    /*If schedule is being reused, reset only used requests
    else it is a new schedule being generated and therefore
    reset all the requests**/
    int nrequests = (sched->total==0)?MPIC_MPICH_MAX_REQUESTS:sched->total;
    
    for(i=0; i<nrequests; i++){
        sched->requests[i].next_issued = NULL;
    }
    sched->req_iter = NULL;
}

static inline void MPIC_MPICH_sched_init(MPIC_MPICH_sched_t *sched)
{
    sched->total      = 0;
    sched->num_completed  = 0;
    sched->last_wait = -1;
    sched->nbufs     = 0;
    MPIC_MPICH_reset_issued_list(sched);
}

static inline void MPIC_MPICH_sched_reset(MPIC_MPICH_sched_t *sched, int tag){
    int i;
    sched->num_completed=0;
    sched->tag = tag;
    for(i=0; i<sched->total; i++){
        MPIC_MPICH_req_t* req = &sched->requests[i];
        req->state = MPIC_MPICH_STATE_INIT;
        req->num_unfinished_dependencies = req->invtcs.used;
        if(req->kind==MPIC_MPICH_KIND_RECV_REDUCE)
            req->nbargs.recv_reduce.done=0;
    }
    MPIC_MPICH_reset_issued_list(sched);
}

static inline void MPIC_MPICH_sched_commit(MPIC_MPICH_sched_t *sched)
{

}

static inline void MPIC_MPICH_sched_start(MPIC_MPICH_sched_t *sched)
{

}

static inline void MPIC_MPICH_sched_finalize(MPIC_MPICH_sched_t *sched)
{

}


static inline void MPIC_MPICH_opinfo(MPIC_MPICH_op_t  op,
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

static inline int MPIC_MPICH_isinplace(const void *buf)
{
    if(buf == MPI_IN_PLACE) return 1;

    return 0;
}
static inline void MPIC_MPICH_dtinfo(MPIC_MPICH_dt_t dt,
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

static inline void MPIC_MPICH_addref_dt(MPIC_MPICH_dt_t dt,
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

static inline void init_request(MPIC_MPICH_req_t *req, int id){
    req->state                       = MPIC_MPICH_STATE_INIT;
    req->id                          = id;   
    req->num_unfinished_dependencies = 0;
    req->invtcs.used                 = 0;
    req->outvtcs.used                = 0;
}

/*this vertex sets the incoming edges (inedges) to vtx
and also add vtx to the outgoing edge list of vertices in inedges*/
static inline void MPIC_MPICH_add_vtx_dependencies(MPIC_MPICH_sched_t *sched, int vtx, int n_invtcs, int *invtcs){
    int i;
    MPIC_MPICH_req_t *req = &sched->requests[vtx];
    MPIC_MPICH_IntArray *in = &req->invtcs;
    if(0) fprintf(stderr,"updating invtcs of vtx %d, kind %d, in->used %d, n_invtcs %d\n",vtx,req->kind,in->used, n_invtcs);
    /*insert the incoming edges*/
    assert(in->used+n_invtcs <= MPIC_MPICH_MAX_EDGES);
    memcpy(in->array+in->used, invtcs, sizeof(int)*n_invtcs);
    in->used += n_invtcs;

    MPIC_MPICH_IntArray *outvtcs;
    /*update the outgoing edges of incoming vertices*/
    for(i=0; i<n_invtcs; i++){
        if(0)fprintf(stderr,"invtx: %d\n", invtcs[i]);
        outvtcs = &sched->requests[invtcs[i]].outvtcs;
        assert(outvtcs->used+1<=MPIC_MPICH_MAX_EDGES);
        outvtcs->array[outvtcs->used++] = vtx;
        if(sched->requests[invtcs[i]].state != MPIC_MPICH_STATE_COMPLETE)
            req->num_unfinished_dependencies++;
    }

    /*check if there was any MPIC_MPICH_wait operation and add appropriate dependencies*/
    if(sched->last_wait != -1 && sched->last_wait != vtx){
        /*add incoming edge to vtx*/
        assert(in->used+1 <= MPIC_MPICH_MAX_EDGES);
        in->array[in->used++] = sched->last_wait;

        /*add vtx as outgoing vtx of last_wait*/
        outvtcs = &sched->requests[sched->last_wait].outvtcs;
        assert(outvtcs->used+1<=MPIC_MPICH_MAX_EDGES);
        outvtcs->array[outvtcs->used++] = vtx;
        if(sched->requests[sched->last_wait].state != MPIC_MPICH_STATE_COMPLETE)
            req->num_unfinished_dependencies++;
    }
}

/*This function should go away, keeping it there for smooth transition
from phase array to completely DAG based collectives*/
static inline int MPIC_MPICH_fence(MPIC_MPICH_sched_t *sched)
{
    assert(sched->total < 32);
    if(0) fprintf(stderr, "TSP(mpich) : sched [fence] total=%ld \n", sched->total);
    MPIC_MPICH_req_t *req = &sched->requests[sched->total];
    req->kind = MPIC_MPICH_KIND_NOOP;
    init_request(req,sched->total);
    int *invtcs = (int*)MPL_malloc(sizeof(int)*sched->total);
    int i, n_invtcs=0;
    for(i=sched->total-1; i>=0; i--){
        if(sched->requests[i].kind == MPIC_MPICH_KIND_NOOP)
            break;
        else{
            invtcs[sched->total-1-i]=i;
            n_invtcs++;
        }
    }

    MPIC_MPICH_add_vtx_dependencies(sched, sched->total, n_invtcs, invtcs);
    MPL_free(invtcs);
    return sched->total++;
}

/*MPIC_MPICH_wait waits for all the operations posted before it to complete
before issuing any operations posted after it. This is useful in composing
multiple schedules, for example, allreduce can be written as
COLL_sched_reduce(s)
MPIC_MPICH_wait(s)
COLL_sched_bcast(s)
This is different from the fence operation in the sense that fence requires
any vertex to post dependencies on it while MPIC_MPICH_wait is used internally
by the transport to add it as a dependency to any operations poster after it
*/

static inline int MPIC_MPICH_wait(MPIC_MPICH_sched_t *sched)
{
    if(0) fprintf(stderr, "scheduling a wait\n");
    sched->last_wait = sched->total;
    return MPIC_MPICH_fence(sched);
}

static inline void MPIC_MPICH_addref_op(MPIC_MPICH_op_t op,
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

static inline int MPIC_MPICH_addref_dt_nb(MPIC_MPICH_dt_t    dt,
                                    int          up,
                                    MPIC_MPICH_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_MPICH_KIND_ADDREF_DT;
    init_request(req,sched->total);
    MPIC_MPICH_add_vtx_dependencies(sched, sched->total, n_invtcs, invtcs);
    req->nbargs.addref_dt.dt = dt;
    req->nbargs.addref_dt.up = up;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [addref_dt]\n",sched->total);
    return sched->total++;
}

static inline int MPIC_MPICH_addref_op_nb(MPIC_MPICH_op_t    op,
                                    int          up,
                                    MPIC_MPICH_sched_t *sched,
                                    int          n_invtcs,
                                    int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_MPICH_KIND_ADDREF_OP;
    init_request(req,sched->total);
    MPIC_MPICH_add_vtx_dependencies(sched, sched->total, n_invtcs, invtcs);
    req->nbargs.addref_op.op = op;
    req->nbargs.addref_op.up = up;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [addref_op]\n",sched->total);
    return sched->total++;
}


static inline int MPIC_MPICH_test(MPIC_MPICH_sched_t *sched);
static inline int MPIC_MPICH_send(const void  *buf,
                            int          count,
                            MPIC_MPICH_dt_t    dt,
                            int          dest,
                            int          tag,
                            MPIC_MPICH_comm_t  *comm,
                            MPIC_MPICH_sched_t *sched,
                            int          n_invtcs,
                            int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    sched->tag = tag;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_MPICH_KIND_SEND;
    init_request(req,vtx);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx, n_invtcs, invtcs);
    req->nbargs.sendrecv.buf   = (void *)buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = dest;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [send]\n",vtx);
    sched->total++;
    //MPIC_MPICH_issue_request(vtx, req, sched);
    return vtx;
}

static inline int MPIC_MPICH_send_accumulate(const void  *buf,
                                       int          count,
                                       MPIC_MPICH_dt_t    dt,
                                       MPIC_MPICH_op_t    op,
                                       int          dest,
                                       int          tag,
                                       MPIC_MPICH_comm_t  *comm,
                                       MPIC_MPICH_sched_t *sched,
                                       int          n_invtcs,
                                       int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    sched->tag = tag;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_MPICH_KIND_SEND;
    init_request(req,vtx);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx, n_invtcs, invtcs);
    req->nbargs.sendrecv.buf   = (void *)buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = dest;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [send_accumulate]\n",sched->total);
    sched->total++;
    //MPIC_MPICH_issue_request(vtx, req, sched);
    return vtx;
}

static inline int MPIC_MPICH_recv(void        *buf,
                            int          count,
                            MPIC_MPICH_dt_t    dt,
                            int          source,
                            int          tag,
                            MPIC_MPICH_comm_t  *comm,
                            MPIC_MPICH_sched_t *sched,
                            int          n_invtcs,
                            int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    sched->tag = tag;
    req = &sched->requests[vtx];
    req->kind                  = MPIC_MPICH_KIND_RECV;
    init_request(req,vtx);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx, n_invtcs, invtcs);
    req->nbargs.sendrecv.buf   = buf;
    req->nbargs.sendrecv.count = count;
    req->nbargs.sendrecv.dt    = dt;
    req->nbargs.sendrecv.dest  = source;
    req->nbargs.sendrecv.comm  = comm;
    req->mpid_req[1]           = NULL;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [recv]\n",sched->total);
    sched->total++;
    //MPIC_MPICH_issue_request(vtx, req, sched);
    return vtx;
}

static inline int MPIC_MPICH_queryfcn(
    void       *data,
    MPI_Status *status)
{
    MPIC_MPICH_recv_reduce_arg_t *rr
        = (MPIC_MPICH_recv_reduce_arg_t *)data;
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

static inline int MPIC_MPICH_recv_reduce(void        *buf,
                                   int          count,
                                   MPIC_MPICH_dt_t    datatype,
                                   MPIC_MPICH_op_t    op,
                                   int          source,
                                   int          tag,
                                   MPIC_MPICH_comm_t  *comm,
                                   uint64_t     flags,
                                   MPIC_MPICH_sched_t *sched,
                                   int          n_invtcs,
                                   int         *invtcs)
{
    int        iscontig;
    size_t     type_size, out_extent, lower_bound;
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    int vtx = sched->total;
    sched->tag = tag;
    req = &sched->requests[vtx];
    req->kind                = MPIC_MPICH_KIND_RECV_REDUCE;
    init_request(req, vtx);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx, n_invtcs, invtcs);

    MPIC_MPICH_dtinfo(datatype,&iscontig,&type_size,&out_extent,&lower_bound);
    req->nbargs.recv_reduce.inbuf    = MPL_malloc(count*out_extent);
    req->nbargs.recv_reduce.inoutbuf = buf;
    req->nbargs.recv_reduce.count    = count;
    req->nbargs.recv_reduce.datatype = datatype;
    req->nbargs.recv_reduce.op       = op;
    req->nbargs.recv_reduce.source   = source;
    req->nbargs.recv_reduce.comm     = comm;
    req->nbargs.recv_reduce.req      = req;
    req->nbargs.recv_reduce.done     = 0;
    req->nbargs.recv_reduce.flags    = flags;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [recv_reduce]\n",sched->total);
    
    sched->total++;
    //MPIC_MPICH_issue_request(vtx, req, sched);
    return vtx;
}

static inline int MPIC_MPICH_rank(MPIC_MPICH_comm_t  *comm)
{
    return comm->mpid_comm->rank;
}

static inline int MPIC_MPICH_size(MPIC_MPICH_comm_t *comm)
{
    return comm->mpid_comm->local_size;
}

static inline void MPIC_MPICH_reduce_local(const void  *inbuf,
                                    void        *inoutbuf,
                                    int          count,
                                    MPIC_MPICH_dt_t    datatype,
                                    MPIC_MPICH_op_t    operation,
                                    MPIC_MPICH_sched_t *sched)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                         = MPIC_MPICH_KIND_REDUCE_LOCAL;
    req->state                        = MPIC_MPICH_STATE_INIT;
    req->num_unfinished_dependencies = 0;
    sched->total++;
    req->nbargs.reduce_local.inbuf    = inbuf;
    req->nbargs.reduce_local.inoutbuf = inoutbuf;
    req->nbargs.reduce_local.count    = count;
    req->nbargs.reduce_local.dt       = datatype;
    req->nbargs.reduce_local.op       = operation;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [reduce_local]\n",sched->total);
}

static inline int MPIC_MPICH_dtcopy(void       *tobuf,
                             int         tocount,
                             MPIC_MPICH_dt_t   totype,
                             const void *frombuf,
                             int         fromcount,
                             MPIC_MPICH_dt_t   fromtype)
{
    return MPIR_Localcopy(frombuf,   /* yes, parameters are reversed        */
                          fromcount, /* MPICH forgot what memcpy looks like */
                          fromtype,
                          tobuf,
                          tocount,
                          totype);
}

static inline int MPIC_MPICH_dtcopy_nb(void        *tobuf,
                                 int          tocount,
                                 MPIC_MPICH_dt_t    totype,
                                 const void  *frombuf,
                                 int          fromcount,
                                 MPIC_MPICH_dt_t    fromtype,
                                 MPIC_MPICH_sched_t *sched,
                                 int          n_invtcs,
                                 int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind   = MPIC_MPICH_KIND_DTCOPY;
    init_request(req,sched->total);
    MPIC_MPICH_add_vtx_dependencies(sched, sched->total, n_invtcs, invtcs);

    req->nbargs.dtcopy.tobuf      = tobuf;
    req->nbargs.dtcopy.tocount    = tocount;
    req->nbargs.dtcopy.totype     = totype;
    req->nbargs.dtcopy.frombuf    = frombuf;
    req->nbargs.dtcopy.fromcount  = fromcount;
    req->nbargs.dtcopy.fromtype   = fromtype;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [dt_copy]\n",sched->total);
    return sched->total++;
}

static inline void *MPIC_MPICH_allocate_mem(size_t size)
{
    return MPL_malloc(size);
}

/*This function allocates memory required for schedule execution.
 *This is recorded in the schedule so that this memory can be 
 *freed when the schedule is destroyed
 */
static inline void *MPIC_MPICH_allocate_buffer(size_t size, MPIC_MPICH_sched_t *s){
    void *buf = MPIC_MPICH_allocate_mem(size);
    /*record memory allocation*/
    assert(s->nbufs< MPIC_MPICH_MAX_REQUESTS);
    s->buf[s->nbufs] = buf;
    s->nbufs += 1;
    return buf;
}

static inline void MPIC_MPICH_free_mem(void *ptr)
{
    MPL_free(ptr);
}

static inline int MPIC_MPICH_free_mem_nb(void        *ptr,
                                   MPIC_MPICH_sched_t *sched,
                                   int          n_invtcs,
                                   int         *invtcs)
{
    MPIC_MPICH_req_t *req;
    MPIR_Assert(sched->total < 32);
    req = &sched->requests[sched->total];
    req->kind                = MPIC_MPICH_KIND_FREE_MEM;
    init_request(req,sched->total);
    MPIC_MPICH_add_vtx_dependencies(sched, sched->total, n_invtcs, invtcs);
    req->nbargs.free_mem.ptr = ptr;

    if(0) fprintf(stderr, "TSP(mpich) : sched [%ld] [free_mem]\n",sched->total);
    return sched->total++;
}

static inline void MPIC_MPICH_issue_request(int vtxid, MPIC_MPICH_req_t *rp, MPIC_MPICH_sched_t *sched){
    if(rp->state == MPIC_MPICH_STATE_INIT && rp->num_unfinished_dependencies==0) {
        if(0) fprintf(stderr, "issuing request: %d\n", vtxid);
        switch(rp->kind) {
            case MPIC_MPICH_KIND_SEND: {
                if(0) fprintf(stderr, "  --> MPICH transport (isend) issued\n");

                MPIR_Errflag_t  errflag  = MPIR_ERR_NONE;
                MPIC_Isend(rp->nbargs.sendrecv.buf,
                           rp->nbargs.sendrecv.count,
                           rp->nbargs.sendrecv.dt,
                           rp->nbargs.sendrecv.dest,
                           sched->tag,
                           rp->nbargs.sendrecv.comm->mpid_comm,
                           &rp->mpid_req[0],
                           &errflag);
                MPIC_MPICH_record_request_issue(rp, sched);
            }
            break;

            case MPIC_MPICH_KIND_RECV: {
                if(0) fprintf(stderr, "  --> MPICH transport (irecv) issued\n");

                MPIC_Irecv(rp->nbargs.sendrecv.buf,
                           rp->nbargs.sendrecv.count,
                           rp->nbargs.sendrecv.dt,
                           rp->nbargs.sendrecv.dest,
                           sched->tag,
                           rp->nbargs.sendrecv.comm->mpid_comm,
                           &rp->mpid_req[0]);
                MPIC_MPICH_record_request_issue(rp, sched);
            }
            break;

            case MPIC_MPICH_KIND_ADDREF_DT:
                MPIC_MPICH_addref_dt(rp->nbargs.addref_dt.dt,
                              rp->nbargs.addref_dt.up);

                if(0) fprintf(stderr, "  --> MPICH transport (addref dt) complete\n");
                MPIC_MPICH_record_request_completion(rp, sched);
                break;

            case MPIC_MPICH_KIND_ADDREF_OP:
                MPIC_MPICH_addref_op(rp->nbargs.addref_op.op,
                              rp->nbargs.addref_op.up);


                if(0) fprintf(stderr, "  --> MPICH transport (addref op) complete\n");

                MPIC_MPICH_record_request_completion(rp, sched);
                break;

            case MPIC_MPICH_KIND_DTCOPY:
                MPIC_MPICH_dtcopy(rp->nbargs.dtcopy.tobuf,
                           rp->nbargs.dtcopy.tocount,
                           rp->nbargs.dtcopy.totype,
                           rp->nbargs.dtcopy.frombuf,
                           rp->nbargs.dtcopy.fromcount,
                           rp->nbargs.dtcopy.fromtype);

                if(0) fprintf(stderr, "  --> MPICH transport (dtcopy) complete\n");
                MPIC_MPICH_record_request_completion(rp, sched);

                break;

            case MPIC_MPICH_KIND_FREE_MEM:
                if(0) fprintf(stderr, "  --> MPICH transport (freemem) complete\n");

                MPIC_MPICH_free_mem(rp->nbargs.free_mem.ptr);
                MPIC_MPICH_record_request_completion(rp, sched);
                break;

            case MPIC_MPICH_KIND_NOOP:
                if(0) fprintf(stderr, "  --> MPICH transport (noop) complete\n");

                MPIC_MPICH_record_request_completion(rp, sched);
                break;

            case MPIC_MPICH_KIND_RECV_REDUCE: {
                MPIC_Irecv(rp->nbargs.recv_reduce.inbuf,
                           rp->nbargs.recv_reduce.count,
                           rp->nbargs.recv_reduce.datatype,
                           rp->nbargs.recv_reduce.source,
                           sched->tag,
                           rp->nbargs.recv_reduce.comm->mpid_comm,
                           &rp->mpid_req[0]);
                MPIR_Grequest_start_impl(MPIC_MPICH_queryfcn,
                                         NULL,
                                         NULL,
                                         &rp->nbargs.recv_reduce,
                                         &rp->mpid_req[1]);

                if(0) fprintf(stderr, "  --> MPICH transport (recv_reduce) issued\n");

                MPIC_MPICH_record_request_issue(rp, sched);
            }
            break;

            case MPIC_MPICH_KIND_REDUCE_LOCAL:
                MPIR_Reduce_local_impl(rp->nbargs.reduce_local.inbuf,
                                       rp->nbargs.reduce_local.inoutbuf,
                                       rp->nbargs.reduce_local.count,
                                       rp->nbargs.reduce_local.dt,
                                       rp->nbargs.reduce_local.op);

                if(0) fprintf(stderr, "  --> MPICH transport (reduce local) complete\n");

                MPIC_MPICH_record_request_completion(rp, sched);
                break;
        }
    }
}


static inline int MPIC_MPICH_test(MPIC_MPICH_sched_t *sched)
{
    MPIC_MPICH_req_t *req, *rp;
    int i;
    req = &sched->requests[0];
    //if(0)fprintf(stderr, "in TSP_test, num_completed=%d, total=%d\n", sched->num_completed, sched->total);
    /*if issued list is empty, generate it*/
    if(sched->issued_head == NULL){
        if(0) fprintf(stderr, "issued list is empty, issue ready requests\n");
        if(sched->total > 0 && sched->num_completed != sched->total){
            for(i=0; i<sched->total; i++)
                MPIC_MPICH_issue_request(i, &sched->requests[i], sched);
            if(0) fprintf(stderr, "completed traversal of requests\n");
            return 0;
        }
        else
            return 1;
    }
    if(sched->total == sched->num_completed){
        return 1;
    }
        /*fprintf(stderr, "issued list: ");
        TSP_req_t *tmp = sched->issued_head;
        while(tmp){fprintf(stderr, "%d ", req->kind); tmp = tmp->next_issued;}
        fprintf(stderr,"\n");*/
   
    assert(sched->issued_head != NULL);
    sched->req_iter = sched->issued_head;
    sched->issued_head = NULL;
    /* Check for issued ops that have been completed */
    while(sched->req_iter!=NULL) {
        rp = sched->req_iter;
        sched->req_iter = sched->req_iter->next_issued;

        if(rp->state == MPIC_MPICH_STATE_ISSUED) {
            MPI_Status     status;
            MPIR_Request  *mpid_req0 = rp->mpid_req[0];
            MPIR_Request  *mpid_req1 = rp->mpid_req[1];

            if(mpid_req1) {
                (mpid_req1->u.ureq.greq_fns->query_fn)
                (mpid_req1->u.ureq.greq_fns->grequest_extra_state,
                 &status);
            }

            switch(rp->kind) {
                case MPIC_MPICH_KIND_SEND:
                case MPIC_MPICH_KIND_RECV:
                    if(mpid_req0 && MPIR_Request_is_complete(mpid_req0)) {
                        MPIR_Request_free(mpid_req0);
                        rp->mpid_req[0] = NULL;
                    }
                    if(!rp->mpid_req[0]) {
                        if(0){ fprintf(stderr, "  --> MPICH transport (kind=%d) complete\n",
                                          rp->kind);
                             if(rp->nbargs.sendrecv.count>=1) fprintf(stderr, "data send/recvd: %d\n", *(int *)(rp->nbargs.sendrecv.buf));
                        }
                        MPIC_MPICH_record_request_completion(rp,sched);
                    }else
                        MPIC_MPICH_record_request_issue(rp,sched); /*record it again as issued*/
                    break;
                case MPIC_MPICH_KIND_RECV_REDUCE:
                    if(mpid_req0 && MPIR_Request_is_complete(mpid_req0)) {
                        MPIR_Request_free(mpid_req0);
                        if(0)fprintf(stderr,"recv in recv_reduce completed\n");
                        rp->mpid_req[0] = NULL;
                    }

                    if(mpid_req1 && MPIR_Request_is_complete(mpid_req1)) {
                        MPIR_Request_free(mpid_req1);
                        rp->mpid_req[1] = NULL;
                    }

                    if(!rp->mpid_req[0] && !rp->mpid_req[1]) {
                        if(0){ fprintf(stderr, "  --> MPICH transport (kind=%d) complete\n",
                                          rp->kind);
                             if(rp->nbargs.sendrecv.count>=1) fprintf(stderr, "data send/recvd: %d\n", *(int *)(rp->nbargs.sendrecv.buf));
                        }
                        MPIC_MPICH_record_request_completion(rp,sched);
                    }else
                        MPIC_MPICH_record_request_issue(rp,sched);/*record it again as issued*/

                    break;

                default:
                    break;
            }
        }
        //sched->req_iter = sched->req_iter->next_issued;
    }
    sched->last_issued->next_issued = NULL;

    if(0){

        if(sched->num_completed==sched->total) {
            if(0) fprintf(stderr, "  --> MPICH transport (test) complete:  sched->total=%ld\n",
                              sched->total);
        }
    }
    return sched->num_completed==sched->total;
}

 /*frees any memory allocated for execution of this schedule*/
static inline void MPIC_MPICH_free_buffers(MPIC_MPICH_sched_t *s){
    int i;
    for(i=0; i<s->total; i++){
        /*free the temporary memory allocated by recv_reduce call*/
        if(s->requests[i].kind == MPIC_MPICH_KIND_RECV_REDUCE){
            MPIC_MPICH_free_mem(s->requests[i].nbargs.recv_reduce.inbuf);
        }
    }
    /*free temporary buffers*/
    for(i=0; i<s->nbufs; i++){
        MPIC_MPICH_free_mem(s->buf[i]);
    }
}



#endif

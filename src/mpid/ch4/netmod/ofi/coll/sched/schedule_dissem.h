/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include <math.h>

/*brucks Pack, UnPack (PUP) utility function
This functions packs (unpacks) non-contiguous (contiguous)
data from (to) rbuf to (from) pupbuf. It goes to every offset
that has the value "digitval" at the "phase"th digit in the
base k representation of the offset. The argument phase refers
to the phase of the brucks algorithm.
*/
static inline int
COLL_brucks_pup(bool pack, void *rbuf, void *pupbuf, COLL_dt_t* rtype, int count, \
        int phase, int k, int digitval, int comm_size, int *pupsize, COLL_sched_t *s, int ninvtcs, int *invtcs){
#if 0
    size_t type_size, extent, lb;
    int is_contig;
    TSP_dtinfo(&rtype->tsp_dt,&is_contig,&type_size,&extent,&lb);

    int pow_k_phase = pow(k,phase);
    int offset = pow_k_phase*digitval; /*first offset where the phase'th bit has value digitval*/
    int nconsecutive_occurrences = pow_k_phase;/*number of consecutive occurences of digitval*/
    int delta = (k-1)*pow_k_phase;/*distance between non-consecutive occurences of digitval*/

    *pupsize = 0;/*points to the first empty location in pupbuf*/
    while(offset < comm_size){
        if(pack){
            TSP_dtcopy_nb(pupbuf+*pupsize, count, rtype, rbuf+offset*count*extent, count, rtype, &s->tsp_sched, ninvtcs, invtcs);
            if(0) fprintf(stderr,"packing rbuf+%d to pupbuf+%d\n",offset*count*extent,*pupsize);
        }
        else{
            TSP_dtcopy_nb(rbuf+offset*count*extent, count, rtype, pupbuf+*pupsize, count, rtype, &s->tsp_sched, ninvtcs, invtcs);
            if(0) fprintf(stderr,"unpacking from pupbuf+%d to rbuf+%d\n",*pupsize,offset*count*extent);
        }

        offset += 1;
        nconsecutive_occurrences -=1;

        if(nconsecutive_occurrences==0) { /*consecutive occurrences are over*/
            offset += delta;
            nconsecutive_occurrences = pow_k_phase;
        }

        *pupsize += count*extent;
    }
#endif
}

static inline int
COLL_sched_alltoall(const void *sendbuf, int sendcount, COLL_dt_t *sendtype,
                    void *recvbuf, int recvcount, COLL_dt_t *recvtype,
                    COLL_comm_t *comm, int tag, COLL_sched_t *s)
{
#if 0
    int invtcs[MAX_REQUESTS];
    int n_invtcs;

    int k=2;
    int rank = TSP_rank(&comm->tsp_comm);
    int size = TSP_size(&comm->tsp_comm);

    int nphases = 0;
    int max = size-1;

    /*calculate the number of bits required to represent a rank in base k*/
    while(max) {
        nphases++;
        max/=k;
    }

    if(0) fprintf(stderr,"num phases: %d\n", nphases);

    if(TSP_isinplace(sendbuf)) {
        sendcount = recvcount;
        sendtype = recvtype;
    }

    /*get dt info of sendtype and recvtype*/
    size_t s_type_size, s_extent, s_lb, r_type_size, r_extent, r_lb;
    int s_iscontig, r_iscontig;
    TSP_dtinfo(&sendtype->tsp_dt,&s_iscontig,&s_type_size,&s_extent,&s_lb);
    TSP_dtinfo(&recvtype->tsp_dt,&r_iscontig,&r_type_size,&r_extent,&r_lb);

    void *tmp_buf = (void *)TSP_allocate_mem(recvcount*size*r_extent);/*temporary buffer used for rotation
                                                                      also used as sendbuf when inplace is true*/
    const void *senddata;/*pointer to send data*/

    if(TSP_isinplace(sendbuf)) {
        /*copy from recvbuf to tmp_buf*/
        invtcs[0] = TSP_dtcopy_nb(tmp_buf, size*recvcount, recvtype, \
                        recvbuf, size*recvcount, recvtype, &s->tsp_sched, 0, NULL);
        senddata = tmp_buf;

        n_invtcs=1;
    }
    else{
        senddata = sendbuf;
        n_invtcs=0;
    }

    /*Brucks algo Step 1: rotate the data locally*/
    TSP_dtcopy_nb(recvbuf, (size-rank)*recvcount, recvtype, \
        (void*)((char*)senddata+rank*sendcount*s_extent), (size-rank)*sendcount, sendtype, &s->tsp_sched, n_invtcs, invtcs);
    TSP_dtcopy_nb((void*)((char*)recvbuf+(size-rank)*recvcount*r_extent), \
        rank*recvcount, recvtype, senddata, rank*sendcount, sendtype, &s->tsp_sched, n_invtcs, invtcs);
    if(0) fprintf(stderr,"Step 1 data rotation scheduled\n");

    /*Step 2: Allocate buffer space for packing of data*/
    int i, delta=1, src, dst, j;
    void **tmp_sbuf = (void **)TSP_allocate_mem(sizeof(void *)*(k-1));
    void **tmp_rbuf = (void **)TSP_allocate_mem(sizeof(void *)*(k-1));

    for(i=0; i<k-1; i++) {
        tmp_sbuf[i] = TSP_allocate_mem((int)r_extent*recvcount*ceil((float)size/k));
        tmp_rbuf[i] = TSP_allocate_mem((int)r_extent*recvcount*ceil((float)size/k));
    }

    if(0) fprintf(stderr,"allocated temporary buffer space for packing\n");

    /*This is TSP_dt for packed buffer (for referring to one byte sized elements,
    currently just using control_dt which is MPI_CHAR in mpich transport*/
    TSP_dt_t *pack_dt = &COLL_global.control_dt;
    /*use invtcs in the following manner
    0..k-2 for pack ids
    k-1..2k-3 for send ids
    2k-2..3k-4 for recv ids
    3k-3..4k-5 for unpack ids
    */
    int *packids = invtcs;
    int *sendids = invtcs+k-1;
    int *recvids = invtcs+2k-2;
    int *unpackids = invtcs+3k-3;

    int fenceid = TSP_fence(&s->tsp_sched);
    int packsize=0;

    for(i=0; i<nphases; i++) {
        for(j=1; j<k; j++) { /*for every non-zero value of digitval*/
            if(pow(k,i)*j >= size)/*if the first location exceeds comm size, nothing is to be sent*/
                break;

            src = (rank - delta*j + size)%size;
            dst = (rank + delta*j)%size;

            packids[j-1] = COLL_brucks_pup(1,recvbuf,tmp_sbuf[j-1],recvtype,recvcount,i,k,j,size,&packsize,s,(i==0)?1:k-1,(i==0)?&fenceid:unpackids);
            TSP_add_vtx_dependencies(s,packids[j-1],1,sendids[j-1]);
            if(0) fprintf(stderr,"phase %d, digit %d packing scheduled\n", i,j);
            sendids[j-1] = TSP_send(tmp_sbuf[j-1],packsize,pack_dt,dst,tag,&comm->tsp_comm,&s->tsp_sched,1,packids[j-1]);
            if(0) fprintf(stderr,"phase %d, digit %d  send scheduled\n", i,j);
            recvids[j-1] = TSP_recv(tmp_rbuf[j-1],packsize,pack_dt,src,tag,&comm->tsp_comm,&s->tsp_sched,1,(i==0)?&fenceid:unpackids[j-1]);
            if(0) fprintf(stderr,"phase %d, digit %d recv scheduled\n", i,j);
            unpackids[j-1] = COLL_brucks_pup(0,recvbuf,tmp_rbuf[j-1],recvtype,recvcount,i,k,j,size,&packsize,s,1,packids[j-1]);
            TSP_add_vtx_dependencies(s,unpackids[j-1],1,recvids[j-1]);
            if(0) fprintf(stderr,"phase %d, digit %d unpacking scheduled\n", i,j);
        }

        if(0) fprintf(stderr,"phase %d scheduled\n", i);

        delta *=k;
    }

    if(0) fprintf(stderr,"Step 2 %d scheduled\n", i);

    /*Step 3: rotate the buffer*/
    /*TODO: MPICH implementation does some lower_bound adjustment here for derived datatypes,
    I am skipping that for now, will come back to it later on - will require adding API
    for getting true_lb*/
    invtcs[0] = TSP_dtcopy_nb(tmp_buf, (size-rank-1)*recvcount, recvtype, \
        (void*)((char*)recvbuf+(rank+1)*recvcount*r_extent), (size-rank-1)*recvcount, recvtype, &s->tsp_sched, k-1, unpackids);
    invtcs[1] = TSP_dtcopy_nb((void*)((char*)tmp_buf+(size-rank-1)*recvcount*r_extent), \
        (rank+1)*recvcount, recvtype, recvbuf, (rank+1)*recvcount, recvtype, &s->tsp_sched, k-1, unpackids);

    /*invert the buffer now to get the result in desired order*/
    for(i=0; i<size; i++)
        TSP_dtcopy_nb((void*)((char*)recvbuf+(size-i-1)*recvcount*r_extent), \
            recvcount, recvtype, tmp_buf+i*recvcount*r_extent, recvcount, recvtype, &s->tsp_sched,2,invtcs);
    fenceid = TSP_fence(&s->tsp_sched);

    if(0) fprintf(stderr,"Step 3: data rearrangement scheduled\n");

    /*free all allocated memory*/
    for(i=0; i<k-1; i++){
        TSP_free_mem_nb(tmp_sbuf[i], &s->tsp_sched,1,sendids[i]);
        TSP_free_mem_nb(tmp_rbuf[i], &s->tsp_sched,1,recvids[i]);
    }
    TSP_free_mem(tmp_sbuf, &s->tsp_sched);
    TSP_free_mem(tmp_rbuf, &s->tsp_sched);
    TSP_free_mem_nb(tmp_buf, &s->tsp_sched,1,&fenceid);
#endif
}

static inline int
COLL_sched_barrier_dissem(int                 tag,
                          COLL_comm_t        *comm,
                          COLL_sched_t *s)
{
    int               i,n;
    int               nphases = 0;
    COLL_tree_comm_t *mycomm  = &comm->tree_comm;
    TSP_dt_t         *dt      = &COLL_global.control_dt;

    for(n=mycomm->tree.nranks-1; n>0; n>>=1)nphases++;

    int *recvids = TSP_allocate_mem(sizeof(int)*nphases);
    for(i=0; i<nphases; i++) {
        int shift   = (1<<i);
        int to      = (mycomm->tree.rank+shift)%mycomm->tree.nranks;
        int from    = (mycomm->tree.rank)-shift;

        if(from<0) from = mycomm->tree.nranks+from;

        recvids[i] = TSP_recv(NULL,0,dt,from,tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
        TSP_send(NULL,0,dt,to,tag,&comm->tsp_comm,&s->tsp_sched,i,recvids);
    }

    TSP_sched_commit(&s->tsp_sched);
    TSP_free_mem(recvids);
    return 0;
}

static inline int
COLL_sched_allreduce_dissem(const void         *sendbuf,
                            void               *recvbuf,
                            int                 count,
                            COLL_dt_t          *datatype,
                            COLL_op_t          *op,
                            int                 tag,
                            COLL_comm_t        *comm,
                            COLL_sched_t *s)
{
    /* does not handle in place or communative */
    int               upperPow,lowerPow,nphases = 0;
    int               i,n,is_contig,notPow2,inLower,dissemPhases,dissemRanks;
    COLL_tree_comm_t *mycomm  = &comm->tree_comm;
    size_t            extent,lb,type_size;

    TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);

    for(n=mycomm->tree.nranks-1; n>0; n>>=1)nphases++;

    upperPow = (1<<nphases);
    lowerPow = (1<<(nphases-1));
    notPow2  = (upperPow!=mycomm->tree.nranks);

    int dtcopy_id = TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt,
                  sendbuf,count,&datatype->tsp_dt,
                  &s->tsp_sched,0,NULL);

    inLower        = mycomm->tree.rank<lowerPow;
    dissemPhases = nphases-1;
    dissemRanks  = lowerPow;

    int rrid=-1, sid;/*recv_reduce id and send id for supporting DAG*/
    if(notPow2 && inLower) {
        int from = mycomm->tree.rank+lowerPow;

        if(from < mycomm->tree.nranks) {
            rrid = TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,
                            &op->tsp_op,from,tag,&comm->tsp_comm, TSP_FLAG_REDUCE_L,
                            &s->tsp_sched,1, &dtcopy_id);
        }
    }
    else if(notPow2) {
        int to = mycomm->tree.rank%lowerPow;
        TSP_send_accumulate(sendbuf,count,&datatype->tsp_dt,
                            &op->tsp_op,to,tag,&comm->tsp_comm,
                            &s->tsp_sched,0, NULL);
    } else {
        inLower        = 1;
        dissemPhases = nphases;
        dissemRanks  = mycomm->tree.nranks;
    }
    int id[2]; id[0]=(rrid==-1)?dtcopy_id:rrid;
    if(inLower) {
        void *tmpbuf = TSP_allocate_mem(extent*count);
        for(i=0; i<dissemPhases; i++) {
            int shift      = (1<<i);
            int to         = (mycomm->tree.rank+shift)%dissemRanks;
            int from       = (mycomm->tree.rank)-shift;

            if(from<0)from = dissemRanks+from;

            dtcopy_id = TSP_dtcopy_nb(tmpbuf,count,&datatype->tsp_dt,
                          recvbuf,count,&datatype->tsp_dt,
                          &s->tsp_sched,(i==0)?1:2,id);
            id[0] = TSP_send_accumulate(tmpbuf,count,&datatype->tsp_dt,
                                &op->tsp_op,to,tag,&comm->tsp_comm,
                                &s->tsp_sched,1, &dtcopy_id);
            id[1] = TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,
                            &op->tsp_op,from,tag,&comm->tsp_comm,
                            TSP_FLAG_REDUCE_L,&s->tsp_sched,1, &dtcopy_id);
        }
        /***FREE tmpbuf***/
        TSP_free_mem_nb(tmpbuf,&s->tsp_sched,1,id);
    }

    if(notPow2 && inLower) {
        int to = mycomm->tree.rank+lowerPow;

        if(to < mycomm->tree.nranks) {
            TSP_send(recvbuf,count,&datatype->tsp_dt,to,tag,
                     &comm->tsp_comm,&s->tsp_sched,1,id+1);
        }
    } else if(notPow2) {
        int from = mycomm->tree.rank%lowerPow;
        TSP_recv(recvbuf,count,&datatype->tsp_dt,
                 from,tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
    }
    return 0;
}

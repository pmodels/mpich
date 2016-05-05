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


#ifndef COLL_NAMESPACE
#error "The ring template must be namespaced with COLL_NAMESPACE"
#endif
static inline int
COLL_sched_alltoall_ring(const void     *sendbuf,
                         int                 sendcount,
                         COLL_dt_t          *sendtype,
                         void               *recvbuf,
                         int                 recvcount,
                         COLL_dt_t          *recvtype,
                         int                 tag,
                         COLL_comm_t        *comm,
                         COLL_sched_t       *s,
                         int                 finalize)
{
    void    *tmp_buf = NULL;
    void    *tmp_sendbuf = NULL;
    void    *swp_buf = NULL;
    int      i, is_contig, src, dst;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               comm_size= mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    size_t  rt_lb, recvtype_extent, type_size;
    size_t  sendtype_extent;
    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);

    if(TSP_isinplace(sendbuf)) {
        sendcount=recvcount;
        sendtype=recvtype;
        swp_buf=(char *)recvbuf;
    } else
        swp_buf = (char *)sendbuf;

    tmp_buf = TSP_allocate_mem(comm_size*recvcount*recvtype_extent);
    tmp_sendbuf = TSP_allocate_mem(comm_size*recvcount*recvtype_extent);
    TSP_dtinfo(&sendtype->tsp_dt,&is_contig,&type_size, &sendtype_extent,&rt_lb);
    TSP_dtcopy_nb((char *)tmp_sendbuf, comm_size*sendcount,
                  &sendtype->tsp_dt,(char *)swp_buf,
                  comm_size*sendcount, &sendtype->tsp_dt, &s->tsp_sched);

    if(0) fprintf(stderr, "rank =%d, swp_buf=%p, sendbuf=%p"
                      " tmp_buf=%p,tmp_sendbuf=%p, recvbuf=%p \n",
                      rank, swp_buf, sendbuf, tmp_buf, tmp_sendbuf,recvbuf);

    if(!TSP_isinplace(sendbuf))
        TSP_dtcopy_nb((char *)recvbuf+rank*recvcount*recvtype_extent,
                      recvcount, &recvtype->tsp_dt,
                      (char *)tmp_sendbuf+rank*sendcount*sendtype_extent,
                      sendcount, &sendtype->tsp_dt, &s->tsp_sched);

    if(0) fprintf(stderr, "copying your own data to %dth location at add:%p on"
                      " %d rank before for loop from tmp_sendbuf:%p\n",
                      rank, recvbuf+rank*recvcount*recvtype_extent,rank,
                      tmp_sendbuf+rank*sendcount*sendtype_extent);

    for(i=0; i<comm_size-1; i++) {
        /*Copy the recvd data in correct spot in recvbuf*/
        src = (comm_size+rank-1)%comm_size;
        dst = (rank+1)%comm_size;

        if(0) fprintf(stderr, "i=%d, src=%d, dst =%d\n", i,src, dst);

        /*recv data into tmp_buf*/
        if(0) fprintf(stderr,"Post recv at rank:%d at add tmp_buf:%p\n",rank, tmp_buf);

        TSP_recv((char *)tmp_buf, comm_size*recvcount, &recvtype->tsp_dt,
                 src, tag, &comm->tsp_comm, &s->tsp_sched);

        /*send data from tmp_sendbuf*/
        if(0) fprintf(stderr,"Post send at rank:%d at add tmp_sendbuf:%p\n",rank, tmp_sendbuf);

        TSP_send((char *)tmp_sendbuf, comm_size*recvcount, &recvtype->tsp_dt,
                 dst, tag, &comm->tsp_comm, &s->tsp_sched);
        TSP_fence(&s->tsp_sched);
        swp_buf=tmp_sendbuf;
        tmp_sendbuf=tmp_buf;
        tmp_buf=swp_buf;

        dst=(comm_size+rank-i-1)%comm_size;

        if(0) fprintf(stderr, "storing data to %dth location at add:%p"
                          "on %d rank for i=%d from add:%p\n",
                          dst, recvbuf+dst*recvcount*recvtype_extent, rank, i,
                          tmp_sendbuf+rank*recvcount*recvtype_extent);

        TSP_dtcopy_nb((char *)recvbuf+dst*recvcount*recvtype_extent,
                      recvcount, &recvtype->tsp_dt,
                      (char *)tmp_sendbuf+rank*recvcount*recvtype_extent,
                      recvcount, &recvtype->tsp_dt, &s->tsp_sched);

        if(0) fprintf(stderr, "rank =%d, swp_buf=%p, sendbuf=%p, tmp_buf=%p\n", rank, swp_buf, sendbuf, tmp_buf);
    }

    if(0) fprintf(stderr, "freeing tmp_buf at address:%p\n", tmp_buf);

    if(0) fprintf(stderr, "freeing tmp_sendbuf at address:%p\n", tmp_sendbuf);

    TSP_free_mem_nb(tmp_buf, &s->tsp_sched);
    TSP_free_mem_nb(tmp_sendbuf, &s->tsp_sched);

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

static inline int
COLL_sched_allgather_ring(const void    *sendbuf,
                          int                 sendcount,
                          COLL_dt_t          *sendtype,
                          void               *recvbuf,
                          int                 recvcount,
                          COLL_dt_t          *recvtype,
                          int                 tag,
                          COLL_comm_t        *comm,
                          COLL_sched_t       *s,
                          int                 finalize)
{
    void    *tmp_buf = NULL;
    void    *tmp_sendbuf = NULL;
    void    *swp_buf = NULL;
    int      i, is_contig, src, dst;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               comm_size= mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    size_t  rt_lb, recvtype_extent, type_size;
    size_t  st_lb, sendtype_extent;

    if(TSP_isinplace(sendbuf)) {
        sendcount=recvcount;
        sendtype=recvtype;
        swp_buf=(char *)recvbuf;
    } else
        swp_buf=(char *)sendbuf;

    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);
    TSP_dtinfo(&sendtype->tsp_dt,&is_contig,&type_size, &sendtype_extent,&st_lb);

    tmp_buf = TSP_allocate_mem(sendcount*sendtype_extent);
    tmp_sendbuf = TSP_allocate_mem(sendcount*sendtype_extent);

    if(0) fprintf(stderr, "Size of tmp_bufs:count(%d)*sendtype.\n",sendcount);

    /*copy your data from recv buf in the tmp_sendbuf*/
    /*or copy your sendbuf in the tmp_sendbuf*/
    if(TSP_isinplace(sendbuf)) {
        if(0) fprintf(stderr, "copying data  to tmp_buf  on add:%p on %d rank before for loop from sendbuf:%p\n", tmp_sendbuf, rank, swp_buf+rank*sendtype_extent);

        TSP_dtcopy_nb((char *)tmp_sendbuf, sendcount,
                      &sendtype->tsp_dt,(char *)swp_buf+rank*sendcount*sendtype_extent,
                      sendcount, &sendtype->tsp_dt, &s->tsp_sched);
    } else {
        TSP_dtcopy_nb((char *)tmp_sendbuf, sendcount,
                      &sendtype->tsp_dt,(char *)swp_buf,
                      sendcount, &sendtype->tsp_dt, &s->tsp_sched);
    }

    if(!TSP_isinplace(sendbuf)) {
        TSP_dtcopy_nb((char *)recvbuf+rank*recvcount*recvtype_extent,
                      recvcount, &recvtype->tsp_dt,
                      (char *)tmp_sendbuf, sendcount,
                      &sendtype->tsp_dt, &s->tsp_sched);
    }

    for(i=0; i<comm_size-1; i++) {
        /*Copy the recvd data in correct spot in recvbuf*/
        src = (comm_size+rank-1)%comm_size;
        dst = (rank+1)%comm_size;

        /*recv data into tmp_buf*/
        TSP_recv((char *)tmp_buf, sendcount, &sendtype->tsp_dt,
                 src, tag, &comm->tsp_comm, &s->tsp_sched);
        /*send data from tmp_sendbuf*/
        TSP_send((char *)tmp_sendbuf, sendcount, &sendtype->tsp_dt,
                 dst, tag, &comm->tsp_comm, &s->tsp_sched);
        TSP_fence(&s->tsp_sched);

        swp_buf=tmp_sendbuf;
        tmp_sendbuf=tmp_buf;
        tmp_buf=swp_buf;

        /*Copy the data in the correct location*/
        dst=(comm_size+rank-i-1)%comm_size;

        if(0) fprintf(stderr, "storing data to %dth location at add:%p on %d rank for i=%d from add:%p\n",dst, recvbuf+dst*recvcount*recvtype_extent, rank, i,tmp_sendbuf);

        TSP_dtcopy_nb((char *)recvbuf+dst*recvcount*recvtype_extent,
                      recvcount, &recvtype->tsp_dt,
                      (char *)tmp_sendbuf, sendcount,
                      &sendtype->tsp_dt, &s->tsp_sched);
    }

    TSP_free_mem_nb(tmp_buf, &s->tsp_sched);
    TSP_free_mem_nb(tmp_sendbuf, &s->tsp_sched);

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

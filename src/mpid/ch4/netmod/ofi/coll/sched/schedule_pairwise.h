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
#error "The pairwise template must be namespaced with COLL_NAMESPACE"
#endif


static inline int
COLL_sched_alltoall_pairwise(const void *sendbuf,
                             int                 sendcount,
                             COLL_dt_t          *sendtype,
                             void               *recvbuf,
                             int                 recvcount,
                             COLL_dt_t          *recvtype,
                             int                 tag,
                             COLL_comm_t        *comm,
                             COLL_sched_t *s,
                             int                 finalize)
{
    void    *tmp_buf = NULL;
    int     pof2, i, dst, src, is_contig;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               comm_size= mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    size_t  rt_lb, recvtype_extent, type_size;
    size_t  st_lb, sendtype_extent;
    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);

    /* Is comm_size a power-of-two? */
    i = 1;

    while(i < comm_size)
        i *= 2;

    if(i == comm_size)
        pof2 = 1;
    else
        pof2 = 0;

    assert(!TSP_isinplace(sendbuf) || pof2==1);

    if(TSP_isinplace(sendbuf)) {
        tmp_buf = TSP_allocate_mem(recvcount*recvtype_extent);
    } else {
        TSP_dtinfo(&sendtype->tsp_dt,&is_contig,&type_size, &sendtype_extent,&st_lb);
        TSP_dtcopy_nb(recvbuf+rank*recvcount*recvtype_extent, recvcount, &recvtype->tsp_dt,
                      (char *)sendbuf+rank*sendcount*sendtype_extent,
                      sendcount, &sendtype->tsp_dt, &s->tsp_sched);
        TSP_fence(&s->tsp_sched);
    }

    /* Do the pairwise exchanges */
    for(i=1; i<comm_size; i++) {
        if(pof2 == 1) {
            /* use exclusive-or algorithm */
            src = dst = rank ^ i;
        } else {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;

            if(0) fprintf(stderr, "src:%d, dst:%d\n", src,dst);
        }

        //copy tmp_buf here
        if(TSP_isinplace(sendbuf)) {
            /* copy recv buffer to a temp buffer*/
            TSP_dtcopy_nb(tmp_buf, recvcount, &recvtype->tsp_dt,
                          (char *)recvbuf+dst*recvcount*recvtype_extent,
                          recvcount, &recvtype->tsp_dt, &s->tsp_sched);
            TSP_fence(&s->tsp_sched);
        } else {
            tmp_buf=(char *)sendbuf+dst*sendcount*sendtype_extent;
        }

        TSP_recv((char *)recvbuf+src*recvcount*recvtype_extent, recvcount,
                 &recvtype->tsp_dt, src, tag, &comm->tsp_comm, &s->tsp_sched);
        TSP_send((char *)tmp_buf, recvcount,
                 &recvtype->tsp_dt, dst, tag, &comm->tsp_comm, &s->tsp_sched);
        TSP_fence(&s->tsp_sched);
    }

    if(TSP_isinplace(sendbuf))
        TSP_free_mem_nb(tmp_buf, &s->tsp_sched);

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

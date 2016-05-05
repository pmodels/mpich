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
#error "The scattered template must be namespaced with COLL_NAMESPACE"
#endif


static inline int
COLL_sched_alltoall_scattered(const void *sendbuf,
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
    int         ii, ss, bblock, dst, i, is_contig;
    size_t      rt_lb, recvtype_extent, type_size;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               comm_size= mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    bblock = 32; /*TODO  create a replacement for "MPIR_CVAR_ALLTOALL_THROTTLE"; */

    if(bblock == 0) bblock = comm_size;

    /* Get extent of send and recv types */
    TSP_dtinfo(&recvtype->tsp_dt,&is_contig, &type_size, &recvtype_extent,&rt_lb);

    if(TSP_isinplace(sendbuf)) {
        void *tmp_buf = NULL;
        /* TODO create a min function, ask if really necessary*/
        tmp_buf = TSP_allocate_mem(bblock*recvcount*recvtype_extent);

        for(ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;
            /* copy recv buffer to a temp buffer: */
            TSP_dtcopy_nb(tmp_buf, ss*recvcount, &recvtype->tsp_dt,
                          (char *)recvbuf+ii*recvcount*recvtype_extent,
                          ss*recvcount, &recvtype->tsp_dt, &s->tsp_sched);
            /*fence for copy to complete*/
            TSP_fence(&s->tsp_sched);
            /* do the communication -- post ss sends and receives: */

            for(i=0; i<ss; i++) {
                dst = (rank+i+ii) % comm_size;
                TSP_recv((char *)recvbuf+dst*recvcount*recvtype_extent, recvcount,
                         &recvtype->tsp_dt, dst, tag,
                         &comm->tsp_comm, &s->tsp_sched);

                if(0) fprintf(stderr, "receving from: %d\n", dst);

                dst = (rank-i-ii+comm_size) % comm_size;
                TSP_send((char *)tmp_buf+dst*recvcount*recvtype_extent, recvcount,
                         &recvtype->tsp_dt, dst, tag,
                         &comm->tsp_comm, &s->tsp_sched);

                if(0) fprintf(stderr, "sending %d to: %d\n", *((int *)recvbuf+dst*recvcount),dst);
            }

            /* ... then wait for them to finish: */
            TSP_fence(&s->tsp_sched);
        }

        TSP_free_mem_nb(tmp_buf, &s->tsp_sched);
    } else {
        size_t  st_lb, sendtype_extent;
        TSP_dtinfo(&sendtype->tsp_dt,&is_contig,&type_size, &sendtype_extent,&st_lb);

        for(ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for(i=0; i<ss; i++) {
                dst = (rank+i+ii) % comm_size;
                TSP_recv((char *)recvbuf+dst*recvcount*recvtype_extent, recvcount,
                         &recvtype->tsp_dt, dst, tag,
                         &comm->tsp_comm, &s->tsp_sched);

                if(0) fprintf(stderr, "receving from: %d\n", dst);

                dst = (rank-i-ii+comm_size) % comm_size;
                TSP_send((char *)sendbuf+dst*sendcount*sendtype_extent, sendcount,
                         &sendtype->tsp_dt, dst, tag,
                         &comm->tsp_comm, &s->tsp_sched);

                if(0) fprintf(stderr, "sending %d to: %d\n", *((int *)sendbuf+dst*sendcount),dst);
            }

            /* ... then wait for them to finish: */
            TSP_fence(&s->tsp_sched);
        }

    }

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

static inline int
COLL_sched_alltoallv_scattered(const void   *sendbuf,
                               const int    *sendcounts,
                               const int    *sdispls,
                               COLL_dt_t    *sendtype,
                               void         *recvbuf,
                               const int    *recvcounts,
                               const int    *rdispls,
                               COLL_dt_t    *recvtype,
                               int           tag,
                               COLL_comm_t  *comm,
                               COLL_sched_t *s,
                               int           finalize)
{
    int         ii, ss, bblock, dst, i, is_contig;
    size_t      rt_lb, recvtype_extent, type_size;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               comm_size= mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    bblock = 32; /*TODO  create a replacement for "MPIR_CVAR_ALLTOALL_THROTTLE"; */

    if(bblock == 0) bblock = comm_size;

    if(TSP_isinplace(sendbuf)) {
        void *tmp_buf = NULL;
        int total_count, buf_count=0;

        /* TODO create a min function, ask if really necessary*/
        for(ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /*Calculate the size  of the tmp buf needed*/
            for(i=ii; i<ss; i++) {
                buf_count+=recvcounts[i];
            }

            tmp_buf = TSP_allocate_mem(buf_count*recvtype_extent);
            /* copy recv buffer to a temp buffer: */
            TSP_dtcopy_nb(tmp_buf, buf_count, &recvtype->tsp_dt,
                          (char *)recvbuf+rdispls[ii]*recvtype_extent,
                          buf_count, &recvtype->tsp_dt, &s->tsp_sched);
            /*fence for copy to complete*/
            TSP_fence(&s->tsp_sched);
            /* do the communication -- post ss sends and receives: */

            for(i=0; i<ss; i++) {
                dst = (rank+i+ii) % comm_size;

                if(recvcounts[dst]) {
                    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);

                    if(type_size) {
                        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                         rdispls[dst]*recvtype_extent);
                        TSP_recv((char *)recvbuf+rdispls[dst]*recvtype_extent, recvcounts[dst],
                                 &recvtype->tsp_dt, dst, tag,
                                 &comm->tsp_comm, &s->tsp_sched);

                        if(0) fprintf(stderr, "receving from: %d\n", dst);
                    }
                }

                dst = (rank-i-ii+comm_size) % comm_size;

                if(recvcounts[dst]) {
                    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);

                    if(type_size) {
                        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT tmp_buf +
                                                         (rdispls[dst]-rdispls[ii])*recvtype_extent);
                        TSP_send((char *)tmp_buf+(rdispls[dst]-rdispls[ii])*recvtype_extent, recvcounts[dst],
                                 &recvtype->tsp_dt, dst, tag,
                                 &comm->tsp_comm, &s->tsp_sched);

                        if(0) fprintf(stderr, "sending %d to: %d\n", *((int *)tmp_buf+(rdispls[dst])*recvcounts[dst]),dst);
                    }
                }

                /*                TSP_send((char*)tmp_buf+dst*recvcount*recvtype_extent, recvcount,
                                                        &recvtype->tsp_dt, dst, tag,
                                                        &comm->tsp_comm, &s->tsp_sched);
                                 if(0) fprintf(stderr, "sending %d to: %d\n", *((int*)recvbuf+dst*recvcount),dst);
                  */
            }

            /* ... then wait for them to finish: */
            TSP_fence(&s->tsp_sched);
            TSP_free_mem_nb(tmp_buf, &s->tsp_sched);
            total_count+=buf_count;
            buf_count=0;
        }
    } else {
        /* Get extent of send and recv types */
        size_t  st_lb, sendtype_extent;

        for(ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for(i=0; i<ss; i++) {
                dst = (rank+i+ii) % comm_size;

                if(recvcounts[dst]) {
                    TSP_dtinfo(&recvtype->tsp_dt,&is_contig,&type_size, &recvtype_extent,&rt_lb);

                    if(type_size) {
                        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                         rdispls[dst]*recvtype_extent);
                        TSP_recv((char *)recvbuf+rdispls[dst]*recvtype_extent, recvcounts[dst],
                                 &recvtype->tsp_dt, dst, tag,
                                 &comm->tsp_comm, &s->tsp_sched);

                        if(0) fprintf(stderr, "receving from: %d\n", dst);
                    }
                }

                dst = (rank-i-ii+comm_size) % comm_size;

                if(sendcounts[dst]) {
                    TSP_dtinfo(&sendtype->tsp_dt,&is_contig,&type_size, &sendtype_extent,&st_lb);

                    if(type_size) {
                        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                         sdispls[dst]*sendtype_extent);
                        TSP_send((char *)sendbuf+sdispls[dst]*sendtype_extent, sendcounts[dst],
                                 &sendtype->tsp_dt, dst, tag,
                                 &comm->tsp_comm, &s->tsp_sched);

                        if(0) fprintf(stderr, "sending %d to: %d\n", *((int *)sendbuf+sdispls[dst]*sendcounts[dst]),dst);
                    }
                }
            }

            /* ... then wait for them to finish: */
            TSP_fence(&s->tsp_sched);
        }
    }

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}


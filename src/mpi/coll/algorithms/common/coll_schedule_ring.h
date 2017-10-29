/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */


#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif

/* The below code is an implementation of alltoall ring algorithm,
 * the dependencies are show below (buf1 and buf2 are temporary buffers
 * used for execution of the ring algorithm) :
 *
               send (buf1)           recv (buf2) --> copy (buf2)
                   |                     |              /
                   |                     |             /
                   v                     V           /
copy (buf1)<--recv (buf1)            send (buf2)   /
      \            |                     |       /
        \          |                     |     /
          \        V                     V   v
            \  send (buf1)           recv (buf2) --> copy (buf2)
             \     |                     |
               \   |                     |
                 v v                     v
               recv (buf1)           send (buf2)
*/

MPL_STATIC_INLINE_PREFIX int
COLL_sched_alltoall_ring(const void     * sendbuf,
                         int            sendcount,
                         COLL_dt_t      sendtype,
                         void           * recvbuf,
                         int            recvcount,
                         COLL_dt_t      recvtype,
                         int            tag,
                         COLL_comm_t    * comm,
                         TSP_sched_t    * sched,
                         int            finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLTOALL_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLTOALL_RING);

    int mpi_errno = MPI_SUCCESS;
    int i, is_contig, src, dst, copy_dst;

    /* Temporary buffers to execute the ring algorithm */
    void *buf1, *buf2, *data_buf;

    TSP_comm_t *tsp_comm = &comm->tsp_comm;
    int comm_size        = TSP_size(tsp_comm);
    int rank             = TSP_rank(tsp_comm);

    size_t  recvtype_lb, recvtype_extent, recvtype_size;
    size_t  sendtype_lb, sendtype_extent, sendtype_size;

    TSP_dtinfo(recvtype, &is_contig, &recvtype_size, &recvtype_extent, &recvtype_lb);
    int is_inplace = TSP_isinplace((void *) sendbuf);

    /* allocate space for temporary buffers */
    buf1 = TSP_allocate_buffer(comm_size*recvcount*recvtype_extent, sched);
    buf2 = TSP_allocate_buffer(comm_size*recvcount*recvtype_extent, sched);

    /* for storing task dependencies */
    int nvtcs;
    int *vtcs = (int*) TSP_allocate_mem(3*sizeof(int));

    /* for storing task ids, we need only upto two phases back */
    int *send_vtx = (int*) TSP_allocate_mem (3*sizeof(int));
    int *recv_vtx = (int*) TSP_allocate_mem (3*sizeof(int));
    int *dtcopy_vtx = (int*) TSP_allocate_mem (3*sizeof(int));
    int dtcopy_id;

    /* find out the buffer which has the send data and point data_buf to it */
    if(is_inplace){
        sendcount=recvcount;
        sendtype=recvtype;
        data_buf=recvbuf;
    }
    else
        data_buf = (void *)sendbuf;

    /* copy my data to buf1 which will be forwarded in phase 0 of the ring.
     * TODO: We could avoid this copy but that would make the implementation more
     * complicated */
    dtcopy_id = TSP_dtcopy_nb(buf1, comm_size*recvcount, recvtype, data_buf, \
                              comm_size*recvcount, recvtype, sched, 0, NULL);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"rank =%d, data_buf=%04x, sendbuf=%04x, buf1=%04x, buf2=%04x, recvbuf=%04x \n", rank, data_buf, sendbuf, buf1, buf2, recvbuf));

    if (!is_inplace) { /* copy my part of my sendbuf to recv_buf */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"copying my part from sendbuf address=%04x to my recvbuf address=%04x\n",
                    sendbuf+rank*sendcount*sendtype_extent, recvbuf+rank*recvcount*recvtype_extent));

        TSP_dtinfo(sendtype, &is_contig, &sendtype_size, &sendtype_extent, &sendtype_lb);
        TSP_dtcopy_nb((char*)recvbuf+rank*recvcount*recvtype_extent, recvcount, recvtype,
                            (char*)sendbuf+rank*sendcount*sendtype_extent, sendcount, sendtype, sched, 0, NULL);
    }


    /* in ring algorithm, source and destination of messages are fixed */
    src = (comm_size+rank-1)%comm_size;
    dst = (rank+1)%comm_size;

    void *sbuf = buf1;
    void *rbuf = buf2;
    for(i=0; i<comm_size-1; i++){
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"scheduling phase=%d\n", i));

        /* schedule send */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"posting send\n"));
        nvtcs = 1;
        if (i==0)
            vtcs[0] = dtcopy_id;
        else
            vtcs[0] = recv_vtx[(i-1)%3];

        send_vtx[i%3] = TSP_send(sbuf, comm_size*recvcount, recvtype, dst, tag, tsp_comm, sched, nvtcs, vtcs);

        /* schedule recv */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"posting receive\n"));
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"posting recv at address=%04x, count=%d\n", rbuf, comm_size*recvcount));
        if (i==0) nvtcs=0;
        else if (i==1) { nvtcs=1; vtcs[0] = send_vtx[(i-1)%3];}
        else {
            nvtcs = 2;
            vtcs[0] = send_vtx[(i-1)%3];
            vtcs[1] = dtcopy_vtx[(i-2)%3];
        }

        recv_vtx[i%3] = TSP_recv(rbuf, comm_size*recvcount, recvtype, src, tag, tsp_comm, sched, nvtcs, vtcs);

        /* schedule data copy */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"posting data copy\n"));
        /* destination offset of the copy */
        copy_dst=(comm_size+rank-i-1)%comm_size;
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"copying from location=%04x to location=%04x\n", (char*)rbuf+rank*recvcount*recvtype_extent, (char*)recvbuf+copy_dst*recvcount*recvtype_extent));
        dtcopy_vtx[i%3] =  TSP_dtcopy_nb((char*)recvbuf+copy_dst*recvcount*recvtype_extent, recvcount, recvtype,
                                        (char*)rbuf+rank*recvcount*recvtype_extent, recvcount, recvtype, sched, 1, &recv_vtx[i%3]);

        /* swap sbuf and rbuf - using data_buf as intermeidate buffer */
        data_buf = sbuf;
        sbuf = rbuf;
        rbuf = data_buf;
    }

    if(finalize) {
        TSP_fence(sched);
        TSP_sched_commit(sched);
    }

    /* free all temporary memory allocation */
    TSP_free_mem(vtcs);
    TSP_free_mem(send_vtx);
    TSP_free_mem(recv_vtx);
    TSP_free_mem(dtcopy_vtx);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLTOALL_RING);

    return 0;
}

#undef FUNCNAME
#define FUNCNAME COLL_sched_bcast_ring
/* Routine to schedule a ring based broadcast */
MPL_STATIC_INLINE_PREFIX int
COLL_sched_bcast_ring(void *buffer, int count, COLL_dt_t datatype, int root, int tag,
                      COLL_comm_t * comm, TSP_sched_t * sched, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_BCAST_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_BCAST_RING);

    int mpi_errno = MPI_SUCCESS;
    int i, j;
    int size = TSP_size(&comm->tsp_comm);
    int rank = TSP_rank(&comm->tsp_comm);
    int recv_id;
    
    /* Receive message from predecessor */
    if (rank != 0)
        recv_id = TSP_recv(buffer, count, datatype, rank-1, tag,
                           &comm->tsp_comm, sched, 0, NULL);

    /* Send message to successor */
    if (rank != size-1)
        TSP_send(buffer, count, datatype, rank+1, tag, &comm->tsp_comm, sched, (rank==0)?0:1, &recv_id);

    if (finalize) {
        TSP_sched_commit(sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_BCAST_RING);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int COLL_sched_bcast_ring_pipelined (const void * buffer, int count, COLL_dt_t datatype, 
                        int root, int tag, COLL_comm_t *comm, int segsize, TSP_sched_t *sched, int finalize) {
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_BCAST_RING_PIPELINED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_BCAST_RING_PIPELINED);

    int mpi_errno = MPI_SUCCESS;
    int i;

    /* variables to store pipelining information */
    int num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil;
    int offset = 0;

    /* variables for storing datatype information */
    int is_contig;
    size_t lb, extent, type_size;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Scheduling pipelined broadcast on %d ranks, root=%d\n", TSP_size(&comm->tsp_comm),
             root));

    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    /* calculate chunking information for pipelining */
    MPIC_calculate_chunk_info(segsize, type_size, count, &num_chunks, &num_chunks_floor,
                              &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,
        "Broadcast pipeline info: segsize=%d count=%d num_chunks=%d num_chunks_floor=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
         segsize, count, num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil));

    /* do pipelined broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes correctly with pipelined
     * broadcast, for example, buffer+offset if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        int msgsize = (i < num_chunks_floor) ? chunk_size_floor : chunk_size_ceil;
        mpi_errno =
            COLL_sched_bcast_ring((char *) buffer + offset * extent, msgsize, datatype, root, tag,
                                  comm, sched, 0);
        offset += msgsize;
    }

    /* if this is final part of the schedule, commit it */
    if (finalize)
        TSP_sched_commit(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_BCAST_RING_PIPELINED);

    return mpi_errno;
}

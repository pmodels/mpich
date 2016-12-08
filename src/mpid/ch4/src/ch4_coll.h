/* -*- Mode: C ; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"


MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_BARRIER);

    ret = MPIDI_NM_mpi_barrier(comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_BARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int algo_number;
    MPIDI_algo_parameters_t *ch4_algo_parameters_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_BCAST);

    algo_number = MPIDI_CH4_Bcast_select(buffer, count, datatype, root, comm,
                                         errflag, &ch4_algo_parameters_ptr);

    mpi_errno = MPIDI_CH4_Bcast_call(buffer, count, datatype, root, comm, errflag, algo_number, ch4_algo_parameters_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_BCAST);

    return mpi_errno;

}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_topo(void *buffer, int count, MPI_Datatype datatype,
                                              int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                              MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if(comm->node_roots_comm != NULL)
    {
        MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm->node_roots_comm, errflag, ch4_algo_parameters_ptr);
    }
    if(comm->node_comm != NULL)
    {
        MPIDI_SHM_mpi_bcast(buffer, count, datatype, root, comm->node_comm, errflag, ch4_algo_parameters_ptr);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Bcast_knomial(
    void *buffer,
    int count,
    MPI_Datatype datatype,
    int root,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag,
    MPIDI_algo_parameters_t *params)
{
    int rank, comm_size;
    int relative_rank, mask, max_mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint nbytes=0, recvd_size = 0;
    MPI_Aint type_size = 0, position = 0;
    MPI_Aint true_extent, true_lb;
    int is_contig, k;
    void *tmp_buf = NULL, *sbuf = NULL;
    MPIR_Datatype *dtp;
    MPI_Datatype stype;
    MPI_Status status;
    MPIR_CHKLMEM_DECL(1);
    int parent_rank = -1, child = -1;
    int is_split = 0;
    MPI_Aint tot_size, seg_size;

    /****************************from algo params****************************/
    int knomial_radix = params->generic_bcast_knomial_parameters.radix;
    int64_t block_size = params->generic_bcast_knomial_parameters.block_size;
    /************************************************************************/

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;


    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
        is_contig = 1;
    }
    else 
    {
        MPID_Datatype_get_ptr(datatype, dtp); 
        is_contig = dtp->is_contig;
    }

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    if (true_lb) is_contig = 0;

    /* Default values sutiable for contig messages which less than block_size */ 
    sbuf = buffer;
    stype = datatype;
    seg_size = (int64_t)count;
    if (!is_contig)
    {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        position = 0;
        if (rank == root) 
        {
            mpi_errno = MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes,
                                       &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        /* Redefine buffer vars to transfer noncontig message as bytes array */
        sbuf = tmp_buf;
        stype = MPI_BYTE;
        seg_size = nbytes;
    }
    mask = 1; 

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Find my parent rank */
    while (mask < comm_size) 
    {
        if (relative_rank % (knomial_radix*mask)) 
        {
             parent_rank = relative_rank/(knomial_radix*mask)*(knomial_radix*mask) + root;
             if (parent_rank >= comm_size ) parent_rank -= comm_size;
             break;
        }
        mask *= knomial_radix;
    }

    mask /= knomial_radix;
    max_mask= mask;

    if (comm_size > 2 && nbytes > block_size)
    {
        /* Message is big and we have several processes, lets split the message
         * sbuf is alredy initialized to either buffer (contig case) or tmp_buf (non-contig) */
        is_split = 1;
        seg_size = block_size;
        stype = MPI_BYTE;
    }
    else
    {
        is_split = 0;
        /* message will not be split, so set block_size to the total number of bytes 
         * to have just one iteration of "while" cycle below */
        block_size = nbytes;
    }

    tot_size = nbytes;
    while (tot_size > 0)
    {
        /* Receive from the parent */
        if (rank != root)
        {
            mpi_errno = MPIC_Recv(sbuf, seg_size, stype, parent_rank,
                                  MPIR_BCAST_TAG,comm_ptr, &status, errflag);
            if (mpi_errno) 
            {
                /* for communication errors, just record the error but continue */
                *errflag = TRUE;
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* Send to the children */
        while (mask > 0) 
        {
            for(k = 1; k < knomial_radix; k++) 
            {

                if (relative_rank + mask*k < comm_size) 
                {
                    child = rank + mask*k;
                    if (child >= comm_size) child -= comm_size;
                    mpi_errno = MPIC_Send(sbuf, seg_size, stype, child, MPIR_BCAST_TAG, comm_ptr, errflag); 
                    if (mpi_errno) 
                    {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
            mask /= knomial_radix;
        }
        mask = max_mask; 
        tot_size -= block_size;
        sbuf = (char *)sbuf + seg_size;
        seg_size = MPL_MIN(block_size, tot_size);
    }
    if (!is_contig)
    {
        if (rank != root)
        {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,
                                         count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int algo_number;
    MPIDI_algo_parameters_t *ch4_algo_parameters_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLREDUCE);

    algo_number = MPIDI_CH4_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm,
                                             errflag, &ch4_algo_parameters_ptr);

    MPIDI_CH4_Allreduce_call(sendbuf, recvbuf, count, datatype, op, comm, errflag, algo_number, ch4_algo_parameters_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLREDUCE);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_0(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_algo_parameters_t ch4_reduce_algo_parameters_ptr;
    MPIDI_algo_parameters_t ch4_allreduce_algo_parameters_ptr;
    MPIDI_algo_parameters_t ch4_bcast_algo_parameters_ptr;

    ch4_reduce_algo_parameters_ptr.ch4_reduce.nm_reduce = -1;
    ch4_reduce_algo_parameters_ptr.ch4_reduce.shm_reduce = ch4_algo_parameters_ptr->ch4_allreduce_0.shm_reduce;

    ch4_allreduce_algo_parameters_ptr.ch4_allreduce.nm_allreduce = ch4_algo_parameters_ptr->ch4_allreduce_0.nm_allreduce;
    ch4_allreduce_algo_parameters_ptr.ch4_allreduce.shm_allreduce = -1;

    ch4_bcast_algo_parameters_ptr.ch4_bcast.nm_bcast = -1;
    ch4_bcast_algo_parameters_ptr.ch4_bcast.shm_bcast = ch4_algo_parameters_ptr->ch4_allreduce_0.shm_bcast;

    if(comm->node_comm != NULL)
    {
        MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag, &ch4_reduce_algo_parameters_ptr);
    }
    if(comm->node_roots_comm != NULL)
    {
        MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm->node_roots_comm, errflag, &ch4_allreduce_algo_parameters_ptr);
    }
    if(comm->node_comm != NULL)
    {
        MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag, &ch4_bcast_algo_parameters_ptr);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_1(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_algo_parameters_t ch4_reduce_algo_parameters_ptr;
    MPIDI_algo_parameters_t ch4_bcast_algo_parameters_ptr;

    ch4_reduce_algo_parameters_ptr.ch4_reduce.nm_reduce = ch4_algo_parameters_ptr->ch4_allreduce_1.nm_reduce;
    ch4_reduce_algo_parameters_ptr.ch4_reduce.shm_reduce = ch4_algo_parameters_ptr->ch4_allreduce_1.shm_reduce;

    ch4_bcast_algo_parameters_ptr.ch4_bcast.nm_bcast = ch4_algo_parameters_ptr->ch4_allreduce_1.nm_bcast;
    ch4_bcast_algo_parameters_ptr.ch4_bcast.shm_bcast = ch4_algo_parameters_ptr->ch4_allreduce_1.shm_bcast;

    if(comm->node_comm != NULL)
    {
        MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag, &ch4_reduce_algo_parameters_ptr);
    }
    if(comm->node_roots_comm != NULL)
    {
        MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_roots_comm, errflag, &ch4_reduce_algo_parameters_ptr);
        MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_roots_comm, errflag, &ch4_bcast_algo_parameters_ptr);
    }
    if(comm->node_comm != NULL)
    {
        MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag, &ch4_bcast_algo_parameters_ptr);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_2(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_algo_parameters_t ch4_reduce_algo_parameters_ptr;
    MPIDI_algo_parameters_t ch4_bcast_algo_parameters_ptr;

    ch4_reduce_algo_parameters_ptr.ch4_reduce.nm_reduce = ch4_algo_parameters_ptr->ch4_allreduce_2.nm_reduce;
    ch4_reduce_algo_parameters_ptr.ch4_reduce.shm_reduce = -1;

    ch4_bcast_algo_parameters_ptr.ch4_bcast.nm_bcast = ch4_algo_parameters_ptr->ch4_allreduce_2.nm_bcast;
    ch4_bcast_algo_parameters_ptr.ch4_bcast.shm_bcast = -1;
  
    if(comm->node_roots_comm != NULL)
    {
        MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_roots_comm, errflag, &ch4_reduce_algo_parameters_ptr);
        MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_roots_comm, errflag, &ch4_bcast_algo_parameters_ptr);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLGATHER);

    ret = MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLGATHERV);

    ret = MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCATTER);

    ret = MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCATTERV);

    ret = MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype,
                                recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GATHER);

    ret = MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GATHERV);

    ret = MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALL);

    ret = MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALLV);

    ret = MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw(const void *sendbuf, const int sendcounts[],
                                             const int sdispls[], const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int recvcounts[],
                                             const int rdispls[], const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALLW);

    ret = MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce(const void *sendbuf, void *recvbuf,
                                          int count, MPI_Datatype datatype, MPI_Op op,
                                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int algo_number;
    MPIDI_algo_parameters_t *ch4_algo_parameters_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE);

    algo_number = MPIDI_CH4_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                          errflag, &ch4_algo_parameters_ptr);

    MPIDI_CH4_Reduce_call(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag, algo_number, ch4_algo_parameters_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE);

}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_0(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag, ch4_algo_parameters_ptr); 

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE_SCATTER);

    ret = MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                      errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCAN);

    ret = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Exscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_EXSCAN);

    ret = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_EXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes, void *recvbuf,
                                                      const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int *recvcounts, const int *displs,
                                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                                        MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                       const int *sdispls, MPI_Datatype sendtype,
                                                       void *recvbuf, const int *recvcounts,
                                                       const int *rdispls, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                       const MPI_Aint * sdispls,
                                                       const MPI_Datatype * sendtypes,
                                                       void *recvbuf, const int *recvcounts,
                                                       const MPI_Aint * rdispls,
                                                       const MPI_Datatype * recvtypes,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IBARRIER);

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IBCAST);

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallgather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLGATHER);

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallgatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLGATHERV);

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLREDUCE);

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoall(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALL);

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALLV);

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, const MPI_Datatype * sendtypes,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, const MPI_Datatype * recvtypes,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALLW);

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IEXSCAN);

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Igather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IGATHER);

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Igatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IGATHERV);

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                         int recvcount, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm,
                                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                   const int *recvcounts, MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE_SCATTER);

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE);

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCAN);

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscatter(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCATTER);

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscatterv(const void *sendbuf, const int *sendcounts,
                                             const int *displs, MPI_Datatype sendtype,
                                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCATTERV);
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */

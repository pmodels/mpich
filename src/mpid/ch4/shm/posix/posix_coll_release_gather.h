/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef POSIX_COLL_RELEASE_GATHER_H_INCLUDED
#define POSIX_COLL_RELEASE_GATHER_H_INCLUDED

#include "mpiimpl.h"
#include "algo_common.h"
#include "release_gather.h"

/* Intra-node bcast is implemented as a release step followed by gather step in release_gather
 * framework. The actual data movement happens in release step. Gather step makes sure that
 * the shared bcast buffer can be reused for next bcast call. Release gather framework has
 * multitple cells in bcast buffer, so that the copying in next cell can be overlapped with
 * copying out of previous cells (pipelining).
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bcast_release_gather(void *buffer,
                                                                  int count,
                                                                  MPI_Datatype datatype,
                                                                  int root, MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_BCAST_RELEASE_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_BCAST_RELEASE_GATHER);

    int i, my_rank, num_chunks, chunk_count_floor, chunk_count_ceil;
    int offset = 0, is_contig, ori_count = count;
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint position;
    MPI_Aint lb, true_extent, extent, type_size;
    void *ori_buffer = buffer;
    MPI_Datatype ori_datatype = datatype;

    /* If there is only one process or no data, return */
    if (count == 0 || (MPIR_Comm_size(comm_ptr) == 1)) {
        goto fn_exit;
    }

    /* Lazy initialization of release_gather specific struct */
    mpi_errno =
        MPIDI_POSIX_mpi_release_gather_comm_init(comm_ptr, MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST);
    if (mpi_errno) {
        /* Fall back to other algo as release_gather based bcast cannot be used */
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
        goto fn_exit;
    }

    my_rank = MPIR_Comm_rank(comm_ptr);
    MPIR_Type_get_extent_impl(datatype, &lb, &extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, &type_size);
    }

    if (!is_contig || type_size >= MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE) {
        /* Convert to MPI_BYTE datatype */
        count = type_size * count;
        datatype = MPI_BYTE;
        type_size = 1;
        extent = 1;

        if (!is_contig) {
            buffer = MPL_malloc(count, MPL_MEM_COLL);
            if (my_rank == root) {
                /* Root packs the data before sending, for non contiguous datatypes */
                position = 0;
                mpi_errno =
                    MPIR_Pack_impl(ori_buffer, ori_count, ori_datatype, buffer, count, &position);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
    }

    /* Calculate chunking information for pipelining */
    MPIR_Algo_calculate_pipeline_chunk_info(MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE, type_size,
                                            count, &num_chunks, &chunk_count_floor,
                                            &chunk_count_ceil);
    /* Print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Bcast shmgr pipeline info: segsize=%d count=%d num_chunks=%d chunk_count_floor=%d chunk_count_ceil=%d \n",
                                             MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE, count,
                                             num_chunks, chunk_count_floor, chunk_count_ceil));

    /* Do pipelined release-gather */
    for (i = 0; i < num_chunks; i++) {
        int chunk_count = (i == 0) ? chunk_count_floor : chunk_count_ceil;

        mpi_errno =
            MPIDI_POSIX_mpi_release_gather_release((char *) buffer + offset * extent,
                                                   chunk_count, datatype, root, comm_ptr,
                                                   errflag,
                                                   MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno =
            MPIDI_POSIX_mpi_release_gather_gather(NULL, NULL, 0, MPI_DATATYPE_NULL,
                                                  MPI_OP_NULL, root, comm_ptr, errflag,
                                                  MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        offset += chunk_count;
    }

    if (!is_contig) {
        if (my_rank != root) {
            /* Non-root unpack the data if expecting non-contiguous datatypes */
            position = 0;
            mpi_errno =
                MPIR_Unpack_impl(buffer, count, &position, ori_buffer, ori_count, ori_datatype);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        MPL_free(buffer);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_BCAST_RELEASE_GATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Intra-node reduce is implemented as a release step followed by gather step in release_gather
 * framework. The actual data movement happens in gather step. Release step makes sure that
 * the shared reduce buffer can be reused for next reduce call. Release gather framework has
 * multitple cells in reduce buffer, so that the copying in next cell can be overlapped with
 * reduction and copying out of previous cells (pipelining).
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_reduce_release_gather(const void *sendbuf,
                                                                   void *recvbuf, int count,
                                                                   MPI_Datatype datatype,
                                                                   MPI_Op op, int root,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag)
{
    int i, num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0, is_contig;
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint lb, true_extent, extent, type_size;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_REDUCE_RELEASE_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_REDUCE_RELEASE_GATHER);

    /* If there is no data, return */
    if (count == 0) {
        goto fn_exit;
    }

    if ((MPIR_Comm_size(comm_ptr) == 1) && (sendbuf != MPI_IN_PLACE)) {
        /* Simply copy the data from sendbuf to recvbuf if there is only 1 rank and MPI_IN_PLACE
         * is not used */
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    }

    /* Lazy initialization of release_gather specific struct */
    mpi_errno =
        MPIDI_POSIX_mpi_release_gather_comm_init(comm_ptr,
                                                 MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE);
    if (mpi_errno) {
        /* Fall back to other algo as release_gather algo cannot be used */
        mpi_errno =
            MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        goto fn_exit;
    }

    MPIR_Type_get_extent_impl(datatype, &lb, &extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, &type_size);
    }

    if (sendbuf == MPI_IN_PLACE) {
        sendbuf = recvbuf;
    }

    /* Calculate chunking information, taking max(extent, type_size) handles contiguous and non-contiguous datatypes both */
    MPIR_Algo_calculate_pipeline_chunk_info(MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE,
                                            MPL_MAX(extent, type_size), count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    /* Print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Reduce shmgr pipeline info: segsize=%d count=%d num_chunks=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
                                             MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE, count,
                                             num_chunks, chunk_size_floor, chunk_size_ceil));

    /* Do pipelined release-gather */
    for (i = 0; i < num_chunks; i++) {
        int chunk_count = (i == 0) ? chunk_size_floor : chunk_size_ceil;

        mpi_errno =
            MPIDI_POSIX_mpi_release_gather_release(NULL, 0, MPI_DATATYPE_NULL, root,
                                                   comm_ptr, errflag,
                                                   MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno =
            MPIDI_POSIX_mpi_release_gather_gather((char *) sendbuf + offset * extent,
                                                  (char *) recvbuf + offset * extent,
                                                  chunk_count, datatype, op, root, comm_ptr,
                                                  errflag,
                                                  MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        offset += chunk_count;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_REDUCE_RELEASE_GATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_COLL_RELEASE_GATHER_H_INCLUDED */

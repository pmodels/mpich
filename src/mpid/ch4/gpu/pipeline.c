/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pipeline.h"

int MPIDI_gpu_pipeline_ctx_create(int num_stages, size_t stage_sz, MPIDI_gpu_pipeline_ctx_t ** ctx)
{
    int mpi_errno = MPI_SUCCESs;

    return mpi_errno;
}

int MPIDI_gpu_pipeline_ctx_free(MPIDI_gpu_pipeline_ctx_t * ctx)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIDI_gpu_pipeline_attach_buf(MPIDI_gpu_pipeline_ctx_t * ctx, void *buf, size_t buf_sz)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIDI_gpu_pipeline_detach_buf(MPIDI_gpu_pipeline_ctx_t * ctx)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIDI_gpu_pipeline_set_dir(MPIDI_gpu_pipeline_ctx_t * ctx, MPIDI_gpu_pipeline_dir_e direction)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIDI_gpu_pipeline_get_next_chunk(MPIDI_gpu_pipeline_ctx_t * ctx, void **chunk_ptr,
                                      size_t * chunk_len, MPIR_Request ** chunk_req)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

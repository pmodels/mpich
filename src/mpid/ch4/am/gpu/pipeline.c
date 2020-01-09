/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pipeline.h"

struct MPIDI_gpu_pipeline_ctx {
    /* TODO: define context */
} MPIDI_gpu_pipeline_ctx_t;

/* Define pipeline context pool entry */
typedef struct gpu_pipeline_ctx {
    MPIDI_gpu_pipeline_ctx *ptr;
    struct gpu_pipeline_ctx *next;
};

/* Pipeline context pool for internode communication */
static gpu_pipeline_ctx_t *gpu_pipeline_ctx_pool_head;
static gpu_pipeline_ctx_t *gpu_pipeline_ctx_pool_tail;

/* Pipeline context pool for intranode communication */
static gpu_pipeline_ctx_t *gpu_pipeline_ctx_pool_shared_head;
static gpu_pipeline_ctx_t *gpu_pipeline_ctx_pool_shared_tail;

int MPIDI_gpu_pipeline_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDI_gpu_pipeline_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDI_gpu_pipeline_ctx_create(int num_stages, size_t stage_sz, MPIDI_gpu_pipeline_ctx_t ** ctx)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDI_gpu_pipeline_ctx_create_shared(int num_stages,
                                         size_t stage_sz, MPIDI_gpu_pipeline_ctx_t ** ctx)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDI_gpu_pipeline_ctx_destroy(MPIDI_gpu_pipeline_ctx_t * ctx)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

MPIDI_gpu_pipeline_ctx_t *MPIDI_gpu_pipeline_attach_shared_ctx(char *serialized_handle, size_t size)
{
    MPIDI_gpu_pipeline_ctx_t *ctx = NULL;
    return ctx;
}

int MPIDI_gpu_pipeline_detach_shared_ctx(MPIDI_gpu_pipeline_ctx_t * ctx)
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

int MPIDI_gpu_pipeline_get_shared_ctx_info(MPIDI_gpu_pipeline_ctx_t * ctx,
                                           char **serialized_handle, size_t * size)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}


int MPIDI_gpu_pipeline_get_next_chunk(MPIDI_gpu_pipeline_ctx_t * ctx,
                                      void **chunk_ptr,
                                      size_t * chunk_len, MPIR_Request ** chunk_req)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

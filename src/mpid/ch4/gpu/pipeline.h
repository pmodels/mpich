/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDI_GPU_PIPELINE_H_INCLUDED
#define MPIDI_GPU_PIPELINE_H_INCLUDED

/* Opaque pipeline context object */
typedef struct MPIDI_gpu_pipeline_ctx MPIDI_gpu_pipeline_ctx_t;

/* Pipeline transfer direction */
typedef enum {
    MPIDI_GPU_PIPELINE_DIR__H2D,
    MPIDI_GPU_PIPELINE_DIR__D2H
} MPIDI_gpu_pipeline_dir_e;

/* MPIDI_gpu_pipeline_ctx_create - create a new gpu pipeline context
 *
 * Description:
 * Pipeline contexts are used when transferring GPU data across nodes over the network fabric. A pipeline
 * context is created by allocating a staging memory area in the host and registering such memory with the
 * GPU so that direct DMA transfers can be performed by both the GPU and the NIC without additional memory
 * copies. The size of the staging area is defined through the interface by the number of stages and the
 * size of each stage. Because memory registration is expensive before actually creating a new context the
 * create function first checks a context pool to see whether a previously allocated context is free to be
 * reused, if this is not the case a new context is allocated and returned.
 *
 * Arguments:
 * - num_stages :  number of stage buffers in the pipeline
 * - stage_sz   :  size of each stage buffer (chunk) in the pipeline
 * - ctx        :  pointer to context
 *
 * Return MPI_SUCCESS if a new context is successfully created, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_ctx_create(int num_stages, size_t stage_sz, MPIDI_gpu_pipeline_ctx_t ** ctx);

/* MPIDI_gpu_pipeline_ctx_free - free an existing pipeline context
 *
 * Description:
 * As described for the context create interface, pipeline contexts are not actually destroyed. Instead
 * the free function returns the context to a pool from where they can be picked by following calls to
 * create. This allows to amortize the cost of memory registration.
 *
 * Arguments:
 * - ctx :  context to be freed
 *
 * Return MPI_SUCCESS if a context is successfully freed, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_ctx_free(MPIDI_gpu_pipeline_ctx_t * ctx);

/* MPIDI_gpu_pipeline_attach_buf - attach GPU buffer to the pipeline
 *
 * Description:
 * When the pipeline context is created it has no information on the origin of the data in GPU
 * memory. A GPU buffer has to be attached to the other end of the pipeline in order to perform
 * the chunking.
 *
 * Arguments:
 * - ctx    :  pipeline context to which the GPU buffer should be attached to
 * - buf    :  pointer to origin buffer in GPU memory
 * - buf_sz :  size of the origin buffer in GPU memory
 *
 * Return MPI_SUCCESS if the GPU buffer is successfully attached to the context, MPI_ERR_OTHER otherwise
 */
int MPIDI_gpu_pipeline_attach_buf(MPIDI_gpu_pipeline_ctx_t * ctx, void *buf, size_t buf_sz);

/* MPIDI_gpu_pipeline_detach_buf - detach buffer from pipeline before freeing it
 *
 * Description:
 * Before the pipeline can be returned to the context pool the origin GPU buffer that was attached
 * to it has to be detached. Alternatively, the detach will be implicitly done by the free function
 * described above. The reason for having an explicit detach is that a buffer can be detached and a
 * new one attached without returning the context to the pool.
 *
 * Arguments:
 * - ctx :  context from which the GPU buffer should be detached from
 *
 * Return MPI_SUCCESS if the GPU buffer is successfully detached from the context, MPI_ERR_OTHER otherwise
 */
int MPIDI_gpu_pipeline_detach_buf(MPIDI_gpu_pipeline_ctx_t * ctx);

/* MPIDI_gpu_pipeline_set_dir - set data transfer direction for the pipeline
 *
 * Description:
 * The direction of the data transfer has to be set as context attribute before this can be used.
 *
 * Arguments:
 * - ctx       :  context for which the data transfer direction is being set
 * - direction :  direction of the data transfer (either host-to-device or device-to-host)
 *
 * Return MPI_SUCCESS if the direction is set successfully, MPI_ERR_OTHER otherwise
 */
int MPIDI_gpu_pipeline_set_dir(MPIDI_gpu_pipeline_ctx_t * ctx, MPIDI_gpu_pipeline_dir_e direction);

/* MPIDI_gpu_pipeline_get_next_chunk - start pipeline and get the next available chunk
 *
 * Description:
 * This function kicks off the pipelining by starting a group of asynchronous data transfer between
 * attached GPU buffer and staging area in the host memory. The function blocks until at least one
 * chunk has been transferred. Besides the chunk and its length the function also returns an MPI
 * request object associated to the chunk that can be used to progress communication on it and is
 * also used by the pipeline infrastructure to check whether it is safe to reuse a staging buffer.
 * Similarly, the pipeline infrastructure internally makes sure that appropriate synchronization
 * happens between GPU memory and staging area in the host so that a chunk is returned only when
 * data transfer between GPU and CPU is complete.
 *
 * Arguments:
 * - ctx       :  context from which a new chunk is requested
 * - chunk_ptr :  pointer to chunk in host staging area
 * - chunk_len :  length of data in the chunk
 * - chunk_req :  MPI_Request object handle to be used for send/recv operations of chunk
 *
 * Return MPI_SUCCESS if the next chunk is returned successfully, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_get_next_chunk(MPIDI_gpu_pipeline_ctx_t * ctx, void **chunk_ptr,
                                      size_t * chunk_len, MPIR_Request ** chunk_req);

#endif /* MPIDI_GPU_PIPELINE_H_INCLUDED */

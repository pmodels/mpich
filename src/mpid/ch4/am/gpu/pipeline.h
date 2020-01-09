/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDI_AM_GPU_PIPELINE_H_INCLUDED
#define MPIDI_AM_GPU_PIPELINE_H_INCLUDED

/* Data transfer direction between host and device is only
 * required for internode communication as each of the players
 * (sender and receiver) only does one-directional transfers.
 * For intranode communication this attribute is not needed as
 * the receiver performs the transfers in both directions: first
 * from orig GPU to host shared memory and then from host shared
 * memory to target GPU. */
typedef enum {
    MPIDI_GPU_PIPELINE_DIR__H2D,
    MPIDI_GPU_PIPELINE_DIR__D2H
} MPIDI_gpu_pipeline_dir_e;

/* The GPU pipeline context contains all the information to
 * transfer data between host and device using chunking. It
 * is exported as opaque object and can be manipulated or
 * queried using appropriate APIs, following defined. */
typedef struct gpu_pipeline_ctx MPIDI_gpu_pipeline_ctx_t;

/*
 * The pipeline APIs are divided into three set of interfaces
 * 1. Context Managing Interfaces
 * 2. Context Access Interfaces
 * 3. Context Progress Interfaces
 */

/*
 * Context Managing Interfaces start here. These are used to create/destroy contexts and
 * init/finalize context pools at MPI init time.
 */

/* MPIDI_gpu_pipeline_init - create a predefined number of pipeline context (for both
 *                           intra and internode communication) and add them to the pool
 *
 * Returns MPI_SUCCESS if initialization is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_init(void);

/* MPIDI_gpu_pipeline_finalize - releases all the pipeline contexts in the pools
 *
 * Returns MPI_SUCCESS if finalization is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_finalize(void);

/* MPIDI_gpu_pipeline_ctx_create - creates a new context for internode communication and adds it to
 *                                 the corresponding pool
 *
 * - num_stages : number of stages in the pipeline
 * - stage_sz   : size of each stage (chunk) in staging memory area
 * - ctx        : return opaque context object
 *
 * Returns MPI_SUCCESS if create is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_ctx_create(int num_stages, size_t stage_sz, MPIDI_gpu_pipeline_ctx_t ** ctx);

/* MPIDI_gpu_pipeline_ctx_create_shared - creates a new context for intranode communication and adds it
 *                                        to the corresponding pool
 *
 * - num_stage : number of stages in the pipeline
 * - stage_sz  : size of each stage (chunk) in the staging memory area
 * - ctx       : return opaque context object
 *
 * Returns MPI_SUCCESS if create is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_ctx_create_shared(int num_stages, size_t stage_sz,
                                         MPIDI_gpu_pipeline_ctx_t ** ctx);

/* MPIDI_gpu_pipeline_ctx_destroy - destroys the context `ctx`. This might not actually free the
 *                                  context resources. Instead the context might be returned to
 *                                  the corresponding pool
 *
 * - ctx : context to be destroyed
 *
 * Returns MPI_SUCCESS if destroy is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_ctx_destroy(MPIDI_gpu_pipeline_ctx_t * ctx);

/* MPIDI_gpu_pipeline_attach_shared_ctx - attaches to shared context through the serialized handle
 *                                        exported by sender process using shared memory send
 *
 * - serialized_handle : shared memory object handle for attach
 * - size              : size of the shared memory
 *
 * Returns pointer to attached context if successful, NULL otherwise.
 */
MPIDI_gpu_pipeline_ctx_t *MPIDI_gpu_pipeline_attach_shared_ctx(char *serialized_handle,
                                                               size_t size);

/* MPIDI_gpu_pipeline_detach_shared_ctx - detaches shared context so that sender can destroy it
 *
 * - ctx: shared context to be detached
 *
 * Return MPI_SUCCESS is detach is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_detach_shared_ctx(MPIDI_gpu_pipeline_ctx_t * ctx);


/*
 * Context Access Interfaces start here. These are used to customize context attributes depending
 * on the data transfer requirements.
 */

/* MPIDI_gpu_pipeline_attach_buf - attaches a user gpu buffer to the context for later data transfer
 *
 * - ctx    : context to which attach user buffer
 * - buf    : user gpu buffer
 * - buf_sz : user gpu buffer size
 *
 * Returns MPI_SUCCESS if attach is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_attach_buf(MPIDI_gpu_pipeline_ctx_t * ctx, void *buf, size_t buf_sz);

/* MPIDI_gpu_pipeline_detach_buf - detaches a user gpu buffer from the context so that the context
 *                                 can be safely destroyed
 *
 * - ctx : context from which detach user buffer
 *
 * Return MPI_SUCCESS is detach is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_detach_buf(MPIDI_gpu_pipeline_ctx_t * ctx);

/* MPIDI_gpu_pipeline_set_dir - sets direction for pipelined data transfer
 *
 * - ctx       : context for which data transfer direction is set
 * - direction : direction of the transfer
 *
 * Returns MPI_SUCCESS if set is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_set_dir(MPIDI_gpu_pipeline_ctx_t * ctx, MPIDI_gpu_pipeline_dir_e direction);

/* MPIDI_gpu_pipeline_get_shared_ctx_info - gets shared context info so that the context can be exported to
 *                                          a remote process that can thus attach to it.
 *
 * - ctx               : context for the query
 * - serialized_handle : returned serialized handle for shared memory object
 * - size              : size of the shared memory object
 *
 * Return MPI_SUCCESS if get is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_get_shared_ctx_info(MPIDI_gpu_pipeline_ctx_t * ctx, char **serialized_handle,
                                           size_t * size);


/*
 * Context Progress Interfaces start here. These are used to make progress in the pipelined data transfer
 */

/* MPIDI_gpu_pipeline_get_next_chunk - kicks pipeline progress and returns the next chunk ready for data transfer.
 *
 * - ctx       : context to do progress on
 * - chunk_ptr : pointer to next ready chunk
 * - chunk_len : chunk length
 * - chunk_req : chunk request handle
 *
 * Returns MPI_SUCCESS if get is successful, MPI_ERR_OTHER otherwise.
 */
int MPIDI_gpu_pipeline_get_next_chunk(MPIDI_gpu_pipeline_ctx_t * ctx, void **chunk_ptr,
                                      size_t * chunk_len, MPIR_Request ** chunk_req);

#endif /* End of MPIDI_AM_GPU_PIPELINE_H_INCLUDED */

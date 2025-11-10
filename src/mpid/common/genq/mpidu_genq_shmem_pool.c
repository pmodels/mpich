/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_shmem_pool.h"
#include "mpidu_genqi_shmem_types.h"
#include "mpidu_init_shm.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GENQ_SHMEM_POOL_FREE_QUEUE_SENDER_SIDE
      category    : CH4
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The genq shmem code allocates pools of cells on each process and, when needed, a cell is
        removed from the pool and passed to another process. This can happen by either removing a
        cell from the pool of the sending process or from the pool of the receiving process. This
        CVAR determines which pool to use. If true, the cell will come from the sender-side. If
        false, the cell will com from the receiver-side.

        There are specific advantages of using receiver-side cells when combined with the "avx" fast
        configure option, which allows MPICH to use AVX streaming copy intrintrinsics, when
        available, to avoid polluting the cache of the sender with the data being copied to the
        receiver. Using receiver-side cells does have the trade-off of requiring an MPMC lock for
        the free queue rather than an MPSC lock, which is used for sender-side cells. Initial
        performance analysis shows that using the MPMC lock in this case had no significant
        performance loss.

        By default, the queue will continue to use sender-side queues until the performance impact
        is verified.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int cell_block_alloc(MPIDU_genqi_shmem_pool_s * pool, int rank);
static int cell_block_alloc(MPIDU_genqi_shmem_pool_s * pool, int rank)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s **new_cell_headers = NULL;

    new_cell_headers =
        (MPIDU_genqi_shmem_cell_header_s **) MPL_malloc(pool->cells_per_free_queue
                                                        * pool->num_free_queue
                                                        * sizeof(MPIDU_genqi_shmem_cell_header_s *),
                                                        MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!new_cell_headers, rc, MPI_ERR_OTHER, "**nomem");
    pool->cell_headers = new_cell_headers;

    /* init cell headers */
    int cells_per_fq = pool->cells_per_free_queue;
    int hdr_idx = 0;
    for (int queue_id = 0; queue_id < pool->num_free_queue; queue_id++) {
        int fq_idx = rank * pool->num_free_queue + queue_id;
        char *fq_cell_base =
            (char *) pool->cell_header_base + fq_idx * pool->queue_cells_alloc_size;

        for (int i = 0; i < cells_per_fq; i++) {
            MPIDU_genqi_shmem_cell_header_s *cell_h = (MPIDU_genqi_shmem_cell_header_s *)
                (fq_cell_base + i * pool->cell_alloc_size);
            /* Have the "host" process for each cell zero-out the cell contents to force the first-touch
             * policy to make the pages resident to that process. */
            memset(cell_h, 0, pool->cell_alloc_size);
            /* The handle value is the one being stored in the next, prev, head, tail pointers.
             * All valid handle must to be non-zero, a zero handle is equivalent to a NULL pointer. */
            cell_h->handle = (uintptr_t) cell_h - (uintptr_t) pool->cell_header_base + 1;
            cell_h->block_idx = fq_idx;
            pool->cell_headers[hdr_idx] = cell_h;

            rc = MPIDU_genq_shmem_queue_enqueue(pool, &pool->free_queues[fq_idx],
                                                HEADER_TO_CELL(cell_h));
            MPIR_ERR_CHECK(rc);
            hdr_idx++;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    MPL_free(new_cell_headers);
    goto fn_exit;
}

typedef struct {
    int cell_alloc_size;
    int queue_cells_alloc_size;
    int total_cells_size;
    int free_queue_size;
} alloc_size_t;

static alloc_size_t calculate_alloc_size(int cell_size, int cells_per_free_queue, int num_proc,
                                         int num_free_queue)
{
    alloc_size_t sizes = { 0 };
    /* Round up header+cell size to cache line boundary to prevent false sharing. A header
     * maybe share the same cache line with the cell preceding it. */
    sizes.cell_alloc_size = MPL_ROUND_UP_ALIGN(sizeof(MPIDU_genqi_shmem_cell_header_s) + cell_size,
                                               MPL_CACHELINE_SIZE);
    /* total size of cells in a free queue need to round up to page size to make
     * sure queue's NUMA affinity set correctly with first-touch policy */
    sizes.queue_cells_alloc_size = MPL_ROUND_UP_ALIGN(sizes.cell_alloc_size * cells_per_free_queue,
                                                      sysconf(_SC_PAGESIZE));
    sizes.total_cells_size = num_proc * num_free_queue * sizes.queue_cells_alloc_size;
    sizes.free_queue_size = num_proc * num_free_queue * sizeof(MPIDU_genq_shmem_queue_u);
    return sizes;
}

int MPIDU_genq_shmem_pool_size(int cell_size, int cells_per_free_queue,
                               int num_proc, int num_free_queue)
{
    alloc_size_t sizes = calculate_alloc_size(cell_size, cells_per_free_queue, num_proc,
                                              num_free_queue);
    return sizes.total_cells_size + sizes.free_queue_size;
}

int MPIDU_genq_shmem_pool_create(void *slab, int slab_size,
                                 int cell_size, int cells_per_free_queue,
                                 int num_proc, int rank, int num_free_queue,
                                 int *queue_types, MPIDU_genq_shmem_pool_t * pool)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj;

    MPIR_FUNC_ENTER;

    pool_obj = MPL_malloc(sizeof(MPIDU_genqi_shmem_pool_s), MPL_MEM_OTHER);

    pool_obj->cell_size = cell_size;
    /* cells_per_proc */
    pool_obj->cells_per_free_queue = cells_per_free_queue;
    pool_obj->num_proc = num_proc;
    pool_obj->num_free_queue = num_free_queue;

    alloc_size_t sizes = calculate_alloc_size(cell_size, cells_per_free_queue, num_proc,
                                              num_free_queue);
    pool_obj->cell_alloc_size = sizes.cell_alloc_size;
    pool_obj->queue_cells_alloc_size = sizes.queue_cells_alloc_size;

    pool_obj->rank = rank;
    pool_obj->gpu_registered = false;
    pool_obj->slab = slab;
    pool_obj->slab_size = slab_size;

    MPIR_Assertp(slab_size >= sizes.total_cells_size + sizes.free_queue_size);

    pool_obj->cell_header_base = (MPIDU_genqi_shmem_cell_header_s *) pool_obj->slab;
    /* the global_block_index is at the end of the slab to avoid extra need of alignment */
    pool_obj->free_queues =
        (MPIDU_genq_shmem_queue_u *) ((char *) pool_obj->slab + sizes.total_cells_size);

    for (int i = 0; i < num_free_queue; i++) {
        int idx = rank * num_free_queue + i;
        rc = MPIDU_genq_shmem_queue_init(&pool_obj->free_queues[idx], queue_types[i]);
        MPIR_ERR_CHECK(rc);
    }

    rc = cell_block_alloc(pool_obj, rank);
    MPIR_ERR_CHECK(rc);

    *pool = (MPIDU_genq_shmem_pool_t) pool_obj;

  fn_exit:
    MPIR_FUNC_EXIT;
    return rc;
  fn_fail:
    MPL_free(pool_obj);
    goto fn_exit;
}

int MPIDU_genq_shmem_pool_destroy(MPIDU_genq_shmem_pool_t pool)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    MPIR_FUNC_ENTER;

    MPL_free(pool_obj->cell_headers);

    if (pool_obj->gpu_registered) {
        MPIR_gpu_unregister_host(pool_obj->slab);
    }

    /* free self */
    MPL_free(pool_obj);

    MPIR_FUNC_EXIT;
    return rc;
}

int MPIDU_genqi_shmem_pool_register(MPIDU_genqi_shmem_pool_s * pool_obj)
{
    int rc = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    rc = MPIR_gpu_register_host(pool_obj->slab, pool_obj->slab_size);
    MPIR_ERR_CHECK(rc);
    pool_obj->gpu_registered = true;

  fn_fail:
    MPIR_FUNC_EXIT;
    return rc;
}

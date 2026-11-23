/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the size (in bytes) of a chunk buffer in the pool of the circ_graph
        algorithms, 0 for no pool. A chunk that needs packing (a noncontiguous datatype or a
        device buffer) is at most this size; without a pool such a message is not chunked.

    - name        : MPIR_CVAR_BCAST_CIRC_GRAPH_CHUNK_SIZE
      alt-env     : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the chunk size (in bytes) for the pipeline of the circ_graph Bcast
        algorithm, 0 for no chunking.

    - name        : MPIR_CVAR_BCAST_CIRC_GRAPH_Q_LEN
      alt-env     : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph Bcast
        algorithm (at least 2).

    - name        : MPIR_CVAR_IBCAST_CIRC_GRAPH_CHUNK_SIZE
      alt-env     : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the chunk size (in bytes) for the pipeline of the circ_graph Ibcast
        algorithm, 0 for no chunking.

    - name        : MPIR_CVAR_IBCAST_CIRC_GRAPH_Q_LEN
      alt-env     : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph Ibcast
        algorithm (at least 2).

    - name        : MPIR_CVAR_ALLGATHER_CIRC_GRAPH_CHUNK_SIZE
      alt-env     : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the chunk size (in bytes) for the pipeline of the circ_graph Allgather
        algorithm, 0 for no chunking.

    - name        : MPIR_CVAR_ALLGATHER_CIRC_GRAPH_Q_LEN
      alt-env     : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph Allgather
        algorithm (at least 2).

    - name        : MPIR_CVAR_REDUCE_CIRC_GRAPH_CHUNK_SIZE
      alt-env     : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the chunk size (in bytes) for the pipeline of the circ_graph Reduce
        algorithm, 0 for no chunking.

    - name        : MPIR_CVAR_REDUCE_CIRC_GRAPH_Q_LEN
      alt-env     : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph Reduce
        algorithm (at least 2).

    - name        : MPIR_CVAR_ALLREDUCE_CIRC_GRAPH_CHUNK_SIZE
      alt-env     : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the chunk size (in bytes) for the pipeline of the circ_graph Allreduce
        algorithm, 0 for no chunking.

    - name        : MPIR_CVAR_ALLREDUCE_CIRC_GRAPH_Q_LEN
      alt-env     : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph Allreduce
        algorithm (at least 2).

    - name        : MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS
      category    : COLLECTIVE
      type        : int
      default     : 32
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of chunk buffers for pipeline data transfer.

    - name        : MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS
      category    : COLLECTIVE
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the max number of chunk buffers to be reserved for pipeline data transfer.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "circ_graph.h"

MPIDU_genq_private_pool_t MPIR_cga_chunk_pool;
/* Cell size of MPIR_cga_chunk_pool, 0 if there is no pool. The queues chunk with this size
 * rather than the CVAR, which can change after init, so that every chunk fits in a cell. */
int MPIR_cga_chunk_pool_cell_size;

static void *host_alloc_registered(uintptr_t size);
static void host_free_registered(void *ptr);

int MPIR_cga_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Create chunk buffer pool */
    if (MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE > 0) {
        /* a collective runs outside the per-VCI critical sections */
        mpi_errno = MPIDU_genq_private_pool_create_locked(MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE,
                                                          MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS,
                                                          MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS,
                                                          host_alloc_registered,
                                                          host_free_registered,
                                                          &MPIR_cga_chunk_pool);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_cga_chunk_pool_cell_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_cga_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_cga_chunk_pool_cell_size > 0) {
        MPIDU_genq_private_pool_destroy(MPIR_cga_chunk_pool);
        MPIR_cga_chunk_pool_cell_size = 0;
    }

    return mpi_errno;
}

/* ---- internal routines ---- */

static void *host_alloc_registered(uintptr_t size)
{
    void *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(ptr);
    MPIR_gpu_register_host(ptr, size);
    return ptr;
}

static void host_free_registered(void *ptr)
{
    MPIR_gpu_unregister_host(ptr);
    MPL_free(ptr);
}

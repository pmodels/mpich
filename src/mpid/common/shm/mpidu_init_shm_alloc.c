/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "mpl_shm.h"
#include "mpidu_init_shm.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#if defined (HAVE_SYSV_SHARED_MEM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

extern int MPIDU_Init_shm_local_size;
extern int MPIDU_Init_shm_local_rank;

struct memory_seg {
    size_t segment_len;
    MPL_shm_hnd_t hnd;
    char *base_addr;
    bool symmetrical;
    bool is_shm;
};

typedef struct memory_list {
    void *ptr;
    struct memory_seg *memory;
    struct memory_list *next;
} memory_list_t;

static memory_list_t *memory_head = NULL;
static memory_list_t *memory_tail = NULL;

static int check_alloc(struct memory_seg *memory);

/* MPIDU_Init_shm_alloc(len, ptr_p)

   This function allocates a shared memory segment
 */
int MPIDU_Init_shm_alloc(size_t len, void **ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    void *current_addr;
    size_t segment_len = len;
    struct memory_seg *memory = NULL;
    memory_list_t *memory_node = NULL;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;

    MPIR_Assert(segment_len > 0);

    if (MPIDU_Init_shm_local_size == 1) {
        *ptr = MPL_aligned_alloc(MPL_CACHELINE_SIZE, len, MPL_MEM_SHM);
        MPIR_ERR_CHKANDJUMP(!*ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");
        goto fn_exit;
    }

    MPIR_CHKPMEM_MALLOC(memory, sizeof(*memory), MPL_MEM_SHM);

    mpl_err = MPL_shm_hnd_init(&(memory->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    memory->segment_len = segment_len;

    char *serialized_hnd = NULL;
    int serialized_hnd_size = 0;

    {
        if (MPIDU_Init_shm_local_rank == 0) {
            /* root prepare shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len,
                                                    (void **) &(memory->base_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            MPIR_Assert(MPIR_Process.node_local_map[0] == MPIR_Process.rank);

            mpl_err = MPL_shm_hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
            serialized_hnd_size = strlen(serialized_hnd) + 1;   /* add 1 for null char */

            MPIDU_Init_shm_put(serialized_hnd, serialized_hnd_size);
            MPIDU_Init_shm_barrier();
        } else {
            MPIDU_Init_shm_barrier();
            MPIDU_Init_shm_query(0, (void **) &serialized_hnd);

            mpl_err = MPL_shm_hnd_deserialize(memory->hnd, serialized_hnd, strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_seg_attach(memory->hnd, memory->segment_len,
                                         (void **) &memory->base_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");
        }

        MPIDU_Init_shm_barrier();

        if (MPIDU_Init_shm_local_rank == 0) {
            /* memory->hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(memory->hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        current_addr = memory->base_addr;
        memory->symmetrical = false;
        memory->is_shm = true;

        mpi_errno = check_alloc(memory);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* assign sections of the shared memory segment to their pointers */

    *ptr = current_addr;

    MPIR_CHKPMEM_MALLOC(memory_node, sizeof(*memory_node), MPL_MEM_SHM);
    memory_node->ptr = *ptr;
    memory_node->memory = memory;
    LL_APPEND(memory_head, memory_tail, memory_node);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (MPIDU_Init_shm_local_size > 1) {
        MPL_shm_seg_remove(memory->hnd);
        MPL_shm_hnd_finalize(&(memory->hnd));
    }
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIDU_Init_shm_comm_alloc(MPIR_Comm * comm, size_t len, void **ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    void *current_addr;
    size_t segment_len = len;
    struct memory_seg *memory = NULL;
    memory_list_t *memory_node = NULL;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_size == 1) {
        *ptr = MPL_aligned_alloc(MPL_CACHELINE_SIZE, len, MPL_MEM_SHM);
        MPIR_ERR_CHKANDJUMP(!*ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");
        goto fn_exit;
    }

    MPIR_Comm *node_comm = comm->node_comm;
    bool is_root;
    if (node_comm) {
        is_root = (node_comm->rank == 0);
    } else {
        is_root = true;
    }

    MPIR_Assert(segment_len > 0);
    MPIR_CHKPMEM_MALLOC(memory, sizeof(*memory), MPL_MEM_OTHER);
    mpl_err = MPL_shm_hnd_init(&(memory->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    memory->segment_len = segment_len;

    char *serialized_hnd = NULL;
    int serialized_hnd_size = 0;
    char serialized_hnd_buffer[MPIDU_INIT_SHM_BLOCK_SIZE];
    bool need_attach;
    bool need_remove;
    if (is_root) {
        if (!MPIDU_Init_shm_atomic_key_exist()) {
            /* We need to create the shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len,
                                                    (void **) &(memory->base_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
            serialized_hnd_size = strlen(serialized_hnd) + 1;   /* add 1 for null char */

            MPIDU_Init_shm_atomic_put(serialized_hnd, serialized_hnd_size);
            need_attach = false;
            need_remove = true;
        } else {
            /* Just retrieve the existing serialized handle */
            MPIDU_Init_shm_atomic_get(serialized_hnd_buffer, MPIDU_INIT_SHM_BLOCK_SIZE);
            serialized_hnd = serialized_hnd_buffer;
            serialized_hnd_size = strlen(serialized_hnd) + 1;   /* add 1 for null char */
            need_attach = true;
            need_remove = false;
        }
        MPIR_Assert(serialized_hnd_size <= MPIDU_INIT_SHM_BLOCK_SIZE);
        if (node_comm) {
            mpi_errno = MPIR_Bcast_impl(serialized_hnd, MPIDU_INIT_SHM_BLOCK_SIZE,
                                        MPIR_BYTE_INTERNAL, 0, node_comm, MPIR_ERR_NONE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        mpi_errno = MPIR_Bcast_impl(serialized_hnd_buffer, MPIDU_INIT_SHM_BLOCK_SIZE,
                                    MPIR_BYTE_INTERNAL, 0, node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        serialized_hnd = serialized_hnd_buffer;
        serialized_hnd_size = strlen(serialized_hnd) + 1;       /* add 1 for null char */
        need_attach = true;
        need_remove = false;
    }
    if (need_attach) {
        mpl_err = MPL_shm_hnd_deserialize(memory->hnd, serialized_hnd, strlen(serialized_hnd));
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        mpl_err = MPL_shm_seg_attach(memory->hnd, memory->segment_len,
                                     (void **) &memory->base_addr, 0);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");
    }

    if (node_comm) {
        mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }
    if (need_remove) {
        /* memory->hnd no longer needed */
        mpl_err = MPL_shm_seg_remove(memory->hnd);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
    }

    current_addr = memory->base_addr;
    memory->symmetrical = false;
    memory->is_shm = true;

    mpi_errno = check_alloc(memory);
    MPIR_ERR_CHECK(mpi_errno);

    /* assign sections of the shared memory segment to their pointers */
    *ptr = current_addr;

    MPIR_CHKPMEM_MALLOC(memory_node, sizeof(*memory_node), MPL_MEM_OTHER);
    memory_node->ptr = *ptr;
    memory_node->memory = memory;
    LL_APPEND(memory_head, memory_tail, memory_node);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPL_shm_seg_remove(memory->hnd);
    MPL_shm_hnd_finalize(&(memory->hnd));
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* MPIDU_SHM_Seg_free() free the shared memory segment */
int MPIDU_Init_shm_free(void *ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    struct memory_seg *memory = NULL;
    memory_list_t *el = NULL;

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_size == 1) {
        MPL_free(ptr);
        goto fn_exit;
    }

    /* retrieve memory handle for baseaddr */
    LL_FOREACH(memory_head, el) {
        if (el->ptr == ptr) {
            memory = el->memory;
            LL_DELETE(memory_head, memory_tail, el);
            MPL_free(el);
            break;
        }
    }

    MPIR_Assert(memory != NULL);

    mpl_err = MPL_shm_seg_detach(memory->hnd, (void **) &(memory->base_addr), memory->segment_len);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

  fn_exit:
    if (MPIDU_Init_shm_local_size > 1) {
        MPL_shm_hnd_finalize(&(memory->hnd));
        MPL_free(memory);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_is_symm(void *ptr)
{
    int ret = -1;
    memory_list_t *el;

    if (MPIDU_Init_shm_local_size == 1) {
        return 1;
    }

    /* retrieve memory handle for baseaddr */
    LL_FOREACH(memory_head, el) {
        if (el->ptr == ptr) {
            ret = (el->memory->symmetrical) ? 1 : 0;
            break;
        }
    }

    return ret;
}

/* check_alloc() checks to see whether the shared memory segment is
   allocated at the same virtual memory address at each process.
*/
static int check_alloc(struct memory_seg *memory)
{
    int mpi_errno = MPI_SUCCESS;
    int is_sym;
    void *baseaddr;

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_rank == 0) {
        MPIDU_Init_shm_put(memory->base_addr, sizeof(void *));
    }

    MPIDU_Init_shm_barrier();

    MPIDU_Init_shm_get(0, sizeof(void *), &baseaddr);

    if (baseaddr == memory->base_addr) {
        is_sym = 1;
        MPIDU_Init_shm_put(&is_sym, sizeof(int));
    } else {
        is_sym = 0;
        MPIDU_Init_shm_put(&is_sym, sizeof(int));
    }

    MPIDU_Init_shm_barrier();

    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDU_Init_shm_get(i, sizeof(int), &is_sym);
        if (is_sym == 0)
            break;
    }

    if (is_sym) {
        memory->symmetrical = true;
    } else {
        memory->symmetrical = false;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

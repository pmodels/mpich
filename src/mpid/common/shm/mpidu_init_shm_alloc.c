/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "mpl_shm.h"
#include "mpidu_init_shm.h"
#include "mpidu_shm_seg.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#if defined (HAVE_SYSV_SHARED_MEM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if defined(HAVE_MKSTEMP) && defined(NEEDS_MKSTEMP_DECL)
extern int mkstemp(char *t);
#endif

typedef struct memory_list {
    void *ptr;
    MPIDU_shm_seg_t *memory;
    struct memory_list *next;
} memory_list_t;

static memory_list_t *memory_head = NULL;
static memory_list_t *memory_tail = NULL;

static int check_alloc(MPIDU_shm_seg_t * memory);

/* MPIDU_Init_shm_alloc(len, ptr_p)

   This function allocates a shared memory segment
 */
int MPIDU_Init_shm_alloc(size_t len, void **ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    void *current_addr;
    size_t segment_len = len;
    int rank = MPIR_Process.rank;
    int local_rank = MPIR_Process.local_rank;
    int num_local = MPIR_Process.local_size;
    int local_procs_0 = MPIR_Process.node_local_map[0];
    MPIDU_shm_seg_t *memory = NULL;
    memory_list_t *memory_node = NULL;
    MPIR_CHKPMEM_DECL(3);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_ALLOC);

    MPIR_Assert(segment_len > 0);

    MPIR_CHKPMEM_MALLOC(memory, MPIDU_shm_seg_t *, sizeof(*memory), mpi_errno, "memory_handle",
                        MPL_MEM_OTHER);
    MPIR_CHKPMEM_MALLOC(memory_node, memory_list_t *, sizeof(*memory_node), mpi_errno,
                        "memory_node", MPL_MEM_OTHER);

    mpl_err = MPL_shm_hnd_init(&(memory->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    memory->segment_len = segment_len;

    char *serialized_hnd = NULL;
    int serialized_hnd_size = 0;
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1) {
        char *addr;

        MPIR_CHKPMEM_MALLOC(addr, char *, segment_len + MPIDU_SHM_CACHE_LINE_LEN, mpi_errno,
                            "segment", MPL_MEM_SHM);

        memory->base_addr = addr;
        current_addr =
            (char *) (((uintptr_t) addr + (uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1) &
                      (~((uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1)));
        memory->symmetrical = 1;
    } else {
        if (local_rank == 0) {
            /* root prepare shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len,
                                                    (void **) &(memory->base_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            MPIR_Assert(local_procs_0 == rank);

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

        if (local_rank == 0) {
            /* memory->hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(memory->hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0;

        mpi_errno = check_alloc(memory);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* assign sections of the shared memory segment to their pointers */

    *ptr = current_addr;

    memory_node->ptr = *ptr;
    memory_node->memory = memory;
    LL_APPEND(memory_head, memory_tail, memory_node);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_ALLOC);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPL_shm_seg_remove(memory->hnd);
    MPL_shm_hnd_finalize(&(memory->hnd));
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPIDU_SHM_Seg_free() free the shared memory segment */
int MPIDU_Init_shm_free(void *ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    MPIDU_shm_seg_t *memory = NULL;
    memory_list_t *el = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_FREE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_FREE);

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

    if (MPIR_Process.local_size == 1)
        MPL_free(memory->base_addr);
    else {
        mpl_err = MPL_shm_seg_detach(memory->hnd, (void **) &(memory->base_addr),
                                     memory->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    }

  fn_exit:
    MPL_shm_hnd_finalize(&(memory->hnd));
    MPL_free(memory);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_is_symm(void *ptr)
{
    int ret = -1;
    memory_list_t *el;

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
static int check_alloc(MPIDU_shm_seg_t * memory)
{
    int mpi_errno = MPI_SUCCESS;
    int is_sym;
    void *baseaddr;

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CHECK_ALLOC);

    if (MPIR_Process.local_rank == 0) {
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
        memory->symmetrical = 1;
    } else {
        memory->symmetrical = 0;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CHECK_ALLOC);
    return mpi_errno;
}

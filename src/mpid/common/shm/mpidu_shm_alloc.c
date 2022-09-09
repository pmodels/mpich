/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "mpl_shm.h"
#include "mpidu_shm.h"
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

#include <sys/mman.h>

#include <stddef.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_SHM_RANDOM_ADDR_RETRY
      category    : MEMORY
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for generating a random address. A retrying
        involves only local operations.

    - name        : MPIR_CVAR_SHM_SYMHEAP_RETRY
      category    : MEMORY
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for allocating a symmetric heap in shared
        memory. A retrying involves collective communication over the group in
        the shared memory.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

enum {
    SYMSHM_SUCCESS,
    SYMSHM_MAP_FAIL,            /* mapping failure when specified with a fixed start addr */
    SYMSHM_OTHER_FAIL           /* other failure reported by MPL shm */
};

/* Linked list internally used to keep track
 * of allocate shared memory segments */
typedef struct seg_list {
    void *key;
    MPIDU_shm_seg_t *shm_seg;
    struct seg_list *next;
} seg_list_t;

static seg_list_t *seg_list_head = NULL;
static seg_list_t *seg_list_tail = NULL;

size_t MPIDU_shm_get_mapsize(size_t size, size_t * psz)
{
    size_t page_sz, mapsize;

    if (*psz == 0)
        page_sz = (size_t) sysconf(_SC_PAGESIZE);
    else
        page_sz = *psz;

    mapsize = (size + (page_sz - 1)) & (~(page_sz - 1));
    *psz = page_sz;

    return mapsize;
}

#ifdef USE_SYM_HEAP
static int check_maprange_ok(void *start, size_t size)
{
    int rc = 0;
    int ret = 0;
    size_t page_sz = 0;
    size_t mapsize = MPIDU_shm_get_mapsize(size, &page_sz);
    size_t i, num_pages = mapsize / page_sz;
    char *ptr = (char *) start;

    for (i = 0; i < num_pages; i++) {
        rc = msync(ptr, page_sz, 0);

        if (rc == -1) {
            if (errno != ENOMEM)
                goto fn_fail;
            ptr += page_sz;
        } else
            goto fn_exit;
    }

  fn_fail:
    ret = 1;
  fn_exit:
    return ret;
}

static void *generate_random_addr(size_t size)
{
    /* starting position for pointer to map
     * This is not generic, probably only works properly on Linux
     * but it's not fatal since we bail after a fixed number of iterations
     */
#define MAP_POINTER ((random_unsigned&((0x00006FFFFFFFFFFF&(~(page_sz-1)))|0x0000600000000000)))
    uintptr_t map_pointer;
    char random_state[256];
    size_t page_sz = 0;
    uint64_t random_unsigned;
    size_t mapsize = MPIDU_shm_get_mapsize(size, &page_sz);
    MPL_time_t ts;
    unsigned int ts_32 = 0;
    int iter = MPIR_CVAR_SHM_RANDOM_ADDR_RETRY;
    int32_t rh, rl;
    struct random_data rbuf;

    /* rbuf must be zero-cleared otherwise it results in SIGSEGV in glibc
     * (http://stackoverflow.com/questions/4167034/c-initstate-r-crashing) */
    memset(&rbuf, 0, sizeof(rbuf));

    MPL_wtime(&ts);
    MPL_wtime_touint(&ts, &ts_32);

    initstate_r(ts_32, random_state, sizeof(random_state), &rbuf);
    random_r(&rbuf, &rh);
    random_r(&rbuf, &rl);
    random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
    map_pointer = MAP_POINTER;

    while (check_maprange_ok((void *) map_pointer, mapsize) == 0) {
        random_r(&rbuf, &rh);
        random_r(&rbuf, &rl);
        random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
        map_pointer = MAP_POINTER;
        iter--;

        if (iter == 0) {
            map_pointer = UINTPTR_MAX;
            goto fn_exit;
        }
    }

  fn_exit:
    return (void *) map_pointer;
}

typedef struct {
    unsigned long long sz;
    int loc;
} ull_maxloc_t;

/* Compute maxloc for unsigned long long type.
 * If more than one max value exists, the loc with lower rank is returned. */
static void ull_maxloc_op_func(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    ull_maxloc_t *inmaxloc = (ull_maxloc_t *) invec;
    ull_maxloc_t *outmaxloc = (ull_maxloc_t *) inoutvec;
    int i;
    for (i = 0; i < *len; i++) {
        if (inmaxloc->sz > outmaxloc->sz) {
            outmaxloc->sz = inmaxloc->sz;
            outmaxloc->loc = inmaxloc->loc;
        } else if (inmaxloc->sz == outmaxloc->sz && inmaxloc->loc < outmaxloc->loc)
            outmaxloc->loc = inmaxloc->loc;
    }
}

/* Allreduce MAXLOC for unsigned size type by using user defined operator
 * and derived datatype. We have to customize it because standard MAXLOC
 * supports only pairtypes with signed {short, int, long}. We internally
 * cast size_t to unsigned long long which is large enough to hold size type
 * and matches an MPI basic datatype. */
static int allreduce_maxloc(size_t mysz, int myloc, MPIR_Comm * comm, size_t * maxsz,
                            int *maxsz_loc)
{
    int mpi_errno = MPI_SUCCESS;
    int blocks[2] = { 1, 1 };
    MPI_Aint disps[2];
    MPI_Datatype types[2], maxloc_type = MPI_DATATYPE_NULL;
    ull_maxloc_t maxloc, maxloc_result;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_Op *maxloc_op = NULL;

    types[0] = MPI_UNSIGNED_LONG_LONG;
    types[1] = MPI_INT;
    disps[0] = 0;
    disps[1] = (MPI_Aint) ((uintptr_t) & maxloc.loc - (uintptr_t) & maxloc.sz);

    mpi_errno = MPIR_Type_create_struct_impl(2, blocks, disps, types, &maxloc_type);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Type_commit_impl(&maxloc_type);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Op_create_impl(ull_maxloc_op_func, 0, &maxloc_op);
    MPIR_ERR_CHECK(mpi_errno);

    maxloc.sz = (unsigned long long) mysz;
    maxloc.loc = myloc;

    mpi_errno =
        MPIR_Allreduce(&maxloc, &maxloc_result, 1, maxloc_type, maxloc_op->handle, comm, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    *maxsz_loc = maxloc_result.loc;
    *maxsz = (size_t) maxloc_result.sz;

  fn_exit:
    if (maxloc_type != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&maxloc_type);
    if (maxloc_op)
        MPIR_Op_free_impl(maxloc_op);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Collectively allocate symmetric shared memory region with address
 * defined by base_ptr. MPL_shm routines and MPI collectives are internally used.
 *
 * The caller should ensure segment_len is page aligned and base_addr
 * is a symmetric non-NULL address on all processes.
 *
 * map_result indicates the mapping result of the node. It can be
 * SYMSHM_SUCCESS, SYMSHM_MAP_FAIL or SYMSHM_OTHER_FAIL.
 * If it is SYMSHM_MAP_FAIL, the caller can try it again with a different
 * start address; if it is SYMSHM_OTHER_FAIL, it usually means no more shm
 * segment can be allocated, thus the caller should stop retrying. */
static int map_symm_shm(MPIR_Comm * shm_comm_ptr, MPIDU_shm_seg_t * shm_seg, int *map_result_ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;
    int all_map_result = SYMSHM_MAP_FAIL;
    bool mapped_flag = false;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    if (shm_seg->segment_len > 0) {
        if (shm_comm_ptr) {
            if (shm_comm_ptr->rank == 0) {
                char *serialized_hnd = NULL;

                /* try to map with specified symmetric address. */
                mpl_err = MPL_shm_fixed_seg_create_and_attach(shm_seg->hnd, shm_seg->segment_len,
                                                              (void **) &(shm_seg->base_addr), 0);
                if (mpl_err) {
                    *map_result_ptr =
                        (mpl_err == MPL_ERR_SHM_INVAL) ? SYMSHM_MAP_FAIL : SYMSHM_OTHER_FAIL;
                    goto root_sync;
                } else
                    mapped_flag = true;

                mpl_err = MPL_shm_hnd_get_serialized_by_ref(shm_seg->hnd, &serialized_hnd);
                if (mpl_err) {
                    *map_result_ptr = SYMSHM_OTHER_FAIL;
                }

              root_sync:
                /* broadcast the mapping result on rank 0 */
                mpi_errno = MPIR_Bcast(map_result_ptr, 1, MPI_INT, 0, shm_comm_ptr, &errflag);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

                if (*map_result_ptr != SYMSHM_SUCCESS)
                    goto map_fail;

                mpi_errno =
                    MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPI_BYTE, 0,
                               shm_comm_ptr, &errflag);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

            } else {
                char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

                /* receive the mapping result of rank 0 */
                mpi_errno = MPIR_Bcast(map_result_ptr, 1, MPI_INT, 0, shm_comm_ptr, &errflag);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

                if (*map_result_ptr != SYMSHM_SUCCESS)
                    goto map_fail;

                /* if rank 0 mapped successfully, others on the node attach shared memory region */

                /* get serialized handle from rank 0 and deserialize it */
                mpi_errno =
                    MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPI_BYTE, 0,
                               shm_comm_ptr, &errflag);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

                mpl_err =
                    MPL_shm_hnd_deserialize(shm_seg->hnd, serialized_hnd, strlen(serialized_hnd));
                if (mpl_err) {
                    *map_result_ptr = SYMSHM_OTHER_FAIL;
                    goto result_sync;
                }

                /* try to attach with specified symmetric address */
                mpl_err = MPL_shm_fixed_seg_attach(shm_seg->hnd, shm_seg->segment_len,
                                                   (void **) &(shm_seg->base_addr), 0);
                if (mpl_err) {
                    *map_result_ptr =
                        (mpl_err == MPL_ERR_SHM_INVAL) ? SYMSHM_MAP_FAIL : SYMSHM_OTHER_FAIL;
                } else
                    mapped_flag = true;
            }

          result_sync:
            /* check results of all processes. If any failure happens (max result > 0),
             * return SYMSHM_OTHER_FAIL if anyone reports it (max result == 2).
             * Otherwise return SYMSHM_MAP_FAIL (max result == 1). */
            mpi_errno = MPIR_Allreduce(map_result_ptr, &all_map_result, 1, MPI_INT,
                                       MPI_MAX, shm_comm_ptr, &errflag);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

            if (all_map_result != SYMSHM_SUCCESS)
                goto map_fail;

            if (shm_comm_ptr->rank == 0) {
                /* unlink shared memory region so it gets deleted when all processes exit */
                mpl_err = MPL_shm_seg_remove(shm_seg->hnd);
                MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
            }
        } else {
            /* if there is only one process on this processor, don't use shared memory */
            int rc = check_maprange_ok(shm_seg->base_addr, shm_seg->segment_len);
            if (rc) {
                shm_seg->base_addr = MPL_mmap(shm_seg->base_addr, shm_seg->segment_len,
                                              PROT_READ | PROT_WRITE,
                                              MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0,
                                              MPL_MEM_SHM);
            } else {
                shm_seg->base_addr = (void *) MAP_FAILED;
            }
            *map_result_ptr = (shm_seg->base_addr == (void *) MAP_FAILED) ?
                SYMSHM_MAP_FAIL : SYMSHM_SUCCESS;
        }
    }

  fn_exit:
    return mpi_errno;
  map_fail:
    if (mapped_flag) {
        /* destroy successful shm segment */
        mpl_err =
            MPL_shm_seg_detach(shm_seg->hnd, (void **) &(shm_seg->base_addr), shm_seg->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    }
    *map_result_ptr = all_map_result;
    goto fn_exit;
  fn_fail:
    goto fn_exit;
}

static int unmap_symm_shm(MPIR_Comm * shm_comm_ptr, MPIDU_shm_seg_t * shm_seg)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;

    if (shm_comm_ptr != NULL) {
        /* destroy successful shm segment */
        mpl_err =
            MPL_shm_seg_detach(shm_seg->hnd, (void **) &(shm_seg->base_addr), shm_seg->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    } else {
        MPL_munmap(shm_seg->base_addr, shm_seg->segment_len, MPL_MEM_SHM);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Allocate symmetric shared memory across all processes in comm */
static int shm_alloc_symm_all(MPIR_Comm * comm_ptr, size_t offset, MPIDU_shm_seg_t * shm_seg,
                              bool * fail_flag)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    int all_map_result = SYMSHM_MAP_FAIL;
    int iter = MPIR_CVAR_SHM_SYMHEAP_RETRY;
    int maxsz_loc = 0;
    size_t maxsz = 0;
    char *map_pointer = NULL;
    MPIR_Comm *shm_comm_ptr = comm_ptr->node_comm;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* we let the process with larger amount of requested memory to calculate
     * the random address. This should reduce the number of attempts allocating
     * symmetric shared memory as the other processes are more likely to accept
     * the returned pointer when mapping memory into their address space. */
    mpi_errno = allreduce_maxloc(shm_seg->segment_len, rank, comm_ptr, &maxsz, &maxsz_loc);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    if (maxsz == 0)
        goto fn_exit;

    while (all_map_result == SYMSHM_MAP_FAIL && iter-- > 0) {
        int map_result = SYMSHM_SUCCESS;

        /* rank maxsz_loc in comm generates random address */
        if (comm_ptr->rank == maxsz_loc)
            map_pointer = generate_random_addr(shm_seg->segment_len);

        /* broadcast fixed address to the other processes in comm */
        mpi_errno =
            MPIR_Bcast(&map_pointer, sizeof(char *), MPI_CHAR, maxsz_loc, comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* optimization: make sure every process memory in the shared segment is mapped
         * at the same virtual address in the corresponding address space. Example with
         * 4 processes: if after calling MPIDU_shm_alloc_symm_all() process 0 gets addr
         * 0x4000,  process 1 will get 0x3000, process 2 will get 0x2000, and process 3
         * will get 0x1000. This way all processes have their own memory inside the shm
         * segment starting at addr 0x4000. Processes in other nodes will also have the
         * same address. */
        shm_seg->base_addr = map_pointer - offset;

        /* try mapping symmetric memory */
        mpi_errno = map_symm_shm(shm_comm_ptr, shm_seg, &map_result);
        MPIR_ERR_CHECK(mpi_errno);

        /* check if any mapping failure occurs */
        mpi_errno = MPIR_Allreduce(&map_result, &all_map_result, 1, MPI_INT,
                                   MPI_MAX, comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* cleanup local shm segment if mapping failed on other process */
        if (all_map_result != SYMSHM_SUCCESS && map_result == SYMSHM_SUCCESS &&
            shm_seg->segment_len > 0) {
            mpi_errno = unmap_symm_shm(shm_comm_ptr, shm_seg);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    if (all_map_result != SYMSHM_SUCCESS) {
        /* if fail to allocate, return and let the caller choose another method */
        *fail_flag = true;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* end of USE_SYM_HEAP */

static int shm_alloc(MPIR_Comm * shm_comm_ptr, MPIDU_shm_seg_t * shm_seg, bool * fail_flag)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;
    bool shm_fail_flag = false;
    bool any_shm_fail_flag = false;
    bool mapped_flag = false;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    if (shm_comm_ptr->rank == 0) {
        char *serialized_hnd = NULL;
        char mpl_err_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* root prepare shm segment */
        mpl_err = MPL_shm_seg_create_and_attach(shm_seg->hnd, shm_seg->segment_len,
                                                (void **) &(shm_seg->base_addr), 0);
        if (mpl_err != MPL_SUCCESS) {
            shm_fail_flag = true;
            goto hnd_sync;
        } else
            mapped_flag = true;

        mpl_err = MPL_shm_hnd_get_serialized_by_ref(shm_seg->hnd, &serialized_hnd);
        if (mpl_err != MPL_SUCCESS)
            shm_fail_flag = true;

      hnd_sync:
        if (shm_fail_flag)
            serialized_hnd = &mpl_err_hnd[0];

        mpi_errno =
            MPIR_Bcast_impl(serialized_hnd, MPL_SHM_GHND_SZ, MPI_BYTE, 0, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        if (shm_fail_flag)
            goto map_fail;

        /* ensure all other processes have mapped successfully */
        mpi_errno = MPIR_Allreduce_impl(&shm_fail_flag, &any_shm_fail_flag, 1, MPI_C_BOOL,
                                        MPI_LOR, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpl_err = MPL_shm_seg_remove(shm_seg->hnd);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");

        if (any_shm_fail_flag)
            goto map_fail;

    } else {
        char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno = MPIR_Bcast_impl(serialized_hnd, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                                    shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* empty handler means root fails */
        if (strlen(serialized_hnd) == 0)
            goto map_fail;

        mpl_err = MPL_shm_hnd_deserialize(shm_seg->hnd, serialized_hnd, strlen(serialized_hnd));
        if (mpl_err != MPL_SUCCESS) {
            shm_fail_flag = true;
            goto result_sync;
        }

        mpl_err = MPL_shm_seg_attach(shm_seg->hnd, shm_seg->segment_len,
                                     (void **) &shm_seg->base_addr, 0);
        if (mpl_err != MPL_SUCCESS) {
            shm_fail_flag = true;
            goto result_sync;
        } else
            mapped_flag = true;

      result_sync:
        mpi_errno = MPIR_Allreduce_impl(&shm_fail_flag, &any_shm_fail_flag, 1, MPI_C_BOOL,
                                        MPI_LOR, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        if (any_shm_fail_flag)
            goto map_fail;
    }

  fn_exit:
    return mpi_errno;
  map_fail:
    if (mapped_flag) {
        /* destroy successful shm segment */
        mpl_err =
            MPL_shm_seg_detach(shm_seg->hnd, (void **) &(shm_seg->base_addr), shm_seg->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    }
    *fail_flag = true;
    goto fn_exit;
  fn_fail:
    goto fn_exit;
}

int MPIDU_shm_alloc_symm_all(MPIR_Comm * comm_ptr, size_t len, size_t offset, void **ptr,
                             bool * fail_flag)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_SYM_HEAP
    int mpl_err = MPL_SUCCESS;
    MPIDU_shm_seg_t *shm_seg = NULL;
    seg_list_t *el = NULL;
    MPIR_CHKPMEM_DECL(2);

    *ptr = NULL;

    MPIR_CHKPMEM_MALLOC(shm_seg, MPIDU_shm_seg_t *, sizeof(*shm_seg), mpi_errno, "shm_seg_handle",
                        MPL_MEM_OTHER);
    MPIR_CHKPMEM_MALLOC(el, seg_list_t *, sizeof(*el), mpi_errno,
                        "seg_list_element", MPL_MEM_OTHER);

    mpl_err = MPL_shm_hnd_init(&(shm_seg->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    shm_seg->segment_len = len;

    mpi_errno = shm_alloc_symm_all(comm_ptr, offset, shm_seg, fail_flag);
    if (mpi_errno || *fail_flag)
        goto fn_fail;

    if (len == 0) {
        /* process requested no memory, cleanup and return */
        MPL_shm_seg_remove(shm_seg->hnd);
        MPL_shm_hnd_finalize(&(shm_seg->hnd));
        MPIR_CHKPMEM_REAP();
        goto fn_exit;
    }

    *ptr = shm_seg->base_addr;

    /* store shm_seg handle in linked list for later retrieval */
    el->key = *ptr;
    el->shm_seg = shm_seg;
    LL_APPEND(seg_list_head, seg_list_tail, el);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (shm_seg) {
        MPL_shm_seg_remove(shm_seg->hnd);
        MPL_shm_hnd_finalize(&(shm_seg->hnd));
    }
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#else
    /* always fail, return and let the caller choose another method */
    *fail_flag = true;
    return mpi_errno;
#endif /* end of USE_SYM_HEAP */
}

int MPIDU_shm_alloc(MPIR_Comm * shm_comm_ptr, size_t len, void **ptr, bool * fail_flag)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;
    MPIDU_shm_seg_t *shm_seg = NULL;
    seg_list_t *el = NULL;
    MPIR_CHKPMEM_DECL(2);

    *ptr = NULL;

    MPIR_Assert(shm_comm_ptr != NULL);
    MPIR_Assert(len > 0);

    MPIR_CHKPMEM_MALLOC(shm_seg, MPIDU_shm_seg_t *, sizeof(*shm_seg), mpi_errno, "shm_seg_handle",
                        MPL_MEM_OTHER);
    MPIR_CHKPMEM_MALLOC(el, seg_list_t *, sizeof(*el), mpi_errno,
                        "seg_list_element", MPL_MEM_OTHER);

    mpl_err = MPL_shm_hnd_init(&(shm_seg->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    shm_seg->segment_len = len;

    mpi_errno = shm_alloc(shm_comm_ptr, shm_seg, fail_flag);
    if (mpi_errno || *fail_flag)
        goto fn_fail;

    *ptr = shm_seg->base_addr;

    /* store shm_seg handle in linked list for later retrieval */
    el->key = *ptr;
    el->shm_seg = shm_seg;
    LL_APPEND(seg_list_head, seg_list_tail, el);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (shm_seg) {
        MPL_shm_seg_remove(shm_seg->hnd);
        MPL_shm_hnd_finalize(&(shm_seg->hnd));
    }
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIDU_shm_free(void *ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;
    MPIDU_shm_seg_t *shm_seg = NULL;
    seg_list_t *el = NULL;

    /* retrieve memory handle for baseaddr */
    LL_FOREACH(seg_list_head, el) {
        if (el->key == ptr) {
            shm_seg = el->shm_seg;
            LL_DELETE(seg_list_head, seg_list_tail, el);
            MPL_free(el);
            break;
        }
    }

    MPIR_Assert(shm_seg != NULL);

    /* if there is only one process in the node the serialized handle points
     * to NULL as there is no shared file backing up memory. This is used to
     * differentiate between shared memory and private memory allocations
     * when symmetric shared memory is being requested. */
    char *serialized_hnd = NULL;
    MPL_shm_hnd_get_serialized_by_ref(shm_seg->hnd, &serialized_hnd);

    if (serialized_hnd) {
        mpl_err = MPL_shm_seg_detach(shm_seg->hnd, (void **) &(shm_seg->base_addr),
                                     shm_seg->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    } else {
        MPL_munmap(shm_seg->base_addr, shm_seg->segment_len, MPL_MEM_SHM);
    }

  fn_exit:
    MPL_shm_hnd_finalize(&(shm_seg->hnd));
    MPL_free(shm_seg);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

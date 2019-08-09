/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include "mpidimpl.h"
#include "ch4r_symheap.h"

enum {
    MPIDIU_SYMSHM_SUCCESS,
    MPIDIU_SYMSHM_MAP_FAIL,     /* mapping failure when specified with a fixed start addr */
    MPIDIU_SYMSHM_OTHER_FAIL    /* other failure reported by MPL shm */
};

#ifdef USE_SYM_HEAP
static inline int check_maprange_ok(void *start, size_t size);
static void *generate_random_addr(size_t size);
static int allocate_symshm_segment(MPIR_Comm * shm_comm_ptr, MPI_Aint shm_segment_len,
                                   MPL_shm_hnd_t * shm_segment_hdl_ptr, void **base_ptr,
                                   int *map_result_ptr);
static void ull_maxloc_op_func(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype);
static int allreduce_maxloc(size_t mysz, int myloc, MPIR_Comm * comm, size_t * maxsz,
                            int *maxsz_loc);
#endif

/* Returns aligned size and the page size. The page size parameter must
 * be always initialized to 0 or a previously returned value. If page size
 * is set, we can reuse the value and skip sysconf. */
size_t MPIDIU_get_mapsize(size_t size, size_t * psz)
{
    size_t page_sz, mapsize;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_MAPSIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_MAPSIZE);

    if (*psz == 0)
        page_sz = (size_t) sysconf(_SC_PAGESIZE);
    else
        page_sz = *psz;

    mapsize = (size + (page_sz - 1)) & (~(page_sz - 1));
    *psz = page_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_MAPSIZE);
    return mapsize;
}

#ifdef USE_SYM_HEAP
static int check_maprange_ok(void *start, size_t size)
{
    int rc = 0;
    int ret = 0;
    size_t page_sz = 0;
    size_t mapsize = MPIDIU_get_mapsize(size, &page_sz);
    size_t i, num_pages = mapsize / page_sz;
    char *ptr = (char *) start;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_CHECK_MAPRANGE_OK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_CHECK_MAPRANGE_OK);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_CHECK_MAPRANGE_OK);
    return ret;
}

static void *generate_random_addr(size_t size)
{
    /* starting position for pointer to map
     * This is not generic, probably only works properly on Linux
     * but it's not fatal since we bail after a fixed number of iterations
     */
#define MPIDIU_MAP_POINTER ((random_unsigned&((0x00006FFFFFFFFFFF&(~(page_sz-1)))|0x0000600000000000)))
    uintptr_t map_pointer;
    char random_state[256];
    size_t page_sz = 0;
    uint64_t random_unsigned;
    size_t mapsize = MPIDIU_get_mapsize(size, &page_sz);
    MPL_time_t ts;
    unsigned int ts_32 = 0;
    int iter = MPIR_CVAR_CH4_RANDOM_ADDR_RETRY;
    int32_t rh, rl;
    struct random_data rbuf;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);

    /* rbuf must be zero-cleared otherwise it results in SIGSEGV in glibc
     * (http://stackoverflow.com/questions/4167034/c-initstate-r-crashing) */
    memset(&rbuf, 0, sizeof(rbuf));

    MPL_wtime(&ts);
    MPL_wtime_touint(&ts, &ts_32);

    initstate_r(ts_32, random_state, sizeof(random_state), &rbuf);
    random_r(&rbuf, &rh);
    random_r(&rbuf, &rl);
    random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
    map_pointer = MPIDIU_MAP_POINTER;

    while (check_maprange_ok((void *) map_pointer, mapsize) == 0) {
        random_r(&rbuf, &rh);
        random_r(&rbuf, &rl);
        random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
        map_pointer = MPIDIU_MAP_POINTER;
        iter--;

        if (iter == 0) {
            map_pointer = -1ULL;
            goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);
    return (void *) map_pointer;
}

/* Collectively allocate symmetric shared memory region with address
 * defined by base_ptr. MPL_shm routines and MPI collectives are internally used.
 *
 * The caller should ensure shm_segment_len is page aligned and base_ptr
 * is a symmetric non-NULL address on all processes.
 *
 * map_result indicates the mapping result of the node. It can be
 * MPIDIU_SYMSHM_SUCCESS, MPIDIU_SYMSHM_MAP_FAIL or MPIDIU_SYMSHM_OTHER_FAIL.
 * If it is MPIDIU_SYMSHM_MAP_FAIL, the caller can try it again with a different
 * start address; if it is MPIDIU_SYMSHM_OTHER_FAIL, it usually means no more shm
 * segment can be allocated, thus the caller should stop retrying. */
static int allocate_symshm_segment(MPIR_Comm * shm_comm_ptr, MPI_Aint shm_segment_len,
                                   MPL_shm_hnd_t * shm_segment_hdl_ptr, void **base_ptr,
                                   int *map_result_ptr)
{
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SHM_SUCCESS;
    bool mapped_flag = false;
    int all_map_result = MPIDIU_SYMSHM_SUCCESS, map_result = MPIDIU_SYMSHM_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOCATE_SYMSHM_SEGMENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOCATE_SYMSHM_SEGMENT);

    *map_result_ptr = MPIDIU_SYMSHM_SUCCESS;

    mpl_err = MPL_shm_hnd_init(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    /* rank 0 initializes shared memory segment */
    if (shm_comm_ptr->rank == 0) {
        char *serialized_hnd_ptr = NULL;

        /* try to map with specified symmetric address. */
        mpl_err =
            MPL_shm_fixed_seg_create_and_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
        if (mpl_err != MPL_SHM_SUCCESS) {
            all_map_result = map_result = (mpl_err == MPL_SHM_EINVAL) ?
                MPIDIU_SYMSHM_MAP_FAIL : MPIDIU_SYMSHM_OTHER_FAIL;
            goto root_sync;
        } else
            mapped_flag = true;

        mpl_err = MPL_shm_hnd_get_serialized_by_ref(*shm_segment_hdl_ptr, &serialized_hnd_ptr);
        if (mpl_err != MPL_SHM_SUCCESS)
            all_map_result = map_result = MPIDIU_SYMSHM_OTHER_FAIL;

      root_sync:
        /* broadcast the mapping result on rank 0 */
        mpi_errno = MPIR_Bcast(&map_result, 1, MPI_INT, 0, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* broadcast handle to the others if mapping is successful, otherwise stop */
        if (map_result != MPIDIU_SYMSHM_SUCCESS)
            goto map_fail;

        mpi_errno = MPIR_Bcast(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                               shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* ensure all other processes have mapped successfully */
        mpi_errno = MPIR_Allreduce(&map_result, &all_map_result, 1, MPI_INT,
                                   MPI_MAX, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpl_err = MPL_shm_seg_remove(*shm_segment_hdl_ptr);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**remove_shar_mem");

        if (all_map_result != MPIDIU_SYMSHM_SUCCESS)
            goto map_fail;
    } else {
        char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* receive the mapping result of rank 0 */
        mpi_errno = MPIR_Bcast(&map_result, 1, MPI_INT, 0, shm_comm_ptr, &errflag);
        if (map_result != MPIDIU_SYMSHM_SUCCESS) {
            all_map_result = map_result;
            goto map_fail;
        }

        /* if rank 0 mapped successfully, others on the node attach shared memory region */

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno = MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                               shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        mpl_err = MPL_shm_hnd_deserialize(*shm_segment_hdl_ptr, serialized_hnd,
                                          strlen(serialized_hnd));
        if (mpl_err != MPL_SHM_SUCCESS) {
            map_result = MPIDIU_SYMSHM_OTHER_FAIL;
            goto result_sync;
        }

        /* try to attach with specified symmetric address */
        mpl_err = MPL_shm_fixed_seg_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
        if (mpl_err != MPL_SHM_SUCCESS) {
            map_result = (mpl_err == MPL_SHM_EINVAL) ?
                MPIDIU_SYMSHM_MAP_FAIL : MPIDIU_SYMSHM_OTHER_FAIL;
            goto result_sync;
        } else
            mapped_flag = true;

      result_sync:
        /* check results of all processes. If any failure happens (max result > 0),
         * return MPIDIU_SYMSHM_OTHER_FAIL if anyone reports it (max result == 2).
         * Otherwise return MPIDIU_SYMSHM_MAP_FAIL (max result == 1). */
        mpi_errno = MPIR_Allreduce(&map_result, &all_map_result, 1, MPI_INT,
                                   MPI_MAX, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        if (all_map_result != MPIDIU_SYMSHM_SUCCESS)
            goto map_fail;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOCATE_SYMSHM_SEGMENT);
    return mpi_errno;
  map_fail:
    /* clean up shm mapping resource */
    if (mapped_flag) {
        mpl_err = MPL_shm_seg_detach(*shm_segment_hdl_ptr, base_ptr, shm_segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
        *base_ptr = NULL;
    }
    mpl_err = MPL_shm_hnd_finalize(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
    *map_result_ptr = all_map_result;
    goto fn_exit;
  fn_fail:
    goto fn_exit;
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
    MPI_Op maxloc_op = MPI_OP_NULL;
    ull_maxloc_t maxloc, maxloc_result;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLREDUCE_MAXLOC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLREDUCE_MAXLOC);

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

    mpi_errno = MPIR_Allreduce(&maxloc, &maxloc_result, 1, maxloc_type, maxloc_op, comm, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    *maxsz_loc = maxloc_result.loc;
    *maxsz = (size_t) maxloc_result.sz;

  fn_exit:
    if (maxloc_type != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&maxloc_type);
    if (maxloc_op != MPI_OP_NULL)
        MPIR_Op_free_impl(&maxloc_op);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLREDUCE_MAXLOC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* end of USE_SYM_HEAP */

/* Allocate a symmetric heap over a communicator with same starting address
 * on each process and shared memory over processes of each node. The MPL_shm
 * routines are internally used on node with multiple processes, and MPL_mmap
 * is used on node with single process. It tries MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY
 * times before giving up, and returns with fail_flag=1 if mapping fails.
 *
 * Input shm_size is the total mapping size of shared memory on local node, and
 * shm_offsets presents the expected offset of local process's start address in
 * the mapped shared memory. */
int MPIDIU_get_shm_symheap(MPI_Aint shm_size, MPI_Aint * shm_offsets, MPIR_Comm * comm,
                           MPIR_Win * win, bool * fail_flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_SHM_SYMHEAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_SHM_SYMHEAP);

#ifdef USE_SYM_HEAP
    int iter = MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY;
    MPL_shm_hnd_t *shm_segment_hdl_ptr = &MPIDIG_WIN(win, shm_segment_handle);
    void **base_ptr = &MPIDIG_WIN(win, mmap_addr);
    int all_map_result = MPIDIU_SYMSHM_MAP_FAIL;

    size_t mapsize = 0, page_sz = 0, maxsz = 0;
    int maxsz_loc = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *shm_comm_ptr = comm->node_comm;

    *fail_flag = false;
    mapsize = MPIDIU_get_mapsize(shm_size, &page_sz);

    /* figure out leading process in win */
    mpi_errno = allreduce_maxloc(mapsize, comm->rank, comm, &maxsz, &maxsz_loc);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* zero size window, simply return */
    if (maxsz == 0)
        goto fn_exit;

    /* try symheap multiple times if it is a fixed address mapping failure,
     * otherwise stop. */
    while (all_map_result == MPIDIU_SYMSHM_MAP_FAIL && iter-- > 0) {
        void *map_pointer = NULL;
        int map_result = MPIDIU_SYMSHM_SUCCESS;

        /* the leading process in win get a random address */
        if (comm->rank == maxsz_loc)
            map_pointer = generate_random_addr(mapsize);

        /* broadcast fixed address to the other processes in win */
        mpi_errno = MPIR_Bcast(&map_pointer, sizeof(map_pointer),
                               MPI_CHAR, maxsz_loc, comm, &errflag);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        /* each shm node try to create shm segment with the fixed address,
         * if single process on the node then simply mmap */
        if (mapsize > 0) {
            if (shm_comm_ptr != NULL) {
                /* calculate local start address */

                /* Some static analyzer doesn't understand the relationship
                 * that shm_comm_ptr != NULL => shm_offsets != NULL.
                 * This assertion is to make it easier to understand not only
                 * for static analyzers but also humans. */
                MPIR_Assert(shm_offsets != NULL);

                *base_ptr = (void *) ((char *) map_pointer - shm_offsets[shm_comm_ptr->rank]);
                mpi_errno = allocate_symshm_segment(shm_comm_ptr, mapsize, shm_segment_hdl_ptr,
                                                    base_ptr, &map_result);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            } else {
                int rc = check_maprange_ok(map_pointer, mapsize);
                if (rc) {
                    *base_ptr = MPL_mmap(map_pointer, mapsize,
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0, MPL_MEM_RMA);
                } else
                    *base_ptr = (void *) MAP_FAILED;
                map_result = (*base_ptr == (void *) MAP_FAILED) ?
                    MPIDIU_SYMSHM_MAP_FAIL : MPIDIU_SYMSHM_SUCCESS;
            }
        }

        /* check if any mapping failure occurs */
        mpi_errno = MPIR_Allreduce(&map_result, &all_map_result, 1, MPI_INT,
                                   MPI_MAX, comm, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* cleanup local shm segment if mapping failed on other process */
        if (all_map_result != MPIDIU_SYMSHM_SUCCESS && mapsize > 0 &&
            map_result == MPIDIU_SYMSHM_SUCCESS) {
            if (shm_comm_ptr != NULL) {
                /* destroy successful shm segment */
                mpi_errno = MPIDIU_destroy_shm_segment(mapsize, shm_segment_hdl_ptr, base_ptr);
                MPIR_ERR_CHECK(mpi_errno);
            } else
                MPL_munmap(base_ptr, mapsize, MPL_MEM_RMA);
        }
    }

    if (all_map_result != MPIDIU_SYMSHM_SUCCESS) {
        /* if fail to allocate, return and let the caller choose another method */
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "WARNING: Win_allocate:  Unable to allocate global symmetric heap\n");
        *fail_flag = true;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_SHM_SYMHEAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else

    /* always fail, return and let the caller choose another method */
    MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                "WARNING: Win_allocate:  Unable to allocate global symmetric heap\n");
    *fail_flag = true;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_SHM_SYMHEAP);
    return mpi_errno;
#endif
}

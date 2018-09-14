/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_SYMHEAP_H_INCLUDED
#define CH4R_SYMHEAP_H_INCLUDED

#include <mpichconf.h>

#include <opa_primitives.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_RANDOM_ADDR_RETRY
      category    : CH4
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for generating a random address. A retrying
        involves only local operations.

    - name        : MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY
      category    : CH4
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

/* Returns aligned size and the page size. The page size parameter must
 * be always initialized to 0 or a previously returned value. If page size
 * is set, we can reuse the value and skip sysconf. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_get_mapsize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline size_t MPIDI_CH4R_get_mapsize(size_t size, size_t * psz)
{
    size_t page_sz, mapsize;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_GET_MAPSIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_GET_MAPSIZE);

    if (*psz == 0)
        page_sz = (size_t) sysconf(_SC_PAGESIZE);
    else
        page_sz = *psz;

    mapsize = (size + (page_sz - 1)) & (~(page_sz - 1));
    *psz = page_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_GET_MAPSIZE);
    return mapsize;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_is_valid_mapaddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_is_valid_mapaddr(void *start)
{
    return ((uintptr_t) start == -1ULL) ? 0 : 1;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_check_maprange_ok
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_check_maprange_ok(void *start, size_t size)
{
    int rc = 0;
    int ret = 0;
    size_t page_sz = 0;
    size_t mapsize = MPIDI_CH4R_get_mapsize(size, &page_sz);
    size_t i, num_pages = mapsize / page_sz;
    char *ptr = (char *) start;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_CHECK_MAPRANGE_OK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_CHECK_MAPRANGE_OK);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_CHECK_MAPRANGE_OK);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_generate_random_addr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void *MPIDI_CH4R_generate_random_addr(size_t size)
{
    /* starting position for pointer to map
     * This is not generic, probably only works properly on Linux
     * but it's not fatal since we bail after a fixed number of iterations
     */
#define MPIDI_CH4I_MAP_POINTER ((random_unsigned&((0x00006FFFFFFFFFFF&(~(page_sz-1)))|0x0000600000000000)))
    uintptr_t map_pointer;
#ifdef USE_SYM_HEAP
    char random_state[256];
    size_t page_sz = 0;
    uint64_t random_unsigned;
    size_t mapsize = MPIDI_CH4R_get_mapsize(size, &page_sz);
    struct timeval ts;
    int iter = MPIR_CVAR_CH4_RANDOM_ADDR_RETRY;
    int32_t rh, rl;
    struct random_data rbuf;
#endif

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_GENERATE_RANDOM_ADDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_GENERATE_RANDOM_ADDR);

#ifndef USE_SYM_HEAP
    map_pointer = -1ULL;
    goto fn_exit;
#else

    /* rbuf must be zero-cleared otherwise it results in SIGSEGV in glibc
     * (http://stackoverflow.com/questions/4167034/c-initstate-r-crashing) */
    memset(&rbuf, 0, sizeof(rbuf));

    gettimeofday(&ts, NULL);

    initstate_r(ts.tv_usec, random_state, sizeof(random_state), &rbuf);
    random_r(&rbuf, &rh);
    random_r(&rbuf, &rl);
    random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
    map_pointer = MPIDI_CH4I_MAP_POINTER;

    while (MPIDI_CH4R_check_maprange_ok((void *) map_pointer, mapsize) == 0) {
        random_r(&rbuf, &rh);
        random_r(&rbuf, &rl);
        random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
        map_pointer = MPIDI_CH4I_MAP_POINTER;
        iter--;

        if (iter == 0) {
            map_pointer = -1ULL;
            goto fn_exit;
        }
    }

#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_GENERATE_RANDOM_ADDR);
    return (void *) map_pointer;
}

/* Collectively allocate symmetric shared memory region with address
 * defined by base_ptr. MPL_shm routines and MPI collectives are internally used.
 *
 * The caller should ensure shm_segment_len is page aligned and base_ptr
 * is a symmetric non-NULL address on all processes.
 *
 * mapfail_flag is set to 1 if it is a mapping failure, otherwise it is 0. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_allocate_symshm_segment
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_allocate_symshm_segment(MPIR_Comm * shm_comm_ptr,
                                                     MPI_Aint shm_segment_len,
                                                     MPL_shm_hnd_t * shm_segment_hdl_ptr,
                                                     void **base_ptr, int *mapfail_flag)
{
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SHM_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_ALLOCATE_SYMSHM_SEGMENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_ALLOCATE_SYMSHM_SEGMENT);

    mpl_err = MPL_shm_hnd_init(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    /* rank 0 initializes shared memory segment */
    if (shm_comm_ptr->rank == 0) {
        char *serialized_hnd_ptr = NULL;

        /* try to map with specified symmetric address. */
        mpl_err =
            MPL_shm_fixed_seg_create_and_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
        if (mpl_err == MPL_SHM_EINVAL)
            *mapfail_flag = 1;

        /* handle mapping failure separately, caller may retry predefined times before giving up */
        MPIR_ERR_CHKANDJUMP((mpl_err != MPL_SHM_EINVAL && mpl_err != MPL_SHM_SUCCESS),
                            mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        /* broadcast the mapping result on rank 0 */
        mpi_errno = MPIR_Bcast(mapfail_flag, 1, MPI_INT, 0, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* broadcast handle to the others if mapping is successful */
        if (!*mapfail_flag) {
            mpl_err = MPL_shm_hnd_get_serialized_by_ref(*shm_segment_hdl_ptr, &serialized_hnd_ptr);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**alloc_shar_mem");

            mpi_errno = MPIR_Bcast(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                                   shm_comm_ptr, &errflag);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

            /* wait for other processes to attach to win */
            mpi_errno = MPIR_Barrier(shm_comm_ptr, &errflag);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        }

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpl_err = MPL_shm_seg_remove(*shm_segment_hdl_ptr);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**remove_shar_mem");
    } else {
        /* receive the mapping result of rank 0 */
        mpi_errno = MPIR_Bcast(mapfail_flag, 1, MPI_INT, 0, shm_comm_ptr, &errflag);

        /* if rank 0 mapped successfully, others on the node attach shared memory region */
        if (!*mapfail_flag) {
            char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

            /* get serialized handle from rank 0 and deserialize it */
            mpi_errno = MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                                   shm_comm_ptr, &errflag);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

            mpl_err = MPL_shm_hnd_deserialize(*shm_segment_hdl_ptr, serialized_hnd,
                                              strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SHM_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**alloc_shar_mem");

            /* try to attach with specified symmetric address */
            mpl_err = MPL_shm_fixed_seg_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
            if (mpl_err == MPL_SHM_EINVAL)
                *mapfail_flag = 1;

            /* handle mapping failure separately, caller may retry predefined times before giving up */
            MPIR_ERR_CHKANDJUMP((mpl_err != MPL_SHM_EINVAL && mpl_err != MPL_SHM_SUCCESS),
                                mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpi_errno = MPIR_Barrier(shm_comm_ptr, &errflag);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_ALLOCATE_SYMSHM_SEGMENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

typedef struct {
    unsigned long long sz;
    int loc;
} MPIDI_CH4I_ull_maxloc_t;

/* Compute maxloc for unsigned long long type.
 * If more than one max value exists, the loc with lower rank is returned. */
static inline void MPIDI_CH4I_ull_maxloc_op_func(void *invec, void *inoutvec,
                                                 int *len, MPI_Datatype * datatype)
{
    MPIDI_CH4I_ull_maxloc_t *inmaxloc = (MPIDI_CH4I_ull_maxloc_t *) invec;
    MPIDI_CH4I_ull_maxloc_t *outmaxloc = (MPIDI_CH4I_ull_maxloc_t *) inoutvec;
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
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_allreduce_maxloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_allreduce_maxloc(size_t mysz, int myloc, MPIR_Comm * comm,
                                              size_t * maxsz, int *maxsz_loc)
{
    int mpi_errno = MPI_SUCCESS;
    int blocks[2] = { 1, 1 };
    MPI_Aint disps[2];
    MPI_Datatype types[2], maxloc_type = MPI_DATATYPE_NULL;
    MPI_Op maxloc_op = MPI_OP_NULL;
    MPIDI_CH4I_ull_maxloc_t maxloc, maxloc_result;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_ALLREDUCE_MAXLOC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_ALLREDUCE_MAXLOC);

    types[0] = MPI_UNSIGNED_LONG_LONG;
    types[1] = MPI_INT;
    disps[0] = 0;
    disps[1] = (MPI_Aint) ((uintptr_t) & maxloc.loc - (uintptr_t) & maxloc.sz);

    mpi_errno = MPIR_Type_create_struct_impl(2, blocks, disps, types, &maxloc_type);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Type_commit_impl(&maxloc_type);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Op_create_impl(MPIDI_CH4I_ull_maxloc_op_func, 0, &maxloc_op);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    maxloc.sz = (unsigned long long) mysz;
    maxloc.loc = myloc;

    mpi_errno = MPIR_Allreduce(&maxloc, &maxloc_result, 1, maxloc_type, maxloc_op, comm, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    *maxsz_loc = maxloc_result.loc;
    *maxsz = (size_t) maxloc_result.sz;

  fn_exit:
    if (maxloc_type != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&maxloc_type);
    if (maxloc_op != MPI_OP_NULL)
        MPIR_Op_free_impl(&maxloc_op);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_ALLREDUCE_MAXLOC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Allocate a symmetric heap over a communicator with same starting address
 * on each process and shared memory over processes of each node. The MPL_shm
 * routines are internally used on node with multiple processes, and MPL_mmap
 * is used on node with single process. It tries MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY
 * times before giving up, and returns with fail_flag=1 if mapping fails.
 *
 * Input shm_size is the total mapping size of shared memory on local node, and
 * shm_offsets presents the expected offset of local process's start address in
 * the mapped shared memory. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_get_shm_symheap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_get_shm_symheap(MPI_Aint shm_size, MPI_Aint * shm_offsets,
                                             MPIR_Comm * comm, MPIR_Win * win, int *fail_flag)
{
    int mpi_errno = MPI_SUCCESS;
    int iter = MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY;
    unsigned any_mapfail_flag = 1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_GET_SHM_SYMHEAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_GET_SHM_SYMHEAP);

#ifdef USE_SYM_HEAP
    MPL_shm_hnd_t *shm_segment_hdl_ptr = &MPIDI_CH4U_WIN(win, shm_segment_handle);
    void **base_ptr = &MPIDI_CH4U_WIN(win, mmap_addr);

    size_t mapsize = 0, page_sz = 0, maxsz = 0;
    int maxsz_loc = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *shm_comm_ptr = comm->node_comm;

    *fail_flag = 0;
    mapsize = MPIDI_CH4R_get_mapsize(shm_size, &page_sz);

    /* figure out leading process in win */
    mpi_errno = MPIDI_CH4I_allreduce_maxloc(mapsize, comm->rank, comm, &maxsz, &maxsz_loc);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* zero size window, simply return */
    if (maxsz == 0)
        goto fn_exit;

    while (any_mapfail_flag && iter-- > 0) {
        void *map_pointer = NULL;
        unsigned mapfail_flag = 0;

        /* the leading process in win get a random address */
        if (comm->rank == maxsz_loc)
            map_pointer = MPIDI_CH4R_generate_random_addr(mapsize);

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
                *base_ptr = (void *) ((char *) map_pointer - shm_offsets[shm_comm_ptr->rank]);
                mpi_errno = MPIDI_CH4I_allocate_symshm_segment(shm_comm_ptr, mapsize,
                                                               shm_segment_hdl_ptr, base_ptr,
                                                               &mapfail_flag);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            } else {
                int rc = MPIDI_CH4R_check_maprange_ok(map_pointer, mapsize);
                if (rc) {
                    *base_ptr = MPL_mmap(map_pointer, mapsize,
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0, MPL_MEM_RMA);
                } else
                    *base_ptr = (void *) MAP_FAILED;
                mapfail_flag = (*base_ptr == (void *) MAP_FAILED) ? 1 : 0;
            }
        }

        /* check if any mapping failure occurs */
        mpi_errno = MPIR_Allreduce(&mapfail_flag, &any_mapfail_flag, 1, MPI_UNSIGNED,
                                   MPI_BOR, comm, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* cleanup local shm segment if mapping failed */
        if (any_mapfail_flag && mapsize > 0) {
            if (shm_comm_ptr != NULL) {
                if (mapfail_flag) {
                    /* if locally map failed, only release handle */
                    int mpl_err = MPL_SHM_SUCCESS;
                    mpl_err = MPL_shm_hnd_finalize(shm_segment_hdl_ptr);
                    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
                } else {
                    /* destroy successful shm segment */
                    mpi_errno = MPIDI_CH4U_destroy_shm_segment(mapsize, shm_segment_hdl_ptr,
                                                               base_ptr);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
            } else if (!mapfail_flag) {
                MPL_munmap(base_ptr, mapsize, MPL_MEM_RMA);
            }
        }
    }
#endif

    if (any_mapfail_flag) {
        /* if fail to allocate, return and let the caller choose another method */
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "WARNING: Win_allocate:  Unable to allocate global symmetric heap\n");
        *fail_flag = 1;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_GET_SHM_SYMHEAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_SYMHEAP_H_INCLUDED */

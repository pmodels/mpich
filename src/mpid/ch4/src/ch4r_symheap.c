/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_symheap.h"

/* BEGIN internal function decls */
static void *generate_random_addr(size_t size);
static int check_maprage_ok(void *start, size_t size);
static int is_valid_mapaddr(void *start);
/* END   internal function decls */

#undef FUNCNAME
#define FUNCNAME is_valid_mapaddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int is_valid_mapaddr(void *start)
{
    return ((uintptr_t) start == -1ULL) ? 0 : 1;
}

#undef FUNCNAME
#define FUNCNAME check_maprage_ok
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int check_maprage_ok(void *start, size_t size)
{
    int rc = 0;
    int ret = 0;
    size_t page_sz;
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

#undef FUNCNAME
#define FUNCNAME generate_random_addr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void *generate_random_addr(size_t size)
{
    /* starting position for pointer to map
     * This is not generic, probably only works properly on Linux
     * but it's not fatal since we bail after a fixed number of iterations
     */
#define MAP_POINTER ((random_unsigned&((0x00006FFFFFFFFFFF&(~(page_sz-1)))|0x0000600000000000)))
    uintptr_t map_pointer;
#ifdef USE_SYM_HEAP
    char random_state[256];
    size_t page_sz;
    uint64_t random_unsigned;
    size_t mapsize = MPIDIU_get_mapsize(size, &page_sz);
    struct timeval ts;
    int iter = MPIR_CVAR_CH4_RANDOM_ADDR_RETRY;
    int32_t rh, rl;
    struct random_data rbuf;
#endif

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);

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
    map_pointer = MAP_POINTER;

    while (check_maprage_ok((void *) map_pointer, mapsize) == 0) {
        random_r(&rbuf, &rh);
        random_r(&rbuf, &rl);
        random_unsigned = ((uint64_t) rh) << 32 | (uint64_t) rl;
        map_pointer = MAP_POINTER;
        iter--;

        if (iter == 0) {
            map_pointer = -1ULL;
            goto fn_exit;
        }
    }

#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GENERATE_RANDOM_ADDR);
    return (void *) map_pointer;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_get_mapsize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
size_t MPIDIU_get_mapsize(size_t size, size_t * psz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_MAPSIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_MAPSIZE);

    long page_sz = sysconf(_SC_PAGESIZE);
    size_t mapsize = (size + (page_sz - 1)) & (~(page_sz - 1));
    *psz = page_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_MAPSIZE);
    return mapsize;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_get_symmetric_heap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIU_get_symmetric_heap(MPI_Aint size, MPIR_Comm * comm, void **base, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int iter = MPIR_CVAR_CH4_SYMHEAP_RETRY;
    void *baseP = NULL;
    size_t mapsize = 0;
#ifdef USE_SYM_HEAP
    unsigned test, result;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    size_t page_sz;
#endif

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_SYMMETRIC_HEAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_SYMMETRIC_HEAP);

#ifndef USE_SYM_HEAP
    iter = 0;
#else

    mapsize = MPIDIU_get_mapsize(size, &page_sz);

    struct {
        uint64_t sz;
        int loc;
    } maxloc, maxloc_result;

    maxloc.sz = size;
    maxloc.loc = comm->rank;
    mpi_errno = MPIR_Allreduce(&maxloc,
                               &maxloc_result, 1, MPI_LONG_INT, MPI_MAXLOC, comm, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    if (maxloc_result.sz > 0) {
        result = 0;

        while (!result && --iter != 0) {
            uintptr_t map_pointer = 0ULL;

            baseP = (void *) -1ULL;

            if (comm->rank == maxloc_result.loc) {
                void *p = generate_random_addr(mapsize);
                map_pointer = (uintptr_t) p;
                baseP = MPL_mmap((void *) map_pointer, mapsize,
                                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0,
                                 MPL_MEM_RMA);
            }

            mpi_errno = MPIR_Bcast(&map_pointer,
                                   1, MPI_UNSIGNED_LONG, maxloc_result.loc, comm, &errflag);

            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm->rank != maxloc_result.loc) {
                int rc = check_maprage_ok((void *) map_pointer, mapsize);

                if (rc) {
                    baseP = MPL_mmap((void *) map_pointer, mapsize,
                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1,
                                     0, MPL_MEM_RMA);
                } else
                    baseP = (void *) -1ULL;
            }

            if (mapsize == 0)
                baseP = (void *) map_pointer;

            test = ((uintptr_t) baseP != -1ULL) ? 1 : 0;
            mpi_errno = MPIR_Allreduce(&test, &result, 1, MPI_UNSIGNED, MPI_BAND, comm, &errflag);

            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (result == 0 && baseP != (void *) -1ULL)
                MPL_munmap(baseP, mapsize, MPL_MEM_RMA);
        }
    } else
        baseP = NULL;
#endif

    if (iter == 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "WARNING: Win_allocate:  Unable to allocate symmetric heap\n");
        baseP = MPL_malloc(size, MPL_MEM_RMA);
        MPIR_ERR_CHKANDJUMP((baseP == NULL), mpi_errno, MPI_ERR_BUFFER, "**bufnull");
        MPIDIU_WIN(win, mmap_sz) = -1ULL;
        MPIDIU_WIN(win, mmap_addr) = NULL;
    } else {
        MPIDIU_WIN(win, mmap_sz) = mapsize;
        MPIDIU_WIN(win, mmap_addr) = baseP;
    }

    *base = baseP;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_SYMMETRIC_HEAP);
    return mpi_errno;
  fn_fail:
    *base = NULL;
    goto fn_exit;
}

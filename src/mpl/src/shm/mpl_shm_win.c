/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_USE_NT_SHM

#include <winsock2.h>
#include <windows.h>

/* Returns MPL_SUCCESS on success, MPL_ERR_SHM_INTERN on error */
int MPLI_shm_lhnd_close(MPL_shm_hnd_t hnd)
{
    HANDLE lhnd = MPLI_shm_lhnd_get(hnd);
    if (lhnd == MPLI_SHM_LHND_INVALID)
        return MPL_SUCCESS;
    MPLI_shm_lhnd_set(hnd, MPLI_SHM_LHND_INVALID);
    return CloseHandle(lhnd) ? MPL_SUCCESS : MPL_ERR_SHM_INTERN;
}

/* Generate a name for a new mapping that is unique within the session */
static int shm_ghnd_set_uniq(MPL_shm_hnd_t hnd)
{
    static volatile LONG counter = 0;
    LARGE_INTEGER perf_cnt;
    int rc;

    rc = MPLI_shm_ghnd_alloc(hnd, MPL_MEM_SHM);
    if (rc != MPL_SUCCESS)
        return rc;
    QueryPerformanceCounter(&perf_cnt);
    snprintf(MPLI_shm_ghnd_get_by_ref(hnd), MPLI_SHM_GHND_SZ, "Local\\mpich_shm_%lu_%ld_%lld",
             (unsigned long) GetCurrentProcessId(), (long) InterlockedIncrement(&counter),
             (long long) perf_cnt.QuadPart);
    return MPL_SUCCESS;
}

/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
static int MPL_shm_seg_create_attach_templ(MPL_shm_hnd_t hnd, intptr_t seg_sz,
                                           void **shm_addr_ptr, int offset, int flag)
{
    HANDLE lhnd;
    ULARGE_INTEGER seg_sz_large;
    int rc = MPL_SUCCESS;

    seg_sz_large.QuadPart = seg_sz;

    if (flag & MPLI_SHM_FLAG_SHM_CREATE) {
        rc = shm_ghnd_set_uniq(hnd);
        if (rc != MPL_SUCCESS)
            goto fn_exit;
        lhnd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  seg_sz_large.HighPart, seg_sz_large.LowPart,
                                  MPLI_shm_ghnd_get_by_ref(hnd));
        if (lhnd == NULL) {
            rc = MPL_ERR_SHM_INTERN;
            goto fn_exit;
        }
        MPLI_shm_lhnd_set(hnd, lhnd);
    } else if (!MPLI_shm_lhnd_is_valid(hnd)) {
        lhnd = OpenFileMappingA(FILE_MAP_WRITE, FALSE, MPLI_shm_ghnd_get_by_ref(hnd));
        if (lhnd == NULL) {
            rc = MPL_ERR_SHM_INTERN;
            goto fn_exit;
        }
        MPLI_shm_lhnd_set(hnd, lhnd);
    }

    if (flag & MPLI_SHM_FLAG_SHM_ATTACH) {
        void *start_addr = (flag & MPLI_SHM_FLAG_FIXED_ADDR) ? *shm_addr_ptr : NULL;
        /* For a fixed address, start_addr must be a multiple of the allocation
         * granularity and the range must be free, otherwise this fails. */
        *shm_addr_ptr = MapViewOfFileEx(MPLI_shm_lhnd_get(hnd), FILE_MAP_WRITE, 0, offset,
                                        seg_sz, start_addr);
        if (*shm_addr_ptr == NULL)
            rc = MPL_ERR_SHM_INVAL;
    }

  fn_exit:
    return rc;
}

/* Create new SHM segment
 * hnd : A "init"ed shared memory handle
 * seg_sz : Size of shared memory segment to be created
 */
int MPL_shm_seg_create(MPL_shm_hnd_t hnd, intptr_t seg_sz)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, NULL, 0, MPLI_SHM_FLAG_SHM_CREATE);
}

/* Open an existing SHM segment
 * hnd : A shm handle with a valid global handle
 * seg_sz : Size of shared memory segment to open
 * Currently only using internally within wrapper funcs
 */
int MPL_shm_seg_open(MPL_shm_hnd_t hnd, intptr_t seg_sz)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, NULL, 0, MPLI_SHM_FLAG_CLR);
}

/* Create new SHM segment and attach to it
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_seg_create_and_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz,
                                  void **shm_addr_ptr, int offset)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                                           MPLI_SHM_FLAG_SHM_CREATE | MPLI_SHM_FLAG_SHM_ATTACH);
}

/* Attach to an existing SHM segment
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_seg_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz, void **shm_addr_ptr, int offset)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                                           MPLI_SHM_FLAG_SHM_ATTACH);
}

/* Create new SHM segment and attach to it with specified starting address
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr (inout): Pointer to specified starting address, the address cannot be NULL.
 *                       The actual attached memory address is updated at return.
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_fixed_seg_create_and_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz,
                                        void **shm_addr_ptr, int offset)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                                           MPLI_SHM_FLAG_SHM_CREATE | MPLI_SHM_FLAG_SHM_ATTACH |
                                           MPLI_SHM_FLAG_FIXED_ADDR);
}

/* Attach to an existing SHM segment with specified starting address
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr (inout): Pointer to specified starting address, the address cannot be NULL.
 *                       The actual attached memory address is updated at return.
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_fixed_seg_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz, void **shm_addr_ptr, int offset)
{
    return MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                                           MPLI_SHM_FLAG_SHM_ATTACH | MPLI_SHM_FLAG_FIXED_ADDR);
}

/* Detach from an attached SHM segment */
int MPL_shm_seg_detach(MPL_shm_hnd_t hnd, void **shm_addr_ptr, intptr_t seg_sz)
{
    int rc = UnmapViewOfFile(*shm_addr_ptr);
    *shm_addr_ptr = NULL;

    /* If the function succeeds, the return value is nonzero,
     * otherwise the return value is zero. */
    return (rc != 0) ? MPL_SUCCESS : MPL_ERR_SHM_INTERN;
}

/* Nothing to remove: the mapping goes away with its last handle and view */
int MPL_shm_seg_remove(MPL_shm_hnd_t hnd)
{
    return MPL_SUCCESS;
}

#endif /* MPL_USE_NT_SHM */

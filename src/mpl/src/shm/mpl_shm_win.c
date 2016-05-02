/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#ifdef MPL_USE_NT_SHM

#include<winsock2.h>
#include<windows.h>

/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_create_attach_templ
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPL_shm_seg_create_attach_templ(
    MPL_shm_hnd_t hnd, intptr_t seg_sz, char **shm_addr_ptr,
    int offset, int flag)
{
    HANDLE lhnd = INVALID_HANDLE_VALUE;
    int rc = -1;
    ULARGE_INTEGER seg_sz_large;
    seg_sz_large.QuadPart = seg_sz;

    if(!MPLI_shm_ghnd_is_valid(hnd)){
        rc = MPLI_shm_ghnd_set_uniq(hnd);
    }

    if(flag & MPLI_SHM_FLAG_SHM_CREATE){
        lhnd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
                PAGE_READWRITE, seg_sz_large.HighPart, seg_sz_large.LowPart,
                MPLI_shm_ghnd_get_by_ref(hnd));
        MPLI_shm_lhnd_set(hnd, lhnd);
    }
    else{
        if(!MPLI_shm_lhnd_is_valid(hnd)){
            /* Strangely OpenFileMapping() returns NULL on error! */
            lhnd = OpenFileMapping(FILE_MAP_WRITE, FALSE,
                    MPLI_shm_ghnd_get_by_ref(hnd));

            MPLI_shm_lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPLI_SHM_FLAG_SHM_ATTACH){
        *shm_addr_ptr = (char *)MapViewOfFile(MPLI_shm_lhnd_get(hnd),
                            FILE_MAP_WRITE, 0, offset, 0);
    }

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}

/* Create new SHM segment
 * hnd : A "init"ed shared memory handle
 * seg_sz : Size of shared memory segment to be created
 */
int MPL_shm_seg_create(MPL_shm_hnd_t hnd, intptr_t seg_sz)
{
    int rc = -1;
    rc = MPL_shm_seg_create_attach_templ(hnd, seg_sz, NULL, 0,
                                         MPLI_SHM_FLAG_SHM_CREATE);
    return rc;
}

/* Open an existing SHM segment
 * hnd : A shm handle with a valid global handle
 * seg_sz : Size of shared memory segment to open
 * Currently only using internally within wrapper funcs
 */
int MPL_shm_seg_open(MPL_shm_hnd_t hnd, intptr_t seg_sz)
{
    int rc = -1;
    rc = MPL_shm_seg_create_attach_templ(hnd, seg_sz, NULL, 0, MPLI_SHM_FLAG_CLR);
    return rc;
}

/* Create new SHM segment and attach to it
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_seg_create_and_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz,
                                  char **shm_addr_ptr, int offset)
{
    int rc = 0;
    rc = MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                            MPLI_SHM_FLAG_SHM_CREATE | MPLI_SHM_FLAG_SHM_ATTACH);
    return rc;
}

/* Attach to an existing SHM segment
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
int MPL_shm_seg_attach(MPL_shm_hnd_t hnd, intptr_t seg_sz, char **shm_addr_ptr,
                       int offset)
{
    int rc = 0;
    rc = MPL_shm_seg_create_attach_templ(hnd, seg_sz, shm_addr_ptr, offset,
                                         MPLI_SHM_FLAG_SHM_ATTACH);
    return rc;
}
/* Detach from an attached SHM segment */
#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPL_shm_seg_detach(
    MPL_shm_hnd_t hnd, char **shm_addr_ptr, intptr_t seg_sz)
{
    int rc = -1;

    rc = UnmapViewOfFile(*shm_addr_ptr);
    *shm_addr_ptr = NULL;

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}


#endif /* MPL_USE_NT_SHM */

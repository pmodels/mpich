/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#ifdef MPL_USE_SYSV_SHM

#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
    int rc = -1;
    int lhnd = -1;

    if(flag & MPLI_SHM_FLAG_SHM_CREATE){
        lhnd = shmget(IPC_PRIVATE, seg_sz, IPC_CREAT | S_IRWXU);
        MPL_shm_lhnd_set(hnd, lhnd);
        rc = MPLI_shm_ghnd_alloc(hnd);
        rc = MPLI_shm_ghnd_set_by_val(hnd, "%d", lhnd);
    }
    else{
        /* Open an existing shared memory seg */
        if(!MPLI_shm_lhnd_is_valid(hnd)){
            lhnd = atoi(MPLI_shm_ghnd_get_by_ref(hnd));
            MPLI_shm_lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPLI_SHM_FLAG_SHM_ATTACH){
        /* Attach to shared mem seg */
        *shm_addr_ptr = shmat(MPLI_shm_lhnd_get(hnd), NULL, 0x0);
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
/* Detach from an attached SHM segment
 * hnd : Handle to the shm segment
 * shm_addr_ptr : Pointer to the shm address to detach
 * seg_sz : Size of shm segment
 */
#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPL_shm_seg_detach(MPL_shm_hnd_t hnd, char **shm_addr_ptr, intptr_t seg_sz)
{
    int rc = -1;

    rc = shmdt(*shm_addr_ptr);
    *shm_addr_ptr = NULL;

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}

/* Remove a shared memory segment
 * hnd : Handle to the shared memory segment to be removed
 */
#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_remove
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int  MPL_shm_seg_remove(MPL_shm_hnd_t hnd)
{
    struct shmid_ds ds;
    int rc = -1;

    rc = shmctl(MPL_shm_lhnd_get(hnd), IPC_RMID, &ds);

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}

#endif /* MPL_USE_SYSV_SHM */

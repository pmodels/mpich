/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#ifdef MPL_USE_MMAP_SHM

#include <fcntl.h>
#include <sys/mman.h>
#if defined (MPL_HAVE_MKSTEMP) && defined (MPL_NEEDS_MKSTEMP_DECL)
extern int mkstemp(char *template);
#endif

inline int MPLI_shm_lhnd_close(MPL_shm_hnd_t hnd)
{
    MPLI_shm_lhnd_t lhnd = MPLI_SHM_LHND_INVALID;
    lhnd = MPLI_shm_lhnd_get(hnd);
    if (lhnd != MPLI_SHM_LHND_INVALID) {
        if (close(lhnd) == 0) {
            MPLI_shm_lhnd_set(hnd, MPLI_SHM_LHND_INIT_VAL);
        } else {
            /* close() failed */
            return -1;
        }
    }
    return 0;
}

/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
/* FIXME: Pass (void **)shm_addr_ptr instead of (char **) shm_addr_ptr
 *  since a util func should be generic
 *  Currently not passing (void **) to reduce warning in nemesis
 *  code which passes (char **) ptrs to be attached to a seg
 */

#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_create_attach_templ
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPL_shm_seg_create_attach_templ(
    MPL_shm_hnd_t hnd, intptr_t seg_sz, char **shm_addr_ptr,
    int offset, int flag)
{
    MPLI_shm_lhnd_t lhnd = -1, rc = -1;

    if(flag & MPLI_SHM_FLAG_SHM_CREATE){
        char dev_shm_fname[] = "/dev/shm/mpich_shar_tmpXXXXXX";
        char tmp_fname[] = "/tmp/mpich_shar_tmpXXXXXX";
        char *chosen_fname = NULL;

        chosen_fname = dev_shm_fname;
        lhnd = mkstemp(chosen_fname);
        if(lhnd == -1){
            chosen_fname = tmp_fname;
            lhnd = mkstemp(chosen_fname);
        }

        MPLI_shm_lhnd_set(hnd, lhnd);
        rc = (MPLI_shm_lhnd_t)lseek(lhnd, seg_sz - 1, SEEK_SET);
        do{
            rc = (int) write(lhnd, "", 1);
        }while((rc == -1) && (errno == EINTR));

        rc = MPLI_shm_ghnd_alloc(hnd);
        rc = MPLI_shm_ghnd_set_by_val(hnd, "%s", chosen_fname);
    }
    else{
        /* Open an existing shared memory seg */
        if(!MPLI_shm_lhnd_is_valid(hnd)){
            lhnd = open(MPLI_shm_ghnd_get_by_ref(hnd), O_RDWR);
            MPLI_shm_lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPLI_SHM_FLAG_SHM_ATTACH){
        void *buf_ptr = NULL;
        buf_ptr = mmap(NULL, seg_sz, PROT_READ | PROT_WRITE,
                        MAP_SHARED, MPLI_shm_lhnd_get(hnd), 0);
        *shm_addr_ptr = (char*)buf_ptr;
    }

fn_exit:
    /* FIXME: Close local handle only when closing the shm handle */
    if(MPLI_shm_lhnd_is_valid(hnd)){
        rc = MPLI_shm_lhnd_close(hnd);
    }
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
int MPL_shm_seg_detach(MPL_shm_hnd_t hnd, char **shm_addr_ptr, intptr_t seg_sz)
{
    int rc = -1;

    rc = munmap(*shm_addr_ptr, seg_sz);
    *shm_addr_ptr = NULL;

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}

/* Remove an existing SHM segment */
#undef FUNCNAME
#define FUNCNAME MPL_shm_seg_remove
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int  MPL_shm_seg_remove(MPL_shm_hnd_t hnd)
{
    int rc = -1;

    rc = unlink(MPLI_shm_ghnd_get_by_ref(hnd));

fn_exit:
    return rc;
fn_fail:
    goto fn_exit;
}

#endif /* MPL_USE_MMAP_SHM */

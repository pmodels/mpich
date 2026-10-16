/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_SHM_WIN_H_INCLUDED
#define MPL_SHM_WIN_H_INCLUDED

#include <winsock2.h>
#include <windows.h>

/* Shared memory segments are named, pagefile-backed file mappings.  The
 * global handle is the mapping name; the local handle is the mapping handle.
 *
 * A mapping is destroyed when its last handle and view are gone, so the local
 * handle is kept open until the shm handle is finalized (MPL_shm_hnd_finalize)
 * rather than closed right after creation as on POSIX. */

typedef HANDLE MPLI_shm_lhnd_t;

typedef char *MPLI_shm_ghnd_t;
/* The local handle, lhnd, is valid only for the current process,
 * The global handle, ghnd, is valid across multiple processes
 * The handle flag, flag, is used to set various attributes of the
 *  handle.
 */
typedef struct MPLI_shm_lghnd_t {
    MPLI_shm_lhnd_t lhnd;
    MPLI_shm_ghnd_t ghnd;
    int flag;
} MPLI_shm_lghnd_t;

typedef MPLI_shm_lghnd_t *MPL_shm_hnd_t;

#define MPL_SHM_SEG_NAME_LEN   70
#define MPLI_SHM_GHND_SZ       MPL_SHM_SEG_NAME_LEN
#define MPLI_SHM_LHND_INVALID  NULL
#define MPLI_SHM_LHND_INIT_VAL NULL

#define MPL_SHM_SEG_ALREADY_EXISTS ERROR_ALREADY_EXISTS

int MPLI_shm_lhnd_close(MPL_shm_hnd_t hnd);

#endif /* MPL_SHM_WIN_H_INCLUDED */

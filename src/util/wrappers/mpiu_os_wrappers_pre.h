/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file contains "pre" definitions and declarations for the OS wrappers.
 * That is, things that shouldn't depend on much more than the mpichconf.h
 * values. */

#ifndef MPIU_OS_WRAPPERS_PRE_H_INCLUDED
#define MPIU_OS_WRAPPERS_PRE_H_INCLUDED

/* ------------------------------------------------------------------------ */
/* util wrappers */
/* TODO port defs/decls here as necessary */

/* ------------------------------------------------------------------------ */
/* process wrappers */
/* TODO port defs/decls here as necessary */

/* ------------------------------------------------------------------------ */
/* shm wrappers */

#define MPIU_SHMW_FLAG_CLR              0x0
#define MPIU_SHMW_FLAG_SHM_CREATE       0x1
#define MPIU_SHMW_FLAG_SHM_ATTACH       0x10
#define MPIU_SHMW_FLAG_GHND_STATIC      0x100

#ifdef USE_SYSV_SHM
typedef int MPIU_SHMW_Lhnd_t;
#elif defined USE_MMAP_SHM
typedef int MPIU_SHMW_Lhnd_t;
#elif defined USE_NT_SHM
typedef HANDLE MPIU_SHMW_Lhnd_t;
#endif

typedef char * MPIU_SHMW_Ghnd_t;
/* The local handle, lhnd, is valid only for the current process,
 * The global handle, ghnd, is valid across multiple processes
 * The handle flag, flag, is used to set various attributes of the 
 *  handle.
 */
typedef struct{
    MPIU_SHMW_Lhnd_t lhnd;
    MPIU_SHMW_Ghnd_t ghnd;
    int flag;
} MPIU_SHMW_LGhnd_t;

typedef MPIU_SHMW_LGhnd_t * MPIU_SHMW_Hnd_t;

#define MPIU_SHMW_HND_INVALID     NULL
#define MPIU_SHMW_GHND_INVALID    NULL
#define MPIU_SHMW_GHND_INIT_VAL    '\0'
/* TODO port additional defs/decls here as necessary */

/* ------------------------------------------------------------------------ */
/* sock wrappers */
/* TODO port defs/decls here as necessary */

#endif /* MPIU_OS_WRAPPERS_PRE_H_INCLUDED */

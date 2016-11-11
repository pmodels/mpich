/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_DEFS_H_INCLUDED
#define POSIX_DEFS_H_INCLUDED

/* ************************************************************************** */
/* from mpid/ch3/channels/nemesis/include/mpid_nem_defs.h                     */
/* ************************************************************************** */

#include "mpidu_shm.h"
#define MPIDI_POSIX_MAX_FNAME_LEN 256

/* FIXME: This definition should be gotten from mpidi_ch3_impl.h */
#ifndef MPIDI_POSIX_MAX_HOSTNAME_LEN
#define MPIDI_POSIX_MAX_HOSTNAME_LEN 256
#endif /* MPIDI_POSIX_MAX_HOSTNAME_LEN */

extern char MPIDI_POSIX_hostname[MPIDI_POSIX_MAX_HOSTNAME_LEN];

#define MPIDI_POSIX_RET_OK       1
#define MPIDI_POSIX_RET_NG      -1
#define MPIDI_POSIX_KEY          632236
#define MPIDI_POSIX_ANY_SOURCE  -1
#define MPIDI_POSIX_IN           1
#define MPIDI_POSIX_OUT          0

#define MPIDI_POSIX_POLL_IN      0
#define MPIDI_POSIX_POLL_OUT     1

#define MPIDI_POSIX_ASYMM_NULL_VAL    64
typedef MPI_Aint MPIDI_POSIX_addr_t;
extern char *MPIDI_POSIX_asym_base_addr;

#define MPIDI_POSIX_REL_NULL (0x0)
#define MPIDI_POSIX_IS_REL_NULL(rel_ptr) (OPA_load_ptr(&(rel_ptr).p) == MPIDI_POSIX_REL_NULL)
#define MPIDI_POSIX_SET_REL_NULL(rel_ptr) (OPA_store_ptr(&((rel_ptr).p), MPIDI_POSIX_REL_NULL))
#define MPIDI_POSIX_REL_ARE_EQUAL(rel_ptr1, rel_ptr2) \
    (OPA_load_ptr(&(rel_ptr1).p) == OPA_load_ptr(&(rel_ptr2).p))

#ifndef MPIDI_POSIX_SYMMETRIC_QUEUES

static inline MPIDI_POSIX_cell_ptr_t MPIDI_POSIX_REL_TO_ABS(MPIDI_POSIX_cell_rel_ptr_t r)
{
    return (MPIDI_POSIX_cell_ptr_t) ((char *) OPA_load_ptr(&r.p) +
                                     (MPIDI_POSIX_addr_t) MPIDI_POSIX_asym_base_addr);
}

static inline MPIDI_POSIX_cell_rel_ptr_t MPIDI_POSIX_ABS_TO_REL(MPIDI_POSIX_cell_ptr_t a)
{
    MPIDI_POSIX_cell_rel_ptr_t ret;
    OPA_store_ptr(&ret.p, (char *) a - (MPIDI_POSIX_addr_t) MPIDI_POSIX_asym_base_addr);
    return ret;
}

#else /*MPIDI_POSIX_SYMMETRIC_QUEUES */
#define MPIDI_POSIX_REL_TO_ABS(ptr) (ptr)
#define MPIDI_POSIX_ABS_TO_REL(ptr) (ptr)
#endif /*MPIDI_POSIX_SYMMETRIC_QUEUES */

/* NOTE: MPIDI_POSIX_IS_LOCAL should only be used when the process is known to be
   in your comm_world (such as at init time).  This will generally not work for
   dynamic processes.  Check vc_ch->is_local instead.  If that is true, then
   it's safe to use MPIDI_POSIX_LOCAL_RANK. */
#define MPIDI_POSIX_NON_LOCAL -1
#define MPIDI_POSIX_IS_LOCAL(grank) (MPIDI_POSIX_mem_region.local_ranks[grank] != MPIDI_POSIX_NON_LOCAL)
#define MPIDI_POSIX_LOCAL_RANK(grank) (MPIDI_POSIX_mem_region.local_ranks[grank])
#define MPIDI_POSIX_NUM_BARRIER_VARS 16
#define MPIDI_POSIX_SHM_MUTEX        MPID_shm_mutex
typedef struct MPIDI_POSIX_barrier_vars {
    OPA_int_t context_id;
    OPA_int_t usage_cnt;
    OPA_int_t cnt;
#if MPIDI_POSIX_CACHE_LINE_LEN != SIZEOF_INT
    char padding0[MPIDI_POSIX_CACHE_LINE_LEN - sizeof(int)];
#endif
    OPA_int_t sig0;
    OPA_int_t sig;
    char padding1[MPIDI_POSIX_CACHE_LINE_LEN - 2 * sizeof(int)];
} MPIDI_POSIX_barrier_vars_t;

typedef struct MPIDI_POSIX_mem_region {
    MPIDU_shm_seg_t memory;
    MPIDU_shm_seg_info_t *seg;
    int num_seg;
    int map_lock;
    int num_local;
    int num_procs;
    int *local_procs;           /* local_procs[lrank] gives the global rank of proc with local rank lrank */
    int local_rank;
    int *local_ranks;           /* local_ranks[grank] gives the local rank of proc with global rank grank or MPIDI_POSIX_NON_LOCAL */
    int ext_procs;              /* Number of non-local processes */
    int *ext_ranks;             /* Ranks of non-local processes */
    MPIDI_POSIX_fbox_arrays_t mailboxes;
    MPIDI_POSIX_cell_ptr_t Elements;
    MPIDI_POSIX_queue_ptr_t *FreeQ;
    MPIDI_POSIX_queue_ptr_t *RecvQ;
    MPIDU_shm_barrier_t *barrier;
    MPIDI_POSIX_queue_ptr_t my_freeQ;
    MPIDI_POSIX_queue_ptr_t my_recvQ;
    MPIDI_POSIX_barrier_vars_t *barrier_vars;
    int rank;
    struct MPIDI_POSIX_mem_region *next;
} MPIDI_POSIX_mem_region_t, *MPIDI_POSIX_mem_region_ptr_t;
extern MPIDI_POSIX_mem_region_t MPIDI_POSIX_mem_region;
extern MPID_Thread_mutex_t MPID_shm_mutex;

#endif /* POSIX_DEFS_H_INCLUDED */

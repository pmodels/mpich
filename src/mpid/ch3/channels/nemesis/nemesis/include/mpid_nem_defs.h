/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_DEFS_H
#define MPID_NEM_DEFS_H

#include "mpid_nem_datatypes.h"
#include "mpi.h"
#include "pmi.h"

#define MPID_NEM_MAX_KEY_VAL_LEN 256
#define MPID_NEM_MAX_FNAME_LEN 256
#define MAX_HOSTNAME_LEN 256
extern char MPID_nem_hostname[MAX_HOSTNAME_LEN];

/* #define ENABLED_CHECKPOINTING */

#define MPID_NEM_RET_OK       1
#define MPID_NEM_RET_NG      -1
#define MPID_NEM_KEY          632236
#define MPID_NEM_ANY_SOURCE  -1
#define MPID_NEM_IN           1
#define MPID_NEM_OUT          0 

typedef enum {MPID_NEM_POLL_IN, MPID_NEM_POLL_OUT} MPID_nem_poll_dir_t;

#define MPID_NEM_POLL_IN      0
#define MPID_NEM_POLL_OUT     1

#define MPID_NEM_ASYMM_NULL_VAL    64
typedef MPI_Aint MPID_nem_addr_t;
extern  char *MPID_nem_asymm_base_addr;

#define MPID_NEM_REL_NULL (0x0)
#define MPID_NEM_IS_REL_NULL(rel_ptr) ((rel_ptr).p == MPID_NEM_REL_NULL)
#define MPID_NEM_SET_REL_NULL(rel_ptr) ((rel_ptr).p = MPID_NEM_REL_NULL)
#define MPID_NEM_REL_ARE_EQUAL(rel_ptr1, rel_ptr2) ((rel_ptr1).p == (rel_ptr2).p)

#ifndef MPID_NEM_SYMMETRIC_QUEUES

static inline MPID_nem_cell_ptr_t MPID_NEM_REL_TO_ABS (MPID_nem_cell_rel_ptr_t r)
{
    return (MPID_nem_cell_ptr_t)(r.p + (MPID_nem_addr_t)MPID_nem_asymm_base_addr);
}

static inline MPID_nem_cell_rel_ptr_t MPID_NEM_ABS_TO_REL (MPID_nem_cell_ptr_t a)
{
    MPID_nem_cell_rel_ptr_t ret;
    ret.p = (char *)a - (MPID_nem_addr_t)MPID_nem_asymm_base_addr;
    return ret;
}

#else /*MPID_NEM_SYMMETRIC_QUEUES */
#define MPID_NEM_REL_TO_ABS(ptr) (ptr)
#define MPID_NEM_ABS_TO_REL(ptr) (ptr)
#endif /*MPID_NEM_SYMMETRIC_QUEUES */

typedef struct MPID_nem_barrier
{
    volatile int val;
    volatile int wait;
}
MPID_nem_barrier_t;

typedef struct MPID_nem_seg
{
  /* Sizes */
  int   max_size ;
  int   size_left;
  /* Pointers */
  char *base_addr;
  char *current_addr;
  char *max_addr;
  /* Misc */
  char  file_name[MPID_NEM_MAX_FNAME_LEN];
  int   base_descs; 
  int   symmetrical;
} MPID_nem_seg_t, *MPID_nem_seg_ptr_t;

typedef struct MPID_nem_seg_info
{
    int   size;
    char *addr; 
} MPID_nem_seg_info_t, *MPID_nem_seg_info_ptr_t; 

/* NOTE: MPID_NEM_IS_LOCAL should only be used when the process is known to be
   in your comm_world (such as at init time).  This will generally not work for
   dynamic processes.  Check vc_ch->is_local instead.  If that is true, then
   it's safe to use MPID_NEM_LOCAL_RANK. */
#define MPID_NEM_NON_LOCAL -1
#define MPID_NEM_IS_LOCAL(grank) (MPID_nem_mem_region.local_ranks[grank] != MPID_NEM_NON_LOCAL)
#define MPID_NEM_LOCAL_RANK(grank) (MPID_nem_mem_region.local_ranks[grank])

#define MPID_NEM_NUM_BARRIER_VARS 16
typedef struct MPID_nem_barrier_vars
{
    volatile int context_id;
    volatile int usage_cnt;
    volatile int cnt;
    char padding0[MPID_NEM_CACHE_LINE_LEN - sizeof(int)];
    volatile int sig0;
    volatile int sig;
    char padding1[MPID_NEM_CACHE_LINE_LEN - 2* sizeof(int)];
}
MPID_nem_barrier_vars_t;

typedef struct MPID_nem_mem_region
{
    MPID_nem_seg_t              memory;
    MPID_nem_seg_info_t        *seg;
    int                         num_seg;
    int                         map_lock;
    pid_t                      *pid;
    int                         num_local;
    int                         num_nodes;
    int                        *node_ids;
    int                         num_procs;
    int                        *local_procs; /* local_procs[lrank] gives the global rank of proc with local rank lrank */
    int                         local_rank;    
    int                        *local_ranks; /* local_ranks[grank] gives the local rank of proc with global rank grank or MPID_NEM_NON_LOCAL */
    int                         ext_procs; /* Number of non-local processes */
    int                        *ext_ranks; /* Ranks of non-local processes */ 
    MPID_nem_fbox_arrays_t      mailboxes;
    MPID_nem_cell_ptr_t         Elements;
    MPID_nem_queue_ptr_t       *FreeQ;
    MPID_nem_queue_ptr_t       *RecvQ;
    MPID_nem_cell_ptr_t         net_elements;
    MPID_nem_barrier_t         *barrier;
    MPID_nem_queue_ptr_t        my_freeQ;
    MPID_nem_queue_ptr_t        my_recvQ;
    MPID_nem_barrier_vars_t    *barrier_vars;
    int                         rank;
    struct MPID_nem_mem_region *next;
} MPID_nem_mem_region_t, *MPID_nem_mem_region_ptr_t;

/* #define MEM_REGION_IN_HEAP */
#ifdef MEM_REGION_IN_HEAP
#define MPID_nem_mem_region (*MPID_nem_mem_region_ptr)
extern MPID_nem_mem_region_t *MPID_nem_mem_region_ptr;
#else /* MEM_REGION_IN_HEAP */
extern MPID_nem_mem_region_t MPID_nem_mem_region;
#endif /* MEM_REGION_IN_HEAP */




#endif /* MPID_NEM_DEFS_H */

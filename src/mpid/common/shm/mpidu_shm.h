/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SHM_H_INCLUDED
#define MPIDU_SHM_H_INCLUDED

#include "mpidu_shm_obj.h"
#include "mpir_memtype.h"

#ifdef HAVE_HWLOC
#include "hwloc.h"
#define MPIDU_Mempolicy hwloc_membind_policy_t
#else
#define MPIDU_Mempolicy enum
#endif

#define MPIDU_shm_numa_bitmap int
#define MPIDU_SHM_MAX_FNAME_LEN 256
#define MPIDU_SHM_CACHE_LINE_LEN 64
#define MPIDU_SHM_MAX_NUMA_NUM (sizeof(MPIDU_shm_numa_bitmap) * 8)

typedef struct MPIDU_shm_barrier {
    OPA_int_t val;
    OPA_int_t wait;
} MPIDU_shm_barrier_t;

typedef struct MPIDU_shm_seg {
    size_t segment_len;
    /* Handle to shm seg */
    MPL_shm_hnd_t hnd;
    /* Pointers */
    char *base_addr;
    /* Misc */
    char file_name[MPIDU_SHM_MAX_FNAME_LEN];
    int base_descs;
    int symmetrical;
} MPIDU_shm_seg_t;

typedef struct MPIDU_shm_seg_info {
    size_t size;
    char *addr;
} MPIDU_shm_seg_info_t;

/* Per object memory binding information */
typedef struct MPIDU_shm_obj_info_t {
    MPIR_Memtype object_type[MPIDU_SHM_OBJ__NUM];
    MPIDU_Mempolicy object_policy[MPIDU_SHM_OBJ__NUM];
    int object_flags[MPIDU_SHM_OBJ__NUM];
} MPIDU_shm_obj_info_t;

extern MPIDU_shm_obj_info_t MPIDU_shm_obj_info;

/* Numa node specific information for each process */
typedef struct MPIDU_shm_numa_info_t {
    int nodeset_is_valid;       /* set to 1 if valid node binding exists */
    int nnodes;                 /* number of nodes */
    int *nodeid;                /* hwloc node id for each local rank */
    int *siblid;                /* sibling id for each node id. Sibling contains,
                                 * if available, the closest HBM node */
    MPIR_Memtype *type;         /* memory type for each node */
    MPIDU_shm_numa_bitmap bitmap;       /* nodes that have procs bound to them */
} MPIDU_shm_numa_info_t;

extern MPIDU_shm_numa_info_t MPIDU_shm_numa_info;

int MPIDU_shm_seg_alloc(size_t len, void **ptr_p, MPIR_Memtype type, MPL_memory_class class);
int MPIDU_shm_seg_commit(MPIDU_shm_seg_t * memory, MPIDU_shm_barrier_t ** barrier,
                         int num_local, int local_rank, int local_procs_0, int rank,
                         int node_id, MPIDU_shm_obj_t object, MPL_memory_class class);
int MPIDU_shm_seg_destroy(MPIDU_shm_seg_t * memory, int num_local);

int MPIDU_shm_barrier_init(MPIDU_shm_barrier_t * barrier_region,
                           MPIDU_shm_barrier_t ** barrier, int init_values);
int MPIDU_shm_barrier(MPIDU_shm_barrier_t * barrier, int num_local);

/* NUMA utility functions */
int MPIDU_shm_numa_info_init(int rank, int size, int *nodemap);
int MPIDU_shm_numa_info_finalize(void);
int MPIDU_shm_numa_bind_set(void *addr, size_t len, int numa_id, MPIR_Info * info,
                            MPIDU_shm_obj_t object);

#endif /* MPIDU_SHM_H_INCLUDED */

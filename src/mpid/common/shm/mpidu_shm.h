/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SHM_H_INCLUDED
#define MPIDU_SHM_H_INCLUDED

#define MPIDU_SHM_MAX_FNAME_LEN 256
#define MPIDU_SHM_CACHE_LINE_LEN 64

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

int MPIDU_shm_seg_alloc(size_t len, void **ptr_p, MPL_memory_class class);
int MPIDU_shm_seg_commit(MPIDU_shm_seg_t * memory, MPIDU_shm_barrier_t ** barrier,
                         int num_local, int local_rank, int local_procs_0, int rank,
                         MPL_memory_class class);
int MPIDU_shm_seg_destroy(MPIDU_shm_seg_t * memory, int num_local);

int MPIDU_shm_barrier_init(MPIDU_shm_barrier_t * barrier_region,
                           MPIDU_shm_barrier_t ** barrier, int init_values);
int MPIDU_shm_barrier(MPIDU_shm_barrier_t * barrier, int num_local);

#endif /* MPIDU_SHM_H_INCLUDED */

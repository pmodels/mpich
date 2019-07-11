/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_SHM_H_INCLUDED
#define MPIR_SHM_H_INCLUDED

#define MPIR_SHM_MAX_FNAME_LEN 256
#define MPIR_SHM_CACHE_LINE_LEN 64

typedef struct MPIR_shm_barrier {
    OPA_int_t val;
    OPA_int_t wait;
} MPIR_shm_barrier_t;

typedef struct MPIR_shm_seg {
    size_t segment_len;
    /* Handle to shm seg */
    MPL_shm_hnd_t hnd;
    /* Pointers */
    char *base_addr;
    /* Misc */
    char file_name[MPIR_SHM_MAX_FNAME_LEN];
    int base_descs;
    int symmetrical;
} MPIR_shm_seg_t;

typedef struct MPIR_shm_seg_info {
    size_t size;
    char *addr;
} MPIR_shm_seg_info_t;

int MPIR_shm_seg_alloc(size_t len, void **ptr_p, MPL_memory_class class);
int MPIR_shm_seg_commit(MPIR_shm_seg_t * memory, MPIR_shm_barrier_t ** barrier,
                        int num_local, int local_rank, int local_procs_0, int rank,
                        MPL_memory_class class);
int MPIR_shm_seg_destroy(MPIR_shm_seg_t * memory, int num_local);

int MPIR_shm_barrier_init(MPIR_shm_barrier_t * barrier_region,
                          MPIR_shm_barrier_t ** barrier, int init_values);
int MPIR_shm_barrier(MPIR_shm_barrier_t * barrier, int num_local);

#endif /* MPIR_SHM_H_INCLUDED */

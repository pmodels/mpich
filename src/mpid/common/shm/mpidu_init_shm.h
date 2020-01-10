/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_INIT_SHM_H_INCLUDED
#define MPIDU_INIT_SHM_H_INCLUDED

#define MPIDU_SHM_MAX_FNAME_LEN 256
#define MPIDU_SHM_CACHE_LINE_LEN MPL_CACHELINE_SIZE

/* One cache line per process should be enough for all cases */
#define MPIDU_INIT_SHM_BLOCK_SIZE MPIDU_SHM_CACHE_LINE_LEN

typedef struct MPIDU_Init_shm_block {
    char block[MPIDU_INIT_SHM_BLOCK_SIZE];
} MPIDU_Init_shm_block_t;

int MPIDU_Init_shm_init(void);
int MPIDU_Init_shm_finalize(void);
int MPIDU_Init_shm_barrier(void);
int MPIDU_Init_shm_put(void *orig, size_t len);
int MPIDU_Init_shm_get(int local_rank, size_t len, void *target);
int MPIDU_Init_shm_query(int local_rank, void **target_addr);

int MPIDU_Init_shm_alloc(size_t len, void **ptr);
int MPIDU_Init_shm_free(void *ptr);
int MPIDU_Init_shm_is_symm(void *ptr);

#endif /* MPIDU_INIT_SHM_H_INCLUDED */

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

typedef struct MPIDU_shm_seg_info {
    size_t size;
    char *addr;
} MPIDU_shm_seg_info_t;

int MPIDU_shm_seg_alloc(size_t len, void **ptr_p, MPL_memory_class class);
int MPIDU_shm_seg_commit(void **ptr, int num_local, int local_rank,
                         int local_procs_0, int rank, MPL_memory_class class);
int MPIDU_shm_seg_destroy(void *ptr, int num_local);
int MPIDU_shm_seg_is_symm(void *ptr);

#endif /* MPIDU_SHM_H_INCLUDED */

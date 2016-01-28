
#ifndef MPIDU_SHM_H
#define MPIDU_SHM_H

#include "mpiutil.h"

#define MPIDU_SHM_MAX_FNAME_LEN 256
#define MPIDU_SHM_CACHE_LINE_LEN 64

typedef struct MPIDU_shm_barrier
{
    OPA_int_t val;
    OPA_int_t wait;
} MPIDU_shm_barrier_t, *MPIDU_shm_barrier_ptr_t;

typedef struct MPIDU_shm_seg
{
    MPIU_Size_t segment_len;
    /* Handle to shm seg */
    MPIU_SHMW_Hnd_t hnd;
    /* Pointers */
    char *base_addr;
    /* Misc */
    char  file_name[MPIDU_SHM_MAX_FNAME_LEN];
    int   base_descs;
    int   symmetrical;
} MPIDU_shm_seg_t, *MPIDU_shm_seg_ptr_t;

typedef struct MPIDU_shm_seg_info
{
    size_t size;
    char *addr;
} MPIDU_shm_seg_info_t, *MPIDU_shm_seg_info_ptr_t;

int MPIDU_Seg_alloc(size_t len, void **ptr_p);
int MPIDU_Seg_commit(MPIDU_shm_seg_ptr_t memory, MPIDU_shm_barrier_ptr_t *barrier,
                     int num_local, int local_rank, int local_procs_0, int rank);
int MPIDU_Seg_destroy(MPIDU_shm_seg_ptr_t memory, int num_local);

int MPIDU_shm_barrier_init(MPIDU_shm_barrier_t *barrier_region,
                           MPIDU_shm_barrier_ptr_t *barrier, int init_values);
int MPIDU_shm_barrier(MPIDU_shm_barrier_t *barrier, int num_local);

#endif /* MPIDU_SHM_H */
/* vim: ft=c */

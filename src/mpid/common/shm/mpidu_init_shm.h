/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_INIT_SHM_H_INCLUDED
#define MPIDU_INIT_SHM_H_INCLUDED

#define MPIDU_SHM_MAX_FNAME_LEN 256
#define MPIDU_SHM_CACHE_LINE_LEN MPL_CACHELINE_SIZE

/* One cache line per process should be enough for all cases */
#define MPIDU_INIT_SHM_BLOCK_SIZE MPIDU_SHM_CACHE_LINE_LEN

#if defined(MPL_USE_LOCK_BASED_PRIMITIVES) && !defined(ENABLE_NO_LOCAL)
#error Lock-based atomics are used (probably due to missing compiler support), \
       shared memory will not work properly. Consider disable it with --enable-nolocal.
#endif

int MPIDU_Init_shm_init(void);
int MPIDU_Init_shm_finalize(void);
int MPIDU_Init_shm_barrier(void);
int MPIDU_Init_shm_put(void *orig, size_t len);
int MPIDU_Init_shm_get(int local_rank, size_t len, void *target);
int MPIDU_Init_shm_query(int local_rank, void **target_addr);

int MPIDU_Init_shm_alloc(size_t len, void **ptr);
int MPIDU_Init_shm_free(void *ptr);
int MPIDU_Init_shm_is_symm(void *ptr);

/* support routines for MPIDU_Init_shm_root_alloc */
int MPIDU_Init_shm_atomic_count(void);
int MPIDU_Init_shm_atomic_put(void *orig, size_t len);
int MPIDU_Init_shm_atomic_get(void *target, size_t len);

/* comm root allocate a shared memory and put the serialized handle if previously not allocated,
 * Otherwise, retrieve the handle and attach. Broadcast the handle to rest of the comm */
int MPIDU_Init_shm_comm_alloc(MPIR_Comm * comm, size_t len, void **ptr);

#endif /* MPIDU_INIT_SHM_H_INCLUDED */

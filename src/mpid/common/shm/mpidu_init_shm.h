/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_INIT_SHM_H_INCLUDED
#define MPIDU_INIT_SHM_H_INCLUDED

int MPIDU_Init_shm_init(int rank, int size, int *nodemap);
int MPIDU_Init_shm_finalize(void);
int MPIDU_Init_shm_barrier(void);
int MPIDU_Init_shm_put(void *orig, size_t len);
int MPIDU_Init_shm_get(int local_rank, void **target);

#endif /* MPIDU_INIT_SHM_H_INCLUDED */

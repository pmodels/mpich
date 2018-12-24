/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SHM_OBJ_H_INCLUDED
#define MPIDU_SHM_OBJ_H_INCLUDED

typedef enum MPIDU_shm_obj {
    MPIDU_SHM_OBJ__NONE = -1,
    MPIDU_SHM_OBJ__FASTBOXES = 0,
    MPIDU_SHM_OBJ__CELLS,
    MPIDU_SHM_OBJ__COPYBUFS,
    MPIDU_SHM_OBJ__WIN,
    MPIDU_SHM_OBJ__NUM
} MPIDU_shm_obj_t;

#endif /* MPIDU_SHM_OBJ_H_INCLUDED */

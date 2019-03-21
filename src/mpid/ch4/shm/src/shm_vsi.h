/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_VCI_H_INCLUDED
#define SHM_VCI_H_INCLUDED

#define MPIDI_SHM_MAX_VSIS 1

typedef struct MPIDI_SHM_vsi {
    int is_free;
    /* TODO: what is a VSI */
} MPIDI_SHM_vsi_t;

typedef struct MPIDI_SHM_vsi_pool {
    int next_free_vsi;
    int max_vsis;
    MPIDI_SHM_vsi_t vsi[MPIDI_SHM_MAX_VSIS];
} MPIDI_SHM_vsi_pool_t;
extern MPIDI_SHM_vsi_pool_t MPIDI_SHM_vsi_pool_global;
#define MPIDI_SHM_VSI_POOL(field) MPIDI_SHM_vsi_pool_global.field

/* Declare slow-path functions */
void MPIDI_SHM_vsi_pool_init(void);
void MPIDI_SHM_vsi_arm(MPIDI_SHM_vsi_t * vsi);

/* TODO: fast-path functions */

#endif /* SHM_VCI_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _HCOLLPRE_H_
#define _HCOLLPRE_H_

typedef struct {
    int is_hcoll_init;
    struct MPID_Collops *hcoll_origin_coll_fns;
    void *hcoll_context;
} hcoll_comm_priv_t;

#endif

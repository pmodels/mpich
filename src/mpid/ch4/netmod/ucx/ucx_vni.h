/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef UCX_VNI_H_INCLUDED
#define UCX_VNI_H_INCLUDED

#include "ucx_impl.h"

void MPIDI_UCX_vni_pool_init(void);
int MPIDI_UCX_vni_pool_get_free_vni(void);
int MPIDI_UCX_vni_pool_alloc_vni(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties,
                                 int *vni);
int MPIDI_UCX_vni_pool_free_vni(int vni);
void MPIDI_UCX_vni_arm(MPIDI_UCX_vni_t * vni);
void MPIDI_UCX_vni_unarm(MPIDI_UCX_vni_t * vni);

#endif /* UCX_VNI_H_INCLUDED */

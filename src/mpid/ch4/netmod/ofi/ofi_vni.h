/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef OFI_VNI_H_INCLUDED
#define OFI_VNI_H_INCLUDED

#include "ofi_impl.h"

void MPIDI_OFI_vni_pool_init(void);
int MPIDI_OFI_vni_pool_get_free_vni(void);
int MPIDI_OFI_vni_pool_alloc_vni(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties,
                                 int *vni);
int MPIDI_OFI_vni_pool_free_vni(int vni);
void MPIDI_OFI_vni_arm(MPIDI_OFI_vni_t * vni);
void MPIDI_OFI_vni_unarm(MPIDI_OFI_vni_t * vni);

#endif /* OFI_VNI_H_INCLUDED */

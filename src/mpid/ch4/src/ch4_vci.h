/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_vci_types.h"

/** Slow-path function declarations **/
#ifndef MPIDI_CH4_DIRECT_NETMOD
int MPIDI_vci_pool_create_and_init(int num_vsis, int num_vnis);
#else
int MPIDI_vci_pool_create_and_init(int num_vnis);
#endif
int MPIDI_vci_pool_destroy(void);
void MPIDI_vci_pool_get_free_vci(int *free_vci);
void MPIDI_vci_pool_update_next_free_vci();
#ifndef MPIDI_CH4_DIRECT_NETMOD
void MPIDI_vci_arm(MPIDI_vci_t * vci, int vni, int vsi);
#else
void MPIDI_vci_arm(MPIDI_vci_t * vci, int vni);
#endif
void MPIDI_vci_unarm(MPIDI_vci_t * vci);
int MPIDI_vci_alloc(MPIDI_vci_type_t type, MPIDI_vci_resource_t resources,
                    MPIDI_vci_property_t properties, int *vci);
int MPIDI_vci_free(int vci);

/** Fast-path functions **/
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* Only a single VCI per comm */
    return MPIDI_COMM_VCI(comm_ptr, vci);
}

#endif /* CH4_VCI_H_INCLUDED */

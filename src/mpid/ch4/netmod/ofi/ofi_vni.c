/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#include <mpidimpl.h>
#include "ofi_impl.h"

void MPIDI_OFI_vni_pool_init(void)
{
    int vni;

    for (vni = 0; vni < MPIDI_OFI_VNI_POOL(max_vnis); vni++) {
        MPIDI_OFI_VNI(vni).is_free = 1;
    }

    MPIDI_OFI_VNI_POOL(next_free_vni) = 0;
}

int MPIDI_OFI_vni_pool_get_free_vni(void)
{
    int i;

    for (i = 0; i < MPIDI_OFI_VNI_POOL(max_vnis); i++) {
        if (MPIDI_OFI_VNI(i).is_free) {
            return i;
        }
    }

    return MPIDI_NM_VNI_INVALID;
}

void MPIDI_OFI_vni_arm(MPIDI_OFI_vni_t * vni)
{
    vni->is_free = 0;
}

void MPIDI_OFI_vni_unarm(MPIDI_OFI_vni_t * vni)
{
    vni->is_free = 1;
}

int MPIDI_OFI_vni_pool_alloc_vni(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties,
                                 int *vni)
{
    int mpi_errno = MPI_SUCCESS;
    int my_vni, new_next_free_vni;

    my_vni = MPIDI_OFI_VNI_POOL(next_free_vni);

    if (my_vni == MPIDI_NM_VNI_INVALID) {
        /* Search for a free vni. It might have been freed before this alloc */
        my_vni = MPIDI_OFI_vni_pool_get_free_vni();
        if (my_vni != MPIDI_NM_VNI_INVALID) {
            MPIDI_OFI_vni_arm(&MPIDI_OFI_VNI(my_vni));
        } else {
            new_next_free_vni = MPIDI_NM_VNI_INVALID;
            goto fn_exit;
        }
    } else {
        MPIDI_OFI_vni_arm(&MPIDI_OFI_VNI(my_vni));
    }
    /* Update next free VNI */
    new_next_free_vni = MPIDI_OFI_VNI_POOL(next_free_vni)++;
    if (new_next_free_vni >= MPIDI_OFI_VNI_POOL(max_vnis) ||
        !MPIDI_OFI_VNI(new_next_free_vni).is_free) {
        new_next_free_vni = MPIDI_OFI_vni_pool_get_free_vni();
    }
  fn_exit:
    MPIDI_OFI_VNI_POOL(next_free_vni) = new_next_free_vni;
    *vni = my_vni;
    return mpi_errno;
}

int MPIDI_OFI_vni_pool_free_vni(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_vni_unarm(&MPIDI_OFI_VNI(vni));

    return mpi_errno;
}

int MPIDI_NM_vni_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vni)
{
    return MPIDI_OFI_vni_pool_alloc_vni(resources, properties, vni);
}

int MPIDI_NM_vni_free(int vni)
{
    return MPIDI_OFI_vni_pool_free_vni(vni);
}

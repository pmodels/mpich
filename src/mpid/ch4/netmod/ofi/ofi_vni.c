/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#include <mpidimpl.h>
#include "ofi_impl.h"

/* Allocate the pool of OFI VNIs. */
void MPIDI_OFI_vni_pool_alloc(void)
{
    int vni;

    for (vni = 0; vni < MPIDI_OFI_VNI_POOL(max_vnis); vni++) {
        MPIDI_OFI_VNI(vni).is_free = 1;
    }

    MPIDI_OFI_VNI_POOL(next_free_vni) = 0;
}

/* Initialize the VNI. */
void MPIDI_OFI_vni_arm(MPIDI_OFI_vni_t * vni)
{
    vni->is_free = 0;
}

/* Reset the VNI. */
void MPIDI_OFI_vni_unarm(MPIDI_OFI_vni_t * vni)
{
    vni->is_free = 1;
}

/* Return the index of a free VNI. */
int MPIDI_OFI_vni_get_free(void)
{
    int i;

    for (i = 0; i < MPIDI_OFI_VNI_POOL(max_vnis); i++) {
        if (MPIDI_OFI_VNI(i).is_free) {
            return i;
        }
    }

    return MPIDI_NM_VNI_INVALID;
}

/* If available, return a VNI that is ready for use.
 * If none available, return MPIDI_NM_VNI_INVALID.*/
int MPIDI_OFI_vni_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vni)
{
    int mpi_errno = MPI_SUCCESS;
    int my_vni, new_next_free_vni;

    my_vni = MPIDI_OFI_VNI_POOL(next_free_vni);

    if (my_vni == MPIDI_NM_VNI_INVALID) {
        /* Search for a free vni. It might have been freed before this alloc */
        my_vni = MPIDI_OFI_vni_get_free();
        if (my_vni != MPIDI_NM_VNI_INVALID) {
            MPIDI_OFI_vni_arm(&MPIDI_OFI_VNI(my_vni));
        } else {
            /* if no free VNI, the next is still MPIDI_NM_VNI_INVALID */
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
        new_next_free_vni = MPIDI_OFI_vni_get_free();
    }
  fn_exit:
    MPIDI_OFI_VNI_POOL(next_free_vni) = new_next_free_vni;
    *vni = my_vni;
    return mpi_errno;
}

int MPIDI_OFI_vni_free(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_vni_unarm(&MPIDI_OFI_VNI(vni));

    return mpi_errno;
}

int MPIDI_NM_vni_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vni)
{
    return MPIDI_OFI_vni_alloc(resources, properties, vni);
}

int MPIDI_NM_vni_free(int vni)
{
    return MPIDI_OFI_vni_free(vni);
}

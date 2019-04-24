    /* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  *  (C) 2019 by Argonne National Laboratory.
 *   *      See COPYRIGHT in top-level directory.
 *    *
 *     */

#include <mpidimpl.h>
#include "ucx_impl.h"
#include "ucx_types.h"

/* Allocate the pool of UCX VNIs. */
void MPIDI_UCX_vni_pool_alloc(void)
{
    int vni;

    for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
        MPIDI_UCX_VNI(vni).is_free = 1;
    }

    MPIDI_UCX_VNI_POOL(next_free_vni) = 0;
}

/* Initialize the VNI. */
void MPIDI_UCX_vni_arm(MPIDI_UCX_vni_t * vni)
{
    vni->is_free = 0;
}

/* Reset the VNI. */
void MPIDI_UCX_vni_unarm(MPIDI_UCX_vni_t * vni)
{
    vni->is_free = 1;
}

/* Return the index of a free VNI */
int MPIDI_UCX_vni_get_free(void)
{
    int i;

    for (i = 0; i < MPIDI_UCX_VNI_POOL(total_vnis); i++) {
        if (MPIDI_UCX_VNI(i).is_free) {
            return i;
        }
    }
    return MPIDI_NM_VNI_INVALID;
}

/* If available, return a VNI that is ready for use.
 * If none available, return MPIDI_NM_VNI_INVALID. */
int MPIDI_UCX_vni_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vni)
{
    int mpi_errno = MPI_SUCCESS;
    int my_vni, new_next_free_vni;

    my_vni = MPIDI_UCX_VNI_POOL(next_free_vni);

    if (my_vni == MPIDI_NM_VNI_INVALID) {
        /* Search for a free vni. It might have been freed before this alloc */
        my_vni = MPIDI_UCX_vni_get_free();
        if (my_vni != MPIDI_NM_VNI_INVALID) {
            MPIDI_UCX_vni_arm(&MPIDI_UCX_VNI(my_vni));
        } else {
            /* if no free vni, the next is still MPIDI_NM_VNI_INVALID */
            new_next_free_vni = MPIDI_NM_VNI_INVALID;
            goto fn_exit;
        }
    } else {
        MPIDI_UCX_vni_arm(&MPIDI_UCX_VNI(my_vni));
    }
    /* Update next free VNI */
    new_next_free_vni = MPIDI_UCX_VNI_POOL(next_free_vni)++;
    if (new_next_free_vni >= MPIDI_UCX_VNI_POOL(total_vnis) ||
        !MPIDI_UCX_VNI(new_next_free_vni).is_free) {
        /* Search for the next free VNI */
        new_next_free_vni = MPIDI_UCX_vni_get_free();
    }
  fn_exit:
    MPIDI_UCX_VNI_POOL(next_free_vni) = new_next_free_vni;
    *vni = my_vni;
    return mpi_errno;
}

/* Return this VNI to the pool. */
int MPIDI_UCX_vni_free(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_UCX_vni_unarm(&MPIDI_UCX_VNI(vni));

    return mpi_errno;
}

int MPIDI_NM_vni_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vni)
{
    return MPIDI_UCX_vni_alloc(resources, properties, vni);
}

int MPIDI_NM_vni_free(int vni)
{
    return MPIDI_UCX_vni_free(vni);
}

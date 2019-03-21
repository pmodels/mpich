/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include <mpidimpl.h>
#include "shm_vsi.h"

MPIDI_SHM_vsi_pool_t MPIDI_SHM_vsi_pool_global;

void MPIDI_SHM_vsi_pool_init(void)
{
    int i;

    MPIDI_SHM_VSI_POOL(max_vsis) = MPIDI_SHM_MAX_VSIS;

    for (i = 0; i < MPIDI_SHM_VSI_POOL(max_vsis); i++) {
        MPIDI_SHM_VSI_POOL(vsi)[i].is_free = 1;
    }

    MPIDI_SHM_VSI_POOL(next_free_vsi) = 0;
}

void MPIDI_SHM_vsi_arm(MPIDI_SHM_vsi_t * vsi)
{
    vsi->is_free = 0;
}

int MPIDI_SHM_vsi_alloc(MPIDI_vci_resource_t resources, MPIDI_vci_property_t properties, int *vsi)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int my_vsi;

    my_vsi = MPIDI_SHM_VSI_POOL(next_free_vsi);

    if (my_vsi == MPIDI_SHM_VSI_INVALID) {
        /* Check if any VSI has been freed */
        for (i = 0; i < MPIDI_SHM_VSI_POOL(max_vsis); i++) {
            if (MPIDI_SHM_VSI_POOL(vsi)[i].is_free) {
                /* need to set this here to prevent this VSI
                 * from being selected in the search for the
                 * next free VSI */
                my_vsi = i;
                MPIDI_SHM_vsi_arm(&MPIDI_SHM_VSI_POOL(vsi)[my_vsi]);
                goto found_free_vsi;
            }
        }
        my_vsi = MPIDI_SHM_VSI_INVALID;
        goto fn_exit;
    } else {
        MPIDI_SHM_vsi_arm(&MPIDI_SHM_VSI_POOL(vsi)[my_vsi]);
    }
  found_free_vsi:
    /* Update next_free_vsi */
    MPIDI_SHM_VSI_POOL(next_free_vsi) = MPIDI_SHM_VSI_INVALID;
    for (i = 0; i < MPIDI_SHM_VSI_POOL(max_vsis); i++) {
        if (MPIDI_SHM_VSI_POOL(vsi)[i].is_free) {
            MPIDI_SHM_VSI_POOL(next_free_vsi) = i;
            break;
        }
    }
  fn_exit:
    *vsi = my_vsi;
    return mpi_errno;
}

int MPIDI_SHM_vsi_free(int vsi)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_SHM_VSI_POOL(vsi)[vsi].is_free = 1;

    return mpi_errno;
}

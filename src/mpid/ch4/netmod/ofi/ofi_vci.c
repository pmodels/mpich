/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

int MPIDI_OFI_comm_set_vcis(MPIR_Comm * comm, int num_vcis, int *all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* set up local vcis */
    int num_vcis_actual;
    mpi_errno = MPIDI_NM_init_vcis(num_vcis, &num_vcis_actual);
    MPIR_ERR_CHECK(mpi_errno);

    /* gather the number of remote vcis */
    mpi_errno = MPIR_Allgather_impl(&num_vcis_actual, 1, MPIR_INT_INTERNAL,
                                    all_num_vcis, 1, MPIR_INT_INTERNAL, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* Since we allow different process to have different num_vcis, we always need run exchange. */
    mpi_errno = MPIDI_OFI_addr_exchange_all_ctx();
    MPIR_ERR_CHECK(mpi_errno);

    for (int vci = 1; vci < MPIDI_OFI_global.num_vcis; vci++) {
        MPIDI_OFI_am_init(vci);
        MPIDI_OFI_am_post_recv(vci, 0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

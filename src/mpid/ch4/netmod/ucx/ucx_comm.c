/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "mpidu_bc.h"
#ifdef HAVE_LIBHCOLL
#include "../../common/hcoll/hcoll.h"
#endif

int MPIDI_UCX_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;



#if defined HAVE_LIBHCOLL
    hcoll_comm_create(comm, NULL);
#endif

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;



#ifdef HAVE_LIBHCOLL
    hcoll_comm_destroy(comm, NULL);
#endif
  fn_exit:

    return mpi_errno;
}

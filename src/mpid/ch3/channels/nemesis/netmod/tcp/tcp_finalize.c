/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_impl.h"

int MPID_nem_tcp_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;


    mpi_errno = MPID_nem_tcp_send_finalize();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_sm_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPID_nem_tcp_g_lstn_sc.fd) {
        CHECK_EINTR(ret, close(MPID_nem_tcp_g_lstn_sc.fd));
        MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket",
                             "**closesocket %s %d", errno, MPIR_Strerror(errno));
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

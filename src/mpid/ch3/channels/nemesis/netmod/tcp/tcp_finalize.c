/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "tcp_impl.h"

int MPID_nem_tcp_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
#ifdef HAVE_ERROR_CHECKING
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
#endif

    MPIR_FUNC_ENTER;

    mpi_errno = MPID_nem_tcp_send_finalize();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_sm_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPID_nem_tcp_g_lstn_sc.fd) {
        CHECK_EINTR(ret, close(MPID_nem_tcp_g_lstn_sc.fd));
        MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket",
                             "**closesocket %s %d", errno,
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

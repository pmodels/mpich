/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_FINALIZE);

    mpi_errno = MPID_nem_tcp_send_finalize();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_sm_finalize();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
     
    if (MPID_nem_tcp_g_lstn_sc.fd)
    {
        CHECK_EINTR (ret, close(MPID_nem_tcp_g_lstn_sc.fd));
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket", "**closesocket %s %d", errno, MPIU_Strerror (errno));
    }
        
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


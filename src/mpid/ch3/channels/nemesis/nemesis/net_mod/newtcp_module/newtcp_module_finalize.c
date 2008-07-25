/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

extern sockconn_t g_lstn_sc;

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    
    MPID_nem_newtcp_module_poll_finalize();
    MPID_nem_newtcp_module_send_finalize();
    MPID_nem_newtcp_module_sm_finalize();
     
    if (g_lstn_sc.fd)
    {
        CHECK_EINTR (ret, close(g_lstn_sc.fd));
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket", "**closesocket %s %d", errno, strerror (errno));
    }
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_ckpt_shutdown()
{
    return MPID_nem_newtcp_module_finalize();
}


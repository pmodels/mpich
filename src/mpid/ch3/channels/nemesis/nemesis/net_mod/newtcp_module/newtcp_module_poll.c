/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include "socksm.h"
#include <errno.h>

char *MPID_nem_newtcp_module_recv_buf = NULL; /* avoid common symbol */

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(MPID_nem_newtcp_module_recv_buf, char*, MPID_NEM_NEWTCP_MODULE_RECV_MAX_PKT_LEN, mpi_errno, "NewTCP temporary buffer");
    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


int MPID_nem_newtcp_module_poll_finalize()
{
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_newtcp_module_connpoll();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

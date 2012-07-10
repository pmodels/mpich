/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"
#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

/* FIXME : Get rid of these dummy funcs */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
    return MPI_SUCCESS;
}


int MPID_nem_newtcp_module_poll_finalize()
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_newtcp_module_connpoll(in_blocking_poll);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_COMM_H_INCLUDED
#define POSIX_COMM_H_INCLUDED

#include "posix_impl.h"
#include "utlist.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}


#endif /* POSIX_COMM_H_INCLUDED */

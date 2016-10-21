/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_POSIX_COMM_H_INCLUDED
#define SHM_POSIX_COMM_H_INCLUDED

#include "posix_impl.h"
#include "mpl_utlist.h"
#include "posix_coll_select.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_COMM_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_COMM_CREATE);

    MPIDI_SHM_collective_selection_init(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_COMM_CREATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_COMM_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_COMM_DESTROY);

    MPIDI_SHM_collective_selection_free(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_COMM_DESTROY);
    return mpi_errno;
}


#endif /* SHM_POSIX_COMM_H_INCLUDED */

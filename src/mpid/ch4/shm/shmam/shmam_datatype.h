/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_DATATYPE_H_INCLUDED
#define SHM_DATATYPE_H_INCLUDED

#include "shmam_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_type_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_type_create_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_TYPE_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_TYPE_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_TYPE_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_TYPE_FREE_HOOK);
    return mpi_errno;
}
#endif
